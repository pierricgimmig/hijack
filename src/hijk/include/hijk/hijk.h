#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HIJK_API __declspec(dllexport)
typedef void (*PrologueCallback)(void* target_function, struct PrologueContext* prologue_context);
typedef void (*EpilogueCallback)(void* target_function, struct EpilogueContext* epilogue_context);

// Dynamically instrument `target_function` and call prologue/epilogue callbacks on entry/exit.
HIJK_API bool Hijk_CreateHook(void* target_function, PrologueCallback prologue_callback,
                              EpilogueCallback epilogue_callback);

HIJK_API bool Hijk_RemoveHook(void* target_function);
HIJK_API bool Hijk_RemoveAllHooks();

HIJK_API bool Hijk_EnableHook(void* target_function);
HIJK_API bool Hijk_EnableAllHooks();

HIJK_API bool Hijk_DisableHook(void* target_function);
HIJK_API bool Hijk_DisableAllHooks();

struct PrologData {
  void* asm_prolog_stub;
  void* c_prolog_stub;
  void* asm_epilog_stub;
  void* tramploline_to_original_function;
  void* original_function;
  void* user_callback;
};

struct EpilogData {
  void* asm_epilog_stub;
  void* c_prolog_stub;
  void* original_function;
  void* user_callback;
};

#pragma pack(push, 1)
struct HijkIntegerRegisters {
  uint64_t regs[16];
};

struct HijkXmm {
  float data[4];
};

struct HijkXmmRegisters {
  struct HijkXmm xmm0;
  struct HijkXmm xmm1;
  struct HijkXmm xmm2;
  struct HijkXmm xmm3;
  struct HijkXmm xmm4;
  struct HijkXmm xmm5;
  struct HijkXmm xmm6;
  struct HijkXmm xmm7;
  struct HijkXmm xmm8;
  struct HijkXmm xmm9;
  struct HijkXmm xmm10;
  struct HijkXmm xmm11;
  struct HijkXmm xmm12;
  struct HijkXmm xmm13;
  struct HijkXmm xmm14;
  struct HijkXmm xmm15;
};
#pragma pack(pop)

struct PrologueContext {
  PrologData* prolog_data;
  HijkIntegerRegisters integer_registers;
  HijkXmmRegisters xmm_registers;
};

struct EpilogueContext {
  EpilogData* epilog_data;
  HijkIntegerRegisters integer_registers;
  HijkXmmRegisters xmm_registers;
};

struct HijkBuffer {
  struct PrologData prolog_data;
  struct EpilogData epilog_data;
  char code[64];
  uint32_t prolog_code_size;
  uint32_t epilog_code_size;
  uint32_t buffer_size;
};

#ifdef __cplusplus
}
#endif
