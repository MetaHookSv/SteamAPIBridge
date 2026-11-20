#include "Platform.h"
#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>
#include <SteamAPIBridge.h>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
using platform::Resolve;
using InitFlatFn         = ESteamAPIInitResult(S_CALLTYPE*)(SteamErrMsg*);
using GameServerInitFn   = ESteamAPIInitResult(S_CALLTYPE*)(uint32, uint16, uint16, EServerMode, const char*, const char*, SteamErrMsg*);
using VoidFn             = void(S_CALLTYPE*)();
using GetUserFn          = HSteamUser(S_CALLTYPE*)();
using FindInterfaceFn    = void*(S_CALLTYPE*)(HSteamUser, const char*);
using RegisterCallbackFn = void(S_CALLTYPE*)(CCallbackBase*, int);
using UnregisterFn       = void(S_CALLTYPE*)(CCallbackBase*);

// Keep in sync with SteamGameServer_InitEx in steam/steam_gameserver.h.
constexpr char GameServerInterfaceVersions[] =
    STEAMUTILS_INTERFACE_VERSION "\0" STEAMNETWORKINGUTILS_INTERFACE_VERSION "\0" STEAMGAMESERVER_INTERFACE_VERSION "\0" STEAMGAMESERVERSTATS_INTERFACE_VERSION "\0" STEAMHTTP_INTERFACE_VERSION "\0" STEAMINVENTORY_INTERFACE_VERSION "\0" STEAMNETWORKING_INTERFACE_VERSION "\0" STEAMNETWORKINGMESSAGES_INTERFACE_VERSION "\0" STEAMNETWORKINGSOCKETS_INTERFACE_VERSION "\0" STEAMUGC_INTERFACE_VERSION "\0"
                                 "\0";

struct HostCallback;
struct Host
{
    platform::Module                           module{};
    InitFlatFn                                 initFlat{};
    VoidFn                                     shutdown{}, runCallbacks{}, serverShutdown{}, serverRunCallbacks{};
    GetUserFn                                  getUser{}, serverGetUser{};
    FindInterfaceFn                            findUser{}, findServer{};
    GameServerInitFn                           serverInit{};
    RegisterCallbackFn                         registerCallback{};
    UnregisterFn                               unregisterCallback{};
    ISteamUser*                                user{};
    ISteamFriends*                             friends{};
    ISteamApps*                                apps{};
    ISteamGameServer*                          server{};
    bool                                       client{};
    SteamBridgeClientCallback                  clientFn{};
    void*                                      clientUser{};
    SteamBridgeServerCallback                  serverFn{};
    void*                                      serverUser{};
    std::vector<std::unique_ptr<HostCallback>> clientCallbacks, serverCallbacks;
    // Shutdown requested by a host callback runs after the native dispatcher returns.
    unsigned dispatchDepth{};
    bool     pendingServerShutdown{}, pendingHostShutdown{};
} host;

// Copies a truncated, NUL-terminated string; capacity must be nonzero.
void CopyText(char* destination, size_t capacity, const char* source)
{
    size_t length = strnlen(source, capacity - 1);
    std::memcpy(destination, source, length);
    destination[length] = 0;
}
template <size_t Size> void CopyText(char (&destination)[Size], const char* source) { CopyText(destination, Size, source); }
struct HostCallback final : CCallbackBase
{
    int size;
    HostCallback(int id, int bytes, bool gameServer) : size(bytes)
    {
        m_iCallback = id;
        if (gameServer) m_nCallbackFlags |= k_ECallbackFlagsGameServer;
    }
    int  GetCallbackSizeBytes() override { return size; }
    void Run(void* payload, bool, SteamAPICall_t) override { Run(payload); }
    void Run(void* payload) override
    {
        if (!payload) return;
        if (m_nCallbackFlags & k_ECallbackFlagsGameServer) RunServer(payload);
        else
            RunClient(payload);
    }
    void RunClient(void* payload)
    {
        SteamBridgeClientEvent event{};
        if (m_iCallback == ClientGameServerDeny_t::k_iCallback)
        {
            auto p           = static_cast<ClientGameServerDeny_t*>(payload);
            event.kind       = SB_CLIENT_GAMESERVER_DENY;
            event.serverIP   = p->m_unGameServerIP;
            event.serverPort = p->m_usGameServerPort;
            event.secure     = p->m_bSecure;
            event.reason     = p->m_uReason;
        }
        else if (m_iCallback == GameServerChangeRequested_t::k_iCallback)
        {
            auto p     = static_cast<GameServerChangeRequested_t*>(payload);
            event.kind = SB_CLIENT_GAMESERVER_CHANGE;
            CopyText(event.server, p->m_rgchServer);
            CopyText(event.password, p->m_rgchPassword);
        }
        else
        {
            event.kind   = SB_CLIENT_OVERLAY;
            event.active = static_cast<GameOverlayActivated_t*>(payload)->m_bActive != 0;
        }
        Dispatch(host.clientFn, host.clientUser, &event);
    }
    void RunServer(void* payload)
    {
        SteamBridgeServerEvent event{};
        switch (m_iCallback)
        {
            case SteamServersConnected_t::k_iCallback:
                event.kind = SB_SERVER_LOGON_SUCCESS;
                break;
            case SteamServerConnectFailure_t::k_iCallback:
                event.kind   = SB_SERVER_LOGON_FAILURE;
                event.result = static_cast<SteamServerConnectFailure_t*>(payload)->m_eResult;
                break;
            case GSPolicyResponse_t::k_iCallback:
                event.kind   = SB_SERVER_POLICY;
                event.secure = static_cast<GSPolicyResponse_t*>(payload)->m_bSecure != 0;
                break;
            case GSClientApprove_t::k_iCallback:
            {
                auto p             = static_cast<GSClientApprove_t*>(payload);
                event.kind         = SB_SERVER_CLIENT_APPROVE;
                event.steamID      = p->m_SteamID.ConvertToUint64();
                event.ownerSteamID = p->m_OwnerSteamID.ConvertToUint64();
                break;
            }
            case GSClientDeny_t::k_iCallback:
            {
                auto p        = static_cast<GSClientDeny_t*>(payload);
                event.kind    = SB_SERVER_CLIENT_DENY;
                event.steamID = p->m_SteamID.ConvertToUint64();
                event.reason  = p->m_eDenyReason;
                CopyText(event.text, p->m_rgchOptionalText);
                break;
            }
            default:
            {
                auto p        = static_cast<GSClientKick_t*>(payload);
                event.kind    = SB_SERVER_CLIENT_KICK;
                event.steamID = p->m_SteamID.ConvertToUint64();
                event.reason  = p->m_eDenyReason;
                break;
            }
        }
        Dispatch(host.serverFn, host.serverUser, &event);
    }
    template <class F, class E> static void Dispatch(F fn, void* user, const E* event)
    {
        if (!fn) return;
        ++host.dispatchDepth;
        // Exceptions must not cross the C callback boundary.
        try
        {
            fn(user, event);
        }
        catch (...)
        {}
        --host.dispatchDepth;
    }
};
template <class T> void Register(std::vector<std::unique_ptr<HostCallback>>& list, bool gameServer)
{
    auto callback = std::make_unique<HostCallback>(T::k_iCallback, int(sizeof(T)), gameServer);
    host.registerCallback(callback.get(), T::k_iCallback);
    list.push_back(std::move(callback));
}
void Unregister(std::vector<std::unique_ptr<HostCallback>>& list)
{
    for (auto& callback : list)
        host.unregisterCallback(callback.get());
    list.clear();
}
void ShutdownServer()
{
    if (!host.server) return;
    Unregister(host.serverCallbacks);
    host.server = nullptr;
    host.serverShutdown();
}
void ShutdownHost()
{
    if (!host.module) return;
    ShutdownServer();
    Unregister(host.clientCallbacks);
    if (host.client) host.shutdown();
    platform::UnloadRuntime(host.module);
    auto clientFn   = host.clientFn;
    auto clientUser = host.clientUser;
    auto serverFn   = host.serverFn;
    auto serverUser = host.serverUser;
    host            = Host{};
    host.clientFn   = clientFn;
    host.clientUser = clientUser;
    host.serverFn   = serverFn;
    host.serverUser = serverUser;
}
void RunPendingShutdown()
{
    if (host.dispatchDepth) return;
    if (host.pendingHostShutdown) ShutdownHost();
    else if (host.pendingServerShutdown)
        ShutdownServer();
    host.pendingHostShutdown   = false;
    host.pendingServerShutdown = false;
}
bool Resolved(const Host& h)
{
    return h.initFlat && h.shutdown && h.runCallbacks && h.getUser && h.findUser && h.serverInit && h.serverShutdown && h.serverRunCallbacks && h.serverGetUser && h.findServer && h.registerCallback && h.unregisterCallback;
}
SteamBridgeStatus                    Result(bool value) { return value ? SB_OK : SB_FAILED; }
template <class F> SteamBridgeStatus Server(F&& fn)
{
    if (!host.server) return SB_UNAVAILABLE;
    fn(*host.server);
    return SB_OK;
}
} // namespace

SteamBridgeStatus SteamBridge_HostInit(const char* runtimePath, uint32_t initializeClient)
{
    if (host.module) return SB_INVALID_ARGUMENT;
    try
    {
        auto module = platform::LoadRuntime(runtimePath);
        if (!module) return SB_UNAVAILABLE;
        Host h{};
        h.module             = module;
        h.initFlat           = Resolve<InitFlatFn>(module, "SteamAPI_InitFlat");
        h.shutdown           = Resolve<VoidFn>(module, "SteamAPI_Shutdown");
        h.runCallbacks       = Resolve<VoidFn>(module, "SteamAPI_RunCallbacks");
        h.getUser            = Resolve<GetUserFn>(module, "SteamAPI_GetHSteamUser");
        h.findUser           = Resolve<FindInterfaceFn>(module, "SteamInternal_FindOrCreateUserInterface");
        h.serverInit         = Resolve<GameServerInitFn>(module, "SteamInternal_GameServer_Init_V2");
        h.serverShutdown     = Resolve<VoidFn>(module, "SteamGameServer_Shutdown");
        h.serverRunCallbacks = Resolve<VoidFn>(module, "SteamGameServer_RunCallbacks");
        h.serverGetUser      = Resolve<GetUserFn>(module, "SteamGameServer_GetHSteamUser");
        h.findServer         = Resolve<FindInterfaceFn>(module, "SteamInternal_FindOrCreateGameServerInterface");
        h.registerCallback   = Resolve<RegisterCallbackFn>(module, "SteamAPI_RegisterCallback");
        h.unregisterCallback = Resolve<UnregisterFn>(module, "SteamAPI_UnregisterCallback");
        if (!Resolved(h))
        {
            platform::UnloadRuntime(module);
            return SB_UNAVAILABLE;
        }
        if (initializeClient)
        {
            SteamErrMsg message{};
            if (h.initFlat(&message) != k_ESteamAPIInitResult_OK)
            {
                platform::UnloadRuntime(module);
                return SB_FAILED;
            }
            HSteamUser steamUser = h.getUser();
            h.client             = true;
            h.user               = static_cast<ISteamUser*>(h.findUser(steamUser, STEAMUSER_INTERFACE_VERSION));
            h.friends            = static_cast<ISteamFriends*>(h.findUser(steamUser, STEAMFRIENDS_INTERFACE_VERSION));
            h.apps               = static_cast<ISteamApps*>(h.findUser(steamUser, STEAMAPPS_INTERFACE_VERSION));
        }
        h.clientFn   = host.clientFn;
        h.clientUser = host.clientUser;
        h.serverFn   = host.serverFn;
        h.serverUser = host.serverUser;
        host         = std::move(h);
        if (host.client)
        {
            Register<ClientGameServerDeny_t>(host.clientCallbacks, false);
            Register<GameServerChangeRequested_t>(host.clientCallbacks, false);
            Register<GameOverlayActivated_t>(host.clientCallbacks, false);
        }
        return SB_OK;
    }
    catch (...)
    {
        ShutdownHost();
        return SB_FAILED;
    }
}
void SteamBridge_HostShutdown()
{
    host.pendingHostShutdown = true;
    RunPendingShutdown();
}
uint32_t SteamBridge_HostIsActive() { return host.module != nullptr; }
uint32_t SteamBridge_HostIsClientActive() { return host.client; }
void     SteamBridge_HostRunCallbacks()
{
    if (!host.client) return;
    ++host.dispatchDepth;
    host.runCallbacks();
    --host.dispatchDepth;
    RunPendingShutdown();
}
void SteamBridge_HostSetClientCallback(SteamBridgeClientCallback callback, void* user)
{
    host.clientFn   = callback;
    host.clientUser = user;
}

SteamBridgeStatus SteamBridge_UserInitiateGameConnection(void* ticket, uint32_t capacity, uint64_t serverSteamID, uint32_t serverIP, uint16_t serverPort, uint32_t secure, uint32_t* size)
{
    if (!size) return SB_INVALID_ARGUMENT;
    *size = 0;
    if (!ticket || !capacity) return SB_INVALID_ARGUMENT;
    if (!host.user) return SB_UNAVAILABLE;
    int bytes = host.user->InitiateGameConnection_DEPRECATED(ticket, int(capacity), CSteamID(uint64(serverSteamID)), serverIP, serverPort, secure != 0);
    if (bytes <= 0 || uint32_t(bytes) > capacity) return SB_FAILED;
    *size = uint32_t(bytes);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_UserTerminateGameConnection(uint32_t serverIP, uint16_t serverPort)
{
    if (!host.user) return SB_UNAVAILABLE;
    host.user->TerminateGameConnection_DEPRECATED(serverIP, serverPort);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_UserAdvertiseGame(uint64_t serverSteamID, uint32_t serverIP, uint16_t serverPort)
{
    if (!host.user) return SB_UNAVAILABLE;
    host.user->AdvertiseGame(CSteamID(uint64(serverSteamID)), serverIP, serverPort);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_UserGetSteamID(uint64_t* id)
{
    if (!id) return SB_INVALID_ARGUMENT;
    *id = 0;
    if (!host.user) return SB_UNAVAILABLE;
    *id = host.user->GetSteamID().ConvertToUint64();
    return *id ? SB_OK : SB_UNAVAILABLE;
}
SteamBridgeStatus SteamBridge_UserGetPersonaName(char* name, uint32_t capacity)
{
    if (!name || !capacity) return SB_INVALID_ARGUMENT;
    name[0] = 0;
    if (!host.friends) return SB_UNAVAILABLE;
    const char* persona = host.friends->GetPersonaName();
    if (!persona) return SB_FAILED;
    CopyText(name, capacity, persona);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_UserIsSubscribedApp(uint32_t appID, uint32_t* subscribed)
{
    if (!subscribed) return SB_INVALID_ARGUMENT;
    *subscribed = 0;
    if (!host.apps) return SB_UNAVAILABLE;
    *subscribed = host.apps->BIsSubscribedApp(appID);
    return SB_OK;
}

SteamBridgeStatus SteamBridge_VoiceStartRecording()
{
    if (!host.user) return SB_UNAVAILABLE;
    host.user->StartVoiceRecording();
    return SB_OK;
}
SteamBridgeStatus SteamBridge_VoiceStopRecording()
{
    if (!host.user) return SB_UNAVAILABLE;
    host.user->StopVoiceRecording();
    return SB_OK;
}
SteamBridgeStatus SteamBridge_VoiceGetAvailable(uint32_t* compressedBytes)
{
    if (!compressedBytes) return SB_INVALID_ARGUMENT;
    *compressedBytes = 0;
    if (!host.user) return SB_UNAVAILABLE;
    uint32 bytes{};
    if (host.user->GetAvailableVoice(&bytes) != k_EVoiceResultOK) return SB_FAILED;
    *compressedBytes = bytes;
    return SB_OK;
}
SteamBridgeStatus SteamBridge_VoiceGet(void* compressed, uint32_t capacity, uint32_t* compressedBytes, void* pcm, uint32_t pcmCapacity, uint32_t* pcmBytes, uint32_t pcmSampleRate)
{
    if (!compressedBytes || !pcmBytes) return SB_INVALID_ARGUMENT;
    *compressedBytes = 0;
    *pcmBytes        = 0;
    if (!compressed || !capacity || (pcm && !pcmCapacity)) return SB_INVALID_ARGUMENT;
    if (!host.user) return SB_UNAVAILABLE;
    uint32 written{}, pcmWritten{};
    bool   wantPCM = pcm != nullptr;
    if (host.user->GetVoice(true, compressed, capacity, &written, wantPCM, pcm, wantPCM ? pcmCapacity : 0, wantPCM ? &pcmWritten : nullptr, wantPCM ? pcmSampleRate : 0) != k_EVoiceResultOK) return SB_FAILED;
    if (written > capacity || pcmWritten > pcmCapacity) return SB_FAILED;
    *compressedBytes = written;
    *pcmBytes        = pcmWritten;
    return SB_OK;
}
SteamBridgeStatus SteamBridge_VoiceDecompress(const void* compressed, uint32_t bytes, void* pcm, uint32_t pcmCapacity, uint32_t* pcmBytes, uint32_t sampleRate)
{
    if (!pcmBytes) return SB_INVALID_ARGUMENT;
    *pcmBytes = 0;
    if (!compressed || !bytes || !pcm || !pcmCapacity) return SB_INVALID_ARGUMENT;
    if (!host.user) return SB_UNAVAILABLE;
    uint32 written{};
    if (host.user->DecompressVoice(compressed, bytes, pcm, pcmCapacity, &written, sampleRate) != k_EVoiceResultOK || written > pcmCapacity) return SB_FAILED;
    *pcmBytes = written;
    return SB_OK;
}
uint32_t SteamBridge_VoiceGetOptimalSampleRate() { return host.user ? host.user->GetVoiceOptimalSampleRate() : 0; }

SteamBridgeStatus SteamBridge_ServerInit(uint32_t serverIP, uint16_t gamePort, uint16_t queryPort, int32_t mode, const char* version)
{
    if (!version || mode < SB_SERVER_MODE_NO_AUTHENTICATION || mode > SB_SERVER_MODE_SECURE) return SB_INVALID_ARGUMENT;
    if (!host.module) return SB_UNAVAILABLE;
    if (host.server) return SB_INVALID_ARGUMENT;
    SteamErrMsg message{};
    if (host.serverInit(serverIP, gamePort, queryPort, static_cast<EServerMode>(mode), version, GameServerInterfaceVersions, &message) != k_ESteamAPIInitResult_OK) return SB_FAILED;
    auto server = static_cast<ISteamGameServer*>(host.findServer(host.serverGetUser(), STEAMGAMESERVER_INTERFACE_VERSION));
    if (!server)
    {
        host.serverShutdown();
        return SB_FAILED;
    }
    try
    {
        Register<SteamServersConnected_t>(host.serverCallbacks, true);
        Register<SteamServerConnectFailure_t>(host.serverCallbacks, true);
        Register<GSPolicyResponse_t>(host.serverCallbacks, true);
        Register<GSClientApprove_t>(host.serverCallbacks, true);
        Register<GSClientDeny_t>(host.serverCallbacks, true);
        Register<GSClientKick_t>(host.serverCallbacks, true);
    }
    catch (...)
    {
        Unregister(host.serverCallbacks);
        host.serverShutdown();
        return SB_FAILED;
    }
    host.server = server;
    return SB_OK;
}
void SteamBridge_ServerShutdown()
{
    host.pendingServerShutdown = true;
    RunPendingShutdown();
}
uint32_t SteamBridge_ServerIsActive() { return host.server != nullptr; }
void     SteamBridge_ServerRunCallbacks()
{
    if (!host.server) return;
    ++host.dispatchDepth;
    host.serverRunCallbacks();
    --host.dispatchDepth;
    RunPendingShutdown();
}
void SteamBridge_ServerSetCallback(SteamBridgeServerCallback callback, void* user)
{
    host.serverFn   = callback;
    host.serverUser = user;
}
SteamBridgeStatus SteamBridge_ServerSetProduct(const char* product)
{
    if (!product) return SB_INVALID_ARGUMENT;
    return Server([&](ISteamGameServer& api) { api.SetProduct(product); });
}
SteamBridgeStatus SteamBridge_ServerSetModDir(const char* modDir)
{
    if (!modDir) return SB_INVALID_ARGUMENT;
    return Server([&](ISteamGameServer& api) { api.SetModDir(modDir); });
}
SteamBridgeStatus SteamBridge_ServerSetDedicatedServer(uint32_t dedicated)
{
    return Server([&](ISteamGameServer& api) { api.SetDedicatedServer(dedicated != 0); });
}
SteamBridgeStatus SteamBridge_ServerSetGameDescription(const char* description)
{
    if (!description) return SB_INVALID_ARGUMENT;
    return Server([&](ISteamGameServer& api) { api.SetGameDescription(description); });
}
SteamBridgeStatus SteamBridge_ServerLogOnAnonymous()
{
    return Server([](ISteamGameServer& api) { api.LogOnAnonymous(); });
}
SteamBridgeStatus SteamBridge_ServerLogOff()
{
    return Server([](ISteamGameServer& api) { api.LogOff(); });
}
uint32_t          SteamBridge_ServerIsLoggedOn() { return host.server && host.server->BLoggedOn(); }
uint32_t          SteamBridge_ServerIsSecure() { return host.server && host.server->BSecure(); }
uint64_t          SteamBridge_ServerGetSteamID() { return host.server ? host.server->GetSteamID().ConvertToUint64() : 0; }
uint32_t          SteamBridge_ServerWasRestartRequested() { return host.server && host.server->WasRestartRequested(); }
SteamBridgeStatus SteamBridge_ServerSetAdvertiseServerActive(uint32_t active)
{
    return Server([&](ISteamGameServer& api) { api.SetAdvertiseServerActive(active != 0); });
}
SteamBridgeStatus SteamBridge_ServerSetMaxPlayerCount(int32_t players)
{
    return Server([&](ISteamGameServer& api) { api.SetMaxPlayerCount(players); });
}
SteamBridgeStatus SteamBridge_ServerSetBotPlayerCount(int32_t bots)
{
    return Server([&](ISteamGameServer& api) { api.SetBotPlayerCount(bots); });
}
SteamBridgeStatus SteamBridge_ServerSetServerName(const char* name)
{
    if (!name) return SB_INVALID_ARGUMENT;
    return Server([&](ISteamGameServer& api) { api.SetServerName(name); });
}
SteamBridgeStatus SteamBridge_ServerSetMapName(const char* map)
{
    if (!map) return SB_INVALID_ARGUMENT;
    return Server([&](ISteamGameServer& api) { api.SetMapName(map); });
}
SteamBridgeStatus SteamBridge_ServerSetPasswordProtected(uint32_t passwordProtected)
{
    return Server([&](ISteamGameServer& api) { api.SetPasswordProtected(passwordProtected != 0); });
}
SteamBridgeStatus SteamBridge_ServerClearAllKeyValues()
{
    return Server([](ISteamGameServer& api) { api.ClearAllKeyValues(); });
}
SteamBridgeStatus SteamBridge_ServerSetKeyValue(const char* key, const char* value)
{
    if (!key || !value) return SB_INVALID_ARGUMENT;
    return Server([&](ISteamGameServer& api) { api.SetKeyValue(key, value); });
}
SteamBridgeStatus SteamBridge_ServerSendUserConnectAndAuthenticate(uint32_t clientIP, const void* ticket, uint32_t bytes, uint64_t* steamID)
{
    if (!steamID) return SB_INVALID_ARGUMENT;
    *steamID = 0;
    if (!ticket || !bytes) return SB_INVALID_ARGUMENT;
    if (!host.server) return SB_UNAVAILABLE;
    CSteamID id;
    bool     accepted = host.server->SendUserConnectAndAuthenticate_DEPRECATED(clientIP, ticket, bytes, &id);
    *steamID          = id.ConvertToUint64();
    return Result(accepted);
}
SteamBridgeStatus SteamBridge_ServerCreateUnauthenticatedUserConnection(uint64_t* steamID)
{
    if (!steamID) return SB_INVALID_ARGUMENT;
    *steamID = 0;
    if (!host.server) return SB_UNAVAILABLE;
    *steamID = host.server->CreateUnauthenticatedUserConnection().ConvertToUint64();
    return Result(*steamID != 0);
}
SteamBridgeStatus SteamBridge_ServerSendUserDisconnect(uint64_t steamID)
{
    return Server([&](ISteamGameServer& api) { api.SendUserDisconnect_DEPRECATED(CSteamID(uint64(steamID))); });
}
SteamBridgeStatus SteamBridge_ServerUpdateUserData(uint64_t steamID, const char* name, uint32_t score)
{
    if (!name) return SB_INVALID_ARGUMENT;
    if (!host.server) return SB_UNAVAILABLE;
    return Result(host.server->BUpdateUserData(CSteamID(uint64(steamID)), name, score));
}
SteamBridgeStatus SteamBridge_ServerHandleIncomingPacket(const void* data, uint32_t bytes, uint32_t ip, uint16_t port)
{
    if (!data || !bytes) return SB_INVALID_ARGUMENT;
    if (!host.server) return SB_UNAVAILABLE;
    return Result(host.server->HandleIncomingPacket(data, int(bytes), ip, port));
}
uint32_t SteamBridge_ServerGetNextOutgoingPacket(void* data, uint32_t capacity, uint32_t* ip, uint16_t* port)
{
    if (!data || !capacity || !ip || !port || !host.server) return 0;
    *ip       = 0;
    *port     = 0;
    int bytes = host.server->GetNextOutgoingPacket(data, int(capacity), ip, port);
    return bytes > 0 && uint32_t(bytes) <= capacity ? uint32_t(bytes) : 0;
}
