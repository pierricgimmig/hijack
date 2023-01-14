#include "OrbitAsm.h"

#include <Windows.h>

#include <iostream>
#include <sstream>
#include <vector>

#include "hijk/hijk.h"
#include "trampoline.h"

uint64_t g_prolog_count = 0;
uint64_t g_epilog_count = 0;

thread_local std::vector<void*> return_addresses;

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

Prolog GProlog;
Epilog GEpilog;
const struct Prolog* GetOrbitProlog();
const struct Epilog* GetOrbitEpilog();
void* GetOrbitPrologStubAddress();
void* GetOrbitEpilogStubAddress();
void* GetHijkPrologAsmStubAddress();
void* GetHijkEpilogAsmStubAddress();

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

void UserPrologStub(PrologData* prolog_data, void** address_of_return_address) {
  ContextScope context_scope;
  std::cout << "Prolog!\n";
  return_addresses.push_back(*address_of_return_address);
  ++g_prolog_count;

  PrologueCallback user_callback = static_cast<PrologueCallback>(prolog_data->user_callback);
  if (user_callback) user_callback(prolog_data->original_function, nullptr);
}

void UserEpilogStub(EpilogData* epilog_data, void** address_of_return_address) {
  ContextScope context_scope;

  std::cout << "Epilog!\n";
  void* return_address = return_addresses.back();
  return_addresses.pop_back();
  *address_of_return_address = return_address;
  ++g_epilog_count;
  EpilogueCallback user_callback = static_cast<EpilogueCallback>(epilog_data->user_callback);
  if (user_callback) user_callback(epilog_data->original_function, nullptr);
}

std::vector<byte> dummyEnd = {0x49, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F};
std::vector<byte> dummyAddress = {0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01};
#define PRINT_VAR(var) PrintVar(#var, var)
#define PRINT_VARN(name, var) PrintVar(name, var)
#define PRINT_VAR_INL(var) PrintVar(#var, var, true)

template <class T>
inline void PrintVar(const char* a_VarName, const T& a_Value, bool a_SameLine = false) {
  std::stringstream l_StringStream;
  l_StringStream << a_VarName << " = " << a_Value;
  if (!a_SameLine) l_StringStream << std::endl;
  OutputDebugStringA(l_StringStream.str().c_str());
}

size_t FindSize(const byte* a_Code, size_t a_MaxBytes = 1024) {
  size_t matchSize = dummyEnd.size();

  for (size_t i = 0; i < a_MaxBytes; ++i) {
    int j = 0;
    for (j = 0; j < matchSize; ++j) {
      if (a_Code[i + j] != dummyEnd[j]) break;
    }

    if (j == matchSize) return i;
  }

  return 0;
}

std::vector<size_t> FindOffsets(const byte* a_Code, size_t a_NumOffsets,
                                const std::vector<byte>& a_Identifier, size_t a_MaxBytes = 1024) {
  size_t matchSize = a_Identifier.size();
  std::vector<size_t> offsets;

  for (size_t i = 0; i < a_MaxBytes; ++i) {
    int j = 0;
    for (j = 0; j < matchSize; ++j) {
      if (a_Code[i + j] != a_Identifier[j]) break;
    }

    if (j == matchSize) {
      offsets.push_back(i);
      i += matchSize;
      if (offsets.size() == a_NumOffsets) break;
    }
  }

  return offsets;
}

Prolog CreateProlog() {
  Prolog prolog;
  prolog.code = (byte*)&HijkPrologAsm;
  prolog.size = FindSize(prolog.code);
  PRINT_VARN("PrologSize", prolog.size);

  std::vector<size_t> offsets = FindOffsets(prolog.code, Prolog_NumOffsets, dummyAddress);

  if (offsets.size() != Prolog_NumOffsets) {
    OutputDebugStringA("OrbitAsm: Did not find expected number of offsets in the Prolog\n");
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
  PRINT_VARN("EpilogSize", epilog.size);

  std::vector<size_t> offsets = FindOffsets(epilog.code, Epilog_NumOffsets, dummyAddress);

  if (offsets.size() != Epilog_NumOffsets) {
    OutputDebugStringA("OrbitAsm: Did not find expected number of offsets in the Epilog\n");
    return {};
  }

  for (int i = 0; i < Epilog_NumOffsets; ++i) {
    epilog.offsets[i] = offsets[i];
  }
  return epilog;
}

const Prolog* GetOrbitProlog() {
  static Prolog prolog = CreateProlog();
  return &prolog;
}

const Epilog* GetOrbitEpilog() {
  static Epilog epilog = CreateEpilog();
  return &epilog;
}

void* GetOrbitPrologStubAddress() { return &UserPrologStub; }
void* GetOrbitEpilogStubAddress() { return &UserEpilogStub; }

void* GetHijkPrologAsmStubAddress() { return &HijkPrologAsmFixed; }
void* GetHijkEpilogAsmStubAddress() { return &HijkEpilogAsmFixed; }

void* WritePrologAndEpilogForTargetFunction(void* target_function, void* trampoline, void* buffer,
                                            uint64_t buffer_size) {
  HijkBuffer* relay = static_cast<HijkBuffer*>(buffer);

  const struct Prolog* orbitProlog = GetOrbitProlog();
  const struct Epilog* orbitEpilog = GetOrbitEpilog();

  size_t prolog_data_size = sizeof(struct PrologData);
  size_t epilog_data_size = sizeof(struct EpilogData);
  size_t prolog_size = orbitProlog->size;
  size_t epilog_size = orbitEpilog->size;

  size_t total_size = prolog_data_size + epilog_data_size + prolog_size + epilog_size;
  if (total_size > buffer_size) {
    abort();
  }

  char* prolog = relay->code;
  char* epilog = prolog + prolog_size;
  PrologData* prolog_data = &relay->prolog_data;
  EpilogData* epilog_data = &relay->epilog_data;

  void* prolog_stub = GetOrbitPrologStubAddress();
  void* epilog_stub = GetOrbitEpilogStubAddress();

  relay->prolog_data.asm_prolog_stub = GetHijkPrologAsmStubAddress();
  relay->prolog_data.c_prolog_stub = GetOrbitPrologStubAddress();
  relay->prolog_data.asm_epilog_stub = epilog;
  relay->prolog_data.tramploline_to_original_function = trampoline;
  relay->prolog_data.original_function = target_function;
  relay->prolog_data.user_callback = nullptr;  // ct->prologCallback;

  // Create OrbitProlog
  memcpy(prolog, orbitProlog->code, orbitProlog->size);
  memcpy(&prolog[orbitProlog->offsets[Prolog_Data]], &prolog_data, sizeof(LPVOID));

  relay->epilog_data.asm_epilog_stub = GetHijkEpilogAsmStubAddress();
  relay->epilog_data.c_prolog_stub = GetOrbitEpilogStubAddress();
  relay->epilog_data.original_function = target_function;
  relay->epilog_data.user_callback = nullptr;  // ct->epilogCallback;

  // Create OrbitEpilog
  memcpy(epilog, orbitEpilog->code, orbitEpilog->size);
  memcpy(&epilog[orbitEpilog->offsets[Epilog_EpilogData]], &epilog_data, sizeof(LPVOID));

  return relay->code;
}
