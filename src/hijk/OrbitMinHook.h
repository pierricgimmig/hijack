#pragma once

#include "MinHook.h"
#include "trampoline.h"

// Creates a Hook for the specified target function, in disabled state.
// Parameters:
//   pTarget    [in]  A pointer to the target function, which will be
//                    overridden by the detour function.
//   pCallback  [in]  A pointer to the callback function called in the prolog
//   ppOriginal [out] A pointer to the trampoline function, which will be
//                    used to call the original target function.
//                    This parameter can be NULL.
MH_STATUS WINAPI Orbit_MH_CreateHookPrologOnly(LPVOID pTarget, LPVOID pPrologCallback);

// Creates a Hook for the specified target function, in disabled state.
// Parameters:
//   pTarget    [in]  A pointer to the target function, which will be
//                    overridden by the detour function.
//   pCallback  [in]  A pointer to the callback function called in the prolog
//   ppOriginal [out] A pointer to the trampoline function, which will be
//                    used to call the original target function.
//                    This parameter can be NULL.
MH_STATUS WINAPI Orbit_MH_CreateHookPrologEpilog(LPVOID pTarget, LPVOID pPrologCallback, LPVOID pEpilogCallback, LPVOID pReturnAddressCallback);

// Enables an already created hooks.
// Parameters:
//   pTarget [in] An array of pointers to the target functions.
//   numTargets:  Number of functions
//   enable:      Enable/Disable
MH_STATUS Orbit_MH_EnableHooks( DWORD64* pTargets, UINT numTargets, BOOL enable );

// @CodeHook: Disables all hooks
MH_STATUS WINAPI Orbit_MH_DisableAllHooks(VOID);

// Test asm function
void WINAPI Orbit_MH_Test();

BOOL CreatePrologFunction(PTRAMPOLINE ct, LPVOID pPrologCallback, LPVOID pEpilogCallback);