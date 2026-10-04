#include <steam/steam_api.h>
#include "LegacyInterfaces.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

// This fixture exports only old entry points OR the intermediate SteamInternal
// entry point. No fixture exports modern SteamAPI_SteamXXX_vNNN accessors.
static int mode;
static uint32 nextRequest = 10, nextCookie = 100;
static int certificate = -1;
static int cookieSets, released, registrations;
static std::vector<CCallbackBase*> callbacks;
static std::map<CCallbackBase*, SteamAPICall_t> results;
class User : public legacy::User016 {
    HSteamUser GetHSteamUser() override { return 1; }
    bool BLoggedOn() override { return true; }
    CSteamID GetSteamID() override { return CSteamID(uint64(76561198000000001)); }
} userAPI;
class Screenshots : public legacy::Screenshots001 {
    ScreenshotHandle WriteScreenshot(void*, uint32 size, int width, int height) override { return size == 3 && width == 1 && height == 1 ? 42 : 0; }
    ScreenshotHandle AddScreenshotToLibrary(const char*, const char*, int, int) override { return 43; }
    void TriggerScreenshot() override {}
    void HookScreenshots(bool) override {}
    bool SetLocation(ScreenshotHandle h, const char* s) override { return h == 42 && !strcmp(s,"test"); }
    bool TagUser(ScreenshotHandle h, CSteamID id) override { return h == 42 && id.ConvertToUint64() == 76561198000000001; }
} screenshotAPI;
class HTTP : public ISteamHTTP {
public:
    HTTPRequestHandle CreateHTTPRequest(EHTTPMethod, const char*) override { return ++nextRequest; }
    bool SetHTTPRequestContextValue(HTTPRequestHandle, uint64) override { return true; }
    bool SetHTTPRequestNetworkActivityTimeout(HTTPRequestHandle, uint32 seconds) override { return seconds != 0; }
    bool SetHTTPRequestHeaderValue(HTTPRequestHandle, const char*, const char*) override { return !(mode & 16); }
    bool SetHTTPRequestGetOrPostParameter(HTTPRequestHandle, const char*, const char*) override { return true; }
    bool SendHTTPRequest(HTTPRequestHandle h, SteamAPICall_t* call) override { *call = h + 1000; return !(mode & 8); }
    bool SendHTTPRequestAndStreamResponse(HTTPRequestHandle h, SteamAPICall_t* call) override { return SendHTTPRequest(h, call); }
    bool DeferHTTPRequest(HTTPRequestHandle) override { return true; }
    bool PrioritizeHTTPRequest(HTTPRequestHandle) override { return true; }
    bool GetHTTPResponseHeaderSize(HTTPRequestHandle, const char*, uint32* size) override { *size=3; return true; }
    bool GetHTTPResponseHeaderValue(HTTPRequestHandle, const char*, uint8* buffer, uint32 size) override { if(size<3) return false; memcpy(buffer,"ok",3); return true; }
    bool GetHTTPResponseBodySize(HTTPRequestHandle, uint32* size) override { *size=2; return true; }
    bool GetHTTPResponseBodyData(HTTPRequestHandle, uint8* data, uint32 size) override { if(size!=2) return false; memcpy(data,"ok",2); return true; }
    bool GetHTTPStreamingResponseBodyData(HTTPRequestHandle h, uint32, uint8* data, uint32 size) override { return GetHTTPResponseBodyData(h,data,size); }
    bool ReleaseHTTPRequest(HTTPRequestHandle) override { ++released; return true; }
    bool GetHTTPDownloadProgressPct(HTTPRequestHandle, float*) override { return true; }
    bool SetHTTPRequestRawPostBody(HTTPRequestHandle, const char*, uint8*, uint32) override { return true; }
    HTTPCookieContainerHandle CreateCookieContainer(bool) override { return ++nextCookie; }
    bool ReleaseCookieContainer(HTTPCookieContainerHandle) override { return true; }
    bool SetCookie(HTTPCookieContainerHandle, const char*, const char*, const char*) override { ++cookieSets; return true; }
    bool SetHTTPRequestCookieContainer(HTTPRequestHandle, HTTPCookieContainerHandle) override { ++cookieSets; return true; }
    bool SetHTTPRequestUserAgentInfo(HTTPRequestHandle, const char*) override { return true; }
    bool SetHTTPRequestRequiresVerifiedCertificate(HTTPRequestHandle, bool required) override { certificate=required; return true; }
    bool SetHTTPRequestAbsoluteTimeoutMS(HTTPRequestHandle, uint32) override { return true; }
    bool GetHTTPRequestWasTimedOut(HTTPRequestHandle, bool*) override { return true; }
} httpAPI;
static void* Find(const char* version) {
    if (!(mode & 1)) return nullptr;
    if (!strcmp(version,STEAMHTTP_INTERFACE_VERSION)) return mode & 2 ? &httpAPI : nullptr;
    if (!strcmp(version,"SteamUser016")) return &userAPI;
    if (!strcmp(version,"STEAMSCREENSHOTS_INTERFACE_VERSION001")) return &screenshotAPI;
    if (mode & 4) {
        if (!strcmp(version,STEAMUSER_INTERFACE_VERSION)) return &userAPI;
        if (!strcmp(version,STEAMSCREENSHOTS_INTERFACE_VERSION)) return &screenshotAPI;
    }
    return nullptr;
}
class Client : public legacy::Client012 {
    HSteamPipe CreateSteamPipe() override { return 0; }
    bool BReleaseSteamPipe(HSteamPipe) override { return false; }
    HSteamUser ConnectToGlobalUser(HSteamPipe) override { return 0; }
    HSteamUser CreateLocalUser(HSteamPipe*, EAccountType) override { return 0; }
    void ReleaseUser(HSteamPipe, HSteamUser) override {}
    void* GetISteamUser(HSteamUser, HSteamPipe, const char* v) override { return Find(v); }
    void* GetISteamGameServer(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void SetLocalIPBinding(uint32, uint16) override {}
    void* GetISteamFriends(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamUtils(HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamMatchmaking(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamMatchmakingServers(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamGenericInterface(HSteamUser, HSteamPipe, const char* v) override { return Find(v); }
    void* GetISteamUserStats(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamGameServerStats(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamApps(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamNetworking(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamRemoteStorage(HSteamUser, HSteamPipe, const char*) override { return nullptr; }
    void* GetISteamScreenshots(HSteamUser, HSteamPipe, const char* v) override { return Find(v); }
    void RunFrame() override {}
    uint32 GetIPCCallCount() override { return 0; }
    void SetWarningMessageHook(SteamAPIWarningMessageHook_t) override {}
    bool BShutdownIfAllPipesClosed() override { return false; }
    void* GetISteamHTTP(HSteamUser, HSteamPipe, const char* v) override { return Find(v); }
} client;
extern "C" {
int __cdecl MockGetUser() { return mode & 1 ? 1 : 0; }
int __cdecl MockGetPipe() { return mode & 1 ? 2 : 0; }
void* __cdecl MockClient() { return &client; }
void* __cdecl MockFind(int, const char* v) { return Find(v); }
void __cdecl MockRegister(CCallbackBase* cb, int) { callbacks.push_back(cb); ++registrations; }
void __cdecl MockUnregister(CCallbackBase* cb) { std::erase(callbacks,cb); }
void __cdecl MockRegisterResult(CCallbackBase* cb, SteamAPICall_t call) { results[cb]=call; }
void __cdecl MockUnregisterResult(CCallbackBase* cb, SteamAPICall_t) { results.erase(cb); }
__declspec(dllexport) void __cdecl MockMode(int value) { mode=value; }
__declspec(dllexport) uint32 __cdecl MockLastRequest() { return nextRequest; }
__declspec(dllexport) int __cdecl MockStat(int which) { switch(which) { case 0:return certificate; case 1:return cookieSets; case 2:return released; case 3:return int(callbacks.size()+results.size()); default:return registrations; } }
__declspec(dllexport) void __cdecl MockEmit(int kind, uint32 request) {
    if(kind==3 || kind==4) {
        HTTPRequestCompleted_t p{}; p.m_hRequest=request; p.m_bRequestSuccessful=true; p.m_eStatusCode=k_EHTTPStatusCode200OK; p.m_unBodySize=2;
        auto snapshot=results;
        for(auto [cb,call]:snapshot) if(call==request+1000 && results.contains(cb)) cb->Run(&p,kind==4,call);
        return;
    }
    HTTPRequestHeadersReceived_t header{}; header.m_hRequest=request;
    HTTPRequestDataReceived_t data{}; data.m_hRequest=request; data.m_cBytesReceived=2;
    ScreenshotReady_t shot{}; shot.m_hLocal=42; shot.m_eResult=k_EResultOK;
    auto snapshot=callbacks;
    int id=kind==1 ? HTTPRequestHeadersReceived_t::k_iCallback : kind==2 ? HTTPRequestDataReceived_t::k_iCallback : ScreenshotReady_t::k_iCallback;
    for(auto cb:snapshot) if(std::find(callbacks.begin(),callbacks.end(),cb)!=callbacks.end() && cb->GetICallback()==id)
        cb->Run(kind==1 ? (void*)&header : kind==2 ? (void*)&data : (void*)&shot);
}
}
