#include <Windows.h>
#include <SteamAPIBridge.h>
#include <cstdio>
#include <vector>
int wmain(int argc, wchar_t** argv)
{
    if (argc < 2) return 2;
    std::vector<HMODULE> modules;
    for (int i = 1; i < argc; ++i)
    {
        auto module = LoadLibraryExW(argv[i], nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!module)
        {
            std::fprintf(stderr, "LoadLibrary failed for argument %d: %lu\n", i, GetLastError());
            return 1;
        }
        modules.push_back(module);
    }
    auto context = SteamBridge_CreateContext();
    if (!context) return 1;
    std::printf("Loaded %d module(s); capabilities without host initialization: %u\n", argc - 1, SteamBridge_GetCapabilities(context));
    SteamBridge_DestroyContext(context);
    for (auto it = modules.rbegin(); it != modules.rend(); ++it) FreeLibrary(*it);
    return 0;
}
