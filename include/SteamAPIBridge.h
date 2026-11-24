#pragma once
#include <stdint.h>

#if defined(_WIN32)
#    ifdef STEAMAPIBRIDGE_EXPORTS
#        define STEAMBRIDGE_API __declspec(dllexport)
#    else
#        define STEAMBRIDGE_API __declspec(dllimport)
#    endif
#    define STEAMBRIDGE_CALL __cdecl
#else
#    define STEAMBRIDGE_API __attribute__((visibility("default")))
#    define STEAMBRIDGE_CALL
#endif
#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct SteamBridgeContext* SteamBridgeContextHandle;
    typedef int32_t                    SteamBridgeStatus;
    enum
    {
        SB_OK               = 0,
        SB_UNAVAILABLE      = 1,
        SB_INVALID_ARGUMENT = 2,
        SB_FAILED           = 3
    };
    enum
    {
        SB_CAP_USER        = 1,
        SB_CAP_SCREENSHOTS = 2,
        SB_CAP_HTTP        = 4
    };
    enum
    {
        SB_HTTP_HEADERS  = 1,
        SB_HTTP_DATA     = 2,
        SB_HTTP_COMPLETE = 3
    };
    enum
    {
        SB_HTTP_GET    = 1,
        SB_HTTP_HEAD   = 2,
        SB_HTTP_POST   = 3,
        SB_HTTP_PUT    = 4,
        SB_HTTP_DELETE = 5
    };
    enum
    {
        SB_STEAM_RESULT_OK         = 1,
        SB_STEAM_RESULT_IO_FAILURE = 37
    };

    typedef struct SteamBridgeHTTPEvent
    {
        uint32_t kind;
        uint32_t request;
        uint32_t offset;
        uint32_t bytes;
        int32_t  statusCode;
        uint32_t successful;
        uint32_t ioFailure;
    } SteamBridgeHTTPEvent;
    typedef void(STEAMBRIDGE_CALL* SteamBridgeHTTPCallback)(void*, const SteamBridgeHTTPEvent*);
    typedef void(STEAMBRIDGE_CALL* SteamBridgeScreenshotCallback)(void*, uint32_t screenshot, int32_t result);

    // Callbacks execute on the host Steam callback thread. Event pointers are borrowed
    // for the duration of the call. Destroying a context from its callback is supported.
    // The host owns Steam initialization and callback pumping. Destroy contexts before
    // shutting Steam down; do not use a context concurrently with its destruction.
    STEAMBRIDGE_API SteamBridgeContextHandle STEAMBRIDGE_CALL SteamBridge_CreateContext(void);
    STEAMBRIDGE_API void STEAMBRIDGE_CALL                     SteamBridge_DestroyContext(SteamBridgeContextHandle context);
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL                 SteamBridge_GetCapabilities(SteamBridgeContextHandle context);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL        SteamBridge_GetSteamID(SteamBridgeContextHandle context, uint64_t* id);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL        SteamBridge_SubscribeScreenshots(SteamBridgeContextHandle context, SteamBridgeScreenshotCallback callback, void* user);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL        SteamBridge_WriteScreenshot(SteamBridgeContextHandle context, const void* rgb, uint32_t size, int32_t width, int32_t height, uint32_t* screenshot);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL        SteamBridge_AddScreenshot(SteamBridgeContextHandle context, const char* file, const char* thumbnail, int32_t width, int32_t height, uint32_t* screenshot);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL        SteamBridge_SetScreenshotLocation(SteamBridgeContextHandle context, uint32_t screenshot, const char* location);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL        SteamBridge_TagScreenshotUser(SteamBridgeContextHandle context, uint32_t screenshot, uint64_t id);

    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPCreate(SteamBridgeContextHandle context, int32_t method, const char* url, uint32_t* request);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPRelease(SteamBridgeContextHandle context, uint32_t request);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPSend(SteamBridgeContextHandle context, uint32_t request, uint32_t stream, SteamBridgeHTTPCallback callback, void* user);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPSetTimeout(SteamBridgeContextHandle context, uint32_t request, uint32_t seconds);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPSetHeader(SteamBridgeContextHandle context, uint32_t request, const char* name, const char* value);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPSetBody(SteamBridgeContextHandle context, uint32_t request, const char* contentType, const void* data, uint32_t size);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPSetCertificateVerification(SteamBridgeContextHandle context, uint32_t request, uint32_t required);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPGetHeaderSize(SteamBridgeContextHandle context, uint32_t request, const char* name, uint32_t* size);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPGetHeader(SteamBridgeContextHandle context, uint32_t request, const char* name, void* data, uint32_t size);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPGetBodySize(SteamBridgeContextHandle context, uint32_t request, uint32_t* size);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPGetBody(SteamBridgeContextHandle context, uint32_t request, void* data, uint32_t size);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPGetStreamData(SteamBridgeContextHandle context, uint32_t request, uint32_t offset, void* data, uint32_t size);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPCreateCookies(SteamBridgeContextHandle context, uint32_t allowModification, uint32_t* cookies);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPReleaseCookies(SteamBridgeContextHandle context, uint32_t cookies);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPSetCookies(SteamBridgeContextHandle context, uint32_t request, uint32_t cookies);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HTTPSetCookie(SteamBridgeContextHandle context, uint32_t cookies, const char* host, const char* url, const char* cookie);

    // Host session. A host that has no Steam session of its own (a game engine) asks the
    // bridge to load the runtime and own SteamAPI/SteamGameServer initialization. Consumers
    // of a game that initializes Steam itself must not call these functions.
    // runtimePath names the steam_api runtime file; NULL loads the platform runtime
    // (steam_api.dll, steam_api64.dll or libsteam_api.so) from the bridge's directory.
    // IP addresses and ports are passed to Steam unchanged. Host functions are not
    // thread-safe; call them from the host's main thread.
    enum
    {
        SB_CLIENT_GAMESERVER_DENY   = 1,
        SB_CLIENT_GAMESERVER_CHANGE = 2,
        SB_CLIENT_OVERLAY           = 3
    };
    enum
    {
        SB_SERVER_LOGON_SUCCESS  = 1,
        SB_SERVER_LOGON_FAILURE  = 2,
        SB_SERVER_POLICY         = 3,
        SB_SERVER_CLIENT_APPROVE = 4,
        SB_SERVER_CLIENT_DENY    = 5,
        SB_SERVER_CLIENT_KICK    = 6,
        SB_SERVER_FAKE_IP_RESULT = 7
    };
    // Values of Steamworks EServerMode.
    enum
    {
        SB_SERVER_MODE_NO_AUTHENTICATION = 1,
        SB_SERVER_MODE_AUTHENTICATION    = 2,
        SB_SERVER_MODE_SECURE            = 3
    };
    enum
    {
        SB_SERVER_TEXT_BYTES   = 128,
        SB_CLIENT_SERVER_BYTES = 64
    };

    // serverIP/serverPort are copied from Steam's ClientGameServerDeny_t unchanged.
    typedef struct SteamBridgeClientEvent
    {
        uint32_t kind;
        uint32_t serverIP;
        uint32_t serverPort;
        uint32_t secure;
        uint32_t reason;
        uint32_t active;
        char     server[SB_CLIENT_SERVER_BYTES];
        char     password[SB_CLIENT_SERVER_BYTES];
    } SteamBridgeClientEvent;
    // result is the Steam EResult of a logon failure or a FakeIP request; reason is the EDenyReason of a deny or kick.
    typedef struct SteamBridgeServerEvent
    {
        uint32_t kind;
        int32_t  result;
        uint32_t secure;
        uint32_t reason;
        uint64_t steamID;
        uint64_t ownerSteamID;
        char     text[SB_SERVER_TEXT_BYTES];
    } SteamBridgeServerEvent;
    typedef void(STEAMBRIDGE_CALL* SteamBridgeClientCallback)(void*, const SteamBridgeClientEvent*);
    typedef void(STEAMBRIDGE_CALL* SteamBridgeServerCallback)(void*, const SteamBridgeServerEvent*);

    // Loads the runtime and, when initializeClient is nonzero, initializes the Steam client.
    // A dedicated server passes zero and only uses the game server functions.
    // SB_UNAVAILABLE: the runtime or a required export is missing; SB_FAILED: SteamAPI
    // initialization failed (Steam is not running or the user does not own the application);
    // SB_INVALID_ARGUMENT: the host session is already active.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_HostInit(const char* runtimePath, uint32_t initializeClient);
    // Shuts down the game server (if active) and the client, then unloads the runtime.
    // Called from a host callback, the shutdown runs after the dispatcher returns.
    STEAMBRIDGE_API void STEAMBRIDGE_CALL     SteamBridge_HostShutdown(void);
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL SteamBridge_HostIsActive(void);
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL SteamBridge_HostIsClientActive(void);
    // Dispatches client callbacks; events reach the client callback synchronously.
    STEAMBRIDGE_API void STEAMBRIDGE_CALL SteamBridge_HostRunCallbacks(void);
    STEAMBRIDGE_API void STEAMBRIDGE_CALL SteamBridge_HostSetClientCallback(SteamBridgeClientCallback callback, void* user);

    // Writes the client's connection ticket for a game server; *size receives its length.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UserInitiateGameConnection(void* ticket, uint32_t capacity, uint64_t serverSteamID, uint32_t serverIP, uint16_t serverPort, uint32_t secure, uint32_t* size);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UserTerminateGameConnection(uint32_t serverIP, uint16_t serverPort);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UserAdvertiseGame(uint64_t serverSteamID, uint32_t serverIP, uint16_t serverPort);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UserGetSteamID(uint64_t* id);
    // Copies the persona name, truncated and NUL-terminated.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UserGetPersonaName(char* name, uint32_t capacity);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UserIsSubscribedApp(uint32_t appID, uint32_t* subscribed);
    // Copies the running application's language ("english", "schinese", ...), truncated and
    // NUL-terminated.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UserGetCurrentGameLanguage(char* language, uint32_t capacity);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UserIsLowViolence(uint32_t* lowViolence);

    // Voice. Get and Decompress succeed only for Steam's k_EVoiceResultOK; any other
    // voice result returns SB_FAILED with zero lengths.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_VoiceStartRecording(void);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_VoiceStopRecording(void);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_VoiceGetAvailable(uint32_t* compressedBytes);
    // pcm may be NULL. Otherwise Steam also writes uncompressed PCM at pcmSampleRate.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_VoiceGet(void* compressed, uint32_t capacity, uint32_t* compressedBytes, void* pcm, uint32_t pcmCapacity, uint32_t* pcmBytes, uint32_t pcmSampleRate);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_VoiceDecompress(const void* compressed, uint32_t bytes, void* pcm, uint32_t pcmCapacity, uint32_t* pcmBytes, uint32_t sampleRate);
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL          SteamBridge_VoiceGetOptimalSampleRate(void);

    // Game server. mode is an SB_SERVER_MODE_* value.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerInit(uint32_t serverIP, uint16_t gamePort, uint16_t queryPort, int32_t mode, const char* version);
    // Called from a host callback, the shutdown runs after the dispatcher returns.
    STEAMBRIDGE_API void STEAMBRIDGE_CALL     SteamBridge_ServerShutdown(void);
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL SteamBridge_ServerIsActive(void);
    // Dispatches game server callbacks; events reach the server callback synchronously.
    STEAMBRIDGE_API void STEAMBRIDGE_CALL              SteamBridge_ServerRunCallbacks(void);
    STEAMBRIDGE_API void STEAMBRIDGE_CALL              SteamBridge_ServerSetCallback(SteamBridgeServerCallback callback, void* user);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetProduct(const char* product);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetModDir(const char* modDir);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetDedicatedServer(uint32_t dedicated);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetGameDescription(const char* description);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerLogOnAnonymous(void);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerLogOff(void);
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL          SteamBridge_ServerIsLoggedOn(void);
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL          SteamBridge_ServerIsSecure(void);
    STEAMBRIDGE_API uint64_t STEAMBRIDGE_CALL          SteamBridge_ServerGetSteamID(void);
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL          SteamBridge_ServerWasRestartRequested(void);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetAdvertiseServerActive(uint32_t active);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetMaxPlayerCount(int32_t players);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetBotPlayerCount(int32_t bots);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetServerName(const char* name);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetMapName(const char* map);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetPasswordProtected(uint32_t passwordProtected);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerClearAllKeyValues(void);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSetKeyValue(const char* key, const char* value);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSendUserConnectAndAuthenticate(uint32_t clientIP, const void* ticket, uint32_t bytes, uint64_t* steamID);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerCreateUnauthenticatedUserConnection(uint64_t* steamID);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerSendUserDisconnect(uint64_t steamID);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerUpdateUserData(uint64_t steamID, const char* name, uint32_t score);
    // Shared-socket master server traffic.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerHandleIncomingPacket(const void* data, uint32_t bytes, uint32_t ip, uint16_t port);
    // Returns the packet length, or zero when no packet is pending.
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL SteamBridge_ServerGetNextOutgoingPacket(void* data, uint32_t capacity, uint32_t* ip, uint16_t* port);
    // Asks Steam for a FakeIP with ports fake ports; SB_SERVER_FAKE_IP_RESULT reports the outcome.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerBeginAsyncRequestFakeIP(int32_t ports);
    // *result receives the Steam EResult of the request (k_EResultBusy while it is pending). On
    // k_EResultOK, *ip and up to count ports from firstPort are written; otherwise they are cleared.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServerGetFakeIP(int32_t firstPort, int32_t* result, uint32_t* ip, uint16_t* ports, uint32_t count);

    // FakeIP datagrams (ISteamNetworkingFakeUDPPort), host mode. IPv4 addresses and ports are in
    // host byte order, as in Steam's SteamNetworkingIPAddr. A client port belongs to the client
    // session and a server port to the game server session; shutting a session down destroys its
    // remaining ports and invalidates their handles. Create and destroy ports on the host's main
    // thread; Send, Receive and ScheduleCleanup may run on any thread while the port exists.
    typedef struct SteamBridgeFakeUDPPort* SteamBridgeFakeUDPPortHandle;
    enum
    {
        SB_FAKE_UDP_NONE      = 0,
        SB_FAKE_UDP_MESSAGE   = 1,
        SB_FAKE_UDP_TOO_LARGE = 2,
        SB_FAKE_UDP_NOT_IP    = 3
    };
    enum
    {
        SB_FAKE_UDP_PEER_BYTES = 128
    };
    // kind is an SB_FAKE_UDP_* value. bytes is the message size, also when it did not fit. ip
    // and port are the sender of a received message. peer is the sender as Steam prints it, for
    // a message that was dropped (SB_FAKE_UDP_TOO_LARGE or SB_FAKE_UDP_NOT_IP).
    typedef struct SteamBridgeFakeUDPMessage
    {
        uint32_t kind;
        uint32_t bytes;
        uint32_t ip;
        uint16_t port;
        char     peer[SB_FAKE_UDP_PEER_BYTES];
    } SteamBridgeFakeUDPMessage;
    typedef void(STEAMBRIDGE_CALL* SteamBridgeNetworkingDebugCallback)(int32_t level, const char* message);

    // gameServer selects the game server session. fakeServerPort is the index of a fake port the
    // game server requested, or -1 for a port that sends to FakeIPs from a local FakeIP.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_FakeUDPPortCreate(uint32_t gameServer, int32_t fakeServerPort, SteamBridgeFakeUDPPortHandle* port);
    STEAMBRIDGE_API void STEAMBRIDGE_CALL              SteamBridge_FakeUDPPortDestroy(SteamBridgeFakeUDPPortHandle port);
    // flags are Steam k_nSteamNetworkingSend_* flags; *result receives the Steam EResult, and
    // anything but k_EResultOK returns SB_FAILED.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_FakeUDPPortSend(SteamBridgeFakeUDPPortHandle port, uint32_t ip, uint16_t remotePort, const void* data, uint32_t bytes, int32_t flags, int32_t* result);
    // Takes at most one pending message. A message from an IP address that fits capacity is
    // copied into data; any other message is dropped and only described.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_FakeUDPPortReceive(SteamBridgeFakeUDPPortHandle port, void* data, uint32_t capacity, SteamBridgeFakeUDPMessage* message);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_FakeUDPPortScheduleCleanup(SteamBridgeFakeUDPPortHandle port, uint32_t ip, uint16_t remotePort);
    // Nonzero when ip is a global or local FakeIP.
    STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL SteamBridge_IsFakeIPv4(uint32_t ip);
    // Forwards SteamNetworkingSockets diagnostics up to level (ESteamNetworkingSocketsDebugOutputType
    // values). The callback may run on any thread; NULL stops forwarding.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_NetworkingSetDebugOutput(int32_t level, SteamBridgeNetworkingDebugCallback callback);

    // Server browsing (ISteamMatchmakingServers), host mode with an initialized client. IPv4
    // addresses and ports are in host byte order, as Steam reports them. A list request or a
    // server query answers through its callback, synchronously during SteamBridge_HostRunCallbacks;
    // Steam may also answer before the call that started it returns. Shutting the client down
    // releases the remaining requests and queries and invalidates their handles.
    typedef struct SteamBridgeServerList* SteamBridgeServerListHandle;
    enum
    {
        SB_SERVER_LIST_INTERNET  = 0,
        SB_SERVER_LIST_LAN       = 1,
        SB_SERVER_LIST_FRIENDS   = 2,
        SB_SERVER_LIST_FAVORITES = 3,
        SB_SERVER_LIST_HISTORY   = 4,
        SB_SERVER_LIST_SPECTATOR = 5
    };
    enum
    {
        SB_SERVERS_LIST_RESPONDED   = 1,
        SB_SERVERS_LIST_FAILED      = 2,
        SB_SERVERS_LIST_COMPLETE    = 3,
        SB_SERVERS_PING_RESPONDED   = 4,
        SB_SERVERS_PING_FAILED      = 5,
        SB_SERVERS_PLAYER           = 6,
        SB_SERVERS_PLAYERS_FAILED   = 7,
        SB_SERVERS_PLAYERS_COMPLETE = 8
    };
    // The string sizes of Steam's gameserveritem_t, and the player name capacity.
    enum
    {
        SB_SERVER_NAME_BYTES        = 64,
        SB_SERVER_GAME_DIR_BYTES    = 32,
        SB_SERVER_MAP_BYTES         = 32,
        SB_SERVER_DESCRIPTION_BYTES = 64,
        SB_SERVER_TAGS_BYTES        = 128,
        SB_SERVER_PLAYER_BYTES      = 128
    };
    // A copy of Steam's gameserveritem_t. players includes bots; timeLastPlayed is the Unix time
    // of the last visit for favorite and history servers.
    typedef struct SteamBridgeServerItem
    {
        uint32_t ip;
        uint16_t queryPort;
        uint16_t connectionPort;
        int32_t  ping;
        uint32_t hadSuccessfulResponse;
        uint32_t doNotRefresh;
        uint32_t appID;
        int32_t  players;
        int32_t  maxPlayers;
        int32_t  botPlayers;
        uint32_t password;
        uint32_t secure;
        uint32_t timeLastPlayed;
        int32_t  serverVersion;
        uint64_t steamID;
        char     name[SB_SERVER_NAME_BYTES];
        char     gameDir[SB_SERVER_GAME_DIR_BYTES];
        char     map[SB_SERVER_MAP_BYTES];
        char     description[SB_SERVER_DESCRIPTION_BYTES];
        char     tags[SB_SERVER_TAGS_BYTES];
    } SteamBridgeServerItem;
    // kind is an SB_SERVERS_* value. List events set request and index, and SB_SERVERS_LIST_COMPLETE
    // sets response (EMatchMakingServerResponse). Server query events set query; SB_SERVERS_PING_RESPONDED
    // sets server, and SB_SERVERS_PLAYER sets player, score and seconds (time on the server).
    typedef struct SteamBridgeServersEvent
    {
        uint32_t                    kind;
        SteamBridgeServerListHandle request;
        uint32_t                    query;
        int32_t                     index;
        int32_t                     response;
        int32_t                     score;
        float                       seconds;
        char                        player[SB_SERVER_PLAYER_BYTES];
        SteamBridgeServerItem       server;
    } SteamBridgeServersEvent;
    typedef void(STEAMBRIDGE_CALL* SteamBridgeServersCallback)(void*, const SteamBridgeServersEvent*);
    // A MatchMakingKeyValuePair_t filter; the strings are copied, truncated to 255 bytes.
    typedef struct SteamBridgeServerFilter
    {
        const char* key;
        const char* value;
    } SteamBridgeServerFilter;

    // kind is an SB_SERVER_LIST_* value; LAN lists ignore filters. The request answers until it is released.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServersRequestList(int32_t kind, uint32_t appID, const SteamBridgeServerFilter* filters, uint32_t filterCount, SteamBridgeServersCallback callback, void* user, SteamBridgeServerListHandle* request);
    // Stops the request's events at once. Called from a callback, Steam releases it after the dispatcher returns.
    STEAMBRIDGE_API void STEAMBRIDGE_CALL SteamBridge_ServersReleaseRequest(SteamBridgeServerListHandle request);
    // SB_FAILED: Steam has no server at index.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServersGetDetails(SteamBridgeServerListHandle request, int32_t index, SteamBridgeServerItem* server);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServersCancelQuery(SteamBridgeServerListHandle request);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServersRefreshQuery(SteamBridgeServerListHandle request);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServersRefreshServer(SteamBridgeServerListHandle request, int32_t index);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServersIsRefreshing(SteamBridgeServerListHandle request, uint32_t* refreshing);
    // Server queries. *query identifies the query in its events and is never zero. A ping ends with
    // SB_SERVERS_PING_RESPONDED or SB_SERVERS_PING_FAILED, a player query with
    // SB_SERVERS_PLAYERS_COMPLETE or SB_SERVERS_PLAYERS_FAILED after its SB_SERVERS_PLAYER events.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServersPing(uint32_t ip, uint16_t port, SteamBridgeServersCallback callback, void* user, uint32_t* query);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_ServersPlayerDetails(uint32_t ip, uint16_t port, SteamBridgeServersCallback callback, void* user, uint32_t* query);
    // Stops the query's events at once; ignores a query that has ended. Called from a callback,
    // Steam cancels it after the dispatcher returns.
    STEAMBRIDGE_API void STEAMBRIDGE_CALL SteamBridge_ServersCancelServerQuery(uint32_t query);

    // Favorite and history servers (ISteamMatchmaking). flags are Steam k_unFavoriteFlag* values;
    // lastPlayed is a Unix time.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_MatchmakingGetFavoriteCount(uint32_t* count);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_MatchmakingAddFavorite(uint32_t appID, uint32_t ip, uint16_t connectionPort, uint16_t queryPort, uint32_t flags, uint32_t lastPlayed);
    // SB_FAILED: no such entry.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_MatchmakingRemoveFavorite(uint32_t appID, uint32_t ip, uint16_t connectionPort, uint16_t queryPort, uint32_t flags);
    enum
    {
        SB_MATCHMAKING_FAVORITES_CHANGED = 1,
        SB_MATCHMAKING_PERSONA_CHANGE    = 2
    };
    // The favorites fields copy FavoritesListChanged_t (an ip of zero means the whole list changed);
    // steamID and changeFlags (EPersonaChange) copy PersonaStateChange_t.
    typedef struct SteamBridgeMatchmakingEvent
    {
        uint32_t kind;
        uint32_t ip;
        uint32_t queryPort;
        uint32_t connectionPort;
        uint32_t appID;
        uint32_t flags;
        uint32_t add;
        uint32_t accountID;
        uint64_t steamID;
        int32_t  changeFlags;
    } SteamBridgeMatchmakingEvent;
    typedef void(STEAMBRIDGE_CALL* SteamBridgeMatchmakingCallback)(void*, const SteamBridgeMatchmakingEvent*);
    // A separate slot from the client callback, so another module of the host can own it; events
    // arrive synchronously during SteamBridge_HostRunCallbacks.
    STEAMBRIDGE_API void STEAMBRIDGE_CALL SteamBridge_HostSetMatchmakingCallback(SteamBridgeMatchmakingCallback callback, void* user);

    // Friends (ISteamFriends) and VAC status. gameID is Steam's CGameID.
    typedef struct SteamBridgeFriendGame
    {
        uint64_t gameID;
        uint32_t ip;
        uint16_t gamePort;
        uint16_t queryPort;
        uint64_t lobbySteamID;
    } SteamBridgeFriendGame;
    // Copies the friend's persona name, truncated and NUL-terminated.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_FriendsGetPersonaName(uint64_t steamID, char* name, uint32_t capacity);
    // SB_FAILED: the friend is not in a game.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_FriendsGetGamePlayed(uint64_t steamID, SteamBridgeFriendGame* game);
    // Relationship values are Steam's EFriendRelationship.
    enum
    {
        SB_FRIEND_RELATIONSHIP_NONE              = 0,
        SB_FRIEND_RELATIONSHIP_BLOCKED           = 1,
        SB_FRIEND_RELATIONSHIP_REQUEST_RECIPIENT = 2,
        SB_FRIEND_RELATIONSHIP_FRIEND            = 3,
        SB_FRIEND_RELATIONSHIP_REQUEST_INITIATOR = 4,
        SB_FRIEND_RELATIONSHIP_IGNORED           = 5,
        SB_FRIEND_RELATIONSHIP_IGNORED_FRIEND    = 6
    };
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_FriendsGetRelationship(uint64_t steamID, int32_t* relationship);
    // Avatar sizes: 32x32, 64x64 and 184x184 pixels.
    enum
    {
        SB_AVATAR_SMALL  = 1,
        SB_AVATAR_MEDIUM = 2,
        SB_AVATAR_LARGE  = 3
    };
    // The image handle of the user's avatar, as Steam returns it: 0 when the user has none, -1 while Steam
    // is still loading it (ask again later). Read the image with SteamBridge_UtilsGetImageSize and
    // SteamBridge_UtilsGetImageRGBA.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_FriendsGetAvatar(uint64_t steamID, uint32_t size, int32_t* image);

    // Images (ISteamUtils). SB_FAILED: no such image, or rgba holds less than 4 * width * height bytes.
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UtilsGetImageSize(int32_t image, uint32_t* width, uint32_t* height);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_UtilsGetImageRGBA(int32_t image, uint8_t* rgba, uint32_t capacity);
    STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_AppsIsVACBanned(uint32_t* banned);

#ifdef __cplusplus
}
#endif
