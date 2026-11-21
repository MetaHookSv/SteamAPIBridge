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
