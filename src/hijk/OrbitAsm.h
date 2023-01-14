#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct PrologData
{
    void *asm_prolog_stub;
    void *c_prolog_stub;
    void *asm_epilog_stub;
    void *tramploline_to_original_function;
    void *original_function;
    void *user_callback;
};

struct EpilogData
{
    void *asm_epilog_stub;
    void *c_prolog_stub;
    void *original_function;
    void *user_callback;
};

struct HijkBuffer {
    struct PrologData prolog_data;
    struct EpilogData epilog_data;
    char code[64];
    uint32_t prolog_code_size;
    uint32_t epilog_code_size;
    uint32_t buffer_size;
};

void* WritePrologAndEpilogForTargetFunction(void* target_function, void* trampoline, void* buffer, uint64_t buffer_size);

#pragma pack(push, 1)
struct HijkIntegerRegisters
{
    uint64_t regs[16];
};

struct HijkXmm {
    float data[4];
};

struct HijkXmmRegisters
{
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

#ifdef __cplusplus
}
#endif