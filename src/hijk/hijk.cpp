#include "hijk/hijk.h"
#include "MinHook.h"
#include "trampoline.h"
#include "OrbitAsm.h"
#include <iostream>

#include <assert.h>

extern "C" void* OnTrampolineCreated(PTRAMPOLINE ct, void* relay_buffer, UINT relay_buffer_size) {
	std::cout << "Trampoline created with " << relay_buffer_size << " free space";
	return WritePrologAndEpilogForTargetFunction(ct->pTarget, ct->pTrampoline, relay_buffer, relay_buffer_size);
}

struct MinHookInitializer {
	MinHookInitializer() {
		if (MH_Initialize() == MH_OK) {
			init_status = Hijk_Status::kOk;
		}
		SetTrampolineOverrideCallback(&OnTrampolineCreated);
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
    
	void* original = nullptr;
     MH_STATUS MinHookError = MH_CreateHook(target_function, nullptr, &original );
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
