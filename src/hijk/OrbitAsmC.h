#pragma once

//-----------------------------------------------------------------------------
enum OrbitPrologOffset
{
    Prolog_Data = 0,
    Prolog_NumOffsets
};

//-----------------------------------------------------------------------------
struct Prolog
{
    byte *m_Code;
    size_t m_Size;
    size_t m_Offsets[Prolog_NumOffsets];
};

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

//-----------------------------------------------------------------------------
enum OrbitEpilogOffset
{
    Epilog_EpilogData = 0,
    Epilog_NumOffsets
};

//-----------------------------------------------------------------------------
struct Epilog
{
    byte *m_Code;
    size_t m_Size;
    size_t m_Offsets[Epilog_NumOffsets];
};

//-----------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"
{
#endif
    const struct Prolog *GetOrbitProlog();
    const struct Epilog *GetOrbitEpilog();
    void *GetOrbitPrologStubAddress();
    void *GetOrbitEpilogStubAddress();
    void *GetOrbitPrologAsmStubAddress();
    void *GetOrbitEpilogAsmStubAddress();
#ifdef __cplusplus
}
#endif