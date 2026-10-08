#include <Windows.h>
#include <steam/steam_api.h>
#include <SteamAPIBridge.h>
#include "LegacyInterfaces.h"
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <new>

namespace
{
template <class T> T Resolve(HMODULE module, const char* name) { return reinterpret_cast<T>(GetProcAddress(module, name)); }
struct Runtime
{
    HMODULE                                  module{};
    HSteamUser                               user{};
    HSteamPipe                               pipe{};
    decltype(&SteamAPI_RegisterCallback)     registerCallback{};
    decltype(&SteamAPI_UnregisterCallback)   unregisterCallback{};
    decltype(&SteamAPI_RegisterCallResult)   registerResult{};
    decltype(&SteamAPI_UnregisterCallResult) unregisterResult{};
    ISteamUser*                              userAPI{};
    legacy::User016*                         oldUser{};
    ISteamScreenshots*                       screenshots{};
    legacy::Screenshots001*                  oldScreenshots{};
    ISteamHTTP*                              http{};

    Runtime()
    {
        module = GetModuleHandleW(L"steam_api.dll");
        if (!module) return;
        auto getUser = Resolve<decltype(&SteamAPI_GetHSteamUser)>(module, "SteamAPI_GetHSteamUser");
        auto getPipe = Resolve<decltype(&SteamAPI_GetHSteamPipe)>(module, "SteamAPI_GetHSteamPipe");
        if (!getUser || !getPipe || !(user = getUser()) || !(pipe = getPipe())) return;
        registerCallback   = Resolve<decltype(registerCallback)>(module, "SteamAPI_RegisterCallback");
        unregisterCallback = Resolve<decltype(unregisterCallback)>(module, "SteamAPI_UnregisterCallback");
        registerResult     = Resolve<decltype(registerResult)>(module, "SteamAPI_RegisterCallResult");
        unregisterResult   = Resolve<decltype(unregisterResult)>(module, "SteamAPI_UnregisterCallResult");
        if (auto find = Resolve<decltype(&SteamInternal_FindOrCreateUserInterface)>(module, "SteamInternal_FindOrCreateUserInterface"))
        {
            userAPI     = static_cast<ISteamUser*>(find(user, STEAMUSER_INTERFACE_VERSION));
            screenshots = static_cast<ISteamScreenshots*>(find(user, STEAMSCREENSHOTS_INTERFACE_VERSION));
            http        = static_cast<ISteamHTTP*>(find(user, STEAMHTTP_INTERFACE_VERSION));
        }
        else if (auto getClient = Resolve<legacy::Client012*(__cdecl*)()>(module, "SteamClient"))
        {
            if (auto client = getClient())
            {
                userAPI     = static_cast<ISteamUser*>(client->GetISteamUser(user, pipe, STEAMUSER_INTERFACE_VERSION));
                screenshots = static_cast<ISteamScreenshots*>(client->GetISteamScreenshots(user, pipe, STEAMSCREENSHOTS_INTERFACE_VERSION));
                http        = static_cast<ISteamHTTP*>(client->GetISteamHTTP(user, pipe, STEAMHTTP_INTERFACE_VERSION));
                // Request explicit old versions instead of assuming arbitrary bare accessors' ABI.
                if (!userAPI) oldUser = static_cast<legacy::User016*>(client->GetISteamUser(user, pipe, "SteamUser016"));
                if (!screenshots) oldScreenshots = static_cast<legacy::Screenshots001*>(client->GetISteamScreenshots(user, pipe, "STEAMSCREENSHOTS_INTERFACE_VERSION001"));
            }
        }
    }
    bool callbacks() const { return registerCallback && unregisterCallback; }
    bool httpReady() const { return http && callbacks() && registerResult && unregisterResult; }
};
struct Callback;
struct Request
{
    SteamBridgeHTTPCallback   callback{};
    void*                     user{};
    std::shared_ptr<Callback> complete;
    bool                      sent{}, finished{};
};
} // namespace
struct SteamBridgeContext
{
    HMODULE                               module{};
    HSteamUser                            user{};
    HSteamPipe                            pipe{};
    std::unordered_map<uint32_t, Request> requests;
    std::unordered_set<uint32_t>          cookies;
    std::shared_ptr<Callback>             screenshot, headers, data;
    SteamBridgeScreenshotCallback         screenshotCallback{};
    void*                                 screenshotUser{};
};
namespace
{
std::recursive_mutex                                                         mutex;
std::unordered_map<SteamBridgeContext*, std::shared_ptr<SteamBridgeContext>> contexts;
// Retain unregistered callback objects through the native dispatcher return.
std::vector<std::shared_ptr<Callback>> retired;
unsigned                               dispatchDepth{};
struct Guard
{
    std::unique_lock<std::recursive_mutex> lock{mutex};
    Guard()
    {
        if (!dispatchDepth) retired.clear();
    }
};
std::shared_ptr<SteamBridgeContext> Context(SteamBridgeContext* c)
{
    auto it = contexts.find(c);
    return it == contexts.end() ? nullptr : it->second;
}
bool Bind(SteamBridgeContext& c, const Runtime& r)
{
    if (!r.user || !r.pipe) return false;
    if (!c.module)
    {
        c.module = r.module;
        c.user   = r.user;
        c.pipe   = r.pipe;
    }
    return c.module == r.module && c.user == r.user && c.pipe == r.pipe;
}
struct Callback final : CCallbackBase, std::enable_shared_from_this<Callback>
{
    std::weak_ptr<SteamBridgeContext>        owner;
    HMODULE                                  module;
    decltype(&SteamAPI_UnregisterCallback)   unregisterCallback;
    decltype(&SteamAPI_UnregisterCallResult) unregisterResult;
    SteamAPICall_t                           call{};
    uint32_t                                 request{};
    bool                                     active{};
    int                                      size;
    Callback(const std::shared_ptr<SteamBridgeContext>& c, const Runtime& r, int id, int bytes) : owner(c), module(r.module), unregisterCallback(r.unregisterCallback), unregisterResult(r.unregisterResult), size(bytes) { m_iCallback = id; }
    void Detach()
    {
        if (!active) return;
        active = false;
        if (GetModuleHandleW(L"steam_api.dll") != module) return;
        if (call) unregisterResult(this, call);
        else
            unregisterCallback(this);
    }
    int  GetCallbackSizeBytes() override { return size; }
    void Run(void* payload) override { Run(payload, false, 0); }
    void Run(void* payload, bool failure, SteamAPICall_t completedCall) override
    {
        std::unique_lock lock(mutex);
        auto             self = shared_from_this();
        auto             c    = owner.lock();
        if (!active || !c || !contexts.contains(c.get()) || (call && call != completedCall)) return;
        ++dispatchDepth;
        SteamBridgeScreenshotCallback shotFn{};
        SteamBridgeHTTPCallback       httpFn{};
        void*                         opaque{};
        uint32_t                      shot{};
        int32_t                       result{};
        SteamBridgeHTTPEvent          event{};
        if (m_iCallback == ScreenshotReady_t::k_iCallback)
        {
            if (payload)
            {
                auto p = static_cast<ScreenshotReady_t*>(payload);
                shot   = p->m_hLocal;
                result = p->m_eResult;
            }
            shotFn = c->screenshotCallback;
            opaque = c->screenshotUser;
        }
        else
        {
            event.request = request;
            if (m_iCallback == HTTPRequestCompleted_t::k_iCallback)
            {
                event.kind      = SB_HTTP_COMPLETE;
                event.ioFailure = failure || !payload;
                // HTTP003 uses the current completion ABI. Never interpret HTTP002 here.
                if (payload && !failure)
                {
                    auto p           = static_cast<HTTPRequestCompleted_t*>(payload);
                    event.request    = p->m_hRequest;
                    event.statusCode = p->m_eStatusCode;
                    event.successful = p->m_bRequestSuccessful;
                }
            }
            else if (payload && m_iCallback == HTTPRequestHeadersReceived_t::k_iCallback)
            {
                event.kind    = SB_HTTP_HEADERS;
                event.request = static_cast<HTTPRequestHeadersReceived_t*>(payload)->m_hRequest;
            }
            else if (payload)
            {
                auto p        = static_cast<HTTPRequestDataReceived_t*>(payload);
                event.kind    = SB_HTTP_DATA;
                event.request = p->m_hRequest;
                event.offset  = p->m_cOffset;
                event.bytes   = p->m_cBytesReceived;
            }
            auto it = c->requests.find(event.request);
            if (it != c->requests.end() && it->second.sent && !it->second.finished)
            {
                if (event.kind == SB_HTTP_COMPLETE)
                {
                    it->second.finished = true;
                    Detach();
                }
                httpFn = it->second.callback;
                opaque = it->second.user;
            }
        }
        lock.unlock();
        // Exceptions must not cross the C callback boundary.
        try
        {
            if (shotFn) shotFn(opaque, shot, result);
            if (httpFn) httpFn(opaque, &event);
        }
        catch (...)
        {}
        lock.lock();
        --dispatchDepth;
    }
};
void Retire(std::shared_ptr<Callback>& callback)
{
    if (callback)
    {
        callback->Detach();
        retired.push_back(std::move(callback));
    }
}
std::shared_ptr<Callback> Register(const std::shared_ptr<SteamBridgeContext>& c, const Runtime& r, int id, int size)
{
    auto cb    = std::make_shared<Callback>(c, r, id, size);
    cb->active = true;
    r.registerCallback(cb.get(), id);
    return cb;
}
SteamBridgeStatus                    Result(bool value) { return value ? SB_OK : SB_FAILED; }
template <class F> SteamBridgeStatus HTTP(SteamBridgeContext* context, uint32_t request, F&& fn)
{
    Guard guard;
    auto  c = Context(context);
    if (!c || !c->requests.contains(request)) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!r.httpReady() || !Bind(*c, r)) return SB_UNAVAILABLE;
    return Result(fn(*r.http));
}
} // namespace

SteamBridgeContextHandle SteamBridge_CreateContext()
{
    try
    {
        Guard guard;
        auto  c = std::make_shared<SteamBridgeContext>();
        auto  p = c.get();
        contexts.emplace(p, std::move(c));
        return p;
    }
    catch (...)
    {
        return nullptr;
    }
}
void SteamBridge_DestroyContext(SteamBridgeContextHandle context)
{
    Guard guard;
    auto  c = Context(context);
    if (!c) return;
    contexts.erase(context);
    Retire(c->screenshot);
    Retire(c->headers);
    Retire(c->data);
    Runtime r;
    for (auto& [handle, request] : c->requests)
    {
        Retire(request.complete);
        if (r.http && Bind(*c, r)) r.http->ReleaseHTTPRequest(handle);
    }
    for (auto handle : c->cookies)
        if (r.http && Bind(*c, r)) r.http->ReleaseCookieContainer(handle);
}
uint32_t SteamBridge_GetCapabilities(SteamBridgeContextHandle context)
{
    Guard guard;
    auto  c = Context(context);
    if (!c) return 0;
    Runtime r;
    if (!Bind(*c, r)) return 0;
    return ((r.userAPI || r.oldUser) ? SB_CAP_USER : 0) | (((r.screenshots || r.oldScreenshots) && r.callbacks()) ? SB_CAP_SCREENSHOTS : 0) | (r.httpReady() ? SB_CAP_HTTP : 0);
}
SteamBridgeStatus SteamBridge_GetSteamID(SteamBridgeContextHandle context, uint64_t* id)
{
    if (!id) return SB_INVALID_ARGUMENT;
    *id = 0;
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!Bind(*c, r)) return SB_UNAVAILABLE;
    if (r.userAPI) *id = r.userAPI->GetSteamID().ConvertToUint64();
    else if (r.oldUser)
        *id = r.oldUser->GetSteamID().ConvertToUint64();
    else
        return SB_UNAVAILABLE;
    return *id ? SB_OK : SB_UNAVAILABLE;
}
SteamBridgeStatus SteamBridge_SubscribeScreenshots(SteamBridgeContextHandle context, SteamBridgeScreenshotCallback fn, void* user)
{
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    Retire(c->screenshot);
    c->screenshotCallback = fn;
    c->screenshotUser     = user;
    if (!fn) return SB_OK;
    Runtime r;
    if (!Bind(*c, r) || !r.callbacks() || (!r.screenshots && !r.oldScreenshots)) return SB_UNAVAILABLE;
    c->screenshot = Register(c, r, ScreenshotReady_t::k_iCallback, sizeof(ScreenshotReady_t));
    return SB_OK;
}
SteamBridgeStatus SteamBridge_WriteScreenshot(SteamBridgeContextHandle context, const void* rgb, uint32_t size, int32_t width, int32_t height, uint32_t* screenshot)
{
    if (!screenshot) return SB_INVALID_ARGUMENT;
    *screenshot = 0;
    if (!rgb || width <= 0 || height <= 0 || uint64_t(width) * height * 3 != size) return SB_INVALID_ARGUMENT;
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!Bind(*c, r)) return SB_UNAVAILABLE;
    if (r.screenshots) *screenshot = r.screenshots->WriteScreenshot(const_cast<void*>(rgb), size, width, height);
    else if (r.oldScreenshots)
        *screenshot = r.oldScreenshots->WriteScreenshot(const_cast<void*>(rgb), size, width, height);
    else
        return SB_UNAVAILABLE;
    return Result(*screenshot != 0);
}
SteamBridgeStatus SteamBridge_AddScreenshot(SteamBridgeContextHandle context, const char* file, const char* thumbnail, int32_t width, int32_t height, uint32_t* screenshot)
{
    if (!screenshot) return SB_INVALID_ARGUMENT;
    *screenshot = 0;
    if (!file) return SB_INVALID_ARGUMENT;
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!Bind(*c, r)) return SB_UNAVAILABLE;
    if (r.screenshots) *screenshot = r.screenshots->AddScreenshotToLibrary(file, thumbnail, width, height);
    else if (r.oldScreenshots)
        *screenshot = r.oldScreenshots->AddScreenshotToLibrary(file, thumbnail, width, height);
    else
        return SB_UNAVAILABLE;
    return Result(*screenshot != 0);
}
SteamBridgeStatus SteamBridge_SetScreenshotLocation(SteamBridgeContextHandle context, uint32_t screenshot, const char* location)
{
    if (!location || !screenshot) return SB_INVALID_ARGUMENT;
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!Bind(*c, r)) return SB_UNAVAILABLE;
    if (r.screenshots) return Result(r.screenshots->SetLocation(screenshot, location));
    if (r.oldScreenshots) return Result(r.oldScreenshots->SetLocation(screenshot, location));
    return SB_UNAVAILABLE;
}
SteamBridgeStatus SteamBridge_TagScreenshotUser(SteamBridgeContextHandle context, uint32_t screenshot, uint64_t id)
{
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!Bind(*c, r)) return SB_UNAVAILABLE;
    if (r.screenshots) return Result(r.screenshots->TagUser(screenshot, CSteamID(id)));
    if (r.oldScreenshots) return Result(r.oldScreenshots->TagUser(screenshot, CSteamID(id)));
    return SB_UNAVAILABLE;
}
SteamBridgeStatus SteamBridge_HTTPCreate(SteamBridgeContextHandle context, int32_t method, const char* url, uint32_t* request)
{
    if (!request) return SB_INVALID_ARGUMENT;
    *request = 0;
    if (!url || method < SB_HTTP_GET || method > SB_HTTP_DELETE) return SB_INVALID_ARGUMENT;
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!r.httpReady() || !Bind(*c, r)) return SB_UNAVAILABLE;
    *request = r.http->CreateHTTPRequest(static_cast<EHTTPMethod>(method), url);
    if (!*request) return SB_FAILED;
    c->requests.emplace(*request, Request{});
    return SB_OK;
}
SteamBridgeStatus SteamBridge_HTTPRelease(SteamBridgeContextHandle context, uint32_t request)
{
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    auto it = c->requests.find(request);
    if (it == c->requests.end()) return SB_INVALID_ARGUMENT;
    Retire(it->second.complete);
    c->requests.erase(it);
    Runtime r;
    if (!r.http || !Bind(*c, r)) return SB_UNAVAILABLE;
    return Result(r.http->ReleaseHTTPRequest(request));
}
SteamBridgeStatus SteamBridge_HTTPSend(SteamBridgeContextHandle context, uint32_t request, uint32_t stream, SteamBridgeHTTPCallback fn, void* user)
{
    Guard guard;
    auto  c = Context(context);
    if (!c || !fn) return SB_INVALID_ARGUMENT;
    auto it = c->requests.find(request);
    if (it == c->requests.end() || it->second.sent) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!r.httpReady() || !Bind(*c, r)) return SB_UNAVAILABLE;
    if (stream && !c->headers)
    {
        c->headers = Register(c, r, HTTPRequestHeadersReceived_t::k_iCallback, sizeof(HTTPRequestHeadersReceived_t));
        c->data    = Register(c, r, HTTPRequestDataReceived_t::k_iCallback, sizeof(HTTPRequestDataReceived_t));
    }
    SteamAPICall_t call{};
    if (!(stream ? r.http->SendHTTPRequestAndStreamResponse(request, &call) : r.http->SendHTTPRequest(request, &call)) || !call) return SB_FAILED;
    auto cb             = std::make_shared<Callback>(c, r, HTTPRequestCompleted_t::k_iCallback, sizeof(HTTPRequestCompleted_t));
    cb->call            = call;
    cb->request         = request;
    cb->active          = true;
    it->second.callback = fn;
    it->second.user     = user;
    it->second.sent     = true;
    it->second.complete = cb;
    r.registerResult(cb.get(), call);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_HTTPSetTimeout(SteamBridgeContextHandle c, uint32_t h, uint32_t seconds)
{
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.SetHTTPRequestNetworkActivityTimeout(h, seconds); });
}
SteamBridgeStatus SteamBridge_HTTPSetHeader(SteamBridgeContextHandle c, uint32_t h, const char* name, const char* value)
{
    if (!name || !value) return SB_INVALID_ARGUMENT;
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.SetHTTPRequestHeaderValue(h, name, value); });
}
SteamBridgeStatus SteamBridge_HTTPSetBody(SteamBridgeContextHandle c, uint32_t h, const char* type, const void* data, uint32_t size)
{
    if (!type || (!data && size)) return SB_INVALID_ARGUMENT;
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.SetHTTPRequestRawPostBody(h, type, (uint8*)data, size); });
}
SteamBridgeStatus SteamBridge_HTTPSetCertificateVerification(SteamBridgeContextHandle c, uint32_t h, uint32_t required)
{
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.SetHTTPRequestRequiresVerifiedCertificate(h, required != 0); });
}
SteamBridgeStatus SteamBridge_HTTPGetHeaderSize(SteamBridgeContextHandle c, uint32_t h, const char* name, uint32_t* size)
{
    if (!name || !size) return SB_INVALID_ARGUMENT;
    *size = 0;
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.GetHTTPResponseHeaderSize(h, name, size); });
}
SteamBridgeStatus SteamBridge_HTTPGetHeader(SteamBridgeContextHandle c, uint32_t h, const char* name, void* data, uint32_t size)
{
    if (!name || !data) return SB_INVALID_ARGUMENT;
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.GetHTTPResponseHeaderValue(h, name, (uint8*)data, size); });
}
SteamBridgeStatus SteamBridge_HTTPGetBodySize(SteamBridgeContextHandle c, uint32_t h, uint32_t* size)
{
    if (!size) return SB_INVALID_ARGUMENT;
    *size = 0;
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.GetHTTPResponseBodySize(h, size); });
}
SteamBridgeStatus SteamBridge_HTTPGetBody(SteamBridgeContextHandle c, uint32_t h, void* data, uint32_t size)
{
    if (!data && size) return SB_INVALID_ARGUMENT;
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.GetHTTPResponseBodyData(h, (uint8*)data, size); });
}
SteamBridgeStatus SteamBridge_HTTPGetStreamData(SteamBridgeContextHandle c, uint32_t h, uint32_t offset, void* data, uint32_t size)
{
    if (!data && size) return SB_INVALID_ARGUMENT;
    return HTTP(c, h, [&](ISteamHTTP& api) { return api.GetHTTPStreamingResponseBodyData(h, offset, (uint8*)data, size); });
}
SteamBridgeStatus SteamBridge_HTTPCreateCookies(SteamBridgeContextHandle context, uint32_t allow, uint32_t* cookies)
{
    if (!cookies) return SB_INVALID_ARGUMENT;
    *cookies = 0;
    Guard guard;
    auto  c = Context(context);
    if (!c) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!r.httpReady() || !Bind(*c, r)) return SB_UNAVAILABLE;
    *cookies = r.http->CreateCookieContainer(allow != 0);
    if (!*cookies) return SB_FAILED;
    c->cookies.insert(*cookies);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_HTTPReleaseCookies(SteamBridgeContextHandle context, uint32_t cookies)
{
    Guard guard;
    auto  c = Context(context);
    if (!c || !c->cookies.erase(cookies)) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!r.http || !Bind(*c, r)) return SB_UNAVAILABLE;
    return Result(r.http->ReleaseCookieContainer(cookies));
}
SteamBridgeStatus SteamBridge_HTTPSetCookies(SteamBridgeContextHandle context, uint32_t h, uint32_t cookies)
{
    Guard guard;
    auto  c = Context(context);
    if (!c || !c->cookies.contains(cookies)) return SB_INVALID_ARGUMENT;
    return HTTP(context, h, [&](ISteamHTTP& api) { return api.SetHTTPRequestCookieContainer(h, cookies); });
}
SteamBridgeStatus SteamBridge_HTTPSetCookie(SteamBridgeContextHandle context, uint32_t cookies, const char* host, const char* url, const char* cookie)
{
    if (!host || !url || !cookie) return SB_INVALID_ARGUMENT;
    Guard guard;
    auto  c = Context(context);
    if (!c || !c->cookies.contains(cookies)) return SB_INVALID_ARGUMENT;
    Runtime r;
    if (!r.httpReady() || !Bind(*c, r)) return SB_UNAVAILABLE;
    return Result(r.http->SetCookie(cookies, host, url, cookie));
}
