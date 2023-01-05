#pragma once

#define HIJK_API __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

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

	// Dynamically instrument `target_function` and call prologue/epilogue callbacks on `target_function` entry/exit.
	HIJK_API Hijk_Status Hijk_CreateHook(void* target_function, PrologueCallback prologue_callback, EpilogueCallback epilogue_callback);

	HIJK_API Hijk_Status Hijk_RemoveHook(void* target_function);
	HIJK_API Hijk_Status Hijk_RemoveAllHooks();

	HIJK_API Hijk_Status Hijk_EnableHook(void* target_function);
	HIJK_API Hijk_Status Hijk_EnableAllHooks();

	HIJK_API Hijk_Status Hijk_DisableHook(void* target_function);
	HIJK_API Hijk_Status Hijk_DisableAllHooks();

	HIJK_API const char* Hijk_ToString(Hijk_Status status);

struct PrologueContext {

};

struct EpilogueContext {

};

#ifdef __cplusplus
}
#endif
