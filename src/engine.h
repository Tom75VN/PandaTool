#pragma once

// OctoWoW 1.12.1 (build 5875) engine addresses. Sources and probe status:
// .claude/rules/reference-dlls.md "Phase 1 findings".

#include <cstdint>

static_assert(sizeof(void*) == 4, "PandaTool is a 32-bit DLL");

namespace engine {

struct Vec3 {
    float x;  // north
    float y;  // west
    float z;  // up
};

using LuaCFunction = int(__fastcall*)(void* L);

constexpr int kLuaGlobalsIndex = -10001;

// Lua 5.0 C API inside WoW.exe (fastcall, L in ecx).
int lua_gettop(void* L);
void lua_settop(void* L, int idx);
int lua_isnumber(void* L, int idx);
double lua_tonumber(void* L, int idx);
void lua_pushnil(void* L);
void lua_pushnumber(void* L, double n);
void lua_pushstring(void* L, const char* s);
void lua_pushcclosure(void* L, LuaCFunction fn, int n);
void lua_newtable(void* L);
void lua_settable(void* L, int idx);

void* LuaState();

// The client's Lua function-address check. It rejects C functions outside
// WoW.exe, so PandaTool's Lua calls need it disabled.
// UnitXP_SP3 dllmain.cpp:40-45,499, ClassicAPI DllMain.cpp:38,146.
using FunctionAddressCheck_t = void(__fastcall*)(uint32_t);
constexpr uintptr_t kFunctionAddressCheck = 0x42A320;

// True when the check already starts with a jump or a return (MinHook detour
// from PandaTool, UnitXP_SP3 or ClassicAPI, or a byte patch).
bool FunctionAddressCheckDisabled();

using LoadScriptFunctions_t = void(__fastcall*)();
constexpr uintptr_t kLoadScriptFunctions = 0x490250;

struct Camera {
    Vec3 position;
    Vec3 forward;
    float fov;
    float distance;
};

// All of these return false instead of faulting.
bool ReadCamera(Camera& out);
bool WorldToScreen(const Vec3& world, float& x, float& y, bool& visible);
bool GroundZ(float x, float y, float zTop, float zBottom, float& z);
// The player's own world position (never other units) and the Map.dbc id.
bool PlayerPosition(Vec3& pos, int& mapId);

}  // namespace engine
