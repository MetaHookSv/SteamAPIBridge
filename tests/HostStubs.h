#pragma once
// Empty implementations of every SteamUser023, SteamApps009, SteamGameServer015, SteamNetworkingSockets013,
// SteamNetworkingUtils004, SteamMatchMaking009, SteamMatchMakingServers003 and SteamFriends018 method,
// generated from the Steamworks SDK v1.65 declarations. Mocks override the methods they exercise.
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
class StubApps : public ISteamApps
{
public:
    bool           BIsSubscribed() override { return {}; }
    bool           BIsLowViolence() override { return {}; }
    bool           BIsCybercafe() override { return {}; }
    bool           BIsVACBanned() override { return {}; }
    const char*    GetCurrentGameLanguage() override { return {}; }
    const char*    GetAvailableGameLanguages() override { return {}; }
    bool           BIsSubscribedApp(AppId_t appID) override { return {}; }
    bool           BIsDlcInstalled(AppId_t appID) override { return {}; }
    uint32         GetEarliestPurchaseUnixTime(AppId_t nAppID) override { return {}; }
    bool           BIsSubscribedFromFreeWeekend() override { return {}; }
    int            GetDLCCount() override { return {}; }
    bool           BGetDLCDataByIndex(int iDLC, AppId_t* pAppID, bool* pbAvailable, char* pchName, int cchNameBufferSize) override { return {}; }
    void           InstallDLC(AppId_t nAppID) override {}
    void           UninstallDLC(AppId_t nAppID) override {}
    void           RequestAppProofOfPurchaseKey(AppId_t nAppID) override {}
    bool           GetCurrentBetaName(char* pchName, int cchNameBufferSize) override { return {}; }
    bool           MarkContentCorrupt(bool bMissingFilesOnly) override { return {}; }
    uint32         GetInstalledDepots(AppId_t appID, DepotId_t* pvecDepots, uint32 cMaxDepots) override { return {}; }
    uint32         GetAppInstallDir(AppId_t appID, char* pchFolder, uint32 cchFolderBufferSize) override { return {}; }
    bool           BIsAppInstalled(AppId_t appID) override { return {}; }
    CSteamID       GetAppOwner() override { return {}; }
    const char*    GetLaunchQueryParam(const char* pchKey) override { return {}; }
    bool           GetDlcDownloadProgress(AppId_t nAppID, uint64* punBytesDownloaded, uint64* punBytesTotal) override { return {}; }
    int            GetAppBuildId() override { return {}; }
    void           RequestAllProofOfPurchaseKeys() override {}
    SteamAPICall_t GetFileDetails(const char* pszFileName) override { return {}; }
    int            GetLaunchCommandLine(char* pszCommandLine, int cubCommandLine) override { return {}; }
    bool           BIsSubscribedFromFamilySharing() override { return {}; }
    bool           BIsTimedTrial(uint32* punSecondsAllowed, uint32* punSecondsPlayed) override { return {}; }
    bool           SetDlcContext(AppId_t nAppID) override { return {}; }
    int            GetNumBetas(int* pnAvailable, int* pnPrivate) override { return {}; }
    bool           GetBetaInfo(int iBetaIndex, uint32* punFlags, uint32* punBuildID, char* pchBetaName, int cchBetaName, char* pchDescription, int cchDescription, uint32* punLastUpdated) override { return {}; }
    bool           SetActiveBeta(const char* pchBetaName) override { return {}; }
    void           SetGamePerformanceSetting(EGamePerformanceSetting setting) override {}
    void           SetGameRenderResolution(uint32 unWidth, uint32 unHeight) override {}
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
class StubMatchmaking : public ISteamMatchmaking
{
public:
    int            GetFavoriteGameCount() override { return {}; }
    bool           GetFavoriteGame(int iGame, AppId_t* pnAppID, uint32* pnIP, uint16* pnConnPort, uint16* pnQueryPort, uint32* punFlags, uint32* pRTime32LastPlayedOnServer) override { return {}; }
    int            AddFavoriteGame(AppId_t nAppID, uint32 nIP, uint16 nConnPort, uint16 nQueryPort, uint32 unFlags, uint32 rTime32LastPlayedOnServer) override { return {}; }
    bool           RemoveFavoriteGame(AppId_t nAppID, uint32 nIP, uint16 nConnPort, uint16 nQueryPort, uint32 unFlags) override { return {}; }
    SteamAPICall_t RequestLobbyList() override { return {}; }
    void           AddRequestLobbyListStringFilter(const char* pchKeyToMatch, const char* pchValueToMatch, ELobbyComparison eComparisonType) override {}
    void           AddRequestLobbyListNumericalFilter(const char* pchKeyToMatch, int nValueToMatch, ELobbyComparison eComparisonType) override {}
    void           AddRequestLobbyListNearValueFilter(const char* pchKeyToMatch, int nValueToBeCloseTo) override {}
    void           AddRequestLobbyListFilterSlotsAvailable(int nSlotsAvailable) override {}
    void           AddRequestLobbyListDistanceFilter(ELobbyDistanceFilter eLobbyDistanceFilter) override {}
    void           AddRequestLobbyListResultCountFilter(int cMaxResults) override {}
    void           AddRequestLobbyListCompatibleMembersFilter(CSteamID steamIDLobby) override {}
    CSteamID       GetLobbyByIndex(int iLobby) override { return {}; }
    SteamAPICall_t CreateLobby(ELobbyType eLobbyType, int cMaxMembers) override { return {}; }
    SteamAPICall_t JoinLobby(CSteamID steamIDLobby) override { return {}; }
    void           LeaveLobby(CSteamID steamIDLobby) override {}
    bool           InviteUserToLobby(CSteamID steamIDLobby, CSteamID steamIDInvitee) override { return {}; }
    int            GetNumLobbyMembers(CSteamID steamIDLobby) override { return {}; }
    CSteamID       GetLobbyMemberByIndex(CSteamID steamIDLobby, int iMember) override { return {}; }
    const char*    GetLobbyData(CSteamID steamIDLobby, const char* pchKey) override { return {}; }
    bool           SetLobbyData(CSteamID steamIDLobby, const char* pchKey, const char* pchValue) override { return {}; }
    int            GetLobbyDataCount(CSteamID steamIDLobby) override { return {}; }
    bool           GetLobbyDataByIndex(CSteamID steamIDLobby, int iLobbyData, char* pchKey, int cchKeyBufferSize, char* pchValue, int cchValueBufferSize) override { return {}; }
    bool           DeleteLobbyData(CSteamID steamIDLobby, const char* pchKey) override { return {}; }
    const char*    GetLobbyMemberData(CSteamID steamIDLobby, CSteamID steamIDUser, const char* pchKey) override { return {}; }
    void           SetLobbyMemberData(CSteamID steamIDLobby, const char* pchKey, const char* pchValue) override {}
    bool           SendLobbyChatMsg(CSteamID steamIDLobby, const void* pvMsgBody, int cubMsgBody) override { return {}; }
    int            GetLobbyChatEntry(CSteamID steamIDLobby, int iChatID, CSteamID* pSteamIDUser, void* pvData, int cubData, EChatEntryType* peChatEntryType) override { return {}; }
    bool           RequestLobbyData(CSteamID steamIDLobby) override { return {}; }
    void           SetLobbyGameServer(CSteamID steamIDLobby, uint32 unGameServerIP, uint16 unGameServerPort, CSteamID steamIDGameServer) override {}
    bool           GetLobbyGameServer(CSteamID steamIDLobby, uint32* punGameServerIP, uint16* punGameServerPort, CSteamID* psteamIDGameServer) override { return {}; }
    bool           SetLobbyMemberLimit(CSteamID steamIDLobby, int cMaxMembers) override { return {}; }
    int            GetLobbyMemberLimit(CSteamID steamIDLobby) override { return {}; }
    bool           SetLobbyType(CSteamID steamIDLobby, ELobbyType eLobbyType) override { return {}; }
    bool           SetLobbyJoinable(CSteamID steamIDLobby, bool bLobbyJoinable) override { return {}; }
    CSteamID       GetLobbyOwner(CSteamID steamIDLobby) override { return {}; }
    bool           SetLobbyOwner(CSteamID steamIDLobby, CSteamID steamIDNewOwner) override { return {}; }
    bool           SetLinkedLobby(CSteamID steamIDLobby, CSteamID steamIDLobbyDependent) override { return {}; }
};
class StubMatchmakingServers : public ISteamMatchmakingServers
{
public:
    HServerListRequest RequestInternetServerList(AppId_t iApp, MatchMakingKeyValuePair_t** ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse* pRequestServersResponse) override { return {}; }
    HServerListRequest RequestLANServerList(AppId_t iApp, ISteamMatchmakingServerListResponse* pRequestServersResponse) override { return {}; }
    HServerListRequest RequestFriendsServerList(AppId_t iApp, MatchMakingKeyValuePair_t** ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse* pRequestServersResponse) override { return {}; }
    HServerListRequest RequestFavoritesServerList(AppId_t iApp, MatchMakingKeyValuePair_t** ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse* pRequestServersResponse) override { return {}; }
    HServerListRequest RequestHistoryServerList(AppId_t iApp, MatchMakingKeyValuePair_t** ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse* pRequestServersResponse) override { return {}; }
    HServerListRequest RequestSpectatorServerList(AppId_t iApp, MatchMakingKeyValuePair_t** ppchFilters, uint32 nFilters, ISteamMatchmakingServerListResponse* pRequestServersResponse) override { return {}; }
    void               ReleaseRequest(HServerListRequest hServerListRequest) override {}
    gameserveritem_t*  GetServerDetails(HServerListRequest hRequest, int iServer) override { return {}; }
    void               CancelQuery(HServerListRequest hRequest) override {}
    void               RefreshQuery(HServerListRequest hRequest) override {}
    bool               IsRefreshing(HServerListRequest hRequest) override { return {}; }
    int                GetServerCount(HServerListRequest hRequest) override { return {}; }
    void               RefreshServer(HServerListRequest hRequest, int iServer) override {}
    HServerQuery       PingServer(uint32 unIP, uint16 usPort, ISteamMatchmakingPingResponse* pRequestServersResponse) override { return {}; }
    HServerQuery       PlayerDetails(uint32 unIP, uint16 usPort, ISteamMatchmakingPlayersResponse* pRequestServersResponse) override { return {}; }
    HServerQuery       ServerRules(uint32 unIP, uint16 usPort, ISteamMatchmakingRulesResponse* pRequestServersResponse) override { return {}; }
    HServerQuery       ServerFriends(uint32 unIP, uint16 usPort, ISteamMatchmakingServerFriendsResponse* pRequestServersResponse) override { return {}; }
    void               CancelServerQuery(HServerQuery hServerQuery) override {}
};
class StubFriends : public ISteamFriends
{
public:
    const char*         GetPersonaName() override { return {}; }
    EPersonaState       GetPersonaState() override { return {}; }
    int                 GetFriendCount(int iFriendFlags) override { return {}; }
    CSteamID            GetFriendByIndex(int iFriend, int iFriendFlags) override { return {}; }
    EFriendRelationship GetFriendRelationship(CSteamID steamIDFriend) override { return {}; }
    EPersonaState       GetFriendPersonaState(CSteamID steamIDFriend) override { return {}; }
    const char*         GetFriendPersonaName(CSteamID steamIDFriend) override { return {}; }
    bool                GetFriendGamePlayed(CSteamID steamIDFriend, FriendGameInfo_t* pFriendGameInfo) override { return {}; }
    const char*         GetFriendPersonaNameHistory(CSteamID steamIDFriend, int iPersonaName) override { return {}; }
    int                 GetFriendSteamLevel(CSteamID steamIDFriend) override { return {}; }
    const char*         GetPlayerNickname(CSteamID steamIDPlayer) override { return {}; }
    int                 GetFriendsGroupCount() override { return {}; }
    FriendsGroupID_t    GetFriendsGroupIDByIndex(int iFG) override { return {}; }
    const char*         GetFriendsGroupName(FriendsGroupID_t friendsGroupID) override { return {}; }
    int                 GetFriendsGroupMembersCount(FriendsGroupID_t friendsGroupID) override { return {}; }
    void                GetFriendsGroupMembersList(FriendsGroupID_t friendsGroupID, CSteamID* pOutSteamIDMembers, int nMembersCount) override {}
    bool                HasFriend(CSteamID steamIDFriend, int iFriendFlags) override { return {}; }
    int                 GetClanCount() override { return {}; }
    CSteamID            GetClanByIndex(int iClan) override { return {}; }
    const char*         GetClanName(CSteamID steamIDClan) override { return {}; }
    const char*         GetClanTag(CSteamID steamIDClan) override { return {}; }
    bool                GetClanActivityCounts(CSteamID steamIDClan, int* pnOnline, int* pnInGame, int* pnChatting) override { return {}; }
    SteamAPICall_t      DownloadClanActivityCounts(CSteamID* psteamIDClans, int cClansToRequest) override { return {}; }
    int                 GetFriendCountFromSource(CSteamID steamIDSource) override { return {}; }
    CSteamID            GetFriendFromSourceByIndex(CSteamID steamIDSource, int iFriend) override { return {}; }
    bool                IsUserInSource(CSteamID steamIDUser, CSteamID steamIDSource) override { return {}; }
    void                SetInGameVoiceSpeaking(CSteamID steamIDUser, bool bSpeaking) override {}
    void                ActivateGameOverlay(const char* pchDialog) override {}
    void                ActivateGameOverlayToUser(const char* pchDialog, CSteamID steamID) override {}
    void                ActivateGameOverlayToWebPage(const char* pchURL, EActivateGameOverlayToWebPageMode eMode = k_EActivateGameOverlayToWebPageMode_Default) override {}
    void                ActivateGameOverlayToStore(AppId_t nAppID, EOverlayToStoreFlag eFlag) override {}
    void                SetPlayedWith(CSteamID steamIDUserPlayedWith) override {}
    void                ActivateGameOverlayInviteDialog(CSteamID steamIDLobby) override {}
    int                 GetSmallFriendAvatar(CSteamID steamIDFriend) override { return {}; }
    int                 GetMediumFriendAvatar(CSteamID steamIDFriend) override { return {}; }
    int                 GetLargeFriendAvatar(CSteamID steamIDFriend) override { return {}; }
    bool                RequestUserInformation(CSteamID steamIDUser, bool bRequireNameOnly) override { return {}; }
    SteamAPICall_t      RequestClanOfficerList(CSteamID steamIDClan) override { return {}; }
    CSteamID            GetClanOwner(CSteamID steamIDClan) override { return {}; }
    int                 GetClanOfficerCount(CSteamID steamIDClan) override { return {}; }
    CSteamID            GetClanOfficerByIndex(CSteamID steamIDClan, int iOfficer) override { return {}; }
    bool                SetRichPresence(const char* pchKey, const char* pchValue) override { return {}; }
    void                ClearRichPresence() override {}
    const char*         GetFriendRichPresence(CSteamID steamIDFriend, const char* pchKey) override { return {}; }
    int                 GetFriendRichPresenceKeyCount(CSteamID steamIDFriend) override { return {}; }
    const char*         GetFriendRichPresenceKeyByIndex(CSteamID steamIDFriend, int iKey) override { return {}; }
    void                RequestFriendRichPresence(CSteamID steamIDFriend) override {}
    bool                InviteUserToGame(CSteamID steamIDFriend, const char* pchConnectString) override { return {}; }
    int                 GetCoplayFriendCount() override { return {}; }
    CSteamID            GetCoplayFriend(int iCoplayFriend) override { return {}; }
    int                 GetFriendCoplayTime(CSteamID steamIDFriend) override { return {}; }
    AppId_t             GetFriendCoplayGame(CSteamID steamIDFriend) override { return {}; }
    SteamAPICall_t      JoinClanChatRoom(CSteamID steamIDClan) override { return {}; }
    bool                LeaveClanChatRoom(CSteamID steamIDClan) override { return {}; }
    int                 GetClanChatMemberCount(CSteamID steamIDClan) override { return {}; }
    CSteamID            GetChatMemberByIndex(CSteamID steamIDClan, int iUser) override { return {}; }
    bool                SendClanChatMessage(CSteamID steamIDClanChat, const char* pchText) override { return {}; }
    int                 GetClanChatMessage(CSteamID steamIDClanChat, int iMessage, void* prgchText, int cchTextMax, EChatEntryType* peChatEntryType, CSteamID* psteamidChatter) override { return {}; }
    bool                IsClanChatAdmin(CSteamID steamIDClanChat, CSteamID steamIDUser) override { return {}; }
    bool                IsClanChatWindowOpenInSteam(CSteamID steamIDClanChat) override { return {}; }
    bool                OpenClanChatWindowInSteam(CSteamID steamIDClanChat) override { return {}; }
    bool                CloseClanChatWindowInSteam(CSteamID steamIDClanChat) override { return {}; }
    bool                SetListenForFriendsMessages(bool bInterceptEnabled) override { return {}; }
    bool                ReplyToFriendMessage(CSteamID steamIDFriend, const char* pchMsgToSend) override { return {}; }
    int                 GetFriendMessage(CSteamID steamIDFriend, int iMessageID, void* pvData, int cubData, EChatEntryType* peChatEntryType) override { return {}; }
    SteamAPICall_t      GetFollowerCount(CSteamID steamID) override { return {}; }
    SteamAPICall_t      IsFollowing(CSteamID steamID) override { return {}; }
    SteamAPICall_t      EnumerateFollowingList(uint32 unStartIndex) override { return {}; }
    bool                IsClanPublic(CSteamID steamIDClan) override { return {}; }
    bool                IsClanOfficialGameGroup(CSteamID steamIDClan) override { return {}; }
    int                 GetNumChatsWithUnreadPriorityMessages() override { return {}; }
    void                ActivateGameOverlayRemotePlayTogetherInviteDialog(CSteamID steamIDLobby) override {}
    bool                RegisterProtocolInOverlayBrowser(const char* pchProtocol) override { return {}; }
    void                ActivateGameOverlayInviteDialogConnectString(const char* pchConnectString) override {}
    SteamAPICall_t      RequestEquippedProfileItems(CSteamID steamID) override { return {}; }
    bool                BHasEquippedProfileItem(CSteamID steamID, ECommunityProfileItemType itemType) override { return {}; }
    const char*         GetProfileItemPropertyString(CSteamID steamID, ECommunityProfileItemType itemType, ECommunityProfileItemProperty prop) override { return {}; }
    uint32              GetProfileItemPropertyUint(CSteamID steamID, ECommunityProfileItemType itemType, ECommunityProfileItemProperty prop) override { return {}; }
};
