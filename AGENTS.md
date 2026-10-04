# SteamAPIBridge

Read README.md for runtime ownership and ABI boundaries. Build with the x86 Debug
and Release scripts and run CTest. Keep public exports C-compatible and prefixed
SteamBridge_. Do not link steam_api.lib, initialize or shut down the host Steam
session, or expose vendor C++ objects in the public ABI. External SDK trees are
read-only. Preserve historical interface slots and verify callback layouts against
the referenced Valve headers when extending compatibility. Record simulated and
real-game verification separately. Tests exercise behavior and binary loading,
not documentation or configuration text.
