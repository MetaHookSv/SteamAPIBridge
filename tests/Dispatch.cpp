#include <Windows.h>
#include <SteamAPIBridge.h>
#include <cassert>
#include <cstring>
struct Events { SteamBridgeContextHandle context; int headers{},data{},complete{},shots{}; bool destroy{}; };
void __cdecl HTTPEvent(void* opaque,const SteamBridgeHTTPEvent* event) {
    auto& e=*static_cast<Events*>(opaque);
    if(event->kind==SB_HTTP_HEADERS) ++e.headers;
    if(event->kind==SB_HTTP_DATA) { ++e.data; char data[2]; assert(SB_OK==SteamBridge_HTTPGetStreamData(e.context,event->request,event->offset,data,2)); assert(!memcmp(data,"ok",2)); }
    if(event->kind==SB_HTTP_COMPLETE) ++e.complete;
    if(e.destroy) { SteamBridge_DestroyContext(e.context); e.context=nullptr; }
}
void __cdecl Shot(void* opaque,uint32_t shot,int32_t result) {
    auto& e=*static_cast<Events*>(opaque); ++e.shots;
    assert(42==shot && 1==result);
    assert(SB_OK==SteamBridge_SetScreenshotLocation(e.context,shot,"test"));
    assert(SB_OK==SteamBridge_SubscribeScreenshots(e.context,nullptr,nullptr));
}
int main(int argc,char** argv) {
    assert(argc==3);
    auto module=LoadLibraryA(argv[1]); assert(module);
    auto mode=(void(__cdecl*)(int))GetProcAddress(module,"MockMode");
    auto emit=(void(__cdecl*)(int,uint32_t))GetProcAddress(module,"MockEmit");
    auto stat=(int(__cdecl*)(int))GetProcAddress(module,"MockStat");
    assert(mode && emit && stat);
    int base= !strcmp(argv[2],"legacy") ? 1 : 5;
    Events a{SteamBridge_CreateContext()},b{SteamBridge_CreateContext()};
    assert(0==SteamBridge_GetCapabilities(a.context));
    mode(base);
    assert((SB_CAP_USER|SB_CAP_SCREENSHOTS)==SteamBridge_GetCapabilities(a.context));
    uint64_t id{}; assert(SB_OK==SteamBridge_GetSteamID(a.context,&id)); assert(76561198000000001==id);
    uint32_t request{}; assert(SB_UNAVAILABLE==SteamBridge_HTTPCreate(a.context,SB_HTTP_GET,"https://example.invalid",&request));
    mode(base|2); // Availability is retried, not permanently cached.
    assert(7==SteamBridge_GetCapabilities(a.context));
    uint32_t shot{}; char rgb[3]{};
    assert(SB_OK==SteamBridge_WriteScreenshot(a.context,rgb,3,1,1,&shot)); assert(42==shot);
    assert(SB_OK==SteamBridge_TagScreenshotUser(a.context,shot,id));
    assert(SB_OK==SteamBridge_SubscribeScreenshots(a.context,Shot,&a)); emit(0,0); emit(0,0); assert(1==a.shots);
    assert(SB_OK==SteamBridge_HTTPCreate(a.context,SB_HTTP_GET,"https://example.invalid",&request));
    uint32_t cookie{}; assert(SB_OK==SteamBridge_HTTPCreateCookies(a.context,1,&cookie));
    assert(SB_OK==SteamBridge_HTTPSetCookies(a.context,request,cookie));
    assert(SB_OK==SteamBridge_HTTPSetCookie(a.context,cookie,"example.invalid","/","a=b"));
    assert(2==stat(1));
    assert(SB_OK==SteamBridge_HTTPSetCertificateVerification(a.context,request,1)); assert(1==stat(0));
    assert(SB_INVALID_ARGUMENT==SteamBridge_HTTPSetCookies(b.context,request,cookie));
    assert(SB_OK==SteamBridge_HTTPSend(a.context,request,1,HTTPEvent,&a));
    uint32_t other{}; assert(SB_OK==SteamBridge_HTTPCreate(b.context,SB_HTTP_GET,"https://example.invalid",&other));
    assert(SB_OK==SteamBridge_HTTPSend(b.context,other,1,HTTPEvent,&b));
    emit(1,request); emit(2,request); emit(2,request); emit(3,request); emit(3,request);
    assert(1==a.headers && 2==a.data && 1==a.complete); assert(0==b.headers && 0==b.complete);
    b.destroy=true; emit(2,other); assert(!b.context); emit(3,other); assert(0==b.complete);
    SteamBridge_DestroyContext(a.context); assert(0==stat(3)); assert(2==stat(2));
    Events failure{SteamBridge_CreateContext()};
    assert(SB_OK==SteamBridge_HTTPCreate(failure.context,SB_HTTP_GET,"https://example.invalid",&request));
    mode(base|2|8); assert(SB_FAILED==SteamBridge_HTTPSend(failure.context,request,0,HTTPEvent,&failure));
    mode(base|2); assert(SB_OK==SteamBridge_HTTPSend(failure.context,request,0,HTTPEvent,&failure));
    emit(4,request); assert(1==failure.complete);
    SteamBridge_DestroyContext(failure.context); assert(0==stat(3));
    // Destroyed contexts have no registered callbacks when the host unloads.
    FreeLibrary(module);
}
