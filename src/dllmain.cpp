#include <windows.h>

#include "MinHook.h"
#include "api.h"
#include "engine.h"

namespace {

engine::LoadScriptFunctions_t g_loadScriptFunctionsOriginal = nullptr;
bool g_hooked = false;

// Empty detour, the same as UnitXP_SP3 (dllmain.cpp:43-45) and ClassicAPI.
void __fastcall FunctionAddressCheckHook(uint32_t) {}

// Standalone: disable the check unless another DLL already did. A DLL loaded
// later can still hook on top; MinHook chains through the jump.
void DisableFunctionAddressCheck() {
    if (engine::FunctionAddressCheckDisabled()) {
        return;
    }
    auto* target = reinterpret_cast<LPVOID>(engine::kFunctionAddressCheck);
    if (MH_CreateHook(target, reinterpret_cast<LPVOID>(&FunctionAddressCheckHook), nullptr) != MH_OK) {
        return;
    }
    if (MH_EnableHook(target) != MH_OK) {
        MH_RemoveHook(target);
    }
}

void __fastcall LoadScriptFunctionsHook() {
    g_loadScriptFunctionsOriginal();
    RegisterLua();
}

bool InstallHook() {
    if (MH_Initialize() != MH_OK) {
        return false;
    }
    auto* target = reinterpret_cast<LPVOID>(engine::kLoadScriptFunctions);
    if (MH_CreateHook(target, reinterpret_cast<LPVOID>(&LoadScriptFunctionsHook),
                      reinterpret_cast<LPVOID*>(&g_loadScriptFunctionsOriginal)) != MH_OK) {
        MH_Uninitialize();
        return false;
    }
    if (MH_EnableHook(target) != MH_OK) {
        MH_RemoveHook(target);
        MH_Uninitialize();
        return false;
    }
    // Registration still checks the result, so a failure here only leaves
    // PandaTool absent.
    DisableFunctionAddressCheck();
    g_hooked = true;
    return true;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        // A failed hook leaves PandaTool absent; the client keeps running.
        InstallHook();
    } else if (reason == DLL_PROCESS_DETACH && g_hooked && reserved == nullptr) {
        // Only on FreeLibrary; at process exit another DLL may have hooked on top.
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
    return TRUE;
}
