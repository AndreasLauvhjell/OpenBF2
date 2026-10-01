// Internal overgrowth grid-cell and batched-stream declarations.
#ifndef BF2_GEOM_OVERGROWTHCELL_H
#define BF2_GEOM_OVERGROWTHCELL_H

#ifdef BF2_IMAGE_RENDDX9
#include "OverGrowth.h"
#include "../../Math/Mat4.h"
#include "../../RendDX9/Ra/RaStreamDataEditable.h"

namespace dice { namespace hfe { namespace geom {


// Cell vertex stream plus object parameters. Extra buffer-management slots extend the
// current base declaration while retaining compatibility positions.

class OverGrowthCellMesh : public dx9rend::Ra::RaStreamDataEditable
{
public:
    OverGrowthCellMesh(unsigned int vertexCapacity, unsigned int indexCapacity,
                       const vc71::vector_layout& elements);

    void* destroy(unsigned char flags) override;                     // +0x00
    virtual void releaseBuffers();                                    // +0x30  10048D60 [FRONTIER]
    virtual unsigned int getFreeVertexCount() const;                  // +0x34
    virtual unsigned int getVertexCapacity() const;                   // +0x38
    virtual unsigned int getIndexCapacity() const;                    // +0x3C
    virtual void createBuffers();                                     // +0x40  10048C70 [FRONTIER]

    unsigned char m_objectParameters[0x224];                          // +0x1A4 sub_100458E0
};

#if defined(_M_IX86)
static_assert(sizeof(OverGrowthCellMesh) == 0x3C8, "the cell allocates 0x3C8 per stream (0x100D0ECE)");
#endif


// Visible grid cell with bounds, camera fade, instance list, and leaf/trunk streams.
// The eight-slot interface overlays its abstract base in the 0x2AC-byte compatibility layout.

struct OverGrowthCell
{
    OverGrowthCell();                                                 // 100D0C90 (+ base 1015E6D0)

    virtual void* destroy(unsigned char flags);                       // +0x00
    virtual bool  appendGeometry(void* geometryTemplate, const Mat4* transform, float scale,
                                 int unused, float shadow);           // +0x04
    virtual void  lock();                                             // +0x08
    virtual void  unlock();                                           // +0x0C
    virtual void  draw(IRenderView* view, const Vec3* position, float fade); // +0x10
    virtual unsigned int getVertexCapacity();                         // +0x14
    virtual unsigned int getIndexCapacity();                          // +0x18
    virtual void  clearFarList();                                     // +0x1C

    alignas(4) unsigned char m_storage[0x2A8];
};

#if defined(_M_IX86)
static_assert(sizeof(OverGrowthCell) == 0x2AC, "postLoadResources allocates 0x2AC per cell (0x100CD6DE)");
#endif

// The allocation postLoadResources makes per grid slot (0x100CD6DE..0x100CD6FB): sub_1005F320(0x2AC), then
// the constructor when the heap answered (OverGrowthCells.cpp).
OverGrowthCell* createOverGrowthCell();

// sub_1015EEB0 - the abstract base's updateCells: the camera ring of N x N cells and each cell's build.
// OverGrowth::updateCells calls it first; render calls it directly when +0xD5 is clear
// (OverGrowthCells.cpp).
void overGrowthBaseUpdateCells(OverGrowth* self, IRenderView* view);

} } }
#endif
#endif
