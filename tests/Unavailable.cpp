#include <SteamAPIBridge.h>
#include <cassert>
int main()
{
    auto context = SteamBridge_CreateContext();
    assert(context);
    assert(0 == SteamBridge_GetCapabilities(context));
    uint64_t id = 123;
    assert(SB_UNAVAILABLE == SteamBridge_GetSteamID(context, &id));
    assert(0 == id);
    uint32_t request = 123;
    assert(SB_UNAVAILABLE == SteamBridge_HTTPCreate(context, SB_HTTP_GET, "https://example.invalid", &request));
    assert(0 == request);
    SteamBridge_DestroyContext(context);
}
