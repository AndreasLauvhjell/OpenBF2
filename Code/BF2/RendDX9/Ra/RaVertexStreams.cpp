
// Required vertex elements, declaration construction and the shared declaration cache.
// Keys compare element-vector length and bytes.

#include "RaShader.h"

#ifdef BF2_IMAGE_RENDDX9

#include "../RendDX9Base.h"
#include "../../../RendDX9Abi/BfMemory.h"
#include "../../../RendDX9Abi/Frontier.h"
#include "../../../RendDX9Abi/Vc71Vector.h"

#include <d3d9.h>
#include <list>
#include <map>
#include <stdio.h>
#include <string.h>
#include <vector>

namespace dice {
namespace hfe {
namespace dx9rend {

extern void* g_rendererBase;                // dword_1023B970 - RendDX9Init.cpp
extern void* g_deviceCaps;                  // dword_1023B980 - RendDX9Init.cpp
extern float g_materialCapScale;            // dword_1023ED3C - RenderQueues.cpp
extern float g_materialCapBias;             // dword_1023ED40
extern float g_materialCapC;                // dword_1023ED44
extern float g_materialCapD;                // dword_1023ED48
int vertexDeclarationTypeSize(unsigned int type);   // RendDX9Types.cpp - sub_1000CC30

namespace Ra {

// The renderer base's +0x0C - the IDirect3DDevice9 the declaration cache creates on; the same word
// StateManager.cpp's rendererDevice() reads.
IDirect3DDevice9* raRendererDevice()
{
    return *reinterpret_cast<IDirect3DDevice9**>(static_cast<unsigned char*>(g_rendererBase) + 0x0C);
}


struct RaStreamRequirements
{
    std::list<const RaVertexElementDesc*> items;
};

// sub_10049330 (the head node) then sub_1004A0F0 -> sub_1004A720: insert(end, first, last) of the
// shader's required-descriptor vector (RaShader +0x13C).
RaStreamRequirements* raStreamRequirementsCreate(const vc71::vector_layout& required)
{
    RaStreamRequirements* const r = new RaStreamRequirements;
    const RaVertexElementDesc* const* cursor =
        reinterpret_cast<const RaVertexElementDesc* const*>(required.first);
    const RaVertexElementDesc* const* const last =
        reinterpret_cast<const RaVertexElementDesc* const*>(required.last);
    for (; cursor != last; ++cursor)
        r->items.push_back(*cursor);
    return r;
}

// sub_10049FF0, PE 0x10049FF0.  For every descriptor of one stream, the first still-required element
// with the same elementId and declUsage is unlinked and freed; answers whether the stream provided
// anything at all.  A descriptor matching nothing consumes nothing.
bool raStreamRequirementsConsume(RaStreamRequirements* r, const vc71::vector_layout& stream)
{
    bool provided = false;
    const RaVertexElementDesc* const* cursor =
        reinterpret_cast<const RaVertexElementDesc* const*>(stream.first);
    const RaVertexElementDesc* const* const last =
        reinterpret_cast<const RaVertexElementDesc* const*>(stream.last);
    for (; cursor != last; ++cursor)
    {
        const RaVertexElementDesc* const desc = *cursor;
        for (std::list<const RaVertexElementDesc*>::iterator it = r->items.begin();
             it != r->items.end(); ++it)
        {
            if ((*it)->elementId == desc->elementId && (*it)->declUsage == desc->declUsage)
            {
                r->items.erase(it);
                provided = true;
                break;
            }
        }
    }
    return provided;
}

// sub_10049FE0 - `size != 0`: something the shader needs was provided by no stream.
bool raStreamRequirementsStarving(const RaStreamRequirements* r)
{
    return !r->items.empty();
}

// sub_100495B0 - the operator<< the starving diagnostic streams: every remaining element as
// "Element Data: <name> d3dType: <type> d3dUsage: <usage>", one after the other.
void raStreamRequirementsDescribe(const RaStreamRequirements* r, char* out, unsigned int size)
{
    unsigned int used = 0;
    if (size)
        out[0] = 0;
    for (std::list<const RaVertexElementDesc*>::const_iterator it = r->items.begin();
         it != r->items.end() && used + 1 < size; ++it)
    {
        const int n = _snprintf(out + used, size - used - 1, "Element Data: %s d3dType: %d d3dUsage: %d",
                                (*it)->name.c_str(), (*it)->declType, (*it)->declUsage);
        if (n < 0)
            break;
        used += static_cast<unsigned int>(n);
        out[used < size ? used : size - 1] = 0;
    }
}

// sub_1000EAD0 - unlink and free every node, then the head; here the object with them.
void raStreamRequirementsDestroy(RaStreamRequirements* r)
{
    delete r;
}


static_assert(sizeof(RaDeclarationBuilder) == 0x58, "sub_1004A270 zeroes +0x0C..+0x47 and a vector at +0x48");

// Clears the builder and refreshes packed descriptors/unpack constants from DEC3N support.
void raDeclarationBuilderConstruct(RaDeclarationBuilder& b)
{
    b.elements.alloc = 0;
    b.elements.first = 0;
    b.elements.last  = 0;
    b.elements.end   = 0;
    b.stream   = 0;
    b.offset   = 0;
    b.finished = 0;
    ::memset(b.usageCounts, 0, sizeof(b.usageCounts));

    const unsigned int featureMask = *reinterpret_cast<const unsigned int*>(
        static_cast<const unsigned char*>(g_deviceCaps) + 0xEC);
    if ((featureMask & 0x80u) != 0)
    {
        g_desc_1023EE14.declType = 14;                             // dword_1023EE18
        g_desc_1023EEB4.declType = 14;                             // dword_1023EEB8
        g_materialCapScale = 1.0f;
        g_materialCapBias  = 0.0f;
    }
    else
    {
        g_desc_1023EE14.declType = 4;
        g_desc_1023EEB4.declType = 4;
        g_materialCapScale = 2.0f;
        g_materialCapBias  = -1.0f;
    }
    g_materialCapC = 3.0517578125e-05f;
    g_materialCapD = 0.0f;
}

// sub_1004A120, PE 0x1004A120.  One stream's descriptors become D3DVERTEXELEMENT9 records on the
// current stream number at the running offset, each usage's index counting up; the offset advances
// by the type's size; then the stream number advances and the offset returns to 0.
void raDeclarationBuilderAddStream(RaDeclarationBuilder& b, const vc71::vector_layout& stream)
{
    const RaVertexElementDesc* const* cursor =
        reinterpret_cast<const RaVertexElementDesc* const*>(stream.first);
    const RaVertexElementDesc* const* const last =
        reinterpret_cast<const RaVertexElementDesc* const*>(stream.last);
    for (; cursor != last; ++cursor)
    {
        const RaVertexElementDesc* const desc = *cursor;
        D3DVERTEXELEMENT9 element;
        element.Stream     = static_cast<WORD>(b.stream);
        element.Offset     = static_cast<WORD>(b.offset);
        element.Type       = static_cast<BYTE>(desc->declType);
        element.Method     = 0;
        element.Usage      = static_cast<BYTE>(desc->declUsage);
        element.UsageIndex = static_cast<BYTE>(b.usageCounts[desc->declUsage]);
        vc71::vectorPushBackIn(b.elements, sizeof(element), &element);
        b.offset += static_cast<unsigned int>(
            vertexDeclarationTypeSize(static_cast<unsigned int>(desc->declType)));
        ++b.usageCounts[desc->declUsage];
    }
    ++b.stream;
    b.offset = 0;
}

// sub_1004A230, PE 0x1004A230.  D3DDECL_END() once, then the vector.
const vc71::vector_layout& raDeclarationBuilderFinish(RaDeclarationBuilder& b)
{
    if (!b.finished)
    {
        D3DVERTEXELEMENT9 end;
        end.Stream     = 0xFF;
        end.Offset     = 0;
        end.Type       = 17;                                                    // D3DDECLTYPE_UNUSED
        end.Method     = 0;
        end.Usage      = 0;
        end.UsageIndex = 0;
        vc71::vectorPushBackIn(b.elements, sizeof(end), &end);
        b.finished = 1;
    }
    return b.elements;
}

void raDeclarationBuilderDestroy(RaDeclarationBuilder& b)
{
    if (b.elements.first)
        ::renddx9abi::engineFree(b.elements.first);
    b.elements.first = 0;
    b.elements.last  = 0;
    b.elements.end   = 0;
}


// Returns the cached vertex declaration, creating it on a miss.
// Failed creation is cached as null because HRESULT is not checked.

IDirect3DVertexDeclaration9* raVertexDeclarationCached(const vc71::vector_layout& elements,
                                                        IDirect3DDevice9* device)
{
    typedef std::map<std::vector<unsigned char>, IDirect3DVertexDeclaration9*> Cache;
    static Cache s_cache;

    std::vector<unsigned char> key;
    if (elements.first)
        key.assign(reinterpret_cast<const unsigned char*>(elements.first),
                   reinterpret_cast<const unsigned char*>(elements.last));

    const Cache::iterator found = s_cache.find(key);
    if (found != s_cache.end())
        return found->second;

    IDirect3DVertexDeclaration9* declaration = 0;
    device->CreateVertexDeclaration(
        reinterpret_cast<const D3DVERTEXELEMENT9*>(elements.first), &declaration);
    s_cache.insert(std::make_pair(key, declaration));
    return declaration;
}

} // namespace Ra
} // namespace dx9rend
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
