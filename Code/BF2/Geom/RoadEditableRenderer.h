// Queues editable roads and draws them in ascending template-priority order.
// The renderer owns the queue, while queued roads retain their own lifetimes.


#ifdef BF2_IMAGE_RENDDX9

#ifndef BF2_GEOM_ROADEDITABLERENDERER_H
#define BF2_GEOM_ROADEDITABLERENDERER_H

#include "IRoadEditable.h"
#include "Vc71PtrList.h"
#include "../../RendDX9Abi/HfeAbi.h"

#include <stddef.h>

namespace dice {
namespace hfe {
namespace geom {

typedef Vc71PtrList<IRoadEditable>     RoadEditableList;
typedef Vc71PtrListNode<IRoadEditable> RoadEditableListNode;


// IRoadEditableRenderer (IID 820020) - the five slots after the IBase quartet, table 0x1021DA00.

class IRoadEditableRenderer : public IBase
{
public:
    virtual void add(IRoadEditable* road) = 0;                                  // +0x10
    virtual void remove(IRoadEditable* road) = 0;                               // +0x14
    virtual void drawAll() = 0;                                                 // +0x18
    virtual void setFlag08(bool value) = 0;                                     // +0x1C (ICF)
    virtual bool getFlag08() = 0;                                               // +0x20 (ICF)
};

class RoadEditableRenderer : public IRoadEditableRenderer
{
public:
    RoadEditableRenderer();

    // IBase
    virtual void  addRef();                                                     // +0x00
    virtual int   getRef();                                                     // +0x04
    virtual int   release();                                                    // +0x08
    virtual void* queryInterface(unsigned int iid) const;                       // +0x0C

    // IRoadEditableRenderer
    virtual void add(IRoadEditable* road);
    virtual void remove(IRoadEditable* road);
    virtual void drawAll();
    virtual void setFlag08(bool value);
    virtual bool getFlag08();

    // data, in shipped order
    int               m_refCount;      // +0x04
    unsigned char     m_flag08;        // +0x08
    unsigned char     m_pad09[3];      // +0x09  not written
    RoadEditableList  m_roads;         // +0x0C
    int               m_mode;          // +0x18

private:
    RoadEditableRenderer(const RoadEditableRenderer&);
    RoadEditableRenderer& operator=(const RoadEditableRenderer&);
};

#if defined(_M_IX86)
static_assert(offsetof(RoadEditableRenderer, m_refCount) == 0x04, "refCount at +0x04");
static_assert(offsetof(RoadEditableRenderer, m_flag08) == 0x08, "flag byte at +0x08");
static_assert(offsetof(RoadEditableRenderer, m_roads) == 0x0C, "road list at +0x0C");
static_assert(offsetof(RoadEditableRenderer, m_mode) == 0x18, "mode word at +0x18");
static_assert(sizeof(RoadEditableRenderer) == 0x1C, "the factory 0x100B0A5D allocates 0x1C");
#endif

// RefFactory published by ModuleGeom::getClassList.
IBase* __fastcall createRoadEditableRenderer(unsigned int iid, IBase* outer);

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_GEOM_ROADEDITABLERENDERER_H
#endif // BF2_IMAGE_RENDDX9
