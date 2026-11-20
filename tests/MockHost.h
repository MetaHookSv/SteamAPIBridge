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
};
enum
{
    MOCK_HOST_SHUTDOWNS        = 0,
    MOCK_HOST_SERVER_SHUTDOWNS = 1,
    MOCK_HOST_CALLBACKS        = 2,
    MOCK_HOST_QUEUED           = 3
};
