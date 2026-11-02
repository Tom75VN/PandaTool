#include "engine.h"

#include <windows.h>

#include <cmath>

namespace engine {
namespace {

// Lua 5.0 C API: UnitXP_SP3 Vanilla1121_functions.cpp:47-67,
// ClassicAPI docs/LuaCAPI.md:76-130.
using GetTop_t = int(__fastcall*)(void*);
using SetTop_t = void(__fastcall*)(void*, int);
using IsNumber_t = int(__fastcall*)(void*, int);
using ToNumber_t = double(__fastcall*)(void*, int);
using PushNil_t = void(__fastcall*)(void*);
using PushNumber_t = void(__fastcall*)(void*, double);
using PushString_t = void(__fastcall*)(void*, const char*);
using PushCClosure_t = void(__fastcall*)(void*, LuaCFunction, int);
using NewTable_t = void(__fastcall*)(void*);
using SetTable_t = void(__fastcall*)(void*, int);

const auto p_gettop = reinterpret_cast<GetTop_t>(0x6F3070);
const auto p_settop = reinterpret_cast<SetTop_t>(0x6F3080);
const auto p_isnumber = reinterpret_cast<IsNumber_t>(0x6F34D0);
const auto p_tonumber = reinterpret_cast<ToNumber_t>(0x6F3620);
const auto p_pushnil = reinterpret_cast<PushNil_t>(0x6F37F0);
const auto p_pushnumber = reinterpret_cast<PushNumber_t>(0x6F3810);
const auto p_pushstring = reinterpret_cast<PushString_t>(0x6F3890);
const auto p_pushcclosure = reinterpret_cast<PushCClosure_t>(0x6F3920);
const auto p_newtable = reinterpret_cast<NewTable_t>(0x6F3C90);
const auto p_settable = reinterpret_cast<SetTable_t>(0x6F3E20);

// ClassicAPI Offsets.h:8523.
constexpr uintptr_t kLuaStatePtr = 0x00CEEF74;
// ClassicAPI Offsets.h:10241-10242.
constexpr uintptr_t kWorldFramePtr = 0x00B4B2BC;
constexpr uintptr_t kWorldFrameCamera = 0x65B8;
// UnitXP_SP3 Vanilla1121_functions.cpp:608-671.
constexpr uintptr_t kCameraPosition = 0x08;
constexpr uintptr_t kCameraMatrix = 0x14;
constexpr uintptr_t kCameraFov = 0x40;
constexpr uintptr_t kCameraCurrentDistance = 0xEC;
// ClassicAPI Offsets.h:1797-1806.
using GetScreenCoords_t = bool(__thiscall*)(void* worldFrame, const float* world, float* screen);
const auto p_getScreenCoords = reinterpret_cast<GetScreenCoords_t>(0x483EE0);
// ClassicAPI Game.cpp:35-41, Offsets.h:683-686.
constexpr uintptr_t kDdcWidthScale = 0x00832A44;
constexpr uintptr_t kAspectCompensation = 0x00832A4C;
constexpr float kUiCoordScaleUnit = 1024.0f;
// Points closer to the camera plane than this (yards) count as behind.
constexpr float kMinDepth = 0.05f;
// UnitXP_SP3 Vanilla1121_functions.cpp:165-195, ClassicAPI Offsets.h:4679-4690.
using Intersect_t = bool(__fastcall*)(const Vec3*, const Vec3*, int, Vec3*, float*, uint32_t);
const auto p_intersect = reinterpret_cast<Intersect_t>(0x672170);
constexpr uint32_t kIntersectTerrainWmo = 0x100111;
// Player object and position: UnitXP_SP3 Vanilla1121_functions.cpp:72,161-163
// (UnitGUID), :218-225 (object by GUID), :246-268 (GetPosition, vtable +0x14);
// ClassicAPI unit/Position.h:44-60 uses the same vtable slot.
using UnitGuid_t = uint64_t(__fastcall*)(const char* unit);
const auto p_unitGuid = reinterpret_cast<UnitGuid_t>(0x00515970);
using ObjectByGuid_t = uintptr_t(__fastcall*)(uint64_t guid);
const auto p_objectByGuid = reinterpret_cast<ObjectByGuid_t>(0x00464870);
using GetPosition_t = float*(__thiscall*)(uintptr_t object, float* out);
constexpr uintptr_t kVtableGetPosition = 0x14;
// ClassicAPI Offsets.h:7736 (current Map.dbc id).
constexpr uintptr_t kCurrentMapId = 0x00B4E378;
constexpr float kMaxRayLength = 149.0f;

template <typename T>
T Read(uintptr_t addr) {
    return *reinterpret_cast<const T*>(addr);
}

bool Finite(float v) {
    return std::isfinite(v) != 0;
}

bool Finite(const Vec3& v) {
    return Finite(v.x) && Finite(v.y) && Finite(v.z);
}

uintptr_t WorldFrame() {
    return Read<uintptr_t>(kWorldFramePtr);
}

uintptr_t CameraPtr(uintptr_t worldFrame) {
    if (worldFrame == 0) {
        return 0;
    }
    return Read<uintptr_t>(worldFrame + kWorldFrameCamera);
}

bool ReadCameraRaw(Camera& out) {
    const uintptr_t camera = CameraPtr(WorldFrame());
    if (camera == 0) {
        return false;
    }
    out.position = Read<Vec3>(camera + kCameraPosition);
    out.forward = Read<Vec3>(camera + kCameraMatrix);
    out.fov = Read<float>(camera + kCameraFov);
    out.distance = Read<float>(camera + kCameraCurrentDistance);
    return Finite(out.position) && Finite(out.forward) && Finite(out.fov) && Finite(out.distance);
}

bool WorldToScreenRaw(const Vec3& world, float& x, float& y, bool& visible) {
    const uintptr_t worldFrame = WorldFrame();
    const uintptr_t camera = CameraPtr(worldFrame);
    if (camera == 0) {
        return false;
    }
    const float mul = Read<float>(kDdcWidthScale);
    const float comp = Read<float>(kAspectCompensation);
    if (!(mul > 0.0f) || !Finite(comp)) {
        return false;
    }

    const Vec3 pos = Read<Vec3>(camera + kCameraPosition);
    Vec3 fwd = Read<Vec3>(camera + kCameraMatrix);
    const float len = std::sqrt(fwd.x * fwd.x + fwd.y * fwd.y + fwd.z * fwd.z);
    if (!(len > 0.0f) || !Finite(pos)) {
        return false;
    }
    fwd = {fwd.x / len, fwd.y / len, fwd.z / len};
    const float depth =
        (world.x - pos.x) * fwd.x + (world.y - pos.y) * fwd.y + (world.z - pos.z) * fwd.z;

    // The engine writes nothing for a point behind the camera (in game it
    // came back as 0, 0). Project the point mirrored to the front instead,
    // so x, y stay on the same left / right side for edge pinning.
    float shift = 0.0f;
    if (depth <= kMinDepth) {
        shift = -2.0f * depth;
        if (shift < kMinDepth - depth) {
            shift = kMinDepth - depth;
        }
    }
    const float point[3] = {world.x + fwd.x * shift, world.y + fwd.y * shift,
                            world.z + fwd.z * shift};
    float screen[2] = {0.0f, 0.0f};
    const bool inRect = p_getScreenCoords(reinterpret_cast<void*>(worldFrame), point, screen);

    const float toUi = comp * kUiCoordScaleUnit / mul;
    x = screen[0] * toUi;
    y = screen[1] * toUi;
    visible = inRect && depth > kMinDepth;
    return Finite(x) && Finite(y);
}

bool GroundZRaw(float x, float y, float zTop, float zBottom, float& z) {
    if (CameraPtr(WorldFrame()) == 0) {
        return false;
    }
    const Vec3 from = {x, y, zTop};
    const Vec3 to = {x, y, zBottom};
    Vec3 hit = {0.0f, 0.0f, 0.0f};
    float t = 1.0f;
    if (!p_intersect(&from, &to, 0, &hit, &t, kIntersectTerrainWmo)) {
        return false;
    }
    if (!(t >= 0.0f && t <= 1.0f)) {
        return false;
    }
    z = zTop + (zBottom - zTop) * t;
    return Finite(z);
}

bool PlayerPositionRaw(Vec3& pos, int& mapId) {
    const uint64_t guid = p_unitGuid("player");
    if (guid == 0) {
        return false;
    }
    const uintptr_t player = p_objectByGuid(guid);
    if (player == 0 || (player & 1) != 0) {
        return false;
    }
    const uintptr_t vtable = Read<uintptr_t>(player);
    if (vtable == 0 || (vtable & 1) != 0) {
        return false;
    }
    const uintptr_t fn = Read<uintptr_t>(vtable + kVtableGetPosition);
    if (fn == 0 || (fn & 1) != 0) {
        return false;
    }
    float out[3] = {0.0f, 0.0f, 0.0f};
    const float* p = reinterpret_cast<GetPosition_t>(fn)(player, out);
    if (p == nullptr) {
        return false;
    }
    pos = {p[0], p[1], p[2]};
    mapId = Read<int>(kCurrentMapId);
    return Finite(pos);
}

}  // namespace

int lua_gettop(void* L) { return p_gettop(L); }
void lua_settop(void* L, int idx) { p_settop(L, idx); }
int lua_isnumber(void* L, int idx) { return p_isnumber(L, idx); }
double lua_tonumber(void* L, int idx) { return p_tonumber(L, idx); }
void lua_pushnil(void* L) { p_pushnil(L); }
void lua_pushnumber(void* L, double n) { p_pushnumber(L, n); }
void lua_pushstring(void* L, const char* s) { p_pushstring(L, s); }
void lua_pushcclosure(void* L, LuaCFunction fn, int n) { p_pushcclosure(L, fn, n); }
void lua_newtable(void* L) { p_newtable(L); }
void lua_settable(void* L, int idx) { p_settable(L, idx); }

void* LuaState() {
    return Read<void*>(kLuaStatePtr);
}

bool FunctionAddressCheckDisabled() {
    // MinHook writes a jump at the entry; a byte patch would write a ret.
    // Anything else means the check is live.
    const uint8_t first = Read<uint8_t>(kFunctionAddressCheck);
    return first == 0xE9 || first == 0xEB || first == 0xC3 || first == 0xC2;
}

bool ReadCamera(Camera& out) {
    __try {
        return ReadCameraRaw(out);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool WorldToScreen(const Vec3& world, float& x, float& y, bool& visible) {
    if (!Finite(world)) {
        return false;
    }
    __try {
        return WorldToScreenRaw(world, x, y, visible);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool GroundZ(float x, float y, float zTop, float zBottom, float& z) {
    if (!Finite(x) || !Finite(y) || !Finite(zTop) || !Finite(zBottom)) {
        return false;
    }
    if (!(zTop > zBottom) || zTop - zBottom > kMaxRayLength) {
        return false;
    }
    __try {
        return GroundZRaw(x, y, zTop, zBottom, z);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool PlayerPosition(Vec3& pos, int& mapId) {
    __try {
        return PlayerPositionRaw(pos, mapId);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

}  // namespace engine
