#pragma once

typedef void (*PrologueCallback)(void* original_function, struct PrologueContext* prologue_context);
typedef void (*EpilogueCallback)(void* original_function, struct EpilogueContext* epilogue_context);

typedef enum
{
	kUnknown = -1,
	kOk = 0,
	kHookAlreadyCreated,
	kNoCallbackSpecified,
	kInernalInitializationError,	
} Hijk_Status;

#ifdef __cplusplus
extern "C" {
#endif

	// Dynamically instrument `target_function` and call prologue/epilogue callbacks on `target_function` entry/exit.
	Hijk_Status Hijk_CreateHook(void* target_function, PrologueCallback* prologue_callback, EpilogueCallback* epilogue_callback);

	Hijk_Status Hijk_RemoveHook(void* target_function);
	Hijk_Status Hijk_RemoveAllHooks();

	Hijk_Status Hijk_EnableHook(void* target_function);
	Hijk_Status Hijk_EnableAllHooks();

	Hijk_Status Hijk_DisableHook(void* target_function);
	Hijk_Status Hijk_DisableAllHooks();

	const char* Hijk_ToString(Hijk_Status status);

#ifdef __cplusplus
}
#endif

struct PrologueContext {

};

struct EpilogueContext {

};
