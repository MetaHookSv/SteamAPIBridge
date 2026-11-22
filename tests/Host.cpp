#include "Platform.h"
#include "MockHost.h"
#include <SteamAPIBridge.h>
#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>
#include <steam/steamnetworkingfakeip.h>
#include <cassert>
#include <cstring>

using SetIntFn     = void(S_CALLTYPE*)(int);
using StatFn       = int(S_CALLTYPE*)(int);
using ForwardedFn  = const MockHostForwardedData*(S_CALLTYPE*)();
using QueueFn      = void(S_CALLTYPE*)(int, int, const void*, int);
using QueueFakeFn  = void(S_CALLTYPE*)(int, uint32_t, uint16_t, int);
using SetFakeIPFn  = void(S_CALLTYPE*)(int, uint32_t, uint16_t);
using EmitDebugFn  = int(S_CALLTYPE*)(int, const char*);
using SetAppsFn    = void(S_CALLTYPE*)(int, const char*, int, int);
using SetDetailsFn = void(S_CALLTYPE*)(const char*, const char*, int);
using AnswerFn     = void(S_CALLTYPE*)(int, int);
namespace mock
{
SetIntFn     failInit, voiceResult;
StatFn       stat;
ForwardedFn  forwarded;
QueueFn      queue;
QueueFakeFn  queueFake;
SetFakeIPFn  setFakeIP;
EmitDebugFn  emitDebug;
SetAppsFn    setApps;
SetIntFn     setFriends, pingAnswer, playersAnswer;
SetDetailsFn setServerDetails;
AnswerFn     listAnswer;
} // namespace mock
using namespace mock;

struct ClientEvents
{
    int                    count{};
    SteamBridgeClientEvent last{};
    bool                   shutdown{};
};
struct ServerEvents
{
    int                    count{};
    SteamBridgeServerEvent last{};
    bool                   shutdown{};
};
void STEAMBRIDGE_CALL OnClient(void* user, const SteamBridgeClientEvent* event)
{
    auto& e = *static_cast<ClientEvents*>(user);
    ++e.count;
    e.last = *event;
    if (e.shutdown)
    {
        SteamBridge_HostShutdown();
        // The bridge defers the shutdown until the dispatcher returns.
        assert(SteamBridge_HostIsActive());
    }
}
void STEAMBRIDGE_CALL OnServer(void* user, const SteamBridgeServerEvent* event)
{
    auto& e = *static_cast<ServerEvents*>(user);
    ++e.count;
    e.last = *event;
    if (e.shutdown) SteamBridge_ServerShutdown();
}
struct DebugOutput
{
    int  level{};
    char text[16]{};
} debugOutput;
void STEAMBRIDGE_CALL OnDebug(int32_t level, const char* message)
{
    debugOutput.level = level;
    std::strncpy(debugOutput.text, message, sizeof(debugOutput.text) - 1);
}
struct ServersEvents
{
    int                     count{};
    SteamBridgeServersEvent last{}, player{};
    // Released or cancelled from the callback, with the fixture's count at that moment.
    bool     release{};
    uint32_t cancel{};
    int      releasedInside{}, cancelledInside{};
};
void STEAMBRIDGE_CALL OnServers(void* user, const SteamBridgeServersEvent* event)
{
    auto& e = *static_cast<ServersEvents*>(user);
    ++e.count;
    e.last = *event;
    if (SB_SERVERS_PLAYER == event->kind) e.player = *event;
    if (e.release)
    {
        SteamBridge_ServersReleaseRequest(event->request);
        e.releasedInside = forwarded()->listsReleased;
    }
    if (e.cancel)
    {
        SteamBridge_ServersCancelServerQuery(e.cancel);
        e.cancelledInside = forwarded()->queriesCancelled;
    }
}
struct MatchmakingEvents
{
    int                         count{};
    SteamBridgeMatchmakingEvent first{}, last{};
};
void STEAMBRIDGE_CALL OnMatchmaking(void* user, const SteamBridgeMatchmakingEvent* event)
{
    auto& e = *static_cast<MatchmakingEvents*>(user);
    if (!e.count++) e.first = *event;
    e.last = *event;
}
template <class T> void Queue(const T& payload, bool gameServer) { queue(T::k_iCallback, gameServer, &payload, int(sizeof(T))); }

void Unavailable(const char* missing)
{
    assert(SB_UNAVAILABLE == SteamBridge_HostInit(missing, 1));
    assert(!SteamBridge_HostIsActive());
    uint32_t size = 9;
    char     ticket[8];
    assert(SB_UNAVAILABLE == SteamBridge_UserInitiateGameConnection(ticket, sizeof(ticket), 1, 2, 3, 0, &size));
    assert(0 == size);
    assert(SB_UNAVAILABLE == SteamBridge_ServerInit(0, 27015, 27015, SB_SERVER_MODE_SECURE, "1.0"));
    assert(0 == SteamBridge_VoiceGetOptimalSampleRate());
    char language[8] = "x";
    assert(SB_UNAVAILABLE == SteamBridge_UserGetCurrentGameLanguage(language, sizeof(language)));
    assert(0 == language[0]);
    uint32_t lowViolence = 1;
    assert(SB_UNAVAILABLE == SteamBridge_UserIsLowViolence(&lowViolence));
    assert(0 == lowViolence);
    SteamBridge_HostRunCallbacks();
    SteamBridge_HostShutdown();
}
void Client(const char* mock)
{
    failInit(1);
    assert(SB_FAILED == SteamBridge_HostInit(mock, 1));
    assert(!SteamBridge_HostIsActive());
    failInit(0);
    ClientEvents events;
    SteamBridge_HostSetClientCallback(OnClient, &events);
    assert(SB_OK == SteamBridge_HostInit(mock, 1));
    assert(SB_INVALID_ARGUMENT == SteamBridge_HostInit(mock, 1));
    assert(SteamBridge_HostIsActive() && SteamBridge_HostIsClientActive());
    assert(5 == mock::stat(MOCK_HOST_CALLBACKS));

    char     ticket[8]{};
    uint32_t size{};
    assert(SB_OK == SteamBridge_UserInitiateGameConnection(ticket, sizeof(ticket), 90000000000000042, 0x7f000001, 27015, 1, &size));
    assert(4 == size && !std::memcmp(ticket, "tick", 4));
    assert(90000000000000042 == forwarded()->serverSteamID && 0x7f000001 == forwarded()->ip && 27015 == forwarded()->port && forwarded()->secure);
    assert(SB_FAILED == SteamBridge_UserInitiateGameConnection(ticket, 2, 1, 2, 3, 0, &size));
    assert(0 == size);
    assert(SB_OK == SteamBridge_UserTerminateGameConnection(0x0a000001, 27015));
    assert(0x0a000001 == forwarded()->terminated);
    uint64_t id{};
    assert(SB_OK == SteamBridge_UserGetSteamID(&id));
    assert(76561198000000001 == id);
    // The fixture exposes no SteamFriends, nor SteamApps until Apps enables it; those calls
    // report unavailability.
    char name[4] = "x";
    assert(SB_UNAVAILABLE == SteamBridge_UserGetPersonaName(name, sizeof(name)));
    assert(0 == name[0]);
    uint32_t subscribed = 1;
    assert(SB_UNAVAILABLE == SteamBridge_UserIsSubscribedApp(70, &subscribed));
    assert(0 == subscribed);
    char language[8] = "x";
    assert(SB_UNAVAILABLE == SteamBridge_UserGetCurrentGameLanguage(language, sizeof(language)));
    assert(0 == language[0]);
    uint32_t lowViolence = 1;
    assert(SB_UNAVAILABLE == SteamBridge_UserIsLowViolence(&lowViolence));
    assert(0 == lowViolence);

    assert(SB_OK == SteamBridge_VoiceStartRecording());
    uint32_t available{};
    assert(SB_OK == SteamBridge_VoiceGetAvailable(&available));
    assert(3 == available);
    char     voice[8]{}, pcm[8]{};
    uint32_t voiceBytes{}, pcmBytes{};
    assert(SB_OK == SteamBridge_VoiceGet(voice, sizeof(voice), &voiceBytes, nullptr, 0, &pcmBytes, 0));
    assert(3 == voiceBytes && 0 == pcmBytes && !forwarded()->pcmRequested);
    assert(SB_OK == SteamBridge_VoiceGet(voice, sizeof(voice), &voiceBytes, pcm, sizeof(pcm), &pcmBytes, 11025));
    assert(3 == voiceBytes && 2 == pcmBytes && forwarded()->pcmRequested && 11025 == forwarded()->pcmRate);
    short decoded[8]{};
    assert(SB_OK == SteamBridge_VoiceDecompress(voice, 3, decoded, sizeof(decoded), &pcmBytes, 11025));
    assert(6 == pcmBytes);
    voiceResult(k_EVoiceResultNoData);
    assert(SB_FAILED == SteamBridge_VoiceGet(voice, sizeof(voice), &voiceBytes, pcm, sizeof(pcm), &pcmBytes, 11025));
    assert(0 == voiceBytes && 0 == pcmBytes);
    assert(SB_FAILED == SteamBridge_VoiceGetAvailable(&available));
    voiceResult(k_EVoiceResultOK);
    assert(24000 == SteamBridge_VoiceGetOptimalSampleRate());
    assert(SB_OK == SteamBridge_VoiceStopRecording());

    // A client FakeIP port sends to FakeIPs and receives their datagrams.
    SteamBridgeFakeUDPPortHandle port{};
    assert(SB_UNAVAILABLE == SteamBridge_FakeUDPPortCreate(1, 0, &port) && !port);
    assert(SB_INVALID_ARGUMENT == SteamBridge_FakeUDPPortCreate(0, -2, &port));
    assert(SB_OK == SteamBridge_FakeUDPPortCreate(0, -1, &port) && port);
    assert(-1 == forwarded()->portIndex && 1 == forwarded()->portsCreated);
    assert(SteamBridge_IsFakeIPv4(0xa9fe0102) && !SteamBridge_IsFakeIPv4(0x0a000001));
    int32_t result{};
    assert(SB_OK == SteamBridge_FakeUDPPortSend(port, 0xa9fe0102, 27015, "data", 4, k_nSteamNetworkingSend_NoNagle, &result));
    assert(k_EResultOK == result && 0xa9fe0102 == forwarded()->sendIP && 27015 == forwarded()->sendPort);
    assert(4 == forwarded()->sendBytes && k_nSteamNetworkingSend_NoNagle == forwarded()->sendFlags);
    assert(SB_FAILED == SteamBridge_FakeUDPPortSend(port, 0, 27015, "data", 4, 0, &result) && k_EResultInvalidParam == result);
    char                      datagram[8]{};
    SteamBridgeFakeUDPMessage message{};
    assert(SB_OK == SteamBridge_FakeUDPPortReceive(port, datagram, sizeof(datagram), &message) && SB_FAKE_UDP_NONE == message.kind);
    queueFake(MOCK_FAKE_FROM_IP, 0xa9fe0102, 27015, 8);
    queueFake(MOCK_FAKE_FROM_IP, 0xa9fe0102, 27015, 9);
    queueFake(MOCK_FAKE_FROM_STEAM, 0, 0, 2);
    assert(SB_OK == SteamBridge_FakeUDPPortReceive(port, datagram, sizeof(datagram), &message));
    assert(SB_FAKE_UDP_MESSAGE == message.kind && 8 == message.bytes && 0xa9fe0102 == message.ip && 27015 == message.port && 'f' == datagram[7]);
    // A datagram that does not fit, or that does not come from an IP address, is dropped and described.
    assert(SB_OK == SteamBridge_FakeUDPPortReceive(port, datagram, sizeof(datagram), &message));
    assert(SB_FAKE_UDP_TOO_LARGE == message.kind && 9 == message.bytes && 0 == message.ip && !std::strcmp("169.254.1.2:27015", message.peer));
    assert(SB_OK == SteamBridge_FakeUDPPortReceive(port, datagram, sizeof(datagram), &message));
    assert(SB_FAKE_UDP_NOT_IP == message.kind && !std::strcmp("steamid:76561198000000004", message.peer));
    assert(3 == forwarded()->messagesReleased);
    assert(SB_OK == SteamBridge_FakeUDPPortScheduleCleanup(port, 0xa9fe0102, 27016));
    assert(0xa9fe0102 == forwarded()->cleanupIP && 27016 == forwarded()->cleanupPort);
    assert(SB_INVALID_ARGUMENT == SteamBridge_NetworkingSetDebugOutput(k_ESteamNetworkingSocketsDebugOutputType_Everything + 1, OnDebug));
    assert(SB_OK == SteamBridge_NetworkingSetDebugOutput(k_ESteamNetworkingSocketsDebugOutputType_Msg, OnDebug));
    assert(k_ESteamNetworkingSocketsDebugOutputType_Msg == forwarded()->debugLevel);
    assert(emitDebug(k_ESteamNetworkingSocketsDebugOutputType_Warning, "relay"));
    assert(k_ESteamNetworkingSocketsDebugOutputType_Warning == debugOutput.level && !std::strcmp("relay", debugOutput.text));
    assert(SB_OK == SteamBridge_NetworkingSetDebugOutput(k_ESteamNetworkingSocketsDebugOutputType_None, nullptr));
    assert(k_ESteamNetworkingSocketsDebugOutputType_None == forwarded()->debugLevel && !emitDebug(1, "off"));
    SteamBridge_FakeUDPPortDestroy(port);
    assert(1 == forwarded()->portsDestroyed);
    // A port left open is destroyed with the client session.
    assert(SB_OK == SteamBridge_FakeUDPPortCreate(0, -1, &port));

    ClientGameServerDeny_t deny{};
    deny.m_unGameServerIP   = 0x7f000001;
    deny.m_usGameServerPort = 27015;
    deny.m_bSecure          = 1;
    deny.m_uReason          = 4;
    Queue(deny, false);
    GameServerChangeRequested_t change{};
    std::strcpy(change.m_rgchServer, "10.0.0.1:27015");
    std::strcpy(change.m_rgchPassword, "secret");
    Queue(change, false);
    // Game server payloads are not dispatched by the client dispatcher.
    Queue(GSPolicyResponse_t{1}, true);
    SteamBridge_HostRunCallbacks();
    assert(2 == events.count && SB_CLIENT_GAMESERVER_CHANGE == events.last.kind);
    assert(!std::strcmp("10.0.0.1:27015", events.last.server) && !std::strcmp("secret", events.last.password));
    assert(1 == mock::stat(MOCK_HOST_QUEUED));

    GameOverlayActivated_t overlay{};
    overlay.m_bActive = 1;
    Queue(overlay, false);
    events.shutdown = true;
    SteamBridge_HostRunCallbacks();
    assert(3 == events.count && SB_CLIENT_OVERLAY == events.last.kind && events.last.active);
    assert(!SteamBridge_HostIsActive());
    assert(0 == mock::stat(MOCK_HOST_CALLBACKS) && 1 == mock::stat(MOCK_HOST_SHUTDOWNS));
    assert(2 == forwarded()->portsDestroyed && !SteamBridge_IsFakeIPv4(0xa9fe0102));
    assert(SB_UNAVAILABLE == SteamBridge_UserGetSteamID(&id));
    SteamBridge_HostSetClientCallback(nullptr, nullptr);
}
void Apps(const char* mock)
{
    setApps(1, "schinese", 1, 0);
    assert(SB_OK == SteamBridge_HostInit(mock, 1));
    uint32_t subscribed{};
    assert(SB_OK == SteamBridge_UserIsSubscribedApp(70, &subscribed));
    assert(1 == subscribed);
    assert(SB_OK == SteamBridge_UserIsSubscribedApp(10, &subscribed));
    assert(0 == subscribed);
    char language[16]{};
    assert(SB_OK == SteamBridge_UserGetCurrentGameLanguage(language, sizeof(language)));
    assert(!std::strcmp("schinese", language));
    char truncated[4]{};
    assert(SB_OK == SteamBridge_UserGetCurrentGameLanguage(truncated, sizeof(truncated)));
    assert(!std::strcmp("sch", truncated));
    assert(SB_INVALID_ARGUMENT == SteamBridge_UserGetCurrentGameLanguage(nullptr, sizeof(language)));
    assert(SB_INVALID_ARGUMENT == SteamBridge_UserGetCurrentGameLanguage(language, 0));
    uint32_t lowViolence{};
    assert(SB_OK == SteamBridge_UserIsLowViolence(&lowViolence));
    assert(1 == lowViolence);
    assert(SB_INVALID_ARGUMENT == SteamBridge_UserIsLowViolence(nullptr));
    SteamBridge_HostShutdown();

    // SteamApps without a language fails the query and still answers the others.
    setApps(1, nullptr, 0, 0);
    assert(SB_OK == SteamBridge_HostInit(mock, 1));
    std::strcpy(language, "x");
    assert(SB_FAILED == SteamBridge_UserGetCurrentGameLanguage(language, sizeof(language)));
    assert(0 == language[0]);
    lowViolence = 1;
    assert(SB_OK == SteamBridge_UserIsLowViolence(&lowViolence));
    assert(0 == lowViolence);
    SteamBridge_HostShutdown();
    setApps(0, nullptr, 0, 0);
    assert(!SteamBridge_HostIsActive());
}
void Servers(const char* mock)
{
    // Without a client there is no server browsing.
    ServersEvents               events;
    SteamBridgeServerListHandle request{};
    uint32_t                    query = 1, count = 1;
    assert(SB_UNAVAILABLE == SteamBridge_ServersRequestList(SB_SERVER_LIST_INTERNET, 70, nullptr, 0, OnServers, &events, &request) && !request);
    assert(SB_UNAVAILABLE == SteamBridge_ServersPing(0x0a000001, 27016, OnServers, &events, &query) && 0 == query);
    assert(SB_UNAVAILABLE == SteamBridge_MatchmakingGetFavoriteCount(&count) && 0 == count);

    setFriends(1);
    setApps(1, "english", 0, 1);
    setServerDetails("Crossfire server", "crossfire", 5);
    MatchmakingEvents matchmakingEvents;
    ClientEvents      clientEvents;
    SteamBridge_HostSetMatchmakingCallback(OnMatchmaking, &matchmakingEvents);
    SteamBridge_HostSetClientCallback(OnClient, &clientEvents);
    assert(SB_OK == SteamBridge_HostInit(mock, 1));

    SteamBridgeServerFilter filters[] = {{"gamedir", "valve"}, {"secure", "1"}};
    SteamBridgeServerFilter missing[] = {{"gamedir", nullptr}};
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersRequestList(-1, 70, nullptr, 0, OnServers, &events, &request));
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersRequestList(SB_SERVER_LIST_SPECTATOR + 1, 70, nullptr, 0, OnServers, &events, &request));
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersRequestList(SB_SERVER_LIST_INTERNET, 70, nullptr, 0, nullptr, &events, &request));
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersRequestList(SB_SERVER_LIST_INTERNET, 70, nullptr, 1, OnServers, &events, &request));
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersRequestList(SB_SERVER_LIST_INTERNET, 70, missing, 1, OnServers, &events, &request));
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersRequestList(SB_SERVER_LIST_INTERNET, 70, nullptr, 0, OnServers, &events, nullptr));
    // The fixture lists no servers for app 0.
    assert(SB_FAILED == SteamBridge_ServersRequestList(SB_SERVER_LIST_INTERNET, 0, nullptr, 0, OnServers, &events, &request) && !request);
    SteamBridgeServerListHandle lan{};
    assert(SB_OK == SteamBridge_ServersRequestList(SB_SERVER_LIST_LAN, 70, filters, 2, OnServers, &events, &lan) && lan);
    assert(SB_SERVER_LIST_LAN == forwarded()->listKind && 0 == forwarded()->listFilters);
    SteamBridge_ServersReleaseRequest(lan);
    assert(1 == forwarded()->listsReleased);
    // A released handle is rejected, and releasing it again does nothing.
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersRefreshQuery(lan));
    SteamBridge_ServersReleaseRequest(lan);
    assert(1 == forwarded()->listsReleased);

    // The fixture answers the last request.
    assert(SB_OK == SteamBridge_ServersRequestList(SB_SERVER_LIST_INTERNET, 70, filters, 2, OnServers, &events, &request) && request);
    assert(SB_SERVER_LIST_INTERNET == forwarded()->listKind && 70 == forwarded()->listAppID && 2 == forwarded()->listFilters);
    assert(!std::strcmp("gamedir", forwarded()->listFilterKey) && !std::strcmp("1", forwarded()->listFilterValue));
    SteamBridgeServerItem item{};
    assert(SB_OK == SteamBridge_ServersGetDetails(request, 0, &item));
    assert(0x0a000001 == item.ip && 27016 == item.queryPort && 27015 == item.connectionPort && 70 == item.appID);
    assert(5 == item.players && 32 == item.maxPlayers && item.secure && !item.password && 90000000000000042 == item.steamID);
    assert(!std::strcmp("Crossfire server", item.name) && !std::strcmp("crossfire", item.map) && !std::strcmp("valve", item.gameDir));
    assert(SB_FAILED == SteamBridge_ServersGetDetails(request, 1, &item) && 0 == item.ip && 0 == item.name[0]);
    uint32_t refreshing{};
    assert(SB_OK == SteamBridge_ServersIsRefreshing(request, &refreshing) && refreshing);
    assert(SB_OK == SteamBridge_ServersRefreshServer(request, 4) && 4 == forwarded()->refreshedServer);
    assert(SB_OK == SteamBridge_ServersRefreshQuery(request) && 1 == forwarded()->listRefreshes);
    assert(SB_OK == SteamBridge_ServersCancelQuery(request) && 1 == forwarded()->listCancels);

    // Steam answers during the client dispatch.
    listAnswer(MOCK_LIST_RESPONDED, 0);
    listAnswer(MOCK_LIST_FAILED, 1);
    listAnswer(MOCK_LIST_COMPLETE, eServerResponded);
    assert(0 == events.count);
    SteamBridge_HostRunCallbacks();
    assert(3 == events.count && SB_SERVERS_LIST_COMPLETE == events.last.kind && request == events.last.request);
    assert(eServerResponded == events.last.response && 0 == events.last.query);
    // A request released from its callback answers no more, and Steam releases it after the dispatcher returns.
    listAnswer(MOCK_LIST_RESPONDED, 0);
    listAnswer(MOCK_LIST_RESPONDED, 0);
    events.release = true;
    SteamBridge_HostRunCallbacks();
    assert(4 == events.count && SB_SERVERS_LIST_RESPONDED == events.last.kind && 0 == events.last.index);
    assert(1 == events.releasedInside && 2 == forwarded()->listsReleased);
    events.release = false;

    // The fixture cannot query port 0.
    assert(SB_FAILED == SteamBridge_ServersPing(0x0a000001, 0, OnServers, &events, &query) && 0 == query);
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersPing(0x0a000001, 27016, nullptr, &events, &query));
    assert(SB_OK == SteamBridge_ServersPing(0x0a000001, 27016, OnServers, &events, &query) && query);
    assert(0x0a000001 == forwarded()->queryIP && 27016 == forwarded()->queryPort);
    pingAnswer(1);
    SteamBridge_HostRunCallbacks();
    assert(5 == events.count && SB_SERVERS_PING_RESPONDED == events.last.kind && query == events.last.query && !events.last.request);
    assert(!std::strcmp("crossfire", events.last.server.map) && 27015 == events.last.server.connectionPort);
    // Steam forgets an answered query, so it is not cancelled.
    SteamBridge_ServersCancelServerQuery(query);
    assert(0 == forwarded()->queriesCancelled);
    uint32_t players{};
    assert(SB_OK == SteamBridge_ServersPlayerDetails(0x0a000001, 27016, OnServers, &events, &players) && players && players != query);
    playersAnswer(MOCK_PLAYERS_PLAYER);
    playersAnswer(MOCK_PLAYERS_COMPLETE);
    SteamBridge_HostRunCallbacks();
    assert(7 == events.count && SB_SERVERS_PLAYERS_COMPLETE == events.last.kind && players == events.last.query);
    assert(!std::strcmp("player", events.player.player) && 3 == events.player.score && 1.5f == events.player.seconds);
    // A cancelled query answers no more.
    assert(SB_OK == SteamBridge_ServersPing(0x0a000001, 27016, OnServers, &events, &query));
    pingAnswer(0);
    SteamBridge_ServersCancelServerQuery(query);
    assert(1 == forwarded()->queriesCancelled);
    SteamBridge_HostRunCallbacks();
    assert(7 == events.count);
    // Cancelled from its own callback, Steam cancels the query after the dispatcher returns.
    assert(SB_OK == SteamBridge_ServersPlayerDetails(0x0a000001, 27016, OnServers, &events, &players));
    playersAnswer(MOCK_PLAYERS_PLAYER);
    playersAnswer(MOCK_PLAYERS_PLAYER);
    events.cancel = players;
    SteamBridge_HostRunCallbacks();
    assert(8 == events.count && 1 == events.cancelledInside && 2 == forwarded()->queriesCancelled);
    events.cancel = 0;

    assert(SB_OK == SteamBridge_MatchmakingGetFavoriteCount(&count) && 2 == count);
    assert(SB_OK == SteamBridge_MatchmakingAddFavorite(70, 0x0a000001, 27015, 27016, k_unFavoriteFlagFavorite, 1234));
    assert(70 == forwarded()->favoriteAppID && 0x0a000001 == forwarded()->favoriteIP && 27015 == forwarded()->favoriteConnectionPort);
    assert(27016 == forwarded()->favoriteQueryPort && k_unFavoriteFlagFavorite == forwarded()->favoriteFlags && 1234 == forwarded()->favoriteLastPlayed);
    assert(SB_OK == SteamBridge_MatchmakingRemoveFavorite(70, 0x0a000001, 27015, 27016, k_unFavoriteFlagHistory));
    assert(k_unFavoriteFlagHistory == forwarded()->favoriteFlags);
    // The fixture has no entry for address 0.
    assert(SB_FAILED == SteamBridge_MatchmakingRemoveFavorite(70, 0, 27015, 27016, k_unFavoriteFlagHistory));

    // Favorites and persona changes reach the matchmaking callback, not the client callback.
    FavoritesListChanged_t changed{};
    changed.m_nIP         = 0x0a000001;
    changed.m_nQueryPort  = 27016;
    changed.m_nConnPort   = 27015;
    changed.m_nAppID      = 70;
    changed.m_nFlags      = k_unFavoriteFlagFavorite;
    changed.m_bAdd        = true;
    changed.m_unAccountId = 5;
    Queue(changed, false);
    PersonaStateChange_t persona{};
    persona.m_ulSteamID    = 76561198000000005;
    persona.m_nChangeFlags = k_EPersonaChangeName;
    Queue(persona, false);
    SteamBridge_HostRunCallbacks();
    assert(2 == matchmakingEvents.count && 0 == clientEvents.count);
    const SteamBridgeMatchmakingEvent& favorite = matchmakingEvents.first;
    assert(SB_MATCHMAKING_FAVORITES_CHANGED == favorite.kind && 0x0a000001 == favorite.ip && 27016 == favorite.queryPort && 27015 == favorite.connectionPort);
    assert(70 == favorite.appID && k_unFavoriteFlagFavorite == favorite.flags && favorite.add && 5 == favorite.accountID);
    assert(SB_MATCHMAKING_PERSONA_CHANGE == matchmakingEvents.last.kind && 76561198000000005 == matchmakingEvents.last.steamID);
    assert(k_EPersonaChangeName == matchmakingEvents.last.changeFlags);

    char name[4] = "x";
    assert(SB_OK == SteamBridge_FriendsGetPersonaName(76561198000000005, name, sizeof(name)) && !std::strcmp("fri", name));
    assert(SB_FAILED == SteamBridge_FriendsGetPersonaName(1, name, sizeof(name)) && 0 == name[0]);
    assert(SB_INVALID_ARGUMENT == SteamBridge_FriendsGetPersonaName(1, name, 0));
    SteamBridgeFriendGame game{};
    assert(SB_OK == SteamBridge_FriendsGetGamePlayed(76561198000000005, &game));
    assert(70 == game.gameID && 0x0a000002 == game.ip && 27015 == game.gamePort && 27016 == game.queryPort && 76561198000000006 == game.lobbySteamID);
    assert(SB_FAILED == SteamBridge_FriendsGetGamePlayed(1, &game) && 0 == game.gameID && 0 == game.ip);
    uint32_t banned{};
    assert(SB_OK == SteamBridge_AppsIsVACBanned(&banned) && banned);
    assert(SB_INVALID_ARGUMENT == SteamBridge_AppsIsVACBanned(nullptr));

    // Host shutdown releases the remaining requests and cancels the running queries.
    int released = forwarded()->listsReleased, cancelled = forwarded()->queriesCancelled;
    assert(SB_OK == SteamBridge_ServersRequestList(SB_SERVER_LIST_HISTORY, 70, nullptr, 0, OnServers, &events, &request));
    assert(SB_OK == SteamBridge_ServersPing(0x0a000001, 27016, OnServers, &events, &query));
    SteamBridge_HostShutdown();
    assert(released + 1 == forwarded()->listsReleased && cancelled + 1 == forwarded()->queriesCancelled);
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServersRefreshQuery(request));
    assert(SB_UNAVAILABLE == SteamBridge_AppsIsVACBanned(&banned) && 0 == banned);
    SteamBridge_HostSetMatchmakingCallback(nullptr, nullptr);
    SteamBridge_HostSetClientCallback(nullptr, nullptr);
    setFriends(0);
    setApps(0, nullptr, 0, 0);
}
void DedicatedServer(const char* mock)
{
    int shutdowns = mock::stat(MOCK_HOST_SHUTDOWNS);
    assert(SB_OK == SteamBridge_HostInit(mock, 0));
    assert(SteamBridge_HostIsActive() && !SteamBridge_HostIsClientActive());
    uint64_t id{};
    assert(SB_UNAVAILABLE == SteamBridge_UserGetSteamID(&id));
    assert(SB_UNAVAILABLE == SteamBridge_ServerSetProduct("valve"));
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServerInit(0, 27015, 27015, 0, "1.0"));
    assert(SB_OK == SteamBridge_ServerInit(0, 27015, 27015, SB_SERVER_MODE_SECURE, "1.0"));
    assert(SB_SERVER_MODE_SECURE == forwarded()->mode);
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServerInit(0, 27015, 27015, SB_SERVER_MODE_SECURE, "1.0"));
    assert(SteamBridge_ServerIsActive() && 7 == mock::stat(MOCK_HOST_CALLBACKS));

    // Without a client, the networking utilities come from the game server.
    assert(SteamBridge_IsFakeIPv4(0xa9fe0102));
    assert(SB_INVALID_ARGUMENT == SteamBridge_ServerBeginAsyncRequestFakeIP(0));
    assert(SB_OK == SteamBridge_ServerBeginAsyncRequestFakeIP(1) && 1 == forwarded()->fakeIPPorts);
    assert(SB_FAILED == SteamBridge_ServerBeginAsyncRequestFakeIP(2));
    int32_t  fakeResult = 1;
    uint32_t fakeIP     = 1;
    uint16_t fakePorts[2]{1, 1};
    setFakeIP(k_EResultBusy, 0xa9fe0102, 27015);
    assert(SB_OK == SteamBridge_ServerGetFakeIP(0, &fakeResult, &fakeIP, fakePorts, 2));
    assert(k_EResultBusy == fakeResult && 0 == fakeIP && 0 == fakePorts[0]);
    setFakeIP(k_EResultOK, 0xa9fe0102, 27015);
    assert(SB_OK == SteamBridge_ServerGetFakeIP(0, &fakeResult, &fakeIP, fakePorts, 2));
    assert(k_EResultOK == fakeResult && 0xa9fe0102 == fakeIP && 27015 == fakePorts[0] && 0 == fakePorts[1]);
    SteamBridgeFakeUDPPortHandle serverPort{};
    int                          destroyed = forwarded()->portsDestroyed;
    assert(SB_UNAVAILABLE == SteamBridge_FakeUDPPortCreate(0, -1, &serverPort));
    assert(SB_OK == SteamBridge_FakeUDPPortCreate(1, 0, &serverPort) && 0 == forwarded()->portIndex);
    assert(SB_OK == SteamBridge_ServerSetProduct("valve"));
    assert(!std::strcmp("valve", forwarded()->product));
    assert(SB_OK == SteamBridge_ServerSetKeyValue("protocol", "48"));
    assert(!std::strcmp("protocol", forwarded()->key) && !std::strcmp("48", forwarded()->value));
    assert(SteamBridge_ServerIsSecure());
    assert(90000000000000042 == SteamBridge_ServerGetSteamID());
    char ticket[4] = "abc";
    assert(SB_OK == SteamBridge_ServerSendUserConnectAndAuthenticate(0x0100007f, ticket, 3, &id));
    assert(76561198000000002 == id && 0x0100007f == forwarded()->authIP && 3 == forwarded()->authBytes);
    assert(SB_FAILED == SteamBridge_ServerSendUserConnectAndAuthenticate(0x0100007f, ticket, 1, &id));
    char     packet[4]{};
    uint32_t ip{};
    uint16_t port{};
    assert(2 == SteamBridge_ServerGetNextOutgoingPacket(packet, sizeof(packet), &ip, &port));
    assert(!std::memcmp(packet, "mp", 2) && 0x7f000001 == ip && 27010 == port);
    assert(0 == SteamBridge_ServerGetNextOutgoingPacket(packet, 1, &ip, &port));

    ServerEvents events;
    SteamBridge_ServerSetCallback(OnServer, &events);
    SteamServerConnectFailure_t failure{};
    failure.m_eResult = k_EResultNoConnection;
    Queue(failure, true);
    Queue(GSPolicyResponse_t{1}, true);
    GSClientApprove_t approve{};
    approve.m_SteamID      = CSteamID(uint64(76561198000000002));
    approve.m_OwnerSteamID = CSteamID(uint64(76561198000000003));
    Queue(approve, true);
    GSClientDeny_t deny{};
    deny.m_SteamID     = CSteamID(uint64(76561198000000002));
    deny.m_eDenyReason = k_EDenyNotLoggedOn;
    std::strcpy(deny.m_rgchOptionalText, "denied");
    Queue(deny, true);
    SteamBridge_ServerRunCallbacks();
    // The policy payload that the client dispatcher left queued arrives here too.
    assert(5 == events.count && SB_SERVER_CLIENT_DENY == events.last.kind && 0 == mock::stat(MOCK_HOST_QUEUED));
    assert(76561198000000002 == events.last.steamID && k_EDenyNotLoggedOn == int(events.last.reason) && !std::strcmp("denied", events.last.text));
    SteamNetworkingFakeIPResult_t allocated{};
    allocated.m_eResult = k_EResultOK;
    Queue(allocated, true);
    SteamBridge_ServerRunCallbacks();
    assert(6 == events.count && SB_SERVER_FAKE_IP_RESULT == events.last.kind && k_EResultOK == events.last.result);

    GSClientKick_t kick{};
    kick.m_SteamID     = CSteamID(uint64(76561198000000002));
    kick.m_eDenyReason = k_EDenyCheater;
    Queue(kick, true);
    events.shutdown = true;
    SteamBridge_ServerRunCallbacks();
    assert(7 == events.count && SB_SERVER_CLIENT_KICK == events.last.kind && k_EDenyCheater == int(events.last.reason));
    assert(!SteamBridge_ServerIsActive() && 0 == mock::stat(MOCK_HOST_CALLBACKS) && 1 == mock::stat(MOCK_HOST_SERVER_SHUTDOWNS));
    // The server port is destroyed with the game server, and its utilities go with it.
    assert(destroyed + 1 == forwarded()->portsDestroyed && !SteamBridge_IsFakeIPv4(0xa9fe0102));
    assert(SB_UNAVAILABLE == SteamBridge_ServerBeginAsyncRequestFakeIP(1));
    assert(SteamBridge_HostIsActive());
    SteamBridge_ServerSetCallback(nullptr, nullptr);

    // Host shutdown also shuts down an active game server; no client was initialized.
    assert(SB_OK == SteamBridge_ServerInit(0, 27015, 27015, SB_SERVER_MODE_AUTHENTICATION, "1.0"));
    SteamBridge_HostShutdown();
    assert(!SteamBridge_HostIsActive() && !SteamBridge_ServerIsActive());
    assert(2 == mock::stat(MOCK_HOST_SERVER_SHUTDOWNS) && shutdowns == mock::stat(MOCK_HOST_SHUTDOWNS) && 0 == mock::stat(MOCK_HOST_CALLBACKS));
}

int main(int argc, char** argv)
{
    assert(argc == 3);
    // Hold a fixture reference so its counters persist across bridge sessions.
    auto module = platform::LoadRuntime(argv[1]);
    assert(module);
    mock::failInit         = platform::Resolve<SetIntFn>(module, "MockHostSetFailInit");
    mock::voiceResult      = platform::Resolve<SetIntFn>(module, "MockHostSetVoiceResult");
    mock::stat             = platform::Resolve<StatFn>(module, "MockHostStat");
    mock::forwarded        = platform::Resolve<ForwardedFn>(module, "MockHostForwarded");
    mock::queue            = platform::Resolve<QueueFn>(module, "MockHostQueue");
    mock::queueFake        = platform::Resolve<QueueFakeFn>(module, "MockHostQueueFakeMessage");
    mock::setFakeIP        = platform::Resolve<SetFakeIPFn>(module, "MockHostSetFakeIP");
    mock::emitDebug        = platform::Resolve<EmitDebugFn>(module, "MockHostEmitDebug");
    mock::setApps          = platform::Resolve<SetAppsFn>(module, "MockHostSetApps");
    mock::setFriends       = platform::Resolve<SetIntFn>(module, "MockHostSetFriends");
    mock::setServerDetails = platform::Resolve<SetDetailsFn>(module, "MockHostSetServerDetails");
    mock::listAnswer       = platform::Resolve<AnswerFn>(module, "MockHostServerListAnswer");
    mock::pingAnswer       = platform::Resolve<SetIntFn>(module, "MockHostPingAnswer");
    mock::playersAnswer    = platform::Resolve<SetIntFn>(module, "MockHostPlayersAnswer");
    assert(mock::failInit && mock::voiceResult && mock::stat && mock::forwarded && mock::queue);
    assert(mock::queueFake && mock::setFakeIP && mock::emitDebug && mock::setApps);
    assert(mock::setFriends && mock::setServerDetails && mock::listAnswer && mock::pingAnswer && mock::playersAnswer);
    Unavailable(argv[2]);
    Client(argv[1]);
    Apps(argv[1]);
    Servers(argv[1]);
    DedicatedServer(argv[1]);
    platform::UnloadRuntime(module);
}
