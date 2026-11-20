#include "Platform.h"
#include "MockHost.h"
#include <SteamAPIBridge.h>
#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>
#include <cassert>
#include <cstring>

using SetIntFn    = void(S_CALLTYPE*)(int);
using StatFn      = int(S_CALLTYPE*)(int);
using ForwardedFn = const MockHostForwardedData*(S_CALLTYPE*)();
using QueueFn     = void(S_CALLTYPE*)(int, int, const void*, int);
namespace mock
{
SetIntFn    failInit, voiceResult;
StatFn      stat;
ForwardedFn forwarded;
QueueFn     queue;
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
    assert(3 == mock::stat(MOCK_HOST_CALLBACKS));

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
    // The fixture exposes no SteamFriends/SteamApps; those calls report unavailability.
    char name[4] = "x";
    assert(SB_UNAVAILABLE == SteamBridge_UserGetPersonaName(name, sizeof(name)));
    assert(0 == name[0]);
    uint32_t subscribed = 1;
    assert(SB_UNAVAILABLE == SteamBridge_UserIsSubscribedApp(70, &subscribed));
    assert(0 == subscribed);

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
    assert(SB_UNAVAILABLE == SteamBridge_UserGetSteamID(&id));
    SteamBridge_HostSetClientCallback(nullptr, nullptr);
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
    assert(SteamBridge_ServerIsActive() && 6 == mock::stat(MOCK_HOST_CALLBACKS));
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

    GSClientKick_t kick{};
    kick.m_SteamID     = CSteamID(uint64(76561198000000002));
    kick.m_eDenyReason = k_EDenyCheater;
    Queue(kick, true);
    events.shutdown = true;
    SteamBridge_ServerRunCallbacks();
    assert(6 == events.count && SB_SERVER_CLIENT_KICK == events.last.kind && k_EDenyCheater == int(events.last.reason));
    assert(!SteamBridge_ServerIsActive() && 0 == mock::stat(MOCK_HOST_CALLBACKS) && 1 == mock::stat(MOCK_HOST_SERVER_SHUTDOWNS));
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
    mock::failInit    = platform::Resolve<SetIntFn>(module, "MockHostSetFailInit");
    mock::voiceResult = platform::Resolve<SetIntFn>(module, "MockHostSetVoiceResult");
    mock::stat        = platform::Resolve<StatFn>(module, "MockHostStat");
    mock::forwarded   = platform::Resolve<ForwardedFn>(module, "MockHostForwarded");
    mock::queue       = platform::Resolve<QueueFn>(module, "MockHostQueue");
    assert(mock::failInit && mock::voiceResult && mock::stat && mock::forwarded && mock::queue);
    Unavailable(argv[2]);
    Client(argv[1]);
    DedicatedServer(argv[1]);
    platform::UnloadRuntime(module);
}
