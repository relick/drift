#pragma once

#include "common/Bit.h"

#include <array>
#ifdef _WIN32
// Windows Intrinsics
#include <immintrin.h>
#else
// GCC Intrinsics
#include <cpuid.h>
#include <xsaveintrin.h>
#define _XCR_XFEATURE_ENABLED_MASK 0
#endif

namespace Setup
{

class CpuInfo
{
    std::array<int, 4> m_info{};
#ifdef _WIN32
    // Windows
    void Extract( int _infoType )
    {
        __cpuidex( m_info.data(), _infoType, 0 );
    }
#else
    // GCC
    void Extract( int _infoType )
    {
        __cpuid_count( _infoType, 0, m_info[ 0 ], m_info[ 1 ], m_info[ 2 ], m_info[ 3 ] );
    }
#endif

    // SIMD 16 byte
    bool CPU_SSE{};
    bool CPU_SSE2{};
    bool CPU_SSE3{};
    bool CPU_SSSE3{};
    bool CPU_SSE41{};
    bool CPU_SSE42{};
    bool CPU_SSE4a{};
    bool CPU_AES{};
    bool CPU_SHA{};

    // SIMD 32 byte
    bool CPU_AVX{};
    bool CPU_XOP{};
    bool CPU_FMA3{};
    bool CPU_FMA4{};
    bool CPU_AVX2{};

    // OS
    bool OS_XSAVE_XRSTORE{};
    bool OS_AVX{};

public:
    CpuInfo()
    {
        Extract( 0 );
        int nIDs = m_info[ 0 ];

        Extract( 0x8000'0000 );
        unsigned nExIDs = m_info[ 0 ];

        if ( nIDs >= 0x1 )
        {
            Extract( 0x1 );
            CPU_SSE = ( m_info[ 3 ] & BIT( 25 ) ) != 0;
            CPU_SSE2 = ( m_info[ 3 ] & BIT( 26 ) ) != 0;
            CPU_SSE3 = ( m_info[ 2 ] & BIT( 0 ) ) != 0;
            CPU_SSSE3 = ( m_info[ 2 ] & BIT( 9 ) ) != 0;
            CPU_SSE41 = ( m_info[ 2 ] & BIT( 19 ) ) != 0;
            CPU_SSE42 = ( m_info[ 2 ] & BIT( 20 ) ) != 0;
            CPU_AES = ( m_info[ 2 ] & BIT( 25 ) ) != 0;

            CPU_AVX = ( m_info[ 2 ] & BIT( 28 ) ) != 0;
            CPU_FMA3 = ( m_info[ 2 ] & BIT( 12 ) ) != 0;

            OS_XSAVE_XRSTORE = ( m_info[ 2 ] & BIT( 27 ) ) != 0;

            if ( CPU_AVX && OS_XSAVE_XRSTORE )
            {
                unsigned long long xcrFeatureMask = _xgetbv( _XCR_XFEATURE_ENABLED_MASK );
                OS_AVX = ( xcrFeatureMask & 0x6 ) == 0x6;
            }
        }

        if ( nIDs >= 0x7 )
        {
            Extract( 0x7 );
            CPU_SHA = ( m_info[ 1 ] & BIT( 29 ) ) != 0;

            CPU_AVX2 = ( m_info[ 1 ] & BIT( 5 ) ) != 0;
        }

        if ( nExIDs >= 0x8000'0001 )
        {
            Extract( 0x8000'0001 );
            CPU_SSE4a = ( m_info[ 2 ] & BIT( 6 ) ) != 0;

            CPU_XOP = ( m_info[ 2 ] & BIT( 11 ) ) != 0;
            CPU_FMA4 = ( m_info[ 2 ] & BIT( 16 ) ) != 0;
        }
    }

    // Feature tests, added ad-hoc

    bool AVX() const
    {
        return CPU_AVX && OS_AVX;
    }

    // etc.
};

}