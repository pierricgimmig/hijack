#include "OrbitAsm.h"
#include <sstream>
#include <vector>
#include <iostream>
#include "hijk/hijk.h"

OrbitProlog GProlog;
OrbitEpilog GEpilog;

uint64_t g_prolog_count = 0;
uint64_t g_epilog_count = 0;

thread_local std::vector<void *> return_addresses;

struct ContextScope
{
    ContextScope()
    {
        HijkGetIntegerRegisters(&integer_registers_);
        HijkGetXmmRegisters(&xmm_registers_);
    }

    ~ContextScope()
    {
        HijkSetIntegerRegisters(&integer_registers_);
        HijkSetXmmRegisters(&xmm_registers_);
    }

    HijkIntegerRegisters integer_registers_;
    HijkXmmRegisters xmm_registers_;
};

void UserPrologStub(PrologData *prolog_data, void **address_of_return_address)
{
    ContextScope context_scope;
    std::cout << "Prolog!\n";
    return_addresses.push_back(*address_of_return_address);
    ++g_prolog_count;

    PrologueCallback user_callback = static_cast<PrologueCallback>(prolog_data->user_callback);
    user_callback(prolog_data->original_function, nullptr);
}

void UserEpilogStub(EpilogData *epilog_data, void **address_of_return_address)
{
    ContextScope context_scope;

    std::cout << "Epilog!\n";
    void *return_address = return_addresses.back();
    return_addresses.pop_back();
    *address_of_return_address = return_address;
    ++g_epilog_count;
    EpilogueCallback user_callback = static_cast<EpilogueCallback>(epilog_data->user_callback);
    user_callback(epilog_data->original_function, nullptr);
}

std::vector<byte> dummyEnd     = { 0x49, 0xBB, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F };
std::vector<byte> dummyAddress = { 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01 };
#define PRINT_VAR( var )	    PrintVar( #var, var )
#define PRINT_VARN( name, var )	PrintVar( name, var )
#define PRINT_VAR_INL( var )    PrintVar( #var, var, true )

template<class T>
inline void PrintVar( const char* a_VarName, const T& a_Value, bool a_SameLine = false )
{
    std::stringstream l_StringStream;
    l_StringStream << a_VarName << " = " << a_Value;
    if( !a_SameLine ) l_StringStream << std::endl;
    OutputDebugStringA( l_StringStream.str().c_str() );
}

size_t FindSize( const byte* a_Code, size_t a_MaxBytes = 1024 )
{
    size_t matchSize = dummyEnd.size();

    for( size_t i = 0; i < a_MaxBytes; ++i )
    {
        int j = 0;
        for( j = 0; j < matchSize; ++j )
        {
            if( a_Code[i+j] != dummyEnd[j] )
                break;
        }

        if( j == matchSize )
            return i;
    }

    return 0;
}

std::vector<size_t> FindOffsets( const byte* a_Code, size_t a_NumOffsets, const std::vector<byte> & a_Identifier, size_t a_MaxBytes = 1024 )
{
    size_t matchSize = a_Identifier.size();
    std::vector<size_t> offsets;

    for( size_t i = 0; i < a_MaxBytes; ++i )
    {
        int j = 0;
        for( j = 0; j < matchSize; ++j )
        {
            if( a_Code[i + j] != a_Identifier[j] )
                break;
        }

        if( j == matchSize )
        {
            offsets.push_back(i);
            i += matchSize;
            if( offsets.size() == a_NumOffsets )
                break;
        }
    }

    return offsets;
}

void OrbitProlog::Init()
{
    if( m_Data.m_Code == nullptr )
    {
        m_Data.m_Code = (byte*)&HijkPrologAsm;
        m_Data.m_Size = FindSize( m_Data.m_Code );
        PRINT_VARN( "PrologSize", m_Data.m_Size );

        std::vector<size_t> offsets = FindOffsets( m_Data.m_Code, Prolog_NumOffsets, dummyAddress );

        if( offsets.size() != Prolog_NumOffsets )
        {
            OutputDebugStringA( "OrbitAsm: Did not find expected number of offsets in the Prolog\n" );
            return;
        }

        for( size_t i = 0; i < offsets.size(); ++i )
        {
            m_Data.m_Offsets[i] = offsets[i];
        }
    }
}

void OrbitEpilog::Init()
{
    if( !m_Data.m_Code )
    {
        m_Data.m_Code = (byte*)&HijkEpilogAsm;
        m_Data.m_Size = FindSize( m_Data.m_Code );
        PRINT_VARN( "EpilogSize", m_Data.m_Size );

        std::vector<size_t> offsets = FindOffsets( m_Data.m_Code, Epilog_NumOffsets, dummyAddress );

        if( offsets.size() != Epilog_NumOffsets )
        {
            OutputDebugStringA( "OrbitAsm: Did not find expected number of offsets in the Epilog\n" );
            return;
        }

        for( int i = 0; i < Epilog_NumOffsets; ++i )
        {
            m_Data.m_Offsets[i] = offsets[i];
        }
    }
}

const Prolog* GetOrbitProlog()
{
    if( !GProlog.m_Data.m_Code )
        GProlog.Init();
    return &GProlog.m_Data;
}

const Epilog* GetOrbitEpilog()
{
    if( !GEpilog.m_Data.m_Code )
        GEpilog.Init();
    return &GEpilog.m_Data;
}

void *GetOrbitPrologStubAddress()
{
    return &UserPrologStub;
}
void *GetOrbitEpilogStubAddress()
{
    return &UserEpilogStub;
}

void *GetHijkPrologAsmStubAddress()
{
    return &HijkPrologAsmFixed;
}
void *GetHijkEpilogAsmStubAddress()
{
    return &HijkEpilogAsmFixed;
}

