// Owns shader-parameter handles, types, dimensions, and value application.
// ShaderParameter wrappers refer to these owners, allowing names before effects exist.

#ifdef BF2_IMAGE_RENDDX9

#include "../../RendDX9Abi/HfeAbi.h"
#include "../../RendDX9Abi/ModuleCommon.h"
#include "../../RendDX9Abi/BfMemory.h"
#include "../../RendDX9Abi/D3dxEffect.h"
#include "../../RendDX9Abi/Frontier.h"
#include "ShaderParameter.h"

#include <new>

namespace dice {
namespace hfe {
namespace dx9rend {

// StateManager.cpp - the sparse-matrix arm's bone-palette upload (SM vslot +0x38).
void stateManagerSetVertexShaderConstantF(unsigned int reg, const float* data,
                                          unsigned int count);


// D3DXPARAMETER_DESC - 44 bytes. Declared here because the modern SDK ships no D3DX9 headers.

// The field order is confirmed against setD3DXParam's own stack slots: the decompiler's locals run
// -0x2C, -0x28, -0x24, -0x20, -0x1C, -0x18, -0x14, -0x10 and then jumps to -0x04, and that gap is
// exactly StructMembers and Flags going unread. The value at -0x04 is Bytes.

struct D3dxParameterDesc
{
    const char*  name;           // +0x00
    const char*  semantic;       // +0x04
    unsigned int paramClass;     // +0x08  D3DXPARAMETER_CLASS
    unsigned int type;           // +0x0C  D3DXPARAMETER_TYPE
    unsigned int rows;           // +0x10
    unsigned int columns;        // +0x14
    unsigned int elements;       // +0x18
    unsigned int annotations;    // +0x1C
    unsigned int structMembers;  // +0x20  not read by setD3DXParam
    unsigned int flags;          // +0x24  not read by setD3DXParam
    unsigned int bytes;          // +0x28
};

static_assert(sizeof(D3dxParameterDesc) == 44, "D3DXPARAMETER_DESC is 44 bytes");


// Effect-owned reflected parameter with semantic, type and array metadata.
// The effect pointer is borrowed; setD3DXParam initializes the handle and description.

class FXParameterImpl final : public ParameterOwner
{
public:
    FXParameterImpl(void* effect, void* d3dxHandle);

    int  getRef() const { return m_refCount; }
    int  addRef() { return ++m_refCount; }
    int  release();
    void* getD3DXParam() const { return m_handle; }          // +0x0C,
    void setD3DXParam(void* handle);
    bool isParameterUsed() const;                            // +0x14,
    void setValue(const void* data, unsigned int count);
    void setBool(unsigned char value);
    void setInt(int value);
    void setFloat(float value);                            // +0x24,
    void setMatrixPointerArray(const void* const* matrices,
                               unsigned int count);         // +0x28,
    unsigned int getTypeCode() const { return m_typeCode; } // +0x2C,

private:
    int           m_refCount;     // +0x04
    void*         m_effect;       // +0x08
    void*         m_handle;       // +0x0C
    unsigned int  m_typeCode;     // +0x10
    vc71::string  m_semantic;     // +0x14 .. +0x2F
    unsigned int  m_bytes;        // +0x30
    unsigned int  m_bytesPerElem; // +0x34
    unsigned int  m_elements;     // +0x38
    unsigned int  m_rows;         // +0x3C
    unsigned int  m_columns;      // +0x40
    unsigned char m_sparseArray;  // +0x44
    unsigned char m_pad45[3];
    unsigned int  m_arrayStart;   // +0x48
};

static_assert(sizeof(FXParameterImpl) == 0x4C,
              "sub_1001E440 allocates 0x4C for one - see 0x1001E4A9");

// Effect.cpp - the owning effect's compiled ID3DXEffect, or null before bind.
void* effectD3dxHandle(void* effect);
bool effectIsParameterUsed(void* effect, void* parameter);

namespace {

void logUnknownDatatype(int line)
{
    ::renddx9abi::logf("FXParameter.cpp:%d Unknown datatype in FXParameterImpl::setD3DXParam()",
                       line);
}

} // anonymous namespace

// sub_100219A0, read in full.
FXParameterImpl::FXParameterImpl(void* effect, void* d3dxHandle)
    : m_refCount(1)
    , m_effect(effect)
    , m_handle(0)
    , m_typeCode(0x80000000u)   // the constructor's sentinel; setD3DXParam replaces it
    , m_bytes(0)
    , m_bytesPerElem(0)
    , m_elements(0)
    , m_rows(0)
    , m_columns(0)
    , m_sparseArray(0)
    , m_arrayStart(0)
{
    m_pad45[0] = 0; m_pad45[1] = 0; m_pad45[2] = 0;
    setD3DXParam(d3dxHandle);
}


// Releases this parameter and its semantic string at zero references.
// The effect pointer is borrowed; the effect owns the parameter, not vice versa.
int FXParameterImpl::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
    {
        m_semantic.~string();
        ::renddx9abi::engineFree(this);
    }
    return remaining;
}

// +0x14, 0x100215C0. A null D3DX parameter is never used; otherwise the Effect checks it against
// the current technique (or answers true when no technique is selected).
bool FXParameterImpl::isParameterUsed() const
{
    return m_handle && effectIsParameterUsed(m_effect, m_handle);
}

void FXParameterImpl::setD3DXParam(void* handle)
{
    m_handle = handle;
    if (!handle)
        return;

    void* fx = effectD3dxHandle(m_effect);
    if (!fx)
        return;     // no compiled effect yet - the constructor's defaults stand

    D3dxParameterDesc desc;
    for (unsigned i = 0; i < sizeof(desc) / 4; ++i)
        reinterpret_cast<unsigned int*>(&desc)[i] = 0;

    ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxGetParameterDescFn>(fx, 0x10)(fx, handle, &desc);

    m_elements    = desc.elements;
    m_rows        = desc.rows;
    m_columns     = desc.columns;
    m_bytes       = desc.bytes;
    m_bytesPerElem = desc.bytes / (desc.elements ? desc.elements : 1);   // clamped, not checked

    m_semantic    = desc.semantic ? desc.semantic : "";
    m_sparseArray = 0;
    m_arrayStart  = 0;

    // Both annotations must be present; the original tests them together and reads neither unless
    // both are found.
    if (desc.annotations == 2)
    {
        void* sparse = ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxGetAnnotationByNameFn>(fx, 0x4C)(
            fx, handle, "sparseArray");
        void* start  = ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxGetAnnotationByNameFn>(fx, 0x4C)(
            fx, handle, "arrayStart");
        if (sparse && start)
        {
            int flag = 0;
            ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxGetBoolFn>(fx, 0x5C)(fx, sparse, &flag);
            m_sparseArray = flag ? 1 : 0;

            int base = 0;
            ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxGetIntFn>(fx, 0x6C)(fx, start, &base);
            m_arrayStart = static_cast<unsigned int>(base);
        }
    }

    // The class/type switch. Codes are the original's literals.
    switch (desc.paramClass)
    {
    case 0:  // scalar
        m_typeCode = 256;
        if (desc.type == 1)      m_typeCode = 260;   // bool
        else if (desc.type == 2) m_typeCode = 257;   // int
        else if (desc.type == 3) m_typeCode = 258;   // float
        else logUnknownDatatype(93);
        break;
    case 1:  // vector
        m_typeCode = 512;
        if (desc.type == 2)      m_typeCode = 513;
        else if (desc.type == 3) m_typeCode = 514;
        else logUnknownDatatype(114);
        break;
    case 3:  // MATRIX_COLUMNS - falls through to the MATRIX_ROWS arm after complaining
        ::renddx9abi::logf("FXParameter.cpp:121 D3DXPC_MATRIX_COLUMNS: Explicitly support this?");
        // fall through
    case 2:  // MATRIX_ROWS
        m_typeCode = 1024;
        if (desc.type == 2)      m_typeCode = 1025;
        else if (desc.type == 3) m_typeCode = 1026;
        else logUnknownDatatype(137);
        break;
    case 4:  // object - only one type is recognised, and the code is left unset otherwise
        if (desc.type == 5)
            m_typeCode = 2056;
        break;
    case 5:  // struct
        m_typeCode = 0x10000;
        break;
    default:
        ::renddx9abi::logf("FXParameter.cpp:150 Unknown dataclass in "
                           "FXParameterImpl::setD3DXParam()");
        break;
    }
}

namespace {

// unk_1023C6D0 - the sparse-matrix staging buffer, file-scope in the shipped image. Its extent is
// [INFERRED] from the .bss gap to the next named global (0x1023D08C), 2492 bytes = 51 matrices of
// 12 floats; nothing recovered establishes the declared size.
float s_sparseMatrixStaging[624];

} // anonymous namespace


// Uploads data according to reflected type; zero count uses the parameter's element count.
// Sparse matrices are transposed into three float4 rows per matrix for direct register upload.
// Requires a compiled owner effect.

void FXParameterImpl::setValue(const void* data, unsigned int count)
{
    void* const fx = effectD3dxHandle(m_effect);
    if (!count)
        count = m_elements;

    const unsigned int typeClass = m_typeCode & 0xFFFFFF00u;

    if (typeClass > 0x400u)
    {
        if (typeClass != 0x800u)
        {
            if (typeClass == 0x10000u)
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetValueFn>(fx, 0x50)(
                    fx, m_handle, data, m_bytes);
                return;
            }
            ::renddx9abi::logf("FXParameter.cpp:357 Data type is unknown for parameter.");
            return;
        }
        // object/texture (2056): unwrap the EngineTexture (its D3D base texture at +0x08) and hand
        // D3DX the raw pointer - sub_10014930.
        void* d3dTexture = 0;
        if (data)
            d3dTexture = *reinterpret_cast<void* const*>(
                static_cast<const unsigned char*>(data) + 0x08);
        ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetTextureFn>(fx, 0xD0)(
            fx, m_handle, d3dTexture);
        return;
    }

    switch (typeClass)
    {
    case 0x400u:   // matrix
        if (m_typeCode & 4u)
        {
            ::renddx9abi::logf("FXParameter.cpp:295 Investigate more!");
            return;
        }
        if (m_typeCode & 1u)
        {
            ::renddx9abi::logf("FXParameter.cpp:299 Investigate more!");
            return;
        }
        if (m_typeCode & 2u)
        {
            if (m_elements)
            {
                if (m_sparseArray)
                {
                    const float* src = static_cast<const float*>(data);
                    float* dst = s_sparseMatrixStaging;
                    for (unsigned int i = 0; i < count; ++i, src += 16, dst += 12)
                    {
                        dst[0] = src[0];  dst[1] = src[4];  dst[2]  = src[8];  dst[3]  = src[12];
                        dst[4] = src[1];  dst[5] = src[5];  dst[6]  = src[9];  dst[7]  = src[13];
                        dst[8] = src[2];  dst[9] = src[6];  dst[10] = src[10]; dst[11] = src[14];
                    }
                    stateManagerSetVertexShaderConstantF(m_arrayStart, s_sparseMatrixStaging,
                                                         count * m_columns);
                    return;
                }
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetArrayRangeFn>(fx, 0xE0)(
                    fx, m_handle, 0, count);
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetMatrixArrayFn>(fx, 0xA0)(
                    fx, m_handle, data, count);
                return;
            }
            ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetMatrixFn>(fx, 0x98)(fx, m_handle, data);
            return;
        }
        ::renddx9abi::logf("FXParameter.cpp:326 Unknown parameter type");
        return;

    case 0x100u:   // scalar
        if (m_typeCode & 4u)
        {
            if (m_elements)
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetArrayRangeFn>(fx, 0xE0)(
                    fx, m_handle, 0, count);
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetBoolArrayFn>(fx, 0x60)(
                    fx, m_handle, static_cast<const int*>(data), count);
            }
            else
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetBoolFn>(fx, 0x58)(
                    fx, m_handle, *static_cast<const int*>(data));
            }
            return;
        }
        if (m_typeCode & 1u)
        {
            if (m_elements)
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetArrayRangeFn>(fx, 0xE0)(
                    fx, m_handle, 0, count);
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetIntArrayFn>(fx, 0x70)(
                    fx, m_handle, static_cast<const int*>(data), count);
            }
            else
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetIntFn>(fx, 0x68)(
                    fx, m_handle, *static_cast<const int*>(data));
            }
            return;
        }
        if (m_typeCode & 2u)
        {
            if (m_elements)
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetArrayRangeFn>(fx, 0xE0)(
                    fx, m_handle, 0, count);
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetFloatArrayFn>(fx, 0x80)(
                    fx, m_handle, static_cast<const float*>(data), count);
            }
            else
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetFloatFn>(fx, 0x78)(
                    fx, m_handle, *static_cast<const float*>(data));
            }
            return;
        }
        ::renddx9abi::logf("FXParameter.cpp:256 Unknown parameter type");
        return;

    case 0x200u:   // vector
        if (m_typeCode & 4u)
        {
            ::renddx9abi::logf("FXParameter.cpp:267 Investigate more!");
            return;
        }
        if (m_typeCode & 1u)
        {
            ::renddx9abi::logf("FXParameter.cpp:271 Investigate more!");
            return;
        }
        if (m_typeCode & 2u)
        {
            if (m_elements)
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetArrayRangeFn>(fx, 0xE0)(
                    fx, m_handle, 0, count);
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetVectorArrayFn>(fx, 0x90)(
                    fx, m_handle, data, count);
            }
            else
            {
                ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetVectorFn>(fx, 0x88)(
                    fx, m_handle, data);
            }
            return;
        }
        ::renddx9abi::logf("FXParameter.cpp:284 Unknown parameter type");
        return;

    default:
        ::renddx9abi::logf("FXParameter.cpp:357 Data type is unknown for parameter.");
        return;
    }
}

// +0x1C, straight through to ID3DXBaseEffect::SetBool.
void FXParameterImpl::setBool(unsigned char value)
{
    void* const fx = effectD3dxHandle(m_effect);
    ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetBoolFn>(fx, 0x58)(fx, m_handle, value);
}

// +0x20, straight through to ID3DXBaseEffect::SetInt.
void FXParameterImpl::setInt(int value)
{
    void* const fx = effectD3dxHandle(m_effect);
    ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetIntFn>(fx, 0x68)(fx, m_handle, value);
}

// +0x24, straight through to ID3DXBaseEffect::SetFloat.
void FXParameterImpl::setFloat(float value)
{
    void* const fx = effectD3dxHandle(m_effect);
    ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetFloatFn>(fx, 0x78)(fx, m_handle, value);
}

// +0x28, 0x10021640. This overload accepts pointers to matrices, unlike +0x18's contiguous
// matrix-array arm. The sparse path repacks the first three transposed rows exactly like +0x18;
// the ordinary path selects the range before calling D3DX SetMatrixPointerArray (+0xA8).
void FXParameterImpl::setMatrixPointerArray(const void* const* matrices, unsigned int count)
{
    if (m_sparseArray)
    {
        float* dst = s_sparseMatrixStaging;
        for (unsigned int i = 0; i < count; ++i, dst += 12)
        {
            const float* const src = static_cast<const float*>(matrices[i]);
            dst[0] = src[0];  dst[1] = src[4];  dst[2]  = src[8];  dst[3]  = src[12];
            dst[4] = src[1];  dst[5] = src[5];  dst[6]  = src[9];  dst[7]  = src[13];
            dst[8] = src[2];  dst[9] = src[6];  dst[10] = src[10]; dst[11] = src[14];
        }
        stateManagerSetVertexShaderConstantF(m_arrayStart, s_sparseMatrixStaging,
                                             count * m_columns);
        return;
    }

    void* const fx = effectD3dxHandle(m_effect);
    ::renddx9abi::d3dxSlot< ::renddx9abi::D3dxSetArrayRangeFn>(fx, 0xE0)(
        fx, m_handle, 0, count);
    typedef long (__stdcall* D3dxSetMatrixPointerArrayFn)(
        void*, void*, const void* const*, unsigned int);
    ::renddx9abi::d3dxSlot<D3dxSetMatrixPointerArrayFn>(fx, 0xA8)(
        fx, m_handle, matrices, count);
}

ParameterOwner* createFXParameter(void* effect, void* d3dxHandle)
{
    void* storage = ::renddx9abi::engineAlloc(sizeof(FXParameterImpl));
    if (!storage)
        return 0;
    return new (storage) FXParameterImpl(effect, d3dxHandle);
}

} // namespace dx9rend
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
