# SteamAPIBridge

Read README.md for runtime ownership and ABI boundaries. Build with the x86 Debug
and Release scripts and run CTest. Keep public exports C-compatible and prefixed
SteamBridge_. Do not link steam_api.lib, initialize or shut down the host Steam
session, or expose vendor C++ objects in the public ABI. External SDK trees are
read-only. Preserve historical interface slots and verify callback layouts against
the referenced Valve headers when extending compatibility. Record simulated and
real-game verification separately. Tests exercise behavior and binary loading,
not documentation or configuration text.

## C/C++ formatting

Owned C/C++ sources use the shared DiligentCore style from
[MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation), pinned
to clang-format 23.1.3. Setup, commands and the checked scope are recorded in
`memory/CppFormatting.md` (`CppFormatting`). Do not hand-edit the generated
`.clang-format`; change the shared style. Keep MSVC inline assembly in explicit blocks.
