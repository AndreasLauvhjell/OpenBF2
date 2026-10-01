// Buoyancy attachment fixed to its parent, with its own reference counter. Header path [INFERRED].
// The two trailing floats integrate an additional submersion offset; their names/units remain
// [UNRESOLVED]. Unlike springs/wings, this class inherits sleep behavior unchanged.
// No [FRONTIER] body remains in this class.

#ifndef PHYSICS_PHYSICSFLOATINGBUNDLE_H     // [INFERRED] guard style
#define PHYSICS_PHYSICSFLOATINGBUNDLE_H

#include "PhysicsNode.h"

namespace dice {
namespace hfe {
namespace world {

class FloatingBundleTemplate;

class PhysicsFloatingBundle : public PhysicsNode
{
public:
    PhysicsFloatingBundle(const FloatingBundleTemplate* floatingTemplate, const IObject* object);   // 0x82A9620 (C2) / 0x82A9670 (C1), 76 B - the object is not read
    virtual ~PhysicsFloatingBundle();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;                    // IBase / IPhysicsNode
    virtual void  updatePhysics(float dt);

    friend class FloatingBundle;                                             // handleUpdate writes +0x234

protected:
    /* 0x22C */ int   m_refCount;
    /* 0x230 */ const FloatingBundleTemplate* m_template;
    /* 0x234 */ float m_floaterValue;
    /* 0x238 */ float m_float238;
    /* 0x23C */ float m_float23C;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_PHYSICSFLOATINGBUNDLE_H
