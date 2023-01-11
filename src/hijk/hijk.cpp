#include "hijk/hijk.h"
#include "MinHook.h"

#include <assert.h>

struct MinHookInitializer {
	MinHookInitializer() {
		if (MH_Initialize() == MH_OK) {
			init_status = Hijk_Status::kOk;
		}
	}

	~MinHookInitializer() {
		MH_Uninitialize();
	}

	bool Ok() const { return init_status == Hijk_Status::kOk; }

	Hijk_Status init_status = Hijk_Status::kInernalInitializationError;
};

const MinHookInitializer& GetMinHookInitializer() { 
	static MinHookInitializer initializer;
	return initializer;
}

void* GetReturnAddress()
{
  assert(0);
  return nullptr;
}

extern "C" {

Hijk_Status Hijk_CreateHook(void* target_function, PrologueCallback prologue_callback, EpilogueCallback epilogue_callback) {
    if (!GetMinHookInitializer().Ok()) return Hijk_Status::kInernalInitializationError;
    
     MH_STATUS MinHookError = MH_Orbit_CreateHookPrologEpilog(target_function, prologue_callback, epilogue_callback, &GetReturnAddress );
    return MinHookError == MH_OK ? Hijk_Status::kOk : Hijk_Status::kUnknown;

	return Hijk_Status::kOk;
}
Hijk_Status Hijk_RemoveHook(void* target_function) {
	if (!GetMinHookInitializer().Ok()) return Hijk_Status::kInernalInitializationError;

	return Hijk_Status::kOk;
}
Hijk_Status Hijk_RemoveAllHooks() { return Hijk_Status::kOk; }

Hijk_Status Hijk_EnableHook(void* target_function) {
	if (!GetMinHookInitializer().Ok()) return Hijk_Status::kInernalInitializationError;
	return MH_EnableHook(target_function) == MH_OK ? Hijk_Status::kOk : Hijk_Status::kUnknown;
	return Hijk_Status::kOk;
}
Hijk_Status Hijk_EnableAllHooks() {
	if (!GetMinHookInitializer().Ok()) return Hijk_Status::kInernalInitializationError;
	return Hijk_Status::kOk;
}
Hijk_Status Hijk_DisableHook(void* target_function) { if (!GetMinHookInitializer().Ok()) return Hijk_Status::kInernalInitializationError; return Hijk_Status::kOk; }
Hijk_Status Hijk_DisableAllHooks() { if (!GetMinHookInitializer().Ok()) return Hijk_Status::kInernalInitializationError; return Hijk_Status::kOk; }

const char* Hijk_ToString(Hijk_Status status) {
	switch (status) {
	case kUnknown: return "Hijk unknown error";
	case kOk: return "Hijk OK";
	case kHookAlreadyCreated: return "Hook already created";
	case kNoCallbackSpecified: return "No callback specified";
	case kInernalInitializationError: return "Internal hooking library initialization error";
	default: return "Unhandled status case";
	};
}

}  // extern "C" {
