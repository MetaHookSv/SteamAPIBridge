#include "Platform.h"
#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>
#include <steam/steamnetworkingfakeip.h>
#include <SteamAPIBridge.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
#include <vector>

// The object behind a public FakeIP port handle.
struct SteamBridgeFakeUDPPort
{
    ISteamNetworkingFakeUDPPort* port;
    bool                         gameServer;
};
// The object behind a public server list handle; Steam answers it until it is released.
struct SteamBridgeServerList final : ISteamMatchmakingServerListResponse
{
    HServerListRequest         native{};
    SteamBridgeServersCallback callback{};
    void*                      user{};
    bool                       released{};
    void                       ServerResponded(HServerListRequest request, int index) override;
    void                       ServerFailedToRespond(HServerListRequest request, int index) override;
    void                       RefreshComplete(HServerListRequest request, EMatchMakingServerResponse response) override;
};

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

// A ping or player query; Steam answers it until it ends or is cancelled.
struct ServerQuery final : ISteamMatchmakingPingResponse, ISteamMatchmakingPlayersResponse
{
    uint32_t                   id{};
    HServerQuery               native = HSERVERQUERY_INVALID;
    SteamBridgeServersCallback callback{};
    void*                      user{};
    // ended: no more events. cancelled: Steam still has to cancel the native query.
    bool ended{}, cancelled{};
    void ServerResponded(gameserveritem_t& server) override;
    void ServerFailedToRespond() override;
    void AddPlayerToList(const char* name, int score, float seconds) override;
    void PlayersFailedToRespond() override;
    void PlayersRefreshComplete() override;
    void Dispatch(uint32_t kind, SteamBridgeServersEvent& event, bool end);
};

struct HostCallback;
struct Host
{
    platform::Module               module{};
    InitFlatFn                     initFlat{};
    VoidFn                         shutdown{}, runCallbacks{}, serverShutdown{}, serverRunCallbacks{};
    GetUserFn                      getUser{}, serverGetUser{};
    FindInterfaceFn                findUser{}, findServer{};
    GameServerInitFn               serverInit{};
    RegisterCallbackFn             registerCallback{};
    UnregisterFn                   unregisterCallback{};
    ISteamUser*                    user{};
    ISteamFriends*                 friends{};
    ISteamUtils*                   steamUtils{};
    ISteamApps*                    apps{};
    ISteamMatchmaking*             matchmaking{};
    ISteamMatchmakingServers*      servers{};
    ISteamGameServer*              server{};
    bool                           client{};
    SteamBridgeClientCallback      clientFn{};
    void*                          clientUser{};
    SteamBridgeMatchmakingCallback matchmakingFn{};
    void*                          matchmakingUser{};
    // Server list requests and server queries; released and ended ones are retired outside dispatch.
    std::vector<std::unique_ptr<SteamBridgeServerList>> serverLists;
    std::vector<std::unique_ptr<ServerQuery>>           serverQueries;
    uint32_t                                            lastServerQuery{};
    SteamBridgeServerCallback                           serverFn{};
    void*                                               serverUser{};
    std::vector<std::unique_ptr<HostCallback>>          clientCallbacks, serverCallbacks;
    // Shutdown requested by a host callback runs after the native dispatcher returns.
    unsigned dispatchDepth{};
    bool     pendingServerShutdown{}, pendingHostShutdown{};
    // FakeIP networking. utils is the client's, or the game server's when no client is initialized.
    ISteamNetworkingSockets*                             sockets{};
    ISteamNetworkingSockets*                             serverSockets{};
    ISteamNetworkingUtils*                               utils{};
    bool                                                 utilsFromServer{};
    std::vector<std::unique_ptr<SteamBridgeFakeUDPPort>> ports;
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
        else if (m_iCallback == FavoritesListChanged_t::k_iCallback || m_iCallback == PersonaStateChange_t::k_iCallback)
            RunMatchmaking(payload);
        else
            RunClient(payload);
    }
    void RunMatchmaking(void* payload)
    {
        SteamBridgeMatchmakingEvent event{};
        if (m_iCallback == FavoritesListChanged_t::k_iCallback)
        {
            auto p               = static_cast<FavoritesListChanged_t*>(payload);
            event.kind           = SB_MATCHMAKING_FAVORITES_CHANGED;
            event.ip             = p->m_nIP;
            event.queryPort      = p->m_nQueryPort;
            event.connectionPort = p->m_nConnPort;
            event.appID          = p->m_nAppID;
            event.flags          = p->m_nFlags;
            event.add            = p->m_bAdd;
            event.accountID      = p->m_unAccountId;
        }
        else
        {
            auto p            = static_cast<PersonaStateChange_t*>(payload);
            event.kind        = SB_MATCHMAKING_PERSONA_CHANGE;
            event.steamID     = p->m_ulSteamID;
            event.changeFlags = p->m_nChangeFlags;
        }
        Dispatch(host.matchmakingFn, host.matchmakingUser, &event);
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
            case SteamNetworkingFakeIPResult_t::k_iCallback:
                event.kind   = SB_SERVER_FAKE_IP_RESULT;
                event.result = static_cast<SteamNetworkingFakeIPResult_t*>(payload)->m_eResult;
                break;
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
// Destroys the FakeIP ports a session still holds before the session shuts down.
void DestroyPorts(bool gameServer)
{
    std::erase_if(host.ports, [&](const std::unique_ptr<SteamBridgeFakeUDPPort>& port) {
        if (port->gameServer != gameServer) return false;
        port->port->DestroyFakeUDPPort();
        return true;
    });
}
// Releases the server lists and cancels the server queries that stopped answering, outside dispatch.
void RetireServerQueries()
{
    if (host.dispatchDepth) return;
    std::erase_if(host.serverLists, [](const std::unique_ptr<SteamBridgeServerList>& list) {
        if (!list->released) return false;
        host.servers->ReleaseRequest(list->native);
        return true;
    });
    std::erase_if(host.serverQueries, [](const std::unique_ptr<ServerQuery>& query) {
        if (!query->ended) return false;
        if (query->cancelled) host.servers->CancelServerQuery(query->native);
        return true;
    });
}
void ShutdownServer()
{
    if (!host.server) return;
    DestroyPorts(true);
    Unregister(host.serverCallbacks);
    host.server        = nullptr;
    host.serverSockets = nullptr;
    if (host.utilsFromServer)
    {
        host.utils           = nullptr;
        host.utilsFromServer = false;
    }
    host.serverShutdown();
}
void ShutdownHost()
{
    if (!host.module) return;
    ShutdownServer();
    DestroyPorts(false);
    for (auto& list : host.serverLists)
        list->released = true;
    for (auto& query : host.serverQueries)
    {
        query->cancelled = query->cancelled || !query->ended;
        query->ended     = true;
    }
    RetireServerQueries();
    Unregister(host.clientCallbacks);
    if (host.client) host.shutdown();
    platform::UnloadRuntime(host.module);
    auto clientFn        = host.clientFn;
    auto clientUser      = host.clientUser;
    auto matchmakingFn   = host.matchmakingFn;
    auto matchmakingUser = host.matchmakingUser;
    auto serverFn        = host.serverFn;
    auto serverUser      = host.serverUser;
    host                 = Host{};
    host.clientFn        = clientFn;
    host.clientUser      = clientUser;
    host.matchmakingFn   = matchmakingFn;
    host.matchmakingUser = matchmakingUser;
    host.serverFn        = serverFn;
    host.serverUser      = serverUser;
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
SteamNetworkingIPAddr IPv4(uint32_t ip, uint16_t port)
{
    SteamNetworkingIPAddr address;
    address.SetIPv4(ip, port);
    return address;
}
// Steam calls the debug output function without user data, so the consumer's callback is global.
std::atomic<SteamBridgeNetworkingDebugCallback> networkingDebugOutput{};
void                                            NetworkingDebugOutput(ESteamNetworkingSocketsDebugOutputType level, const char* message)
{
    auto callback = networkingDebugOutput.load();
    if (!callback || !message) return;
    // Exceptions must not cross the C callback boundary.
    try
    {
        callback(int32_t(level), message);
    }
    catch (...)
    {}
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
            h.matchmaking        = static_cast<ISteamMatchmaking*>(h.findUser(steamUser, STEAMMATCHMAKING_INTERFACE_VERSION));
            h.servers            = static_cast<ISteamMatchmakingServers*>(h.findUser(steamUser, STEAMMATCHMAKINGSERVERS_INTERFACE_VERSION));
            h.sockets            = static_cast<ISteamNetworkingSockets*>(h.findUser(steamUser, STEAMNETWORKINGSOCKETS_INTERFACE_VERSION));
            // The networking utilities are global, not per user (SteamNetworkingUtils_SteamAPI).
            h.utils = static_cast<ISteamNetworkingUtils*>(h.findUser(0, STEAMNETWORKINGUTILS_INTERFACE_VERSION));
            // So are the Steam utilities (SteamUtils).
            h.steamUtils = static_cast<ISteamUtils*>(h.findUser(0, STEAMUTILS_INTERFACE_VERSION));
        }
        h.clientFn        = host.clientFn;
        h.clientUser      = host.clientUser;
        h.matchmakingFn   = host.matchmakingFn;
        h.matchmakingUser = host.matchmakingUser;
        h.serverFn        = host.serverFn;
        h.serverUser      = host.serverUser;
        host              = std::move(h);
        if (host.client)
        {
            Register<ClientGameServerDeny_t>(host.clientCallbacks, false);
            Register<GameServerChangeRequested_t>(host.clientCallbacks, false);
            Register<GameOverlayActivated_t>(host.clientCallbacks, false);
            Register<FavoritesListChanged_t>(host.clientCallbacks, false);
            Register<PersonaStateChange_t>(host.clientCallbacks, false);
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
    RetireServerQueries();
}
void SteamBridge_HostSetClientCallback(SteamBridgeClientCallback callback, void* user)
{
    host.clientFn   = callback;
    host.clientUser = user;
}
void SteamBridge_HostSetMatchmakingCallback(SteamBridgeMatchmakingCallback callback, void* user)
{
    host.matchmakingFn   = callback;
    host.matchmakingUser = user;
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
SteamBridgeStatus SteamBridge_UserGetCurrentGameLanguage(char* language, uint32_t capacity)
{
    if (!language || !capacity) return SB_INVALID_ARGUMENT;
    language[0] = 0;
    if (!host.apps) return SB_UNAVAILABLE;
    const char* current = host.apps->GetCurrentGameLanguage();
    if (!current) return SB_FAILED;
    CopyText(language, capacity, current);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_UserIsLowViolence(uint32_t* lowViolence)
{
    if (!lowViolence) return SB_INVALID_ARGUMENT;
    *lowViolence = 0;
    if (!host.apps) return SB_UNAVAILABLE;
    *lowViolence = host.apps->BIsLowViolence();
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
        Register<SteamNetworkingFakeIPResult_t>(host.serverCallbacks, true);
    }
    catch (...)
    {
        Unregister(host.serverCallbacks);
        host.serverShutdown();
        return SB_FAILED;
    }
    host.server        = server;
    host.serverSockets = static_cast<ISteamNetworkingSockets*>(host.findServer(host.serverGetUser(), STEAMNETWORKINGSOCKETS_INTERFACE_VERSION));
    if (!host.utils)
    {
        host.utils           = static_cast<ISteamNetworkingUtils*>(host.findServer(0, STEAMNETWORKINGUTILS_INTERFACE_VERSION));
        host.utilsFromServer = host.utils != nullptr;
    }
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
SteamBridgeStatus SteamBridge_ServerBeginAsyncRequestFakeIP(int32_t ports)
{
    if (ports <= 0) return SB_INVALID_ARGUMENT;
    if (!host.serverSockets) return SB_UNAVAILABLE;
    return Result(host.serverSockets->BeginAsyncRequestFakeIP(ports));
}
SteamBridgeStatus SteamBridge_ServerGetFakeIP(int32_t firstPort, int32_t* result, uint32_t* ip, uint16_t* ports, uint32_t count)
{
    if (!result || !ip || (count && !ports)) return SB_INVALID_ARGUMENT;
    *result = 0;
    *ip     = 0;
    std::fill_n(ports, count, uint16_t(0));
    if (firstPort < 0) return SB_INVALID_ARGUMENT;
    if (!host.serverSockets) return SB_UNAVAILABLE;
    SteamNetworkingFakeIPResult_t info{};
    host.serverSockets->GetFakeIP(firstPort, &info);
    *result = info.m_eResult;
    if (info.m_eResult != k_EResultOK) return SB_OK;
    *ip = info.m_unIP;
    std::copy_n(info.m_unPorts, std::min<uint32_t>(count, SteamNetworkingFakeIPResult_t::k_nMaxReturnPorts), ports);
    return SB_OK;
}

SteamBridgeStatus SteamBridge_FakeUDPPortCreate(uint32_t gameServer, int32_t fakeServerPort, SteamBridgeFakeUDPPortHandle* port)
{
    if (!port) return SB_INVALID_ARGUMENT;
    *port = nullptr;
    if (fakeServerPort < -1) return SB_INVALID_ARGUMENT;
    ISteamNetworkingSockets* sockets = gameServer ? host.serverSockets : host.sockets;
    if (!sockets) return SB_UNAVAILABLE;
    ISteamNetworkingFakeUDPPort* native = sockets->CreateFakeUDPPort(fakeServerPort);
    if (!native) return SB_FAILED;
    try
    {
        host.ports.push_back(std::make_unique<SteamBridgeFakeUDPPort>(SteamBridgeFakeUDPPort{native, gameServer != 0}));
    }
    catch (...)
    {
        native->DestroyFakeUDPPort();
        return SB_FAILED;
    }
    *port = host.ports.back().get();
    return SB_OK;
}
void SteamBridge_FakeUDPPortDestroy(SteamBridgeFakeUDPPortHandle port)
{
    auto found = std::find_if(host.ports.begin(), host.ports.end(), [&](const std::unique_ptr<SteamBridgeFakeUDPPort>& owned) { return owned.get() == port; });
    if (found == host.ports.end()) return;
    (*found)->port->DestroyFakeUDPPort();
    host.ports.erase(found);
}
SteamBridgeStatus SteamBridge_FakeUDPPortSend(SteamBridgeFakeUDPPortHandle port, uint32_t ip, uint16_t remotePort, const void* data, uint32_t bytes, int32_t flags, int32_t* result)
{
    if (!result) return SB_INVALID_ARGUMENT;
    *result = 0;
    if (!port || !data || !bytes) return SB_INVALID_ARGUMENT;
    *result = port->port->SendMessageToFakeIP(IPv4(ip, remotePort), data, bytes, flags);
    return *result == k_EResultOK ? SB_OK : SB_FAILED;
}
SteamBridgeStatus SteamBridge_FakeUDPPortReceive(SteamBridgeFakeUDPPortHandle port, void* data, uint32_t capacity, SteamBridgeFakeUDPMessage* message)
{
    if (!message) return SB_INVALID_ARGUMENT;
    *message = {};
    if (!port || (capacity && !data)) return SB_INVALID_ARGUMENT;
    SteamNetworkingMessage_t* received{};
    if (port->port->ReceiveMessages(&received, 1) != 1 || !received) return SB_OK;
    message->bytes                       = uint32_t(received->m_cbSize);
    const SteamNetworkingIPAddr* address = received->m_identityPeer.GetIPAddr();
    if (!address)
    {
        message->kind = SB_FAKE_UDP_NOT_IP;
        if (host.utils) host.utils->SteamNetworkingIdentity_ToString(received->m_identityPeer, message->peer, sizeof(message->peer));
    }
    else if (message->bytes > capacity)
    {
        message->kind = SB_FAKE_UDP_TOO_LARGE;
        if (host.utils) host.utils->SteamNetworkingIPAddr_ToString(*address, message->peer, sizeof(message->peer), true);
    }
    else
    {
        message->kind = SB_FAKE_UDP_MESSAGE;
        message->ip   = address->GetIPv4();
        message->port = address->m_port;
        std::memcpy(data, received->m_pData, message->bytes);
    }
    received->Release();
    return SB_OK;
}
SteamBridgeStatus SteamBridge_FakeUDPPortScheduleCleanup(SteamBridgeFakeUDPPortHandle port, uint32_t ip, uint16_t remotePort)
{
    if (!port) return SB_INVALID_ARGUMENT;
    port->port->ScheduleCleanup(IPv4(ip, remotePort));
    return SB_OK;
}
uint32_t          SteamBridge_IsFakeIPv4(uint32_t ip) { return host.utils && host.utils->GetIPv4FakeIPType(ip) > k_ESteamNetworkingFakeIPType_NotFake; }
SteamBridgeStatus SteamBridge_NetworkingSetDebugOutput(int32_t level, SteamBridgeNetworkingDebugCallback callback)
{
    if (level < k_ESteamNetworkingSocketsDebugOutputType_None || level > k_ESteamNetworkingSocketsDebugOutputType_Everything) return SB_INVALID_ARGUMENT;
    if (!host.utils) return SB_UNAVAILABLE;
    networkingDebugOutput = callback;
    host.utils->SetDebugOutputFunction(callback ? ESteamNetworkingSocketsDebugOutputType(level) : k_ESteamNetworkingSocketsDebugOutputType_None, callback ? NetworkingDebugOutput : nullptr);
    return SB_OK;
}

namespace
{
void CopyServer(SteamBridgeServerItem& item, const gameserveritem_t& server)
{
    item                       = {};
    item.ip                    = server.m_NetAdr.GetIP();
    item.queryPort             = server.m_NetAdr.GetQueryPort();
    item.connectionPort        = server.m_NetAdr.GetConnectionPort();
    item.ping                  = server.m_nPing;
    item.hadSuccessfulResponse = server.m_bHadSuccessfulResponse;
    item.doNotRefresh          = server.m_bDoNotRefresh;
    item.appID                 = server.m_nAppID;
    item.players               = server.m_nPlayers;
    item.maxPlayers            = server.m_nMaxPlayers;
    item.botPlayers            = server.m_nBotPlayers;
    item.password              = server.m_bPassword;
    item.secure                = server.m_bSecure;
    item.timeLastPlayed        = server.m_ulTimeLastPlayed;
    item.serverVersion         = server.m_nServerVersion;
    item.steamID               = server.m_steamID.ConvertToUint64();
    CopyText(item.name, server.GetName());
    CopyText(item.gameDir, server.m_szGameDir);
    CopyText(item.map, server.m_szMap);
    CopyText(item.description, server.m_szGameDescription);
    CopyText(item.tags, server.m_szGameTags);
}
// The handle when it names a request that has not been released.
SteamBridgeServerList* FindServerList(SteamBridgeServerListHandle request)
{
    auto found = std::find_if(host.serverLists.begin(), host.serverLists.end(), [&](const std::unique_ptr<SteamBridgeServerList>& owned) { return owned.get() == request; });
    return found != host.serverLists.end() && !(*found)->released ? found->get() : nullptr;
}
template <class F> SteamBridgeStatus ServerList(SteamBridgeServerListHandle request, F&& fn)
{
    SteamBridgeServerList* list = FindServerList(request);
    if (!list) return SB_INVALID_ARGUMENT;
    fn(*host.servers, list->native);
    return SB_OK;
}
void EraseServerList(SteamBridgeServerList* list)
{
    std::erase_if(host.serverLists, [&](const std::unique_ptr<SteamBridgeServerList>& owned) { return owned.get() == list; });
}
void ListEvent(SteamBridgeServerList& list, HServerListRequest native, SteamBridgeServersEvent& event)
{
    if (list.released) return;
    // Steam may answer before the request call returns its handle.
    if (!list.native) list.native = native;
    event.request = &list;
    HostCallback::Dispatch(list.callback, list.user, &event);
}
template <class Start> SteamBridgeStatus StartServerQuery(SteamBridgeServersCallback callback, void* user, uint32_t* query, Start&& start)
{
    if (!query) return SB_INVALID_ARGUMENT;
    *query = 0;
    if (!callback) return SB_INVALID_ARGUMENT;
    if (!host.servers) return SB_UNAVAILABLE;
    ServerQuery* owned{};
    try
    {
        host.serverQueries.push_back(std::make_unique<ServerQuery>());
        owned = host.serverQueries.back().get();
    }
    catch (...)
    {
        return SB_FAILED;
    }
    // Zero is never a query.
    if (!++host.lastServerQuery) ++host.lastServerQuery;
    owned->id       = host.lastServerQuery;
    owned->callback = callback;
    owned->user     = user;
    owned->native   = start(*owned);
    if (owned->native == HSERVERQUERY_INVALID)
    {
        // A query that never started has nothing to cancel.
        owned->ended = true;
        RetireServerQueries();
        return SB_FAILED;
    }
    *query = owned->id;
    return SB_OK;
}
} // namespace

void SteamBridgeServerList::ServerResponded(HServerListRequest request, int index)
{
    SteamBridgeServersEvent event{};
    event.kind  = SB_SERVERS_LIST_RESPONDED;
    event.index = index;
    ListEvent(*this, request, event);
}
void SteamBridgeServerList::ServerFailedToRespond(HServerListRequest request, int index)
{
    SteamBridgeServersEvent event{};
    event.kind  = SB_SERVERS_LIST_FAILED;
    event.index = index;
    ListEvent(*this, request, event);
}
void SteamBridgeServerList::RefreshComplete(HServerListRequest request, EMatchMakingServerResponse response)
{
    SteamBridgeServersEvent event{};
    event.kind     = SB_SERVERS_LIST_COMPLETE;
    event.response = response;
    ListEvent(*this, request, event);
}
void ServerQuery::Dispatch(uint32_t kind, SteamBridgeServersEvent& event, bool end)
{
    if (ended) return;
    // Steam forgets a query after its final answer, so it must not be cancelled later.
    ended       = end;
    event.kind  = kind;
    event.query = id;
    HostCallback::Dispatch(callback, user, &event);
}
void ServerQuery::ServerResponded(gameserveritem_t& server)
{
    SteamBridgeServersEvent event{};
    CopyServer(event.server, server);
    Dispatch(SB_SERVERS_PING_RESPONDED, event, true);
}
void ServerQuery::ServerFailedToRespond()
{
    SteamBridgeServersEvent event{};
    Dispatch(SB_SERVERS_PING_FAILED, event, true);
}
void ServerQuery::AddPlayerToList(const char* name, int score, float seconds)
{
    SteamBridgeServersEvent event{};
    CopyText(event.player, name ? name : "");
    event.score   = score;
    event.seconds = seconds;
    Dispatch(SB_SERVERS_PLAYER, event, false);
}
void ServerQuery::PlayersFailedToRespond()
{
    SteamBridgeServersEvent event{};
    Dispatch(SB_SERVERS_PLAYERS_FAILED, event, true);
}
void ServerQuery::PlayersRefreshComplete()
{
    SteamBridgeServersEvent event{};
    Dispatch(SB_SERVERS_PLAYERS_COMPLETE, event, true);
}

SteamBridgeStatus SteamBridge_ServersRequestList(int32_t kind, uint32_t appID, const SteamBridgeServerFilter* filters, uint32_t filterCount, SteamBridgeServersCallback callback, void* user, SteamBridgeServerListHandle* request)
{
    if (!request) return SB_INVALID_ARGUMENT;
    *request = nullptr;
    if (kind < SB_SERVER_LIST_INTERNET || kind > SB_SERVER_LIST_SPECTATOR || !callback || (filterCount && !filters)) return SB_INVALID_ARGUMENT;
    for (uint32_t i = 0; i < filterCount; ++i)
        if (!filters[i].key || !filters[i].value) return SB_INVALID_ARGUMENT;
    if (!host.servers) return SB_UNAVAILABLE;
    SteamBridgeServerList*                 list{};
    std::vector<MatchMakingKeyValuePair_t> pairs;
    try
    {
        pairs.reserve(filterCount);
        for (uint32_t i = 0; i < filterCount; ++i)
            pairs.emplace_back(filters[i].key, filters[i].value);
        host.serverLists.push_back(std::make_unique<SteamBridgeServerList>());
    }
    catch (...)
    {
        return SB_FAILED;
    }
    list           = host.serverLists.back().get();
    list->callback = callback;
    list->user     = user;
    // Steam reads the filters as an array through the pointer that ppchFilters points to.
    MatchMakingKeyValuePair_t* first   = pairs.data();
    uint32                     count   = uint32(pairs.size());
    ISteamMatchmakingServers&  servers = *host.servers;
    HServerListRequest         native{};
    switch (kind)
    {
        case SB_SERVER_LIST_INTERNET: native = servers.RequestInternetServerList(appID, &first, count, list); break;
        case SB_SERVER_LIST_LAN: native = servers.RequestLANServerList(appID, list); break;
        case SB_SERVER_LIST_FRIENDS: native = servers.RequestFriendsServerList(appID, &first, count, list); break;
        case SB_SERVER_LIST_FAVORITES: native = servers.RequestFavoritesServerList(appID, &first, count, list); break;
        case SB_SERVER_LIST_HISTORY: native = servers.RequestHistoryServerList(appID, &first, count, list); break;
        default: native = servers.RequestSpectatorServerList(appID, &first, count, list); break;
    }
    if (!native)
    {
        EraseServerList(list);
        return SB_FAILED;
    }
    list->native = native;
    *request     = list;
    return SB_OK;
}
void SteamBridge_ServersReleaseRequest(SteamBridgeServerListHandle request)
{
    SteamBridgeServerList* list = FindServerList(request);
    if (!list) return;
    list->released = true;
    RetireServerQueries();
}
SteamBridgeStatus SteamBridge_ServersGetDetails(SteamBridgeServerListHandle request, int32_t index, SteamBridgeServerItem* server)
{
    if (!server) return SB_INVALID_ARGUMENT;
    *server                     = {};
    SteamBridgeServerList* list = FindServerList(request);
    if (!list) return SB_INVALID_ARGUMENT;
    gameserveritem_t* details = host.servers->GetServerDetails(list->native, index);
    if (!details) return SB_FAILED;
    CopyServer(*server, *details);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_ServersCancelQuery(SteamBridgeServerListHandle request)
{
    return ServerList(request, [](ISteamMatchmakingServers& servers, HServerListRequest native) { servers.CancelQuery(native); });
}
SteamBridgeStatus SteamBridge_ServersRefreshQuery(SteamBridgeServerListHandle request)
{
    return ServerList(request, [](ISteamMatchmakingServers& servers, HServerListRequest native) { servers.RefreshQuery(native); });
}
SteamBridgeStatus SteamBridge_ServersRefreshServer(SteamBridgeServerListHandle request, int32_t index)
{
    return ServerList(request, [&](ISteamMatchmakingServers& servers, HServerListRequest native) { servers.RefreshServer(native, index); });
}
SteamBridgeStatus SteamBridge_ServersIsRefreshing(SteamBridgeServerListHandle request, uint32_t* refreshing)
{
    if (!refreshing) return SB_INVALID_ARGUMENT;
    *refreshing = 0;
    return ServerList(request, [&](ISteamMatchmakingServers& servers, HServerListRequest native) { *refreshing = servers.IsRefreshing(native); });
}
SteamBridgeStatus SteamBridge_ServersPing(uint32_t ip, uint16_t port, SteamBridgeServersCallback callback, void* user, uint32_t* query)
{
    return StartServerQuery(callback, user, query, [&](ServerQuery& owned) { return host.servers->PingServer(ip, port, &owned); });
}
SteamBridgeStatus SteamBridge_ServersPlayerDetails(uint32_t ip, uint16_t port, SteamBridgeServersCallback callback, void* user, uint32_t* query)
{
    return StartServerQuery(callback, user, query, [&](ServerQuery& owned) { return host.servers->PlayerDetails(ip, port, &owned); });
}
void SteamBridge_ServersCancelServerQuery(uint32_t query)
{
    auto found = std::find_if(host.serverQueries.begin(), host.serverQueries.end(), [&](const std::unique_ptr<ServerQuery>& owned) { return owned->id == query; });
    if (!query || found == host.serverQueries.end() || (*found)->ended) return;
    (*found)->ended     = true;
    (*found)->cancelled = true;
    RetireServerQueries();
}

SteamBridgeStatus SteamBridge_MatchmakingGetFavoriteCount(uint32_t* count)
{
    if (!count) return SB_INVALID_ARGUMENT;
    *count = 0;
    if (!host.matchmaking) return SB_UNAVAILABLE;
    int favorites = host.matchmaking->GetFavoriteGameCount();
    *count        = favorites > 0 ? uint32_t(favorites) : 0;
    return SB_OK;
}
SteamBridgeStatus SteamBridge_MatchmakingAddFavorite(uint32_t appID, uint32_t ip, uint16_t connectionPort, uint16_t queryPort, uint32_t flags, uint32_t lastPlayed)
{
    if (!host.matchmaking) return SB_UNAVAILABLE;
    host.matchmaking->AddFavoriteGame(appID, ip, connectionPort, queryPort, flags, lastPlayed);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_MatchmakingRemoveFavorite(uint32_t appID, uint32_t ip, uint16_t connectionPort, uint16_t queryPort, uint32_t flags)
{
    if (!host.matchmaking) return SB_UNAVAILABLE;
    return Result(host.matchmaking->RemoveFavoriteGame(appID, ip, connectionPort, queryPort, flags));
}

SteamBridgeStatus SteamBridge_FriendsGetPersonaName(uint64_t steamID, char* name, uint32_t capacity)
{
    if (!name || !capacity) return SB_INVALID_ARGUMENT;
    name[0] = 0;
    if (!host.friends) return SB_UNAVAILABLE;
    const char* persona = host.friends->GetFriendPersonaName(CSteamID(uint64(steamID)));
    if (!persona) return SB_FAILED;
    CopyText(name, capacity, persona);
    return SB_OK;
}
SteamBridgeStatus SteamBridge_FriendsGetGamePlayed(uint64_t steamID, SteamBridgeFriendGame* game)
{
    if (!game) return SB_INVALID_ARGUMENT;
    *game = {};
    if (!host.friends) return SB_UNAVAILABLE;
    FriendGameInfo_t info{};
    if (!host.friends->GetFriendGamePlayed(CSteamID(uint64(steamID)), &info)) return SB_FAILED;
    game->gameID       = info.m_gameID.ToUint64();
    game->ip           = info.m_unGameIP;
    game->gamePort     = info.m_usGamePort;
    game->queryPort    = info.m_usQueryPort;
    game->lobbySteamID = info.m_steamIDLobby.ConvertToUint64();
    return SB_OK;
}
SteamBridgeStatus SteamBridge_FriendsGetRelationship(uint64_t steamID, int32_t* relationship)
{
    if (!relationship) return SB_INVALID_ARGUMENT;
    *relationship = SB_FRIEND_RELATIONSHIP_NONE;
    if (!host.friends) return SB_UNAVAILABLE;
    *relationship = host.friends->GetFriendRelationship(CSteamID(uint64(steamID)));
    return SB_OK;
}
SteamBridgeStatus SteamBridge_FriendsGetAvatar(uint64_t steamID, uint32_t size, int32_t* image)
{
    if (!image) return SB_INVALID_ARGUMENT;
    *image = 0;
    if (size != SB_AVATAR_SMALL && size != SB_AVATAR_MEDIUM && size != SB_AVATAR_LARGE) return SB_INVALID_ARGUMENT;
    if (!host.friends) return SB_UNAVAILABLE;
    const CSteamID id{uint64(steamID)};
    switch (size)
    {
        case SB_AVATAR_SMALL: *image = host.friends->GetSmallFriendAvatar(id); break;
        case SB_AVATAR_MEDIUM: *image = host.friends->GetMediumFriendAvatar(id); break;
        default: *image = host.friends->GetLargeFriendAvatar(id); break;
    }
    return SB_OK;
}

SteamBridgeStatus SteamBridge_UtilsGetImageSize(int32_t image, uint32_t* width, uint32_t* height)
{
    if (!width || !height) return SB_INVALID_ARGUMENT;
    *width  = 0;
    *height = 0;
    if (!host.steamUtils) return SB_UNAVAILABLE;
    uint32 nativeWidth{}, nativeHeight{};
    if (!host.steamUtils->GetImageSize(image, &nativeWidth, &nativeHeight)) return SB_FAILED;
    *width  = nativeWidth;
    *height = nativeHeight;
    return SB_OK;
}
SteamBridgeStatus SteamBridge_UtilsGetImageRGBA(int32_t image, uint8_t* rgba, uint32_t capacity)
{
    if (!rgba || !capacity || capacity > uint32_t(INT32_MAX)) return SB_INVALID_ARGUMENT;
    if (!host.steamUtils) return SB_UNAVAILABLE;
    return Result(host.steamUtils->GetImageRGBA(image, rgba, int(capacity)));
}
SteamBridgeStatus SteamBridge_AppsIsVACBanned(uint32_t* banned)
{
    if (!banned) return SB_INVALID_ARGUMENT;
    *banned = 0;
    if (!host.apps) return SB_UNAVAILABLE;
    *banned = host.apps->BIsVACBanned();
    return SB_OK;
}
