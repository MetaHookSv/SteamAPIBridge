# SteamAPIBridge

Windows MSVC x86 compatibility DLL for MetaHookSv SteamAPI consumers. Plugins link
`SteamAPIBridge.lib`; the game supplies and initializes its own `steam_api.dll`.
The bridge neither imports SteamAPI functions nor replaces the game's runtime.

## Runtime contract

The C interface is `include/SteamAPIBridge.h`. It exposes SteamID, screenshot and
HTTP operations, status codes, opaque consumer contexts and function callbacks.
No Steam interface pointer, STL object or allocation ownership crosses the DLL ABI.
`SB_OK` is zero; failed create operations clear their output handles.

* Modern runtimes: dynamically request User023, Screenshots003 and HTTP003 using
  `SteamInternal_FindOrCreateUserInterface`.
* GoldSrc 8684 runtime: use its **SteamClient012 Windows ABI** with the existing
  user/pipe to request the same versions. User016 and Screenshots001 are supported
  fallbacks. Private declarations record their historical Valve SDK provenance.
* HTTP always requires HTTP003. Older HTTP002 revisions have incompatible method
  availability despite sharing a version string; the bridge does not silently
  omit cookies or certificate verification settings.

Create one context per consumer or HTTP client. Requests, cookies and subscriptions
are owned by their context. Destroy requests before their cookies, and destroy
contexts before the host shuts Steam down or unloads the consumer. A context is
bound to its first available module/user/pipe session; create a new context for a
new game session. Interface objects are resolved on each operation, not cached
across sessions. An unavailable initial runtime may be retried on the same context.

The game alone runs `SteamAPI_Init`, `SteamAPI_RunCallbacks` and `SteamAPI_Shutdown`.
Callbacks execute synchronously on the host callback thread, outside bridge locks.
Event pointers are borrowed during the call. Callbacks may unsubscribe, release a
request or destroy their context; native callback objects are retired until dispatch
has returned. Do not destroy a context concurrently with calls using that context.
Application callbacks must not throw across the C ABI.

Screenshot completion results retain Steam's numeric EResult values. HTTP events
distinguish headers, data and completion. Headers/data are ordinary Steam callbacks
filtered by request handle; completion is a call result dispatched at most once.
Request/response buffers belong to the caller. Read response data before releasing
the request. No successful interface lookup certifies game-engine compatibility.

## Build and install

```bat
scripts\build-SteamAPIBridge-x86-Debug.bat
scripts\build-SteamAPIBridge-x86-Release.bat
```

Requires CMake 3.21+, Visual Studio 2022 Win32 and C++20. Uses static CRT and VC-LTL
5.3.1. `STEAMSDK_SOURCE_PATH` and `VC_LTL_Root` accept read-only local dependency
trees; otherwise the pinned SDK and hash-verified VC-LTL package are fetched.
`STEAMAPIBRIDGE_BUILD_TESTS` defaults on for standalone builds, off as a subproject.

CMake target: `SteamAPIBridge` (alias `SteamAPIBridge::SteamAPIBridge`). Consumers
reuse this target in the aggregate; standalone consumers build a fixed bridge
commit through `STEAMAPIBRIDGE_SOURCE_PATH` or FetchContent. Installation stages
the DLL/PDB under `svencoop/metahook/dlls`; the installer maps the selected game mod.
No `steam_api.dll` is installed. The game must make `metahook/dlls` available before
loading consumers, as the MetaHook loader already does.

## Verification

CTest covers absent/uninitialized Steam, legacy and intermediate-modern exports,
exact-version availability and retry, SteamID/screenshots, cookie/certificate
forwarding, stream events, context isolation, cancellation and reentrant destruction.
Fixtures never initialize the real Steam client or make network requests.
`SteamBridgeLoadSmoke` accepts real Steam DLL and consumer DLL paths for loader-only
checks. These checks do not validate Steam authentication, screenshot submission,
real HTTP traffic, engine hooks or gameplay. Those require running each target game.

The SDK notice is installed with the runtime. Historical ABI declarations derive
from Valve Steamworks headers; see their provenance in `src/LegacyInterfaces.h`.

## C/C++ formatting

Formatting uses [MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation)
and clang-format **23.1.3**, with the DiligentCore style (4 spaces, preserved include
order). Install the formatter for the Python interpreter used by CMake:

```sh
python -m pip install clang-format==23.1.3
cmake -S . -B build/format "-DFORMAT_VALIDATION_ONLY=ON"
cmake --build build/format --target format-check
cmake --build build/format --target format
```

The format-only configuration needs CMake 3.21+, Git, Python 3.9+ (CI uses 3.12),
and a build generator; `-G Ninja` works without Visual Studio. It prepares no native
SDK or game dependencies. Formatting targets are explicit and are not part of a
normal DLL build. With a Visual Studio generator, add `--config Debug` or
`--config Release` when building a formatting target.

The aggregate provides `FORMAT_VALIDATION_SOURCE_PATH=thirdparty/FormatValidation`.
Standalone components accept that CMake variable or its environment counterpart;
if empty, FetchContent downloads the fixed tooling commit. Quote relative paths,
for example `"-DFORMAT_VALIDATION_SOURCE_PATH=../../thirdparty/FormatValidation"`.
Configuration generates the ignored root `.clang-format` for editors; change the
shared style rather than that generated copy. An optional
`FORMAT_VALIDATION_CLANG_FORMAT_EXECUTABLE` selects an explicit formatter, whose
version must still match the pin.

Checks cover owned C/C++ files in `src/`, `include/`, and `tests/`, including
non-ignored new files. Repository-relative exclusions live in `.clang-format-ignore`.
Third-party sources and build artifacts are excluded. The `clang-format` workflow
checks the full scope on pushes, pull requests, and manual runs.
