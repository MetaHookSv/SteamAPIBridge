# SteamAPIBridge

C ABI bridge to the Steamworks API for Windows (MSVC x86/x64) and Linux (x86/x64).
It has two modes:

* **Plugin mode** (MetaHookSv consumers): plugins link `SteamAPIBridge.lib`; the game
  supplies and initializes its own `steam_api.dll`. The bridge only uses the runtime
  that is already loaded and never initializes or shuts Steam down.
* **Host mode** (a game engine without its own Steam session): the host asks the bridge
  to load the runtime and own SteamAPI/SteamGameServer initialization, callback pumping
  and shutdown, through the `SteamBridge_Host*`, `SteamBridge_User*`,
  `SteamBridge_Voice*` and `SteamBridge_Server*` functions.

In both modes the bridge resolves the runtime dynamically and does not link
`steam_api.lib`.

## Runtime contract

The C interface is `include/SteamAPIBridge.h`. It exposes status codes, opaque
consumer contexts, function callbacks and plain event structures. No Steam interface
pointer, STL object or allocation ownership crosses the ABI. `SB_OK` is zero; failed
create or query operations clear their outputs.

### Plugin mode

The runtime is the loaded `steam_api.dll` (Windows x86), `steam_api64.dll` (Windows
x64) or `libsteam_api.so` (Linux). Contexts expose SteamID, screenshot and HTTP
operations.

* Modern runtimes: dynamically request User023, Screenshots003 and HTTP003 using
  `SteamInternal_FindOrCreateUserInterface`.
* GoldSrc 8684 runtime (Windows x86 only): use its **SteamClient012 Windows ABI** with
  the existing user/pipe to request the same versions. User016 and Screenshots001 are
  supported fallbacks. Private declarations record their historical Valve SDK provenance.
* HTTP always requires HTTP003. Older HTTP002 revisions have incompatible method
  availability despite sharing a version string; the bridge does not silently
  omit cookies or certificate verification settings.

Create one context per consumer or HTTP client. Requests, cookies and subscriptions
are owned by their context. Destroy requests before their cookies, and destroy
contexts before the host shuts Steam down or unloads the consumer. A context is
bound to its first available module/user/pipe session; create a new context for a
new game session. Interface objects are resolved on each operation, not cached
across sessions. An unavailable initial runtime may be retried on the same context.

In plugin mode the game alone runs `SteamAPI_Init`, `SteamAPI_RunCallbacks` and
`SteamAPI_Shutdown`. Callbacks execute synchronously on the host callback thread,
outside bridge locks. Event pointers are borrowed during the call. Callbacks may
unsubscribe, release a request or destroy their context; native callback objects are
retired until dispatch has returned. Do not destroy a context concurrently with calls
using that context. Application callbacks must not throw across the C ABI.

Screenshot completion results retain Steam's numeric EResult values. HTTP events
distinguish headers, data and completion. Headers/data are ordinary Steam callbacks
filtered by request handle; completion is a call result dispatched at most once.
Request/response buffers belong to the caller. Read response data before releasing
the request. No successful interface lookup certifies game-engine compatibility.

### Host mode

`SteamBridge_HostInit(runtimePath, initializeClient)` loads `runtimePath`, or the
platform runtime from the bridge's own directory when it is `NULL`. Host mode requires
a modern runtime exporting `SteamAPI_InitFlat`, `SteamInternal_GameServer_Init_V2` and
the `SteamInternal_FindOrCreate*Interface` accessors; Steamworks SDK v1.65 is pinned.
A missing runtime or export returns `SB_UNAVAILABLE`, and a failed client initialization
(`SteamAPI_InitFlat`: Steam not running, application not owned) returns `SB_FAILED`;
both leave the runtime unloaded. A dedicated server passes `initializeClient = 0` and
only uses the game server functions.

* Client: game connection tickets (`InitiateGameConnection_DEPRECATED`), SteamID,
  persona name, app subscription, current game language, low violence and voice
  (User023, Friends018, Apps009). Missing interfaces make the corresponding calls return
  `SB_UNAVAILABLE`.
* Game server (`SteamBridge_ServerInit`): GameServer015 logon, server rules and
  heartbeats, ticket authentication (`SendUserConnectAndAuthenticate_DEPRECATED`),
  bots and shared-socket master server packets. The interface version list matches
  `SteamGameServer_InitEx`.
* FakeIP: fake UDP ports (`CreateFakeUDPPort`, SteamNetworkingSockets013) on the client or
  the game server session, the game server's FakeIP request (`BeginAsyncRequestFakeIP`,
  `GetFakeIP`), FakeIP address checks and SteamNetworkingSockets diagnostics
  (SteamNetworkingUtils004: the client's, or the game server's without a client). Ports
  are bridge handles; a session's shutdown destroys the ports it still holds. Received
  datagrams are copied out and released inside the bridge; one that does not fit, or does
  not come from an IP address, is dropped and described.
* Events: ClientGameServerDeny, GameServerChangeRequested and GameOverlayActivated
  reach the client callback during `SteamBridge_HostRunCallbacks`; SteamServersConnected,
  SteamServerConnectFailure, GSPolicyResponse, GSClientApprove/Deny/Kick and
  SteamNetworkingFakeIPResult reach the server callback during
  `SteamBridge_ServerRunCallbacks`. Fields are copied out of
  Steam's payloads; IP addresses and ports pass through unchanged.

Host functions are not thread-safe; call them from the host's main thread. FakeIP port
send, receive and cleanup may run on any thread while the port exists, and the networking
diagnostics callback may run on any thread. A callback
may call `SteamBridge_HostShutdown` or `SteamBridge_ServerShutdown`; the shutdown runs
after the native dispatcher returns. `SteamBridge_HostShutdown` shuts down an active
game server first. Plugin contexts may use the runtime that host mode loaded; destroy
them before `SteamBridge_HostShutdown`.

## Build and install

```bat
scripts\build-SteamAPIBridge-x86-Debug.bat
scripts\build-SteamAPIBridge-x86-Release.bat
scripts\build-SteamAPIBridge-x64-Debug.bat
scripts\build-SteamAPIBridge-x64-Release.bat
```

```sh
scripts/build-SteamAPIBridge-linux.sh x64 Release
scripts/build-SteamAPIBridge-linux.sh x86 Release   # needs g++-multilib
```

Requires CMake 3.21+ and C++20: Visual Studio 2022 (Win32 or x64, static CRT) on
Windows, GCC or Clang on Linux. Linux builds export only the `SteamBridge_` symbols.
`STEAMSDK_SOURCE_PATH` accepts a read-only local dependency tree; otherwise the
pinned SDK is fetched.
`STEAMAPIBRIDGE_BUILD_TESTS` defaults on for standalone builds, off as a subproject.
As a subproject the bridge keeps the parent's build configurations.

CMake target: `SteamAPIBridge` (alias `SteamAPIBridge::SteamAPIBridge`). Consumers
reuse this target in the aggregate; standalone consumers build a fixed bridge
commit through `STEAMAPIBRIDGE_SOURCE_PATH` or FetchContent. Installation stages
the bridge (and PDB on Windows) under `STEAMAPIBRIDGE_INSTALL_DIR`, by default
`svencoop/metahook/dlls`; the installer maps the selected game mod. The game must make
that directory available before loading consumers, as the MetaHook loader already does.
`STEAMAPIBRIDGE_INSTALL_RUNTIME` (default `OFF`) also installs the SDK's `steam_api`
runtime for the target architecture beside the bridge, as host mode expects;
`STEAMAPIBRIDGE_RUNTIME` names that file for consumers that stage it themselves.

## Verification

CTest covers, on every supported platform, absent runtimes and host mode against a
fixture runtime: client initialization failure and retry, connection tickets, app
subscription, game language (including truncation and a missing language), low violence, voice
capture/decoding and non-OK voice results, client and game server event translation
and dispatcher separation, deferred shutdown from callbacks, game server setters and
authentication, packets, dedicated (server-only) sessions, and FakeIP ports: datagram
send results, delivery and dropping, cleanup, session-owned port destruction, the game
server's FakeIP request and result event, and diagnostics forwarding. On Windows it also
covers plugin mode: absent/uninitialized Steam, legacy (x86) and intermediate-modern
exports, exact-version availability and retry, SteamID/screenshots, cookie/certificate
forwarding, stream events, context isolation, cancellation and reentrant destruction.
Fixtures never initialize the real Steam client or make network requests.

Two manual tools are built but not run by CTest. `SteamBridgeLoadSmoke` (Windows)
accepts real Steam DLL and consumer DLL paths for loader-only checks.
`SteamBridgeHostSmoke <runtime> [seconds]` needs a running, logged-in Steam client and
`steam_appid.txt` in the working directory: it initializes the client, logs a game
server on anonymously, authenticates the client's ticket and exits zero on
GSClientApprove. Neither validates screenshot submission, real HTTP traffic, FakeIP
traffic over the Steam relay, engine hooks or gameplay; those require running each target
game (FakeIP traffic also needs a second Steam account).

The SDK notice is installed with the runtime. Historical ABI declarations derive
from Valve Steamworks headers; see their provenance in `src/LegacyInterfaces.h`.
