#pragma once

#define HIJK_API __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

// Dynamically instrument `target_function` and call prologue/epilogue callbacks on
// `target_function` entry/exit.
typedef void (*PrologueCallback)(void* target_function, struct PrologueContext* prologue_context);
typedef void (*EpilogueCallback)(void* target_function, struct EpilogueContext* epilogue_context);

HIJK_API bool Hijk_CreateHook(void* target_function, PrologueCallback prologue_callback,
                              EpilogueCallback epilogue_callback);

HIJK_API bool Hijk_RemoveHook(void* target_function);
HIJK_API bool Hijk_RemoveAllHooks();

HIJK_API bool Hijk_EnableHook(void* target_function);
HIJK_API bool Hijk_EnableAllHooks();

HIJK_API bool Hijk_DisableHook(void* target_function);
HIJK_API bool Hijk_DisableAllHooks();

struct PrologueContext {};

struct EpilogueContext {};

#ifdef __cplusplus
}
#endif
