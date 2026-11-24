// Host-mode fixture runtime. It exports the modern entry points that SteamBridge_HostInit
// resolves, records forwarded arguments and dispatches queued callback payloads.
#include "HostStubs.h"
#include "MockHost.h"
#include <steam/steamnetworkingfakeip.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <vector>

namespace
{
int                         failInit, voiceResult = k_EVoiceResultOK;
int                         shutdowns, serverShutdowns;
bool                        serverInitialized, clientInitialized;
MockHostForwardedData       forwarded;
std::vector<CCallbackBase*> callbacks;
struct Queued
{
    int               id;
    bool              gameServer;
    std::vector<char> payload;
};
std::vector<Queued> queue;

struct Peek : CCallbackBase
{
    static bool GameServer(CCallbackBase* callback) { return static_cast<Peek*>(callback)->m_nCallbackFlags & k_ECallbackFlagsGameServer; }
};
void Dispatch(bool gameServer)
{
    auto pending = std::move(queue);
    queue.clear();
    for (auto& item : pending)
    {
        if (item.gameServer != gameServer)
        {
            queue.push_back(std::move(item));
            continue;
        }
        auto snapshot = callbacks;
        for (auto callback : snapshot)
            if (std::find(callbacks.begin(), callbacks.end(), callback) != callbacks.end() && callback->GetICallback() == item.id && Peek::GameServer(callback) == gameServer)
                callback->Run(item.payload.empty() ? nullptr : item.payload.data());
    }
}

class User final : public StubUser
{
public:
    CSteamID GetSteamID() override { return CSteamID(uint64(76561198000000001)); }
    int      InitiateGameConnection_DEPRECATED(void* blob, int max, CSteamID server, uint32 ip, uint16 port, bool secure) override
    {
        forwarded.serverSteamID = server.ConvertToUint64();
        forwarded.ip            = ip;
        forwarded.port          = port;
        forwarded.secure        = secure;
        if (max < 4) return 0;
        std::memcpy(blob, "tick", 4);
        return 4;
    }
    void         TerminateGameConnection_DEPRECATED(uint32 ip, uint16) override { forwarded.terminated = ip; }
    EVoiceResult GetAvailableVoice(uint32* bytes, uint32*, uint32) override
    {
        *bytes = 3;
        return EVoiceResult(voiceResult);
    }
    EVoiceResult GetVoice(bool, void* dest, uint32 size, uint32* written, bool wantPCM, void* pcm, uint32 pcmSize, uint32* pcmWritten, uint32 rate) override
    {
        if (size < 3) return k_EVoiceResultBufferTooSmall;
        std::memcpy(dest, "abc", 3);
        *written               = 3;
        forwarded.pcmRequested = wantPCM;
        forwarded.pcmRate      = rate;
        if (wantPCM && pcmSize >= 2)
        {
            std::memcpy(pcm, "pc", 2);
            *pcmWritten = 2;
        }
        return EVoiceResult(voiceResult);
    }
    EVoiceResult DecompressVoice(const void*, uint32 bytes, void* dest, uint32 size, uint32* written, uint32 rate) override
    {
        forwarded.pcmRate = rate;
        if (size < bytes * 2) return k_EVoiceResultBufferTooSmall;
        std::memset(dest, 1, bytes * 2);
        *written = bytes * 2;
        return EVoiceResult(voiceResult);
    }
    uint32 GetVoiceOptimalSampleRate() override { return 24000; }
} user;

// MockHostSetApps controls whether SteamApps exists and what it reports.
bool appsAvailable, appsHasLanguage, appsLowViolence, appsVACBanned;
char appsLanguage[32];
char appsLaunchCommandLine[256];
class Apps final : public StubApps
{
public:
    bool        BIsLowViolence() override { return appsLowViolence; }
    const char* GetCurrentGameLanguage() override { return appsHasLanguage ? appsLanguage : nullptr; }
    bool        BIsSubscribedApp(AppId_t appID) override { return appID == 70; }
    bool        BIsVACBanned() override { return appsVACBanned; }
    int         GetLaunchCommandLine(char* commandLine, int capacity) override
    {
        size_t length = std::strlen(appsLaunchCommandLine);
        if (!length)
        {
            if (commandLine && capacity > 0) commandLine[0] = 0;
            return 0;
        }
        if (!commandLine || capacity <= 0) return int(length + 1);
        size_t copied = std::min(length, size_t(capacity - 1));
        std::memcpy(commandLine, appsLaunchCommandLine, copied);
        commandLine[copied] = 0;
        return int(copied + 1);
    }
} apps;

// Answers queued for the next client dispatch.
std::vector<std::function<void()>> answers;

class Matchmaking final : public StubMatchmaking
{
public:
    int  GetFavoriteGameCount() override { return 2; }
    void Record(AppId_t appID, uint32 ip, uint16 connectionPort, uint16 queryPort, uint32 flags)
    {
        forwarded.favoriteAppID          = appID;
        forwarded.favoriteIP             = ip;
        forwarded.favoriteConnectionPort = connectionPort;
        forwarded.favoriteQueryPort      = queryPort;
        forwarded.favoriteFlags          = flags;
    }
    int AddFavoriteGame(AppId_t appID, uint32 ip, uint16 connectionPort, uint16 queryPort, uint32 flags, uint32 lastPlayed) override
    {
        Record(appID, ip, connectionPort, queryPort, flags);
        forwarded.favoriteLastPlayed = lastPlayed;
        return 0;
    }
    bool RemoveFavoriteGame(AppId_t appID, uint32 ip, uint16 connectionPort, uint16 queryPort, uint32 flags) override
    {
        Record(appID, ip, connectionPort, queryPort, flags);
        return ip != 0;
    }
} matchmaking;

// Requests and queries are numbered from 1; a request handle points at its number.
class MatchmakingServers final : public StubMatchmakingServers
{
public:
    std::deque<int>                      requests;
    ISteamMatchmakingServerListResponse* lastList{};
    HServerListRequest                   lastRequest{};
    ISteamMatchmakingPingResponse*       lastPing{};
    ISteamMatchmakingPlayersResponse*    lastPlayers{};
    HServerQuery                         lastQuery{};
    gameserveritem_t                     details;
    HServerListRequest                   Request(int kind, AppId_t appID, MatchMakingKeyValuePair_t** filters, uint32 count, ISteamMatchmakingServerListResponse* response)
    {
        forwarded.listKind    = kind;
        forwarded.listAppID   = appID;
        forwarded.listFilters = count;
        // Steam reads the filters as an array through *filters.
        if (count)
        {
            std::snprintf(forwarded.listFilterKey, sizeof(forwarded.listFilterKey), "%s", (*filters)[0].m_szKey);
            std::snprintf(forwarded.listFilterValue, sizeof(forwarded.listFilterValue), "%s", (*filters)[count - 1].m_szValue);
        }
        // App 0 has no servers to list.
        if (!appID) return nullptr;
        requests.push_back(int(requests.size()) + 1);
        lastList    = response;
        lastRequest = &requests.back();
        return lastRequest;
    }
    HServerListRequest RequestInternetServerList(AppId_t app, MatchMakingKeyValuePair_t** filters, uint32 count, ISteamMatchmakingServerListResponse* response) override { return Request(0, app, filters, count, response); }
    HServerListRequest RequestLANServerList(AppId_t app, ISteamMatchmakingServerListResponse* response) override { return Request(1, app, nullptr, 0, response); }
    HServerListRequest RequestFriendsServerList(AppId_t app, MatchMakingKeyValuePair_t** filters, uint32 count, ISteamMatchmakingServerListResponse* response) override { return Request(2, app, filters, count, response); }
    HServerListRequest RequestFavoritesServerList(AppId_t app, MatchMakingKeyValuePair_t** filters, uint32 count, ISteamMatchmakingServerListResponse* response) override { return Request(3, app, filters, count, response); }
    HServerListRequest RequestHistoryServerList(AppId_t app, MatchMakingKeyValuePair_t** filters, uint32 count, ISteamMatchmakingServerListResponse* response) override { return Request(4, app, filters, count, response); }
    HServerListRequest RequestSpectatorServerList(AppId_t app, MatchMakingKeyValuePair_t** filters, uint32 count, ISteamMatchmakingServerListResponse* response) override { return Request(5, app, filters, count, response); }
    // Steam answers neither a released request nor a cancelled query.
    void ReleaseRequest(HServerListRequest request) override
    {
        ++forwarded.listsReleased;
        *static_cast<int*>(request) = 0;
    }
    gameserveritem_t* GetServerDetails(HServerListRequest, int index) override { return index == 0 ? &details : nullptr; }
    void              CancelQuery(HServerListRequest) override { ++forwarded.listCancels; }
    void              RefreshQuery(HServerListRequest) override { ++forwarded.listRefreshes; }
    bool              IsRefreshing(HServerListRequest) override { return true; }
    void              RefreshServer(HServerListRequest, int index) override { forwarded.refreshedServer = index; }
    HServerQuery      Query(uint32 ip, uint16 port)
    {
        forwarded.queryIP   = ip;
        forwarded.queryPort = port;
        // Port 0 cannot be queried.
        return port ? ++lastQuery : HSERVERQUERY_INVALID;
    }
    HServerQuery PingServer(uint32 ip, uint16 port, ISteamMatchmakingPingResponse* response) override
    {
        lastPing = response;
        return Query(ip, port);
    }
    HServerQuery PlayerDetails(uint32 ip, uint16 port, ISteamMatchmakingPlayersResponse* response) override
    {
        lastPlayers = response;
        return Query(ip, port);
    }
    std::vector<HServerQuery> cancelled;
    void                      CancelServerQuery(HServerQuery query) override
    {
        ++forwarded.queriesCancelled;
        cancelled.push_back(query);
    }
    bool Answers(HServerQuery query) const { return std::find(cancelled.begin(), cancelled.end(), query) == cancelled.end(); }
} matchmakingServers;

// MockHostSetFriends controls whether SteamFriends exists for the next client initialization.
bool friendsAvailable;
class Friends final : public StubFriends
{
public:
    const char* GetFriendPersonaName(CSteamID id) override { return id.ConvertToUint64() == 76561198000000005 ? "friend" : nullptr; }
    bool        GetFriendGamePlayed(CSteamID id, FriendGameInfo_t* info) override
    {
        if (id.ConvertToUint64() != 76561198000000005) return false;
        info->m_gameID       = CGameID(uint64(70));
        info->m_unGameIP     = 0x0a000002;
        info->m_usGamePort   = 27015;
        info->m_usQueryPort  = 27016;
        info->m_steamIDLobby = CSteamID(uint64(76561198000000006));
        return true;
    }
    EFriendRelationship GetFriendRelationship(CSteamID id) override { return id.ConvertToUint64() == 76561198000000005 ? k_EFriendRelationshipIgnoredFriend : k_EFriendRelationshipNone; }
    // The friend's small avatar is image 7, the medium one is still loading and there is no large one.
    int GetSmallFriendAvatar(CSteamID id) override { return id.ConvertToUint64() == 76561198000000005 ? 7 : 0; }
    int GetMediumFriendAvatar(CSteamID id) override { return id.ConvertToUint64() == 76561198000000005 ? -1 : 0; }
} friends;

// Image 7 is 2x1 pixels.
const uint8 avatarPixels[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
class Utils final : public StubUtils
{
public:
    bool GetImageSize(int image, uint32* width, uint32* height) override
    {
        if (image != 7) return false;
        *width  = 2;
        *height = 1;
        return true;
    }
    bool GetImageRGBA(int image, uint8* rgba, int capacity) override
    {
        if (image != 7 || capacity < int(sizeof(avatarPixels))) return false;
        std::memcpy(rgba, avatarPixels, sizeof(avatarPixels));
        return true;
    }
} steamUtils;

class GameServer final : public StubGameServer
{
public:
    void     SetProduct(const char* product) override { std::strncpy(forwarded.product, product, sizeof(forwarded.product) - 1); }
    bool     BSecure() override { return true; }
    CSteamID GetSteamID() override { return CSteamID(uint64(90000000000000042)); }
    void     SetKeyValue(const char* key, const char* value) override
    {
        std::strncpy(forwarded.key, key, sizeof(forwarded.key) - 1);
        std::strncpy(forwarded.value, value, sizeof(forwarded.value) - 1);
    }
    bool SendUserConnectAndAuthenticate_DEPRECATED(uint32 ip, const void*, uint32 bytes, CSteamID* id) override
    {
        forwarded.authIP    = ip;
        forwarded.authBytes = bytes;
        *id                 = CSteamID(uint64(76561198000000002));
        return bytes != 1;
    }
    int GetNextOutgoingPacket(void* out, int max, uint32* ip, uint16* port) override
    {
        if (max < 2) return 0;
        std::memcpy(out, "mp", 2);
        *ip   = 0x7f000001;
        *port = 27010;
        return 2;
    }
} server;

} // namespace

// The SDK declares these destructors and leaves their definition to the runtime.
ISteamNetworkingSockets::~ISteamNetworkingSockets() {}
ISteamNetworkingUtils::~ISteamNetworkingUtils() {}

namespace
{
// A FakeIP datagram waiting on a port; Release frees it.
struct FakeMessage : SteamNetworkingMessage_t
{
    // The base has a protected destructor, so it cannot be aggregate-initialized.
    FakeMessage() : SteamNetworkingMessage_t() {}
    std::vector<char> bytes;
};
void ReleaseFakeMessage(SteamNetworkingMessage_t* message)
{
    ++forwarded.messagesReleased;
    delete static_cast<FakeMessage*>(message);
}
std::deque<FakeMessage*> fakeMessages;

class FakePort final : public ISteamNetworkingFakeUDPPort
{
public:
    void DestroyFakeUDPPort() override
    {
        ++forwarded.portsDestroyed;
        delete this;
    }
    EResult SendMessageToFakeIP(const SteamNetworkingIPAddr& address, const void*, uint32 bytes, int flags) override
    {
        forwarded.sendIP    = address.GetIPv4();
        forwarded.sendPort  = address.m_port;
        forwarded.sendBytes = bytes;
        forwarded.sendFlags = flags;
        return address.GetIPv4() ? k_EResultOK : k_EResultInvalidParam;
    }
    int ReceiveMessages(SteamNetworkingMessage_t** messages, int count) override
    {
        if (fakeMessages.empty() || count < 1) return 0;
        messages[0] = fakeMessages.front();
        fakeMessages.pop_front();
        return 1;
    }
    void ScheduleCleanup(const SteamNetworkingIPAddr& address) override
    {
        forwarded.cleanupIP   = address.GetIPv4();
        forwarded.cleanupPort = address.m_port;
    }
};

SteamNetworkingFakeIPResult_t fakeIP{};

class NetworkingSockets final : public StubNetworkingSockets
{
public:
    bool BeginAsyncRequestFakeIP(int ports) override
    {
        forwarded.fakeIPPorts = ports;
        return ports == 1;
    }
    void                         GetFakeIP(int, SteamNetworkingFakeIPResult_t* info) override { *info = fakeIP; }
    ISteamNetworkingFakeUDPPort* CreateFakeUDPPort(int index) override
    {
        forwarded.portIndex = index;
        ++forwarded.portsCreated;
        return new FakePort;
    }
} clientSockets, serverSockets;

// FakeIPs are 169.254.0.0/16, as Steam allocates them.
FSteamNetworkingSocketsDebugOutput debugOutput;
class NetworkingUtils final : public StubNetworkingUtils
{
public:
    void SetDebugOutputFunction(ESteamNetworkingSocketsDebugOutputType level, FSteamNetworkingSocketsDebugOutput function) override
    {
        forwarded.debugLevel = level;
        debugOutput          = function;
    }
    ESteamNetworkingFakeIPType GetIPv4FakeIPType(uint32 ip) override { return (ip >> 16) == 0xa9fe ? k_ESteamNetworkingFakeIPType_GlobalIPv4 : k_ESteamNetworkingFakeIPType_NotFake; }
    void                       SteamNetworkingIPAddr_ToString(const SteamNetworkingIPAddr& address, char* text, size_t size, bool) override
    {
        uint32 ip = address.GetIPv4();
        std::snprintf(text, size, "%u.%u.%u.%u:%u", ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff, unsigned(address.m_port));
    }
    void SteamNetworkingIdentity_ToString(const SteamNetworkingIdentity& identity, char* text, size_t size) override { std::snprintf(text, size, "steamid:%llu", static_cast<unsigned long long>(identity.GetSteamID64())); }
} utils;
} // namespace

extern "C"
{
    S_API ESteamAPIInitResult S_CALLTYPE SteamAPI_InitFlat(SteamErrMsg*)
    {
        clientInitialized = !failInit;
        return failInit ? k_ESteamAPIInitResult_NoSteamClient : k_ESteamAPIInitResult_OK;
    }
    S_API void S_CALLTYPE SteamAPI_Shutdown()
    {
        clientInitialized = false;
        ++shutdowns;
    }
    S_API void S_CALLTYPE SteamAPI_RunCallbacks()
    {
        auto pending = std::move(answers);
        answers.clear();
        for (auto& answer : pending)
            answer();
        Dispatch(false);
    }
    S_API HSteamUser S_CALLTYPE SteamAPI_GetHSteamUser() { return 1; }
    S_API void* S_CALLTYPE      SteamInternal_FindOrCreateUserInterface(HSteamUser steamUser, const char* version)
    {
        if (!std::strcmp(version, STEAMUSER_INTERFACE_VERSION)) return &user;
        if (!std::strcmp(version, STEAMAPPS_INTERFACE_VERSION)) return appsAvailable ? &apps : nullptr;
        if (!std::strcmp(version, STEAMFRIENDS_INTERFACE_VERSION)) return friendsAvailable ? &friends : nullptr;
        if (!std::strcmp(version, STEAMMATCHMAKING_INTERFACE_VERSION)) return &matchmaking;
        if (!std::strcmp(version, STEAMMATCHMAKINGSERVERS_INTERFACE_VERSION)) return &matchmakingServers;
        if (!std::strcmp(version, STEAMNETWORKINGSOCKETS_INTERFACE_VERSION)) return steamUser == 1 ? &clientSockets : nullptr;
        // The global utilities (user 0) exist once the client is initialized.
        if (!std::strcmp(version, STEAMNETWORKINGUTILS_INTERFACE_VERSION)) return clientInitialized && !steamUser ? &utils : nullptr;
        if (!std::strcmp(version, STEAMUTILS_INTERFACE_VERSION)) return clientInitialized && !steamUser ? &steamUtils : nullptr;
        return nullptr;
    }
    S_API ESteamAPIInitResult S_CALLTYPE SteamInternal_GameServer_Init_V2(uint32, uint16, uint16, EServerMode mode, const char*, const char* versions, SteamErrMsg*)
    {
        forwarded.mode = mode;
        // The bridge must pass the interface list that SteamGameServer_InitEx would pass.
        if (!versions || std::strcmp(versions, STEAMUTILS_INTERFACE_VERSION)) return k_ESteamAPIInitResult_VersionMismatch;
        serverInitialized = true;
        return k_ESteamAPIInitResult_OK;
    }
    S_API void S_CALLTYPE SteamGameServer_Shutdown()
    {
        serverInitialized = false;
        ++serverShutdowns;
    }
    S_API void S_CALLTYPE       SteamGameServer_RunCallbacks() { Dispatch(true); }
    S_API HSteamUser S_CALLTYPE SteamGameServer_GetHSteamUser() { return 2; }
    S_API void* S_CALLTYPE      SteamInternal_FindOrCreateGameServerInterface(HSteamUser steamUser, const char* version)
    {
        if (!serverInitialized) return nullptr;
        if (!std::strcmp(version, STEAMGAMESERVER_INTERFACE_VERSION)) return &server;
        if (!std::strcmp(version, STEAMNETWORKINGSOCKETS_INTERFACE_VERSION)) return steamUser == 2 ? &serverSockets : nullptr;
        if (!std::strcmp(version, STEAMNETWORKINGUTILS_INTERFACE_VERSION)) return !steamUser ? &utils : nullptr;
        return nullptr;
    }
    S_API void S_CALLTYPE SteamAPI_RegisterCallback(CCallbackBase* callback, int) { callbacks.push_back(callback); }
    S_API void S_CALLTYPE SteamAPI_UnregisterCallback(CCallbackBase* callback) { std::erase(callbacks, callback); }

    // Fixture control.
    S_API void S_CALLTYPE MockHostSetFailInit(int value) { failInit = value; }
    S_API void S_CALLTYPE MockHostSetVoiceResult(int value) { voiceResult = value; }
    // Applies to the next client initialization. A NULL language makes SteamApps return NULL.
    S_API void S_CALLTYPE MockHostSetApps(int available, const char* language, int lowViolence, int vacBanned)
    {
        appsAvailable   = available != 0;
        appsHasLanguage = language != nullptr;
        appsLowViolence = lowViolence != 0;
        appsVACBanned   = vacBanned != 0;
        std::snprintf(appsLanguage, sizeof(appsLanguage), "%s", language ? language : "");
    }
    S_API void S_CALLTYPE MockHostSetLaunchCommandLine(const char* commandLine)
    {
        std::snprintf(appsLaunchCommandLine, sizeof(appsLaunchCommandLine), "%s", commandLine ? commandLine : "");
    }
    S_API void S_CALLTYPE MockHostSetFriends(int available) { friendsAvailable = available != 0; }
    // Sets the details of server 0 in every list.
    S_API void S_CALLTYPE MockHostSetServerDetails(const char* name, const char* map, int players)
    {
        auto& details = matchmakingServers.details;
        details       = gameserveritem_t();
        details.m_NetAdr.Init(0x0a000001, 27016, 27015);
        details.SetName(name);
        std::snprintf(details.m_szMap, sizeof(details.m_szMap), "%s", map);
        std::snprintf(details.m_szGameDir, sizeof(details.m_szGameDir), "valve");
        details.m_nAppID      = 70;
        details.m_nPlayers    = players;
        details.m_nMaxPlayers = 32;
        details.m_bSecure     = true;
        details.m_steamID     = CSteamID(uint64(90000000000000042));
    }
    // Queues an answer (MOCK_LIST_*) of the last server list request; value is the server index or
    // the EMatchMakingServerResponse.
    S_API void S_CALLTYPE MockHostServerListAnswer(int answer, int value)
    {
        auto response = matchmakingServers.lastList;
        auto request  = matchmakingServers.lastRequest;
        answers.push_back([=] {
            if (!*static_cast<int*>(request)) return;
            if (answer == MOCK_LIST_RESPONDED) response->ServerResponded(request, value);
            else if (answer == MOCK_LIST_FAILED)
                response->ServerFailedToRespond(request, value);
            else
                response->RefreshComplete(request, EMatchMakingServerResponse(value));
        });
    }
    // Queues the answer of the last ping query: the server 0 details, or a failure.
    S_API void S_CALLTYPE MockHostPingAnswer(int responded)
    {
        auto response = matchmakingServers.lastPing;
        auto query    = matchmakingServers.lastQuery;
        answers.push_back([=] {
            if (!matchmakingServers.Answers(query)) return;
            if (responded) response->ServerResponded(matchmakingServers.details);
            else
                response->ServerFailedToRespond();
        });
    }
    // Queues an answer (MOCK_PLAYERS_*) of the last player query.
    S_API void S_CALLTYPE MockHostPlayersAnswer(int answer)
    {
        auto response = matchmakingServers.lastPlayers;
        auto query    = matchmakingServers.lastQuery;
        answers.push_back([=] {
            if (!matchmakingServers.Answers(query)) return;
            if (answer == MOCK_PLAYERS_PLAYER) response->AddPlayerToList("player", 3, 1.5f);
            else if (answer == MOCK_PLAYERS_FAILED)
                response->PlayersFailedToRespond();
            else
                response->PlayersRefreshComplete();
        });
    }
    S_API int S_CALLTYPE MockHostStat(int which)
    {
        switch (which)
        {
            case MOCK_HOST_SHUTDOWNS: return shutdowns;
            case MOCK_HOST_SERVER_SHUTDOWNS: return serverShutdowns;
            case MOCK_HOST_CALLBACKS: return int(callbacks.size());
            default: return int(queue.size());
        }
    }
    S_API const void* S_CALLTYPE MockHostForwarded() { return &forwarded; }
    S_API void S_CALLTYPE        MockHostQueue(int id, int gameServer, const void* payload, int bytes)
    {
        auto data = static_cast<const char*>(payload);
        queue.push_back({id, gameServer != 0, data && bytes > 0 ? std::vector<char>(data, data + bytes) : std::vector<char>()});
    }
    // Queues a FakeIP datagram of bytes bytes, from an IP address or a SteamID (MOCK_FAKE_FROM_*).
    S_API void S_CALLTYPE MockHostQueueFakeMessage(int from, uint32_t ip, uint16_t port, int bytes)
    {
        auto message = new FakeMessage;
        message->bytes.assign(size_t(bytes), 'f');
        message->m_pData      = message->bytes.data();
        message->m_cbSize     = bytes;
        message->m_pfnRelease = ReleaseFakeMessage;
        if (from == MOCK_FAKE_FROM_IP) message->m_identityPeer.SetIPv4Addr(ip, port);
        else
            message->m_identityPeer.SetSteamID64(76561198000000004);
        fakeMessages.push_back(message);
    }
    S_API void S_CALLTYPE MockHostSetFakeIP(int result, uint32_t ip, uint16_t port)
    {
        fakeIP              = {};
        fakeIP.m_eResult    = EResult(result);
        fakeIP.m_unIP       = ip;
        fakeIP.m_unPorts[0] = port;
    }
    // Calls the debug output function the bridge installed; returns zero when none is.
    S_API int S_CALLTYPE MockHostEmitDebug(int level, const char* text)
    {
        if (!debugOutput) return 0;
        debugOutput(ESteamNetworkingSocketsDebugOutputType(level), text);
        return 1;
    }
}
