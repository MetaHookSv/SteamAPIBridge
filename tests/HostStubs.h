#pragma once
// Empty implementations of every SteamUser023, SteamGameServer015, SteamNetworkingSockets013 and
// SteamNetworkingUtils004 method, generated from the Steamworks SDK v1.65 declarations. Mocks
// override the methods they exercise.
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
class StubNetworkingSockets : public ISteamNetworkingSockets
{
public:
    HSteamListenSocket           CreateListenSocketIP(const SteamNetworkingIPAddr& localAddress, int nOptions, const SteamNetworkingConfigValue_t* pOptions) override { return {}; }
    HSteamNetConnection          ConnectByIPAddress(const SteamNetworkingIPAddr& address, int nOptions, const SteamNetworkingConfigValue_t* pOptions) override { return {}; }
    HSteamListenSocket           CreateListenSocketP2P(int nLocalVirtualPort, int nOptions, const SteamNetworkingConfigValue_t* pOptions) override { return {}; }
    HSteamNetConnection          ConnectP2P(const SteamNetworkingIdentity& identityRemote, int nRemoteVirtualPort, int nOptions, const SteamNetworkingConfigValue_t* pOptions) override { return {}; }
    EResult                      AcceptConnection(HSteamNetConnection hConn) override { return {}; }
    bool                         CloseConnection(HSteamNetConnection hPeer, int nReason, const char* pszDebug, bool bEnableLinger) override { return {}; }
    bool                         CloseListenSocket(HSteamListenSocket hSocket) override { return {}; }
    bool                         SetConnectionUserData(HSteamNetConnection hPeer, int64 nUserData) override { return {}; }
    int64                        GetConnectionUserData(HSteamNetConnection hPeer) override { return {}; }
    void                         SetConnectionName(HSteamNetConnection hPeer, const char* pszName) override {}
    bool                         GetConnectionName(HSteamNetConnection hPeer, char* pszName, int nMaxLen) override { return {}; }
    EResult                      SendMessageToConnection(HSteamNetConnection hConn, const void* pData, uint32 cbData, int nSendFlags, int64* pOutMessageNumber) override { return {}; }
    void                         SendMessages(int nMessages, SteamNetworkingMessage_t** pMessages, int64* pOutMessageNumberOrResult, bool bDeleteFailedMessages) override {}
    EResult                      FlushMessagesOnConnection(HSteamNetConnection hConn) override { return {}; }
    int                          ReceiveMessagesOnConnection(HSteamNetConnection hConn, SteamNetworkingMessage_t** ppOutMessages, int nMaxMessages) override { return {}; }
    bool                         GetConnectionInfo(HSteamNetConnection hConn, SteamNetConnectionInfo_t* pInfo) override { return {}; }
    EResult                      GetConnectionRealTimeStatus(HSteamNetConnection hConn, SteamNetConnectionRealTimeStatus_t* pStatus, int nLanes, SteamNetConnectionRealTimeLaneStatus_t* pLanes) override { return {}; }
    int                          GetDetailedConnectionStatus(HSteamNetConnection hConn, char* pszBuf, int cbBuf) override { return {}; }
    bool                         GetListenSocketAddress(HSteamListenSocket hSocket, SteamNetworkingIPAddr* address) override { return {}; }
    bool                         CreateSocketPair(HSteamNetConnection* pOutConnection1, HSteamNetConnection* pOutConnection2, bool bUseNetworkLoopback, const SteamNetworkingIdentity* pPeerIdentity1, const SteamNetworkingIdentity* pPeerIdentity2) override { return {}; }
    EResult                      ConfigureConnectionLanes(HSteamNetConnection hConn, int nNumLanes, const int* pLanePriorities, const uint16* pLaneWeights) override { return {}; }
    bool                         GetIdentity(SteamNetworkingIdentity* pIdentity) override { return {}; }
    ESteamNetworkingAvailability InitAuthentication() override { return {}; }
    ESteamNetworkingAvailability GetAuthenticationStatus(SteamNetAuthenticationStatus_t* pDetails) override { return {}; }
    HSteamNetPollGroup           CreatePollGroup() override { return {}; }
    bool                         DestroyPollGroup(HSteamNetPollGroup hPollGroup) override { return {}; }
    bool                         SetConnectionPollGroup(HSteamNetConnection hConn, HSteamNetPollGroup hPollGroup) override { return {}; }
    int                          ReceiveMessagesOnPollGroup(HSteamNetPollGroup hPollGroup, SteamNetworkingMessage_t** ppOutMessages, int nMaxMessages) override { return {}; }
    bool                         ReceivedRelayAuthTicket(const void* pvTicket, int cbTicket, SteamDatagramRelayAuthTicket* pOutParsedTicket) override { return {}; }
    int                          FindRelayAuthTicketForServer(const SteamNetworkingIdentity& identityGameServer, int nRemoteVirtualPort, SteamDatagramRelayAuthTicket* pOutParsedTicket) override { return {}; }
    HSteamNetConnection          ConnectToHostedDedicatedServer(const SteamNetworkingIdentity& identityTarget, int nRemoteVirtualPort, int nOptions, const SteamNetworkingConfigValue_t* pOptions) override { return {}; }
    uint16                       GetHostedDedicatedServerPort() override { return {}; }
    SteamNetworkingPOPID         GetHostedDedicatedServerPOPID() override { return {}; }
    EResult                      GetHostedDedicatedServerAddress(SteamDatagramHostedAddress* pRouting) override { return {}; }
    HSteamListenSocket           CreateHostedDedicatedServerListenSocket(int nLocalVirtualPort, int nOptions, const SteamNetworkingConfigValue_t* pOptions) override { return {}; }
    EResult                      GetGameCoordinatorServerLogin(SteamDatagramGameCoordinatorServerLogin* pLoginInfo, int* pcbSignedBlob, void* pBlob) override { return {}; }
    HSteamNetConnection          ConnectP2PCustomSignaling(ISteamNetworkingConnectionSignaling* pSignaling, const SteamNetworkingIdentity* pPeerIdentity, int nRemoteVirtualPort, int nOptions, const SteamNetworkingConfigValue_t* pOptions) override { return {}; }
    bool                         ReceivedP2PCustomSignal(const void* pMsg, int cbMsg, ISteamNetworkingSignalingRecvContext* pContext) override { return {}; }
    bool                         GetCertificateRequest(int* pcbBlob, void* pBlob, SteamNetworkingErrMsg& errMsg) override { return {}; }
    bool                         SetCertificate(const void* pCertificate, int cbCertificate, SteamNetworkingErrMsg& errMsg) override { return {}; }
    void                         ResetIdentity(const SteamNetworkingIdentity* pIdentity) override {}
    void                         RunCallbacks() override {}
    bool                         BeginAsyncRequestFakeIP(int nNumPorts) override { return {}; }
    void                         GetFakeIP(int idxFirstPort, SteamNetworkingFakeIPResult_t* pInfo) override {}
    HSteamListenSocket           CreateListenSocketP2PFakeIP(int idxFakePort, int nOptions, const SteamNetworkingConfigValue_t* pOptions) override { return {}; }
    EResult                      GetRemoteFakeIPForConnection(HSteamNetConnection hConn, SteamNetworkingIPAddr* pOutAddr) override { return {}; }
    ISteamNetworkingFakeUDPPort* CreateFakeUDPPort(int idxFakeServerPort) override { return {}; }
};
class StubNetworkingUtils : public ISteamNetworkingUtils
{
public:
    SteamNetworkingMessage_t*            AllocateMessage(int cbAllocateBuffer) override { return {}; }
    ESteamNetworkingAvailability         GetRelayNetworkStatus(SteamRelayNetworkStatus_t* pDetails) override { return {}; }
    float                                GetLocalPingLocation(SteamNetworkPingLocation_t& result) override { return {}; }
    int                                  EstimatePingTimeBetweenTwoLocations(const SteamNetworkPingLocation_t& location1, const SteamNetworkPingLocation_t& location2) override { return {}; }
    int                                  EstimatePingTimeFromLocalHost(const SteamNetworkPingLocation_t& remoteLocation) override { return {}; }
    void                                 ConvertPingLocationToString(const SteamNetworkPingLocation_t& location, char* pszBuf, int cchBufSize) override {}
    bool                                 ParsePingLocationString(const char* pszString, SteamNetworkPingLocation_t& result) override { return {}; }
    bool                                 CheckPingDataUpToDate(float flMaxAgeSeconds) override { return {}; }
    int                                  GetPingToDataCenter(SteamNetworkingPOPID popID, SteamNetworkingPOPID* pViaRelayPoP) override { return {}; }
    int                                  GetDirectPingToPOP(SteamNetworkingPOPID popID) override { return {}; }
    int                                  GetPOPCount() override { return {}; }
    int                                  GetPOPList(SteamNetworkingPOPID* list, int nListSz) override { return {}; }
    SteamNetworkingMicroseconds          GetLocalTimestamp() override { return {}; }
    void                                 SetDebugOutputFunction(ESteamNetworkingSocketsDebugOutputType eDetailLevel, FSteamNetworkingSocketsDebugOutput pfnFunc) override {}
    ESteamNetworkingFakeIPType           GetIPv4FakeIPType(uint32 nIPv4) override { return {}; }
    EResult                              GetRealIdentityForFakeIP(const SteamNetworkingIPAddr& fakeIP, SteamNetworkingIdentity* pOutRealIdentity) override { return {}; }
    bool                                 SetConfigValue(ESteamNetworkingConfigValue eValue, ESteamNetworkingConfigScope eScopeType, intptr_t scopeObj, ESteamNetworkingConfigDataType eDataType, const void* pArg) override { return {}; }
    ESteamNetworkingGetConfigValueResult GetConfigValue(ESteamNetworkingConfigValue eValue, ESteamNetworkingConfigScope eScopeType, intptr_t scopeObj, ESteamNetworkingConfigDataType* pOutDataType, void* pResult, size_t* cbResult) override { return {}; }
    const char*                          GetConfigValueInfo(ESteamNetworkingConfigValue eValue, ESteamNetworkingConfigDataType* pOutDataType, ESteamNetworkingConfigScope* pOutScope) override { return {}; }
    ESteamNetworkingConfigValue          IterateGenericEditableConfigValues(ESteamNetworkingConfigValue eCurrent, bool bEnumerateDevVars) override { return {}; }
    void                                 SteamNetworkingIPAddr_ToString(const SteamNetworkingIPAddr& addr, char* buf, size_t cbBuf, bool bWithPort) override {}
    bool                                 SteamNetworkingIPAddr_ParseString(SteamNetworkingIPAddr* pAddr, const char* pszStr) override { return {}; }
    ESteamNetworkingFakeIPType           SteamNetworkingIPAddr_GetFakeIPType(const SteamNetworkingIPAddr& addr) override { return {}; }
    void                                 SteamNetworkingIdentity_ToString(const SteamNetworkingIdentity& identity, char* buf, size_t cbBuf) override {}
    bool                                 SteamNetworkingIdentity_ParseString(SteamNetworkingIdentity* pIdentity, const char* pszStr) override { return {}; }
};
