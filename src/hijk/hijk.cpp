#include "hijk/hijk.h"

#include <assert.h>

#include <array>
#include <iostream>
#include <sstream>
#include <vector>

#include "MinHook.h"
#include "trampoline.h"

// hijk.asm functions
extern "C" {
void HijkGetXmmRegisters(struct Hijk_XmmRegisters* a_Context);
void HijkSetXmmRegisters(struct Hijk_XmmRegisters* a_Context);
void HijkGetIntegerRegisters(void* a_Context);
void HijkSetIntegerRegisters(void* a_Context);
void HijkPrologAsm();
void HijkPrologAsmFixed();
void HijkEpilogAsmFixed();
void HijkEpilogAsm();
}

namespace {
static const std::vector<uint8_t> kDummyFunctionEnd = {0x49, 0xBB, 0xFF, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF, 0x0F};

extern "C" void OnTrampolineCreated(PTRAMPOLINE ct, void* relay_buffer, UINT relay_buffer_size);
void WritePrologAndEpilogForTargetFunction(void* target_function, void* trampoline, void* buffer,
                                           uint64_t buffer_size);

struct ContextScope {
  ContextScope() {
    HijkGetIntegerRegisters(&integer_registers_);
    HijkGetXmmRegisters(&xmm_registers_);
  }

  ~ContextScope() {
    HijkSetIntegerRegisters(&integer_registers_);
    HijkSetXmmRegisters(&xmm_registers_);
  }

  Hijk_IntegerRegisters integer_registers_;
  alignas(16) Hijk_XmmRegisters xmm_registers_;
};

thread_local std::vector<void*> return_addresses;
}  // namespace

struct MinHookInitializer {
  MinHookInitializer() {
    if (MH_Initialize() != MH_OK) {
      std::cout << "Could not initialize MinHook, aborting";
      abort();
    }
    SetTrampolineOverrideCallback(&OnTrampolineCreated);
  }
  ~MinHookInitializer() { MH_Uninitialize(); }
  static void EnsureInitialized() { static MinHookInitializer initializer; }
};

extern "C" {

    void dummy_function() {

}

bool Hijk_CreateHook(void* target_function, Hijk_PrologueCallback prologue_callback,
                     Hijk_EpilogueCallback epilogue_callback) {
  MinHookInitializer::EnsureInitialized();
  void* original = nullptr;
  return MH_CreateHook(target_function, &dummy_function, &original) == MH_OK;
}

bool Hijk_RemoveHook(void* target_function) {
  assert(0);
  MinHookInitializer::EnsureInitialized();
  return true;
}

bool Hijk_RemoveAllHooks() {
  assert(0);
  MinHookInitializer::EnsureInitialized();
  return true;
}

bool Hijk_EnableHook(void* target_function) {
  MinHookInitializer::EnsureInitialized();
  return MH_EnableHook(target_function) == MH_OK;
}

bool Hijk_EnableAllHooks() {
  assert(0);
  MinHookInitializer::EnsureInitialized();
  return true;
}

bool Hijk_DisableHook(void* target_function) {
  assert(0);
  MinHookInitializer::EnsureInitialized();
  return true;
}
bool Hijk_DisableAllHooks() {
  assert(0);
  MinHookInitializer::EnsureInitialized();
  return true;
}

void OnTrampolineCreated(PTRAMPOLINE ct, void* relay_buffer, UINT relay_buffer_size) {
  std::cout << "Trampoline created with " << relay_buffer_size << " free space";
  WritePrologAndEpilogForTargetFunction(ct->pTarget, ct->pTrampoline, relay_buffer,
                                        relay_buffer_size);
}

}  // extern "C" {

struct Prolog {
  uint8_t* code;
  size_t size;
  size_t address_to_overwrite_offset;
};

struct Epilog {
  uint8_t* code;
  uint32_t size;
  size_t address_to_overwrite_offset;
};

size_t FindCodeSize(const uint8_t* code, size_t max_uint8_ts = 1024) {
  size_t matchSize = kDummyFunctionEnd.size();
  for (size_t i = 0; i < max_uint8_ts; ++i) {
    size_t j = 0;
    for (j = 0; j < matchSize; ++j) {
      if (code[i + j] != kDummyFunctionEnd[j]) break;
    }

    if (j == matchSize) return i;
  }

  return 0;
}

std::vector<size_t> FindOffsets(const uint8_t* code, size_t num_offsets,
                                const std::vector<uint8_t>& identifier,
                                size_t max_uint8_ts = 1024) {
  size_t matchSize = identifier.size();
  std::vector<size_t> offsets;

  for (size_t i = 0; i < max_uint8_ts; ++i) {
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

size_t FindAbsoluteAddressToOverwrite(const uint8_t* code, size_t size) {
  uint64_t kDummyAddress = 0x0123456789ABCDEF;
  for (size_t i = 0; i < size - sizeof(kDummyAddress); ++i) {
    if (*reinterpret_cast<const uint64_t*>(code + i) == kDummyAddress) {
      return i;
    }
  }
  abort();
  return 0;
}

Prolog CreateProlog() {
  Prolog prolog;
  prolog.code = (uint8_t*)&HijkPrologAsm;
  prolog.size = FindCodeSize(prolog.code);
  prolog.address_to_overwrite_offset = FindAbsoluteAddressToOverwrite(prolog.code, prolog.size);
  return prolog;
}

Epilog CreateEpilog() {
  Epilog epilog;
  epilog.code = (uint8_t*)&HijkEpilogAsm;
  epilog.size = FindCodeSize(epilog.code);
  epilog.address_to_overwrite_offset = FindAbsoluteAddressToOverwrite(epilog.code, epilog.size);
  return epilog;
}

void UserPrologStub(Hijk_PrologData* prolog_data, void** address_of_return_address) {
  ContextScope context_scope;
  std::cout << "Prolog!\n";
  return_addresses.push_back(*address_of_return_address);
  Hijk_PrologueCallback user_callback =
      static_cast<Hijk_PrologueCallback>(prolog_data->user_callback);
  if (user_callback) user_callback(prolog_data->original_function, nullptr);
}

void UserEpilogStub(Hijk_EpilogData* epilog_data, void** address_of_return_address) {
  ContextScope context_scope;
  std::cout << "Epilog!\n";
  void* return_address = return_addresses.back();
  return_addresses.pop_back();
  *address_of_return_address = return_address;
  Hijk_EpilogueCallback user_callback =
      static_cast<Hijk_EpilogueCallback>(epilog_data->user_callback);
  if (user_callback) user_callback(epilog_data->original_function, nullptr);
}

namespace {
void WritePrologAndEpilogForTargetFunction(void* target_function, void* trampoline, void* buffer,
                                           uint64_t buffer_size) {
  static const Prolog prolog_code = CreateProlog();
  static const Epilog epilog_code = CreateEpilog();

  size_t prolog_data_size = sizeof(struct Hijk_PrologData);
  size_t epilog_data_size = sizeof(struct Hijk_EpilogData);
  size_t prolog_size = prolog_code.size;
  size_t epilog_size = epilog_code.size;

  size_t total_size = prolog_data_size + epilog_data_size + prolog_size + epilog_size;
  if (total_size > buffer_size) {
    std::cout << "Hijk: Not enough space in the relay buffer";
    abort();
  }

  uint8_t* prolog = static_cast<uint8_t*>(buffer);
  uint8_t* epilog = prolog + prolog_size;
  Hijk_PrologData* prolog_data = reinterpret_cast<Hijk_PrologData*>(epilog + epilog_size);
  Hijk_EpilogData* epilog_data =
      reinterpret_cast<Hijk_EpilogData*>(epilog + epilog_size + prolog_data_size);

  void* prolog_stub = &UserPrologStub;
  void* epilog_stub = &UserEpilogStub;

  size_t prolog_size_ = (uint8_t*)&HijkPrologAsmFixed - (uint8_t*)&HijkPrologAsm;
  std::cout << "prolgo size : " << prolog_size_ << std::endl;

  prolog_data->asm_prolog_stub = &HijkPrologAsmFixed;
  prolog_data->c_prolog_stub = &UserPrologStub;
  prolog_data->asm_epilog_stub = epilog;
  prolog_data->tramploline_to_original_function = trampoline;
  prolog_data->original_function = target_function;
  prolog_data->user_callback = nullptr;  // ct->prologCallback;

  // Create prolog_code
  memcpy(prolog, prolog_code.code, prolog_code.size);
  memcpy(&prolog[prolog_code.address_to_overwrite_offset], &prolog_data, sizeof(void*));

  epilog_data->asm_epilog_stub = &HijkEpilogAsmFixed;
  epilog_data->c_prolog_stub = &UserEpilogStub;
  epilog_data->original_function = target_function;
  epilog_data->user_callback = nullptr;  // ct->epilogCallback;

  // Create epilog_code
  memcpy(epilog, epilog_code.code, epilog_code.size);
  memcpy(&epilog[epilog_code.address_to_overwrite_offset], &epilog_data, sizeof(void*));
}
}  // namespace