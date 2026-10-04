#pragma once
#include <stdint.h>

#ifdef STEAMAPIBRIDGE_EXPORTS
#define STEAMBRIDGE_API __declspec(dllexport)
#else
#define STEAMBRIDGE_API __declspec(dllimport)
#endif
#define STEAMBRIDGE_CALL __cdecl
#ifdef __cplusplus
extern "C" {
#endif

typedef struct SteamBridgeContext* SteamBridgeContextHandle;
typedef int32_t SteamBridgeStatus;
enum { SB_OK = 0, SB_UNAVAILABLE = 1, SB_INVALID_ARGUMENT = 2, SB_FAILED = 3 };
enum { SB_CAP_USER = 1, SB_CAP_SCREENSHOTS = 2, SB_CAP_HTTP = 4 };
enum { SB_HTTP_HEADERS = 1, SB_HTTP_DATA = 2, SB_HTTP_COMPLETE = 3 };
enum { SB_HTTP_GET = 1, SB_HTTP_HEAD = 2, SB_HTTP_POST = 3, SB_HTTP_PUT = 4, SB_HTTP_DELETE = 5 };
enum { SB_STEAM_RESULT_OK = 1, SB_STEAM_RESULT_IO_FAILURE = 37 };

typedef struct SteamBridgeHTTPEvent {
    uint32_t kind;
    uint32_t request;
    uint32_t offset;
    uint32_t bytes;
    int32_t statusCode;
    uint32_t successful;
    uint32_t ioFailure;
} SteamBridgeHTTPEvent;
typedef void (STEAMBRIDGE_CALL *SteamBridgeHTTPCallback)(void*, const SteamBridgeHTTPEvent*);
typedef void (STEAMBRIDGE_CALL *SteamBridgeScreenshotCallback)(void*, uint32_t screenshot, int32_t result);

// Callbacks execute on the host Steam callback thread. Event pointers are borrowed
// for the duration of the call. Destroying a context from its callback is supported.
// The host owns Steam initialization and callback pumping. Destroy contexts before
// shutting Steam down; do not use a context concurrently with its destruction.
STEAMBRIDGE_API SteamBridgeContextHandle STEAMBRIDGE_CALL SteamBridge_CreateContext(void);
STEAMBRIDGE_API void STEAMBRIDGE_CALL SteamBridge_DestroyContext(SteamBridgeContextHandle context);
STEAMBRIDGE_API uint32_t STEAMBRIDGE_CALL SteamBridge_GetCapabilities(SteamBridgeContextHandle context);
STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_GetSteamID(SteamBridgeContextHandle context, uint64_t* id);
STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_SubscribeScreenshots(SteamBridgeContextHandle context, SteamBridgeScreenshotCallback callback, void* user);
STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_WriteScreenshot(SteamBridgeContextHandle context, const void* rgb, uint32_t size, int32_t width, int32_t height, uint32_t* screenshot);
STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_AddScreenshot(SteamBridgeContextHandle context, const char* file, const char* thumbnail, int32_t width, int32_t height, uint32_t* screenshot);
STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_SetScreenshotLocation(SteamBridgeContextHandle context, uint32_t screenshot, const char* location);
STEAMBRIDGE_API SteamBridgeStatus STEAMBRIDGE_CALL SteamBridge_TagScreenshotUser(SteamBridgeContextHandle context, uint32_t screenshot, uint64_t id);

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

#ifdef __cplusplus
}
#endif
