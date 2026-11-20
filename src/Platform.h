#pragma once
#if defined(_WIN32)
#    include <Windows.h>
#else
#    include <dlfcn.h>
#endif
#include <string>

namespace platform
{
#if defined(_WIN32)
using Module = HMODULE;
#    if defined(_WIN64)
inline constexpr char RuntimeName[] = "steam_api64.dll";
#    else
inline constexpr char RuntimeName[] = "steam_api.dll";
#    endif
#else
using Module                        = void*;
inline constexpr char RuntimeName[] = "libsteam_api.so";
#endif

// Returns the already loaded runtime without taking a reference.
inline Module FindRuntime()
{
#if defined(_WIN32)
    return GetModuleHandleA(RuntimeName);
#else
    Module module = dlopen(RuntimeName, RTLD_NOW | RTLD_NOLOAD);
    if (module) dlclose(module);
    return module;
#endif
}
inline std::string BridgeDirectory()
{
    static const int anchor = 0;
#if defined(_WIN32)
    HMODULE self{};
    char    path[MAX_PATH]{};
    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&anchor), &self)) return {};
    DWORD length = GetModuleFileNameA(self, path, MAX_PATH);
    if (!length || length == MAX_PATH) return {};
    std::string directory(path, length);
#else
    Dl_info info{};
    if (!dladdr(&anchor, &info) || !info.dli_fname) return {};
    std::string directory(info.dli_fname);
#endif
    auto separator = directory.find_last_of("/\\");
    return separator == std::string::npos ? std::string() : directory.substr(0, separator + 1);
}
// Loads path, or the platform runtime beside the bridge when path is null.
inline Module LoadRuntime(const char* path)
{
    std::string file = path ? std::string(path) : BridgeDirectory() + RuntimeName;
#if defined(_WIN32)
    return LoadLibraryExA(file.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
#else
    return dlopen(file.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
}
inline void UnloadRuntime(Module module)
{
#if defined(_WIN32)
    FreeLibrary(module);
#else
    dlclose(module);
#endif
}
template <class T> T Resolve(Module module, const char* name)
{
#if defined(_WIN32)
    return reinterpret_cast<T>(GetProcAddress(module, name));
#else
    return reinterpret_cast<T>(dlsym(module, name));
#endif
}
} // namespace platform
