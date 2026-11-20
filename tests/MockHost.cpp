// Host-mode fixture runtime. It exports the modern entry points that SteamBridge_HostInit
// resolves, records forwarded arguments and dispatches queued callback payloads.
#include "HostStubs.h"
#include "MockHost.h"
#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
int                         failInit, voiceResult = k_EVoiceResultOK;
int                         shutdowns, serverShutdowns;
bool                        serverInitialized;
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
                callback->Run(item.payload.data());
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

extern "C"
{
    S_API ESteamAPIInitResult S_CALLTYPE SteamAPI_InitFlat(SteamErrMsg*) { return failInit ? k_ESteamAPIInitResult_NoSteamClient : k_ESteamAPIInitResult_OK; }
    S_API void S_CALLTYPE                SteamAPI_Shutdown() { ++shutdowns; }
    S_API void S_CALLTYPE                SteamAPI_RunCallbacks() { Dispatch(false); }
    S_API HSteamUser S_CALLTYPE          SteamAPI_GetHSteamUser() { return 1; }
    S_API void* S_CALLTYPE               SteamInternal_FindOrCreateUserInterface(HSteamUser, const char* version) { return !std::strcmp(version, STEAMUSER_INTERFACE_VERSION) ? &user : nullptr; }
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
    S_API void* S_CALLTYPE      SteamInternal_FindOrCreateGameServerInterface(HSteamUser, const char* version) { return serverInitialized && !std::strcmp(version, STEAMGAMESERVER_INTERFACE_VERSION) ? &server : nullptr; }
    S_API void S_CALLTYPE       SteamAPI_RegisterCallback(CCallbackBase* callback, int) { callbacks.push_back(callback); }
    S_API void S_CALLTYPE       SteamAPI_UnregisterCallback(CCallbackBase* callback) { std::erase(callbacks, callback); }

    // Fixture control.
    S_API void S_CALLTYPE MockHostSetFailInit(int value) { failInit = value; }
    S_API void S_CALLTYPE MockHostSetVoiceResult(int value) { voiceResult = value; }
    S_API int S_CALLTYPE  MockHostStat(int which)
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
        queue.push_back({id, gameServer != 0, std::vector<char>(data, data + bytes)});
    }
}
