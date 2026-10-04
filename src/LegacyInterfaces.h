#pragma once
// Windows x86 ABI declarations, derived from Valve SDK v1.22/v1.23:
// https://github.com/rlabrecque/SteamworksSDK/tree/8cfe062785c4568840eb2679a7076f309bafb05f/public/steam
// https://github.com/rlabrecque/SteamworksSDK/tree/b74ea091c52a341a7cf6c1349c625a61f1eb3e3b/public/steam
// Only the used User016 prefix is declared. Never cast it to modern ISteamUser.
namespace legacy {
class User016 {
public:
    virtual HSteamUser GetHSteamUser() = 0;
    virtual bool BLoggedOn() = 0;
    virtual CSteamID GetSteamID() = 0;
};
class Screenshots001 {
public:
    virtual ScreenshotHandle WriteScreenshot(void*, uint32, int, int) = 0;
    virtual ScreenshotHandle AddScreenshotToLibrary(const char*, const char*, int, int) = 0;
    virtual void TriggerScreenshot() = 0;
    virtual void HookScreenshots(bool) = 0;
    virtual bool SetLocation(ScreenshotHandle, const char*) = 0;
    virtual bool TagUser(ScreenshotHandle, CSteamID) = 0;
};
class Client012 {
public:
    virtual HSteamPipe CreateSteamPipe() = 0;
    virtual bool BReleaseSteamPipe(HSteamPipe) = 0;
    virtual HSteamUser ConnectToGlobalUser(HSteamPipe) = 0;
    virtual HSteamUser CreateLocalUser(HSteamPipe*, EAccountType) = 0;
    virtual void ReleaseUser(HSteamPipe, HSteamUser) = 0;
    virtual void* GetISteamUser(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamGameServer(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void SetLocalIPBinding(uint32, uint16) = 0;
    virtual void* GetISteamFriends(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamUtils(HSteamPipe, const char*) = 0;
    virtual void* GetISteamMatchmaking(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamMatchmakingServers(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamGenericInterface(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamUserStats(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamGameServerStats(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamApps(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamNetworking(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamRemoteStorage(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void* GetISteamScreenshots(HSteamUser, HSteamPipe, const char*) = 0;
    virtual void RunFrame() = 0;
    virtual uint32 GetIPCCallCount() = 0;
    virtual void SetWarningMessageHook(SteamAPIWarningMessageHook_t) = 0;
    virtual bool BShutdownIfAllPipesClosed() = 0;
    // GetISteamPS3OverlayRender is PS3-only; it has no slot in the Windows ABI.
    virtual void* GetISteamHTTP(HSteamUser, HSteamPipe, const char*) = 0;
};
}
