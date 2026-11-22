#pragma once
#include <stdint.h>

// Arguments that MockHost.cpp records from the bridge.
struct MockHostForwardedData
{
    uint64_t serverSteamID;
    uint32_t ip;
    uint16_t port;
    bool     secure;
    uint32_t terminated;
    char     product[64];
    char     key[64];
    char     value[64];
    uint32_t authIP;
    uint32_t authBytes;
    uint32_t pcmRate;
    int      mode;
    uint32_t pcmRequested;
    // FakeIP ports and the datagrams sent through them.
    int      portIndex;
    int      portsCreated;
    int      portsDestroyed;
    uint32_t sendIP;
    uint16_t sendPort;
    uint32_t sendBytes;
    int      sendFlags;
    uint32_t cleanupIP;
    uint16_t cleanupPort;
    int      messagesReleased;
    int      fakeIPPorts;
    int      debugLevel;
    // Server list requests (the SB_SERVER_LIST_* kind), their first filter and the calls made on them.
    int      listKind;
    uint32_t listAppID;
    uint32_t listFilters;
    char     listFilterKey[64];
    char     listFilterValue[64];
    int      listsReleased;
    int      listCancels;
    int      listRefreshes;
    int      refreshedServer;
    // Ping and player queries.
    uint32_t queryIP;
    uint16_t queryPort;
    int      queriesCancelled;
    // The favorite entry added or removed last.
    uint32_t favoriteAppID;
    uint32_t favoriteIP;
    uint16_t favoriteConnectionPort;
    uint16_t favoriteQueryPort;
    uint32_t favoriteFlags;
    uint32_t favoriteLastPlayed;
};
// Answers MockHostServerListAnswer, MockHostPingAnswer and MockHostPlayersAnswer queue for the next
// client dispatch, as Steam answers ISteamMatchmakingServers requests in SteamAPI_RunCallbacks.
enum
{
    MOCK_LIST_RESPONDED = 0,
    MOCK_LIST_FAILED    = 1,
    MOCK_LIST_COMPLETE  = 2
};
enum
{
    MOCK_PLAYERS_PLAYER   = 0,
    MOCK_PLAYERS_FAILED   = 1,
    MOCK_PLAYERS_COMPLETE = 2
};
// Kinds of message MockHostQueueFakeMessage puts on a FakeIP port.
enum
{
    MOCK_FAKE_FROM_IP    = 0,
    MOCK_FAKE_FROM_STEAM = 1
};
enum
{
    MOCK_HOST_SHUTDOWNS        = 0,
    MOCK_HOST_SERVER_SHUTDOWNS = 1,
    MOCK_HOST_CALLBACKS        = 2,
    MOCK_HOST_QUEUED           = 3
};
