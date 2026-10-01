// Shared geometry-instance data prefix; this does not define the complete interface.
// [PLATFORM] x86 rendering retains the VC7.1 string layout, including in native-host builds.
// [CHOSEN, not recovered] The 64-bit port uses native string and pointer sizes;
// future native geometry owners must use this prefix. No x64 renderer is supplied here.
#ifndef BF2_GEOM_GEOMETRYINSTANCEDATA_H
#define BF2_GEOM_GEOMETRYINSTANCEDATA_H

#include <cstddef>
#if defined(_MSC_VER) && !defined(_WIN64)
#include "../../RendDX9Abi/Vc71String.h"
#else
#include <string>
#endif

namespace dice { namespace hfe { namespace geom {

struct GeometryInstanceData
{
    explicit GeometryInstanceData(void* const* table)
        : m_vptr(table), m_dword20(0), m_dword24(0), m_float28(1.0f), m_float2C(1.0f) {}

    void* const* m_vptr;
#if defined(_MSC_VER) && !defined(_WIN64)
    vc71::string m_name;
#else
    std::string m_name;
#endif
    union { unsigned int m_dword20; unsigned int m_drawFrame; };
    union { unsigned int m_dword24; unsigned int m_visibleFrame; };
    float m_float28; // base opacity (Armor's destruction fade)
    float m_float2C; // distance/cull opacity

    // Isolated logical slots: PE +F4 = ordinal61, ELF +F8 = ordinal62.
    // RendDX9 0x10103C40 stores the float in the Ra transparency parameter (ret 4).
    // No x86 fastcall dummy register argument may be carried into the native x64 call.
    void setOpacity(float value)
    {
#ifdef _MSC_VER
        typedef void (__thiscall* Function)(void*, float);
        reinterpret_cast<Function>(m_vptr[61])(this, value);
#else
        typedef void (*Function)(void*, float);
        reinterpret_cast<Function>(m_vptr[62])(this, value);
#endif
    }
};

#if defined(_MSC_VER) && !defined(_WIN64)
static_assert(offsetof(GeometryInstanceData, m_float28) == 0x28, "retail geometry opacity");
static_assert(offsetof(GeometryInstanceData, m_float2C) == 0x2C, "retail geometry cull fade");
static_assert(sizeof(GeometryInstanceData) == 0x30, "retail geometry data prefix");
#else
static_assert(offsetof(GeometryInstanceData, m_float28) ==
    sizeof(void*) + sizeof(std::string) + 2 * sizeof(unsigned int), "native geometry data prefix");
#endif

} } }
#endif
