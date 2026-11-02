#pragma once

// major * 10000 + minor * 100 + patch. 1.0.0 only after the acceptance probe.
constexpr int PANDATOOL_VERSION = 103;

// Creates the PandaTool table and PANDATOOL_VERSION in the world Lua state.
void RegisterLua();
