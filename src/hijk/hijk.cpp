#include "hijk/hijk.h"

#include <bit>
#include <iostream>
#include <optional>
#include <span>
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
void HijkEpilogAsm();
void HijkEpilogAsmFixed();
}

namespace {

uint64_t kDummyAddress = 0x0123456789ABCDEF;

extern "C" void OverwriteMinhookRelay(PTRAMPOLINE ct, void* relay_buffer, UINT relay_buffer_size);

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

thread_local std::vector<void*> tls_return_addresses;

std::optional<size_t> Find(std::span<const uint8_t> haystack, std::span<const uint8_t> needle) {
  for (int i = 0; i < (haystack.size() - needle.size()); ++i)
    if (std::memcmp(haystack.data() + i, needle.data(), needle.size()) == 0)
      return static_cast<size_t>(i);
  return std::nullopt;
}

std::optional<size_t> FindCodeSize(const uint8_t* code, size_t max_size = 1024) {
  const std::vector<uint8_t> kEnd({0x49, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F});
  return Find({code, max_size}, kEnd);
}

std::optional<size_t> OffsetOf(const uint8_t* code, size_t size, uint64_t needle) {
  return Find({code, size}, {std::bit_cast<const uint8_t*>(&needle), sizeof(needle)});
}

}  // namespace

void WritePrologAndEpilogForTargetFunction(void* target_function, void* trampoline, void* buffer,
                                           uint64_t buffer_size);

struct MinHookInitializer {
  MinHookInitializer() {
    if (MH_Initialize() != MH_OK) {
      std::cout << "Could not initialize MinHook, aborting";
      abort();
      ;
    }
    MH_SetRelayBufferOverwriteCallback(&OverwriteMinhookRelay);
  }
  ~MinHookInitializer() { MH_Uninitialize(); }
  static void EnsureInitialized() { static MinHookInitializer initializer; }
};

extern "C" {

void DummyFunction() {}

Hijk_PrologCallback g_user_prolog_callback;
Hijk_EpilogCallback g_user_epilog_callback;

bool Hijk_CreateHook(void* target_function, Hijk_PrologCallback prolog_callback,
                     Hijk_EpilogCallback epilog_callback) {
  MinHookInitializer::EnsureInitialized();
  void* original = nullptr;
  g_user_prolog_callback = prolog_callback;
  g_user_epilog_callback = epilog_callback;
  // Minhook internally checks that pDetour is an executable address, use valid dummy pointer.
  return MH_CreateHook(target_function, /*pDetour*/ &DummyFunction, &original) == MH_OK;
}

bool Hijk_EnableHook(void* target_function) {
  MinHookInitializer::EnsureInitialized();
  return MH_EnableHook(target_function) == MH_OK;
}

bool Hijk_DisableHook(void* target_function) {
  MinHookInitializer::EnsureInitialized();
  return MH_DisableHook(target_function) == MH_OK;
}

bool Hijk_RemoveHook(void* target_function) {
  MinHookInitializer::EnsureInitialized();
  return MH_RemoveHook(target_function) == MH_OK;
}

bool Hijk_QueueEnableHook(void* target_function) {
  MinHookInitializer::EnsureInitialized();
  return MH_QueueEnableHook(target_function) == MH_OK;
}

bool Hijk_QueueDisableHook(void* target_function) {
  MinHookInitializer::EnsureInitialized();
  return MH_QueueDisableHook(target_function) == MH_OK;
}
bool Hijk_ApplyQueued() {
  MinHookInitializer::EnsureInitialized();
  return MH_ApplyQueued() == MH_OK;
}

void OverwriteMinhookRelay(PTRAMPOLINE ct, void* relay_buffer, UINT relay_buffer_size) {
  std::cout << "Relay buffer is of size " << relay_buffer_size << std::endl;
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

Prolog CreateProlog() {
  Prolog prolog;
  prolog.code = (uint8_t*)&HijkPrologAsm;
  prolog.size = FindCodeSize(prolog.code).value();
  prolog.address_to_overwrite_offset = OffsetOf(prolog.code, prolog.size, kDummyAddress).value();
  return prolog;
}

Epilog CreateEpilog() {
  Epilog epilog;
  epilog.code = (uint8_t*)&HijkEpilogAsm;
  epilog.size = FindCodeSize(epilog.code).value();
  epilog.address_to_overwrite_offset = OffsetOf(epilog.code, epilog.size, kDummyAddress).value();
  return epilog;
}

void UserPrologStub(Hijk_PrologData* prolog_data, void** address_of_return_address) {
  ContextScope context_scope;
  tls_return_addresses.push_back(*address_of_return_address);
  Hijk_PrologCallback user_callback = static_cast<Hijk_PrologCallback>(prolog_data->user_callback);
  if (user_callback) {
    Hijk_PrologContext context;
    context.prolog_data = prolog_data;
    context.integer_registers = &context_scope.integer_registers_;
    context.xmm_registers = &context_scope.xmm_registers_;
    user_callback(prolog_data->original_function, &context);
  }
}

void UserEpilogStub(Hijk_EpilogData* epilog_data, void** address_of_return_address) {
  ContextScope context_scope;
  void* return_address = tls_return_addresses.back();
  tls_return_addresses.pop_back();
  *address_of_return_address = return_address;
  Hijk_EpilogCallback user_callback = static_cast<Hijk_EpilogCallback>(epilog_data->user_callback);
  if (user_callback) {
    Hijk_EpilogContext context;
    context.epilog_data = epilog_data;
    context.integer_registers = &context_scope.integer_registers_;
    context.xmm_registers = &context_scope.xmm_registers_;
    user_callback(epilog_data->original_function, &context);
  }
}

void WritePrologAndEpilogForTargetFunction(void* target_function, void* trampoline,
                                           void* relay_buffer, uint64_t buffer_size) {
  // Create prolog and epilog data once.
  static const Prolog prolog_code = CreateProlog();
  static const Epilog epilog_code = CreateEpilog();

  // Compute sizes.
  size_t prolog_size = prolog_code.size;
  size_t epilog_size = epilog_code.size;
  size_t prolog_data_size = sizeof(struct Hijk_PrologData);
  size_t epilog_data_size = sizeof(struct Hijk_EpilogData);

  // Check that we have enough space in the relay buffer.
  size_t total_size = prolog_size + epilog_size + prolog_data_size + epilog_data_size;
  if (total_size > buffer_size) {
    std::cout << "Hijk: Not enough space in the relay buffer";
    abort();
    ;
  }
  std::cout << "Overwriting relay buffer with " << total_size << " bytes." << std::endl;

  // Set up pointers into relay_buffer.
  uint8_t* prolog = static_cast<uint8_t*>(relay_buffer);
  uint8_t* epilog = prolog + prolog_size;
  auto* prolog_data = std::bit_cast<Hijk_PrologData*>(epilog + epilog_size);
  auto* epilog_data = std::bit_cast<Hijk_EpilogData*>(epilog + epilog_size + prolog_data_size);

  // Prolog.
  prolog_data->asm_prolog_stub = &HijkPrologAsmFixed;
  prolog_data->c_prolog_stub = &UserPrologStub;
  prolog_data->asm_epilog_stub = epilog;
  prolog_data->tramploline_to_original_function = trampoline;
  prolog_data->original_function = target_function;
  prolog_data->user_callback = g_user_prolog_callback;
  memcpy(prolog, prolog_code.code, prolog_code.size);
  memcpy(&prolog[prolog_code.address_to_overwrite_offset], &prolog_data, sizeof(void*));

  // Epilog.
  epilog_data->asm_epilog_stub = &HijkEpilogAsmFixed;
  epilog_data->c_prolog_stub = &UserEpilogStub;
  epilog_data->original_function = target_function;
  epilog_data->user_callback = g_user_epilog_callback;
  memcpy(epilog, epilog_code.code, epilog_code.size);
  memcpy(&epilog[epilog_code.address_to_overwrite_offset], &epilog_data, sizeof(void*));
}
