# SteamAPIBridge

Read README.md for runtime ownership and ABI boundaries. Build with the Windows
x86/x64 and Linux scripts and run CTest on each platform you change. Keep public exports
C-compatible and prefixed SteamBridge_. Do not link steam_api.lib or expose vendor C++
objects in the public ABI. Only host mode (SteamBridge_Host*/Server*) initializes or
shuts down Steam, and only when the host requests it; plugin-mode code never touches
the game's Steam session. External SDK trees are read-only.
Preserve historical interface slots and verify callback layouts against the
referenced Valve headers when extending compatibility. Record simulated and
real-game verification separately. Tests exercise behavior and binary loading,
not documentation or configuration text.

## C/C++ formatting

Owned C/C++ sources use the shared DiligentCore style from
[MetaHookSv/FormatValidation](https://github.com/MetaHookSv/FormatValidation), pinned
to clang-format 23.1.3. Setup, commands and the checked scope are recorded in
`memory/CppFormatting.md` (`CppFormatting`). Do not hand-edit the generated
`.clang-format`; change the shared style. Keep MSVC inline assembly in explicit blocks.
