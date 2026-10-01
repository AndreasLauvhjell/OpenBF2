
// Editable render-queue material owning an interleaved vertex buffer and index buffer.
// Preserves the nine-slot material interface and adds lock/unlock/append operations.
// [FRONTIER] Resource-tracker registration/removal is not implemented.

#ifndef RENDDX9_RA_RASTREAMDATAEDITABLE_H
#define RENDDX9_RA_RASTREAMDATAEDITABLE_H

#include <cstddef>
#include <d3d9.h>
#include "../../../RendDX9Abi/Vc71Vector.h"
#include "../../../RendDX9Abi/Vc71Map.h"

namespace dice {
namespace hfe {
namespace dx9rend {
namespace Ra {

class RaStreamDataEditable
{
public:
    // sub_10048FF0: the capacities, the vertex stride, the element descriptors (copied), the device
    // that creates the two buffers (D3DUSAGE_WRITEONLY, D3DPOOL_MANAGED; the indices INDEX16).
    RaStreamDataEditable(unsigned int vertexCapacity, unsigned int indexCapacity, unsigned int stride,
                         const vc71::vector_layout& elements, IDirect3DDevice9* device);

    virtual void* destroy(unsigned char flags);                       // +0x00
    virtual bool  applyStreams(void* shaderState);                    // +0x04
    virtual int   getVertexCount() const;                             // +0x08
    virtual int   getIndexCount() const;                              // +0x0C
    virtual int   getPrimitiveCount() const;                          // +0x10
    virtual int   getPrimitiveType() const;                           // +0x14
    virtual int   getVertexStart() const;                             // +0x18
    virtual int   getMinIndex() const;                                // +0x1C
    virtual int   getIndexStart() const;                              // +0x20
    virtual void  lock();                                             // +0x24
    virtual void  unlock();                                           // +0x28
    virtual bool  append(const void* vertices, int stride, int vertexCount,
                         const unsigned short* indices, int indexCount);   // +0x2C

    void destroyMembers();

    IDirect3DVertexBuffer9*  m_vertexBuffer;         // +0x004
    IDirect3DIndexBuffer9*   m_indexBuffer;          // +0x008
    unsigned char            m_materialParameters[0x150];   // +0x00C  raMaterialParametersConstruct (sub_10044270) - unused by the water, which queues the Terrain's block
    vc71::vector_layout      m_elements;             // +0x15C  vector<const RaVertexElementDesc*> (first at +0x160)
    int                      m_unresolved16C;        // +0x16C  0
    IDirect3DDevice9*        m_device;               // +0x170
    unsigned char            m_locked;               // +0x174
    unsigned char            m_pad175[3];
    void*                    m_lockedVertices;       // +0x178
    void*                    m_lockedIndices;        // +0x17C
    unsigned int             m_stride;               // +0x180
    unsigned int             m_indexSize;            // +0x184  2
    unsigned int             m_indexCount;           // +0x188  appended so far
    unsigned int             m_vertexCount;          // +0x18C  appended so far
    unsigned int             m_vertexCapacity;       // +0x190
    unsigned int             m_indexCapacity;        // +0x194
    // +0x198: the per-shader declaration cache, map<RaShader::m_buildSerial, IDirect3DVertexDeclaration9*>.
    // The shipped map's nodes are 0x18 bytes (sub_100493C0); this tree's u32-key helpers use their
    // 0x28-byte node. A private container's node size is an STL detail, not a layout this object
    // shares with anything (agents.md: source-level STL operations).
    vc71::map_layout         m_declarations;         // +0x198  (comp +0x198, head +0x19C, size +0x1A0)
};

#if defined(_M_IX86)
static_assert(sizeof(RaStreamDataEditable) == 0x1A4, "sub_100DC400 allocates 0x1A4 for it");
static_assert(offsetof(RaStreamDataEditable, m_materialParameters) == 0x00C, "material block at +0x0C");
static_assert(offsetof(RaStreamDataEditable, m_elements) == 0x15C, "element vector at +0x15C");
static_assert(offsetof(RaStreamDataEditable, m_device) == 0x170, "device at +0x170");
static_assert(offsetof(RaStreamDataEditable, m_stride) == 0x180, "stride at +0x180");
static_assert(offsetof(RaStreamDataEditable, m_vertexCapacity) == 0x190, "vertex capacity at +0x190");
static_assert(offsetof(RaStreamDataEditable, m_declarations) == 0x198, "declaration cache at +0x198");
#endif

} // namespace Ra
} // namespace dx9rend
} // namespace hfe
} // namespace dice

#endif // RENDDX9_RA_RASTREAMDATAEDITABLE_H
