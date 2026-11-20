// Loader and authentication smoke check against a real Steam client. Not run by CTest:
// it needs a running, logged-in Steam client and steam_appid.txt in the working directory.
// Usage: SteamBridgeHostSmoke <steam_api runtime> [seconds]
#include <SteamAPIBridge.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

static bool                  approved, denied, loggedOn;
static void STEAMBRIDGE_CALL OnServer(void*, const SteamBridgeServerEvent* event)
{
    std::printf("server event %u result %d secure %u reason %u steamID %llu %s\n", event->kind, event->result, event->secure, event->reason, (unsigned long long)event->steamID, event->text);
    loggedOn |= event->kind == SB_SERVER_LOGON_SUCCESS;
    approved |= event->kind == SB_SERVER_CLIENT_APPROVE;
    denied |= event->kind == SB_SERVER_CLIENT_DENY;
}
int main(int argc, char** argv)
{
    if (argc < 2) return 2;
    int seconds = argc > 2 ? std::atoi(argv[2]) : 20;
    if (SteamBridgeStatus status = SteamBridge_HostInit(argv[1], 1))
    {
        std::printf("SteamBridge_HostInit failed: %d\n", status);
        return 1;
    }
    uint64_t id{};
    char     name[64]{};
    SteamBridge_UserGetSteamID(&id);
    SteamBridge_UserGetPersonaName(name, sizeof(name));
    std::printf("client steamID %llu persona %s optimal voice rate %u\n", (unsigned long long)id, name, SteamBridge_VoiceGetOptimalSampleRate());
    SteamBridge_ServerSetCallback(OnServer, nullptr);
    if (SteamBridge_ServerInit(0, 27115, 27115, SB_SERVER_MODE_AUTHENTICATION, "1.0.0.0") || SteamBridge_ServerSetProduct("valve") || SteamBridge_ServerSetModDir("valve") || SteamBridge_ServerSetGameDescription("Half-Life") || SteamBridge_ServerLogOnAnonymous())
    {
        std::printf("game server initialization failed\n");
        SteamBridge_HostShutdown();
        return 1;
    }
    bool authenticated = false;
    for (int tick = 0; tick < seconds * 10 && !approved && !denied; ++tick)
    {
        SteamBridge_HostRunCallbacks();
        SteamBridge_ServerRunCallbacks();
        if (loggedOn && !authenticated)
        {
            authenticated = true;
            char     ticket[2048];
            uint32_t size{};
            uint64_t serverID = SteamBridge_ServerGetSteamID();
            // 127.0.0.1 in host order for the client, network order for the server.
            SteamBridgeStatus initiate = SteamBridge_UserInitiateGameConnection(ticket, sizeof(ticket), serverID, 0x7f000001, 27115, SteamBridge_ServerIsSecure(), &size);
            uint64_t          user{};
            SteamBridgeStatus connect = SteamBridge_ServerSendUserConnectAndAuthenticate(0x0100007f, ticket, size, &user);
            std::printf("server steamID %llu ticket %u bytes (%d), connect %d user %llu\n", (unsigned long long)serverID, size, initiate, connect, (unsigned long long)user);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::printf("logged on %d approved %d denied %d\n", loggedOn, approved, denied);
    SteamBridge_HostShutdown();
    return approved ? 0 : 1;
}
