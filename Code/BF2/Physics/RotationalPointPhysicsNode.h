// Point-mass body with angular velocity, acceleration and friction for tumbling objects.
// Uses its own reference counter. Drag requires nonzero mass; the zero-mass diagnostic is fatal.
// reset clears angular state but preserves centerOfCollisionOffset. Setters retain epsilon snapping.

#ifndef PHYSICS_ROTATIONALPOINTPHYSICSNODE_H     // [INFERRED] guard style
#define PHYSICS_ROTATIONALPOINTPHYSICSNODE_H

#include "PointPhysicsNode.h"

namespace dice {
namespace hfe {
namespace world {

class RotationalPointPhysicsNode : public PointPhysicsNode
{
public:
    RotationalPointPhysicsNode();
    virtual ~RotationalPointPhysicsNode();           // trivial -> ~PointPhysicsNode

    virtual void  addRef();                          // the +0x90 counter
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    virtual const Vec3& getRotationalSpeed() const;  // +0x94
    virtual void  setRotationalSpeed(const Vec3&);   // snap
    virtual void  addRotationalSpeed(const Vec3&);   // snap
    virtual void  setRotationalAcceleration(const Vec3&);   // +0xAC, snap
    virtual void  setPositionalFriction(const Vec3&);       // +0xB8, snap
    virtual void  setRotationalFriction(const Vec3&);       // +0xC4, snap
    virtual const Vec3& getRotationalAcceleration() const;
    virtual const Vec3& getPositionalFriction() const;
    virtual const Vec3& getRotationalFriction() const;
    virtual void  updatePhysics(float dt);           // 415 B - see the header
    virtual void  setCenterOfCollisionOffset(const Vec3&);  // +0xD4
    virtual const Vec3& getCenterOfCollisionOffset() const;
    virtual void  reset(bool full);

    // Non-virtual helpers (hide PointPhysicsNode's updatePositionalDragSimple - same name, own body).
    void updatePositionalDragSimple(float radius, float dragMod);
    void updateRotationalDragSimple(float radius, float dragMod);
    void updateRotationalPhysics(float dt);

protected:
    // Layout 0x90..0xDF, every default 0 except the counter (168-byte ctor + setIsAwake at its end).
    /* +0x90 */ int     m_refCount;                  // ctor 1
    /* +0x94 */ Vec3    m_rotationalSpeed;
    /* +0xA0 */ Vec3    m_angularIntegrationVelocity;
    /* +0xAC */ Vec3    m_rotationalAcceleration;
    /* +0xB8 */ Vec3    m_positionalFriction;
    /* +0xC4 */ Vec3    m_rotationalFriction;
    /* +0xD0 */ int     m_dwordD0;                   // [UNRESOLVED] zeroed after each rotation step
    /* +0xD4 */ Vec3    m_centerOfCollisionOffset;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_ROTATIONALPOINTPHYSICSNODE_H
