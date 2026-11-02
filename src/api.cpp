#include "api.h"

#include "engine.h"

#include <cmath>

namespace {

using namespace engine;

constexpr double kTwoPi = 6.283185307179586;
constexpr float kGroundRayAbove = 50.0f;
constexpr float kGroundRayBelow = 99.0f;

bool ArgNumber(void* L, int idx, float& out) {
    if (lua_gettop(L) < idx || lua_isnumber(L, idx) == 0) {
        return false;
    }
    const double v = lua_tonumber(L, idx);
    if (!std::isfinite(v)) {
        return false;
    }
    out = static_cast<float>(v);
    return true;
}

// PandaTool.GetCameraInfo() -> posY, posX, posZ, yaw, pitch, fov, distance
int __fastcall Lua_GetCameraInfo(void* L) {
    Camera cam;
    if (!ReadCamera(cam)) {
        lua_pushnil(L);
        return 1;
    }
    const double fx = cam.forward.x;
    const double fy = cam.forward.y;
    const double fz = cam.forward.z;
    const double len = std::sqrt(fx * fx + fy * fy + fz * fz);
    if (!(len > 0.0)) {
        lua_pushnil(L);
        return 1;
    }
    double yaw = std::atan2(fy, fx);
    if (yaw < 0.0) {
        yaw += kTwoPi;
    }
    double s = fz / len;
    s = s > 1.0 ? 1.0 : (s < -1.0 ? -1.0 : s);
    const double pitch = std::asin(s);

    lua_pushnumber(L, cam.position.y);
    lua_pushnumber(L, cam.position.x);
    lua_pushnumber(L, cam.position.z);
    lua_pushnumber(L, yaw);
    lua_pushnumber(L, pitch);
    lua_pushnumber(L, cam.fov);
    lua_pushnumber(L, cam.distance);
    return 7;
}

// PandaTool.WorldToScreen(posY, posX, posZ) -> x, y, visible
int __fastcall Lua_WorldToScreen(void* L) {
    Vec3 world;
    float x = 0.0f;
    float y = 0.0f;
    bool visible = false;
    if (!ArgNumber(L, 1, world.y) || !ArgNumber(L, 2, world.x) || !ArgNumber(L, 3, world.z) ||
        !WorldToScreen(world, x, y, visible)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    if (visible) {
        lua_pushnumber(L, 1);
    } else {
        lua_pushnil(L);
    }
    return 3;
}

// PandaTool.GetGroundZ(posY, posX [, posZ]) -> posZ
// The ray runs from posZ + 50 down to posZ - 99; posZ defaults to the camera's.
int __fastcall Lua_GetGroundZ(void* L) {
    float wy = 0.0f;
    float wx = 0.0f;
    float ref = 0.0f;
    if (!ArgNumber(L, 1, wy) || !ArgNumber(L, 2, wx)) {
        lua_pushnil(L);
        return 1;
    }
    if (!ArgNumber(L, 3, ref)) {
        Camera cam;
        if (!ReadCamera(cam)) {
            lua_pushnil(L);
            return 1;
        }
        ref = cam.position.z;
    }
    float z = 0.0f;
    if (!GroundZ(wx, wy, ref + kGroundRayAbove, ref - kGroundRayBelow, z)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, z);
    return 1;
}

// PandaTool.GetPlayerPosition() -> posY, posX, posZ, mapId
// Same order as ClassicAPI UnitPosition("player"); the player only.
int __fastcall Lua_GetPlayerPosition(void* L) {
    Vec3 pos;
    int mapId = 0;
    if (!PlayerPosition(pos, mapId)) {
        lua_pushnil(L);
        return 1;
    }
    lua_pushnumber(L, pos.y);
    lua_pushnumber(L, pos.x);
    lua_pushnumber(L, pos.z);
    lua_pushnumber(L, mapId);
    return 4;
}

struct Entry {
    const char* name;
    LuaCFunction func;
};

const Entry kFunctions[] = {
    {"GetCameraInfo", &Lua_GetCameraInfo},
    {"WorldToScreen", &Lua_WorldToScreen},
    {"GetGroundZ", &Lua_GetGroundZ},
    {"GetPlayerPosition", &Lua_GetPlayerPosition},
};

}  // namespace

void RegisterLua() {
    void* L = LuaState();
    if (L == nullptr || !FunctionAddressCheckDisabled()) {
        return;
    }
    const int top = lua_gettop(L);

    lua_pushstring(L, "PandaTool");
    lua_newtable(L);
    for (const Entry& e : kFunctions) {
        lua_pushstring(L, e.name);
        lua_pushcclosure(L, e.func, 0);
        lua_settable(L, -3);
    }
    lua_settable(L, kLuaGlobalsIndex);

    lua_pushstring(L, "PANDATOOL_VERSION");
    lua_pushnumber(L, PANDATOOL_VERSION);
    lua_settable(L, kLuaGlobalsIndex);

    lua_settop(L, top);
}
