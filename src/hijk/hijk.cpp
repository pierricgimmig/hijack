#include "hijk/hijk.h"

#include <assert.h>

#include <iostream>

#include "MinHook.h"
#include "trampoline.h"

#include <iostream>
#include <sstream>
#include <vector>

// hijk.asm functions
extern "C" {
void HijkGetXmmRegisters(struct HijkXmmRegisters* a_Context);
void HijkSetXmmRegisters(struct HijkXmmRegisters* a_Context);
void HijkGetIntegerRegisters(void* a_Context);
void HijkSetIntegerRegisters(void* a_Context);
void HijkPrologAsm();
void HijkPrologAsmFixed();
void HijkEpilogAsmFixed();
void HijkEpilogAsm();
}

std::vector<byte> dummyEnd = {0x49, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F};
std::vector<byte> dummyAddress = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};

extern "C" void* OnTrampolineCreated(PTRAMPOLINE ct, void* relay_buffer, UINT relay_buffer_size);
void* WritePrologAndEpilogForTargetFunction(void* target_function, void* trampoline, void* buffer,
                                            uint64_t buffer_size);

class MinHookInitializer {
 public:
  MinHookInitializer() {
    ok_ = MH_Initialize() == MH_OK;
    SetTrampolineOverrideCallback(&OnTrampolineCreated);
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

enum PrologOffset { Prolog_Data = 0, Prolog_NumOffsets };
enum EpilogOffset { Epilog_EpilogData = 0, Epilog_NumOffsets };

struct Prolog {
  byte* code;
  size_t size;
  size_t offsets[Prolog_NumOffsets];
};

struct Epilog {
  byte* code;
  uint32_t size;
  uint64_t offsets[Epilog_NumOffsets];
};

struct ContextScope {
  ContextScope() {
    HijkGetIntegerRegisters(&integer_registers_);
    HijkGetXmmRegisters(&xmm_registers_);
  }

  ~ContextScope() {
    HijkSetIntegerRegisters(&integer_registers_);
    HijkSetXmmRegisters(&xmm_registers_);
  }

  HijkIntegerRegisters integer_registers_;
  alignas(16) HijkXmmRegisters xmm_registers_;
};

thread_local std::vector<void*> return_addresses;

void UserPrologStub(PrologData* prolog_data, void** address_of_return_address) {
  ContextScope context_scope;
  std::cout << "Prolog!\n";
  return_addresses.push_back(*address_of_return_address);
  PrologueCallback user_callback = static_cast<PrologueCallback>(prolog_data->user_callback);
  if (user_callback) user_callback(prolog_data->original_function, nullptr);
}

void UserEpilogStub(EpilogData* epilog_data, void** address_of_return_address) {
  ContextScope context_scope;

  std::cout << "Epilog!\n";
  void* return_address = return_addresses.back();
  return_addresses.pop_back();
  *address_of_return_address = return_address;
  EpilogueCallback user_callback = static_cast<EpilogueCallback>(epilog_data->user_callback);
  if (user_callback) user_callback(epilog_data->original_function, nullptr);
}

size_t FindSize(const byte* code, size_t max_bytes = 1024) {
  size_t matchSize = dummyEnd.size();
  for (size_t i = 0; i < max_bytes; ++i) {
    size_t j = 0;
    for (j = 0; j < matchSize; ++j) {
      if (code[i + j] != dummyEnd[j]) break;
    }

    if (j == matchSize) return i;
  }

  return 0;
}

std::vector<size_t> FindOffsets(const byte* code, size_t num_offsets,
                                const std::vector<byte>& identifier, size_t max_bytes = 1024) {
  size_t matchSize = identifier.size();
  std::vector<size_t> offsets;

  for (size_t i = 0; i < max_bytes; ++i) {
    size_t j = 0;
    for (j = 0; j < matchSize; ++j) {
      if (code[i + j] != identifier[j]) break;
    }

    if (j == matchSize) {
      offsets.push_back(i);
      i += matchSize;
      if (offsets.size() == num_offsets) break;
    }
  }

  return offsets;
}

Prolog CreateProlog() {
  Prolog prolog;
  prolog.code = (byte*)&HijkPrologAsm;
  prolog.size = FindSize(prolog.code);

  std::vector<size_t> offsets = FindOffsets(prolog.code, Prolog_NumOffsets, dummyAddress);

  if (offsets.size() != Prolog_NumOffsets) {
    OutputDebugStringA("Hijk: Did not find expected number of offsets in the Prolog\n");
    return {};
  }

  for (size_t i = 0; i < offsets.size(); ++i) {
    prolog.offsets[i] = offsets[i];
  }
  return prolog;
}

Epilog CreateEpilog() {
  Epilog epilog;
  epilog.code = (byte*)&HijkEpilogAsm;
  epilog.size = FindSize(epilog.code);

  std::vector<size_t> offsets = FindOffsets(epilog.code, Epilog_NumOffsets, dummyAddress);

  if (offsets.size() != Epilog_NumOffsets) {
    OutputDebugStringA("Hijk: Did not find expected number of offsets in the Epilog\n");
    return {};
  }

  for (int i = 0; i < Epilog_NumOffsets; ++i) {
    epilog.offsets[i] = offsets[i];
  }
  return epilog;
}

const Prolog* GetProlog() {
  static Prolog prolog = CreateProlog();
  return &prolog;
}

const Epilog* GetEpilog() {
  static Epilog epilog = CreateEpilog();
  return &epilog;
}

void* WritePrologAndEpilogForTargetFunction(void* target_function, void* trampoline, void* buffer,
                                            uint64_t buffer_size) {
  HijkBuffer* relay = static_cast<HijkBuffer*>(buffer);

  const struct Prolog* prolog_code = GetProlog();
  const struct Epilog* epilog_code = GetEpilog();

  size_t prolog_data_size = sizeof(struct PrologData);
  size_t epilog_data_size = sizeof(struct EpilogData);
  size_t prolog_size = prolog_code->size;
  size_t epilog_size = epilog_code->size;

  size_t total_size = prolog_data_size + epilog_data_size + prolog_size + epilog_size;
  if (total_size > buffer_size) {
    std::cout << "Hijk: Not enough space in the relay buffer"; 
    abort();
  }

  char* prolog = relay->code;
  char* epilog = prolog + prolog_size;
  PrologData* prolog_data = &relay->prolog_data;
  EpilogData* epilog_data = &relay->epilog_data;

  void* prolog_stub = &UserPrologStub;
  void* epilog_stub = &UserEpilogStub;

  relay->prolog_data.asm_prolog_stub = &HijkPrologAsmFixed;
  relay->prolog_data.c_prolog_stub = &UserPrologStub;
  relay->prolog_data.asm_epilog_stub = epilog;
  relay->prolog_data.tramploline_to_original_function = trampoline;
  relay->prolog_data.original_function = target_function;
  relay->prolog_data.user_callback = nullptr;  // ct->prologCallback;

  // Create prolog_code
  memcpy(prolog, prolog_code->code, prolog_code->size);
  memcpy(&prolog[prolog_code->offsets[Prolog_Data]], &prolog_data, sizeof(LPVOID));

  relay->epilog_data.asm_epilog_stub = &HijkEpilogAsmFixed;
  relay->epilog_data.c_prolog_stub = &UserEpilogStub;
  relay->epilog_data.original_function = target_function;
  relay->epilog_data.user_callback = nullptr;  // ct->epilogCallback;

  // Create epilog_code
  memcpy(epilog, epilog_code->code, epilog_code->size);
  memcpy(&epilog[epilog_code->offsets[Epilog_EpilogData]], &epilog_data, sizeof(LPVOID));

  return relay->code;
}
