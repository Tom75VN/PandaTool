# PandaTool

Extra read-only game info for Panda addons on the OctoWoW client.

PandaTool is a small Windows DLL. It gives Panda addons game data that the
normal addon (Lua) API cannot reach, starting with the camera, so the
PandaQuest quest marker can sit on the right spot on your screen.

PandaTool is optional. Every Panda addon keeps working without it.

## What it gives you

**With PandaTool, the PandaQuest HUD marker sits on the quest area itself.**

| | Without PandaTool | With PandaTool |
| --- | --- | --- |
| Marker position | Slides left and right only, from your facing | Placed on the real quest spot on screen |
| Camera tilt and zoom | Ignored | Marker moves up and down with them |
| Camera orbit (left mouse) | Ignored | Marker follows the camera |
| Hills and cliffs | Assumes flat ground at your height | Uses the real ground height |
| Target behind you | No hint | Marker pins to the screen edge on the correct side |

Also:

- **Light.** Each call costs a fraction of a microsecond and creates no Lua
  garbage, so no frame stutter.
- **Read-only.** It only reads what the game already shows you: camera,
  ground height and your own position. It never moves the camera or your
  character, never targets, never presses keys, never scans other players
  or creatures, never touches the network.

## Download

**[Download PandaTool.dll](https://github.com/Tom75VN/PandaTool/releases/latest/download/PandaTool.dll)**
(latest version). Older versions and change notes:
[Releases](https://github.com/Tom75VN/PandaTool/releases).

## Install

You need the OctoWoW launcher.

1. [Download `PandaTool.dll`](https://github.com/Tom75VN/PandaTool/releases/latest/download/PandaTool.dll).
2. Put it in your game folder, next to `WoW.exe`.
3. Open the OctoWoW launcher and go to **MODS**.
4. Under **Your DLL mods**, click **Add DLL** and pick `PandaTool.dll`
   (or just check it is listed, since the launcher also finds DLLs dropped
   into the game folder).
5. Make sure the PandaTool toggle is **on**.
6. Start the game from the launcher.

Check it loaded: type this in the chat box.

```
/run print(PANDATOOL_VERSION)
```

- A number (for example `103`) means PandaTool is working.
- `nil` means it did not load: check the toggle in the launcher, then
  restart the game.

Nothing to set up in the addon: PandaQuest finds PandaTool by itself.

### Update

1. Close the game.
2. Replace `PandaTool.dll` in the game folder with the new one.
3. Start the game from the launcher.

### Remove

Turn PandaTool off in the launcher's **MODS** page, or delete
`PandaTool.dll` from the game folder. Your addons go back to their normal
behaviour.

### Do not

- Do not put `PandaTool.dll` in `Interface\AddOns`. It is not an addon.
- Do not try to update it while the game is running: Windows locks a
  loaded DLL.

## Server rules

PandaTool is built to stay within OctoWoW's rules: it is read-only and
gives no gameplay advantage. It follows the same techniques as UnitXP_SP3
and ClassicAPI, which the launcher already offers. OctoWoW staff have not
yet given a formal answer on it. As with any client DLL, use it at your
own risk.

## Status

Version 0.1.3 (`PANDATOOL_VERSION` 103). Tested in game, alone and beside
ClassicAPI. Still 0.x: the Lua functions may change before 1.0.

## For addon authors

All values are plain numbers. Every call returns `nil` when the data is
not available (loading screen, login, cinematics). Always check for
PandaTool before using it, and keep a fallback.

```lua
if PandaTool and PANDATOOL_VERSION and PANDATOOL_VERSION >= 103 then
  local x, y, visible = PandaTool.WorldToScreen(posY, posX, posZ)
end
```

| Function | Returns |
| --- | --- |
| `PANDATOOL_VERSION` | `major*10000 + minor*100 + patch` |
| `PandaTool.WorldToScreen(posY, posX, posZ)` | `x, y, visible` in `WorldFrame` units, origin bottom-left |
| `PandaTool.GetCameraInfo()` | `posY, posX, posZ, yaw, pitch, fov, distance` |
| `PandaTool.GetGroundZ(posY, posX [, posZ])` | ground `posZ`, or `nil` |
| `PandaTool.GetPlayerPosition()` | `posY, posX, posZ, mapId` |

- World coordinates use the same order as ClassicAPI
  `UnitPosition("player")`, in yards.
- Angles are radians. `pitch` is positive when looking up.
- `visible` is `1` when the point is on screen, else `nil`. A point behind
  the camera still returns `x, y`, mirrored to the correct side.

## Build from source

Needs Visual Studio Build Tools (MSVC, x86) with CMake. The client is
32-bit, so the DLL is too.

```
build.cmd
```

Output: `build\Release\PandaTool.dll`. `build.cmd install` also copies it
to the game folder (close the game first).

## Credits

- [MinHook](https://github.com/TsudaKageyu/minhook) (BSD 2-Clause), Tsuda
  Kageyu.
- Engine addresses follow UnitXP_SP3 (AllFoxxy, MIT) and ClassicAPI
  (GPL-3.0, read for reference only; no ClassicAPI code included).

Full notices: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
