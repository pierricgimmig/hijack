#include "hijk/hijk.h"

#include <assert.h>

#include <iostream>

#include "MinHook.h"
#include "OrbitAsm.h"
#include "trampoline.h"

extern "C" void* OnTrampolineCreated(PTRAMPOLINE ct, void* relay_buffer, UINT relay_buffer_size);

class MinHookInitializer {
 public:
  MinHookInitializer() {
    ok_ = MH_Initialize() == MH_OK;
    if (ok_) {
      SetTrampolineOverrideCallback(&OnTrampolineCreated);
    }
  }
  ~MinHookInitializer() { MH_Uninitialize(); }
  bool Ok() const { return ok_; }

 private:
  bool ok_ = false;
};

static void EnsureMinhookInitialized() {
  static MinHookInitializer initializer;
  if (!initializer.Ok()) {
    std::cout << "Could not initialize MinHook, aborting";
    abort();
  }
}

extern "C" {

bool Hijk_CreateHook(void* target_function, PrologueCallback prologue_callback,
                     EpilogueCallback epilogue_callback) {
  EnsureMinhookInitialized();
  void* original = nullptr;
  return MH_CreateHook(target_function, nullptr, &original) == MH_OK;
}

bool Hijk_RemoveHook(void* target_function) {
  assert(0);
  EnsureMinhookInitialized();
  return true;
}

bool Hijk_RemoveAllHooks() {
  assert(0);
  EnsureMinhookInitialized();
  return true;
}

bool Hijk_EnableHook(void* target_function) {
  EnsureMinhookInitialized();
  return MH_EnableHook(target_function) == MH_OK;
}

bool Hijk_EnableAllHooks() {
  assert(0);
  EnsureMinhookInitialized();
  return true;
}

bool Hijk_DisableHook(void* target_function) {
  assert(0);
  EnsureMinhookInitialized();
  return true;
}
bool Hijk_DisableAllHooks() {
  assert(0);
  EnsureMinhookInitialized();
  return true;
}

void* OnTrampolineCreated(PTRAMPOLINE ct, void* relay_buffer, UINT relay_buffer_size) {
  std::cout << "Trampoline created with " << relay_buffer_size << " free space";
  return WritePrologAndEpilogForTargetFunction(ct->pTarget, ct->pTrampoline, relay_buffer,
                                               relay_buffer_size);
}

}  // extern "C" {
