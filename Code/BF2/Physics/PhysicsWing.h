// Aerodynamic surface node, including water rudders. Header path [INFERRED].
// Uses its own reference count and sleep overrides that update the object's tick flag.
// The local matrix's second row is the lift axis; underwater lift uses the configured multiplier.
// startTick -1 means unarmed.

#ifndef PHYSICS_PHYSICSWING_H     // [INFERRED] guard style
#define PHYSICS_PHYSICSWING_H

#include "PhysicsNode.h"
#include "../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace world {

class WingTemplate;

// a free function in the PhysicsWing TU (mangled
// _ZN4dice3hfe5world13calculateLiftERKNS0_4Vec3ES4_S4_f). its second Vec3 is never read; see the
// body. Wing::calculateNeutralLift is the other caller.
float calculateLift(const Vec3& airflow, const Vec3& unused, const Vec3& surfaceNormal, float scale);

class PhysicsWing : public PhysicsNode
{
public:
    PhysicsWing(const WingTemplate* wingTemplate, const IObject* object);
    virtual ~PhysicsWing();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;                    // IBase / IPhysicsNode
    virtual void  updatePhysics(float dt);
    virtual void  setIsSleeping();
    virtual void  setSleepiness(int sleepiness);
    virtual void  setIsSleepier();
    virtual void  setIsAwake();
    virtual void  reset(bool full);

protected:
    /* 0x22C */ int   m_refCount;
    // Wing supplies its orientation matrix directly; PhysicsWing writes the wing's lift
    // regulator. Friendship preserves this mutual field access.
    friend class Wing;

    /* 0x230 */ int   m_startTick;
    /* 0x234 */ const WingTemplate* m_template;
    /* 0x238 */ const IObject* m_object2;
    /* 0x23C */ Mat4  m_matrix;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_PHYSICSWING_H
