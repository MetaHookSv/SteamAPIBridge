#pragma once
// Empty implementations of every SteamUser023 and SteamGameServer015 method, generated from
// the Steamworks SDK v1.65 declarations. Mocks override the methods they exercise.
#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>

class StubUser : public ISteamUser
{
public:
    HSteamUser                  GetHSteamUser() override { return {}; }
    bool                        BLoggedOn() override { return {}; }
    CSteamID                    GetSteamID() override { return {}; }
    int                         InitiateGameConnection_DEPRECATED(void* pAuthBlob, int cbMaxAuthBlob, CSteamID steamIDGameServer, uint32 unIPServer, uint16 usPortServer, bool bSecure) override { return {}; }
    void                        TerminateGameConnection_DEPRECATED(uint32 unIPServer, uint16 usPortServer) override {}
    void                        TrackAppUsageEvent(CGameID gameID, int eAppUsageEvent, const char* pchExtraInfo) override {}
    bool                        GetUserDataFolder(char* pchBuffer, int cubBuffer) override { return {}; }
    void                        StartVoiceRecording() override {}
    void                        StopVoiceRecording() override {}
    EVoiceResult                GetAvailableVoice(uint32* pcbCompressed, uint32* pcbUncompressed_Deprecated, uint32 nUncompressedVoiceDesiredSampleRate_Deprecated) override { return {}; }
    EVoiceResult                GetVoice(bool bWantCompressed, void* pDestBuffer, uint32 cbDestBufferSize, uint32* nBytesWritten, bool bWantUncompressed_Deprecated, void* pUncompressedDestBuffer_Deprecated, uint32 cbUncompressedDestBufferSize_Deprecated, uint32* nUncompressBytesWritten_Deprecated, uint32 nUncompressedVoiceDesiredSampleRate_Deprecated) override { return {}; }
    EVoiceResult                DecompressVoice(const void* pCompressed, uint32 cbCompressed, void* pDestBuffer, uint32 cbDestBufferSize, uint32* nBytesWritten, uint32 nDesiredSampleRate) override { return {}; }
    uint32                      GetVoiceOptimalSampleRate() override { return {}; }
    HAuthTicket                 GetAuthSessionTicket(void* pTicket, int cbMaxTicket, uint32* pcbTicket, const SteamNetworkingIdentity* pSteamNetworkingIdentity) override { return {}; }
    HAuthTicket                 GetAuthTicketForWebApi(const char* pchIdentity) override { return {}; }
    EBeginAuthSessionResult     BeginAuthSession(const void* pAuthTicket, int cbAuthTicket, CSteamID steamID) override { return {}; }
    void                        EndAuthSession(CSteamID steamID) override {}
    void                        CancelAuthTicket(HAuthTicket hAuthTicket) override {}
    EUserHasLicenseForAppResult UserHasLicenseForApp(CSteamID steamID, AppId_t appID) override { return {}; }
    bool                        BIsBehindNAT() override { return {}; }
    void                        AdvertiseGame(CSteamID steamIDGameServer, uint32 unIPServer, uint16 usPortServer) override {}
    SteamAPICall_t              RequestEncryptedAppTicket(void* pDataToInclude, int cbDataToInclude) override { return {}; }
    bool                        GetEncryptedAppTicket(void* pTicket, int cbMaxTicket, uint32* pcbTicket) override { return {}; }
    int                         GetGameBadgeLevel(int nSeries, bool bFoil) override { return {}; }
    int                         GetPlayerSteamLevel() override { return {}; }
    SteamAPICall_t              RequestStoreAuthURL(const char* pchRedirectURL) override { return {}; }
    bool                        BIsPhoneVerified() override { return {}; }
    bool                        BIsTwoFactorEnabled() override { return {}; }
    bool                        BIsPhoneIdentifying() override { return {}; }
    bool                        BIsPhoneRequiringVerification() override { return {}; }
    SteamAPICall_t              GetMarketEligibility() override { return {}; }
    SteamAPICall_t              GetDurationControl() override { return {}; }
    bool                        BSetDurationControlOnlineState(EDurationControlOnlineState eNewState) override { return {}; }
};
class StubGameServer : public ISteamGameServer
{
public:
    bool                        InitGameServer(uint32 unIP, uint16 usGamePort, uint16 usQueryPort, uint32 unFlags, AppId_t nGameAppId, const char* pchVersionString) override { return {}; }
    void                        SetProduct(const char* pszProduct) override {}
    void                        SetGameDescription(const char* pszGameDescription) override {}
    void                        SetModDir(const char* pszModDir) override {}
    void                        SetDedicatedServer(bool bDedicated) override {}
    void                        LogOn(const char* pszToken) override {}
    void                        LogOnAnonymous() override {}
    void                        LogOff() override {}
    bool                        BLoggedOn() override { return {}; }
    bool                        BSecure() override { return {}; }
    CSteamID                    GetSteamID() override { return {}; }
    bool                        WasRestartRequested() override { return {}; }
    void                        SetMaxPlayerCount(int cPlayersMax) override {}
    void                        SetBotPlayerCount(int cBotplayers) override {}
    void                        SetServerName(const char* pszServerName) override {}
    void                        SetMapName(const char* pszMapName) override {}
    void                        SetPasswordProtected(bool bPasswordProtected) override {}
    void                        SetSpectatorPort(uint16 unSpectatorPort) override {}
    void                        SetSpectatorServerName(const char* pszSpectatorServerName) override {}
    void                        ClearAllKeyValues() override {}
    void                        SetKeyValue(const char* pKey, const char* pValue) override {}
    void                        SetGameTags(const char* pchGameTags) override {}
    void                        SetGameData(const char* pchGameData) override {}
    void                        SetRegion(const char* pszRegion) override {}
    void                        SetAdvertiseServerActive(bool bActive) override {}
    HAuthTicket                 GetAuthSessionTicket(void* pTicket, int cbMaxTicket, uint32* pcbTicket, const SteamNetworkingIdentity* pSnid) override { return {}; }
    EBeginAuthSessionResult     BeginAuthSession(const void* pAuthTicket, int cbAuthTicket, CSteamID steamID) override { return {}; }
    void                        EndAuthSession(CSteamID steamID) override {}
    void                        CancelAuthTicket(HAuthTicket hAuthTicket) override {}
    EUserHasLicenseForAppResult UserHasLicenseForApp(CSteamID steamID, AppId_t appID) override { return {}; }
    bool                        RequestUserGroupStatus(CSteamID steamIDUser, CSteamID steamIDGroup) override { return {}; }
    void                        GetGameplayStats() override {}
    SteamAPICall_t              GetServerReputation() override { return {}; }
    SteamIPAddress_t            GetPublicIP() override { return {}; }
    bool                        HandleIncomingPacket(const void* pData, int cbData, uint32 srcIP, uint16 srcPort) override { return {}; }
    int                         GetNextOutgoingPacket(void* pOut, int cbMaxOut, uint32* pNetAdr, uint16* pPort) override { return {}; }
    SteamAPICall_t              AssociateWithClan(CSteamID steamIDClan) override { return {}; }
    SteamAPICall_t              ComputeNewPlayerCompatibility(CSteamID steamIDNewPlayer) override { return {}; }
    bool                        SendUserConnectAndAuthenticate_DEPRECATED(uint32 unIPClient, const void* pvAuthBlob, uint32 cubAuthBlobSize, CSteamID* pSteamIDUser) override { return {}; }
    CSteamID                    CreateUnauthenticatedUserConnection() override { return {}; }
    void                        SendUserDisconnect_DEPRECATED(CSteamID steamIDUser) override {}
    bool                        BUpdateUserData(CSteamID steamIDUser, const char* pchPlayerName, uint32 uScore) override { return {}; }
    void                        SetMasterServerHeartbeatInterval_DEPRECATED(int iHeartbeatInterval) override {}
    void                        ForceMasterServerHeartbeat_DEPRECATED() override {}
};
