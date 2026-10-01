// Mobile point mass with velocity, acceleration, drag and a sleep countdown. Path [INFERRED].
// Uses its own reference counter independently of StaticPhysicsNode's counter.
// Velocity updates clamp speed and snap components below 1e-6 to zero.
// Integration consumes acceleration before the next gravity contribution is added.
// Waking a sleeping root PlayerControlObject also wakes nearby mobile nodes.
// This base has no physics parent; rigid-body subclasses supply object-tree parenting.

#ifndef PHYSICS_POINTPHYSICSNODE_H     // [INFERRED] guard style
#define PHYSICS_POINTPHYSICSNODE_H

#include "StaticPhysicsNode.h"

namespace dice {
namespace hfe {
namespace world {

class PointPhysicsNode : public StaticPhysicsNode
{
public:
    PointPhysicsNode();
    virtual ~PointPhysicsNode();                     // trivial -> ~StaticPhysicsNode

    virtual void  addRef();                          // the +0x1C counter
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;   // IBase / IID_IPhysicsNode -> this

    virtual IPhysicsNode* getParent() const;         // 0 - a point has no physics parent
    virtual const Vec3& getPositionalSpeed() const;  // +0x20
    virtual void  setPositionalSpeed(const Vec3&);   // 254 B - max-speed clamp + epsilon snap
    virtual void  addPositionalSpeed(const Vec3&);   // 265 B - same, additive
    virtual void  setPositionalAcceleration(const Vec3&);   // 139 B - epsilon snap
    virtual void  addSpeedAtAbsolutePosition(const Vec3& pos, const Vec3& v);   // speed += v (pos ignored)
    virtual void  addSpeedAtRelativePosition(const Vec3& pos, const Vec3& v);   // same
    virtual void  addAccelerationAtAbsolutePosition(const Vec3& pos, const Vec3& a);   // acc += a
    virtual void  addAccelerationAtRelativePosition(const Vec3& pos, const Vec3& a);   // same
    virtual const Vec3& getTangentSpeed(const Vec3&);        // = positionalSpeed (a point has none)
    virtual const Vec3& getTangentAcceleration(const Vec3&); // = positionalAcceleration
    virtual const Vec3& getPositionalAcceleration() const;   // +0x2C
    virtual void  updatePhysics(float dt);           // 133 B - see the header
    virtual void  setDrag(float);                    // +0x50
    virtual float getDrag() const;
    virtual void  setMass(float);                    // +0x54
    virtual float getMass() const;
    virtual void  setGravityModifier(float);         // +0x58
    virtual float getGravityModifier() const;
    virtual void  setHasSeparatePhysicsUpdate(bool); // +0x60
    virtual bool  getHasSeparatePhysicsUpdate() const;
    virtual void  setUnderWater(float);              // +0x5C
    virtual float getUnderWater() const;
    virtual void  reset(bool full);                  // 171 B - see the header
    virtual bool  getIsSleeping() const;             // 68 B - the sleep logic, see the header
    virtual void  setIsSleeping();
    virtual void  setSleepiness(int);
    virtual int   getSleepiness() const;
    virtual void  setIsSleepier();
    virtual void  setIsAwake();                      // 148 B - + wakeUpObjects for a root PCO
    virtual bool  getIsMobile() const;               // true
    virtual bool  getIsPoint() const;                // true
    virtual const Vec3& getAbsolutePositionalSpeed();        // +0x78
    virtual IPhysicsNode* getRootNode();             // this
    virtual void  addRelativeImpulse(const Vec3& pos, const Vec3& v);   // localLinearSpeed += v
    virtual void  setFloaterMod(float);              // +0x84
    virtual float getFloaterMod() const;
    virtual const Vec3& getLinearIntegrationVelocity() const;    // +0x44
    virtual void  setLinearIntegrationVelocity(const Vec3&);
    virtual void  postFrameUpdate();                 // 235 B - see the header
    virtual void  setLocalLinearSpeed(const Vec3&);  // +0x38
    virtual const Vec3& getLocalLinearSpeed() const;
    virtual void  setOtherCollisionLod(int);         // +0x8C
    virtual int   getOtherCollisionLod() const;
    virtual void  setSleepTimeout(int);              // +0x64

    // Non-virtual (not in any vtable) - the two integration helpers, called by updatePhysics here
    // and by the derived classes' own updatePhysics.
    void updatePositionalDragSimple(float radius, float dragMod);
    void updatePositionalPhysics(float dt);

protected:
    // Layout 0x1C..0x8F, every default from the 168-byte ctor. StaticPhysicsNode is 0x00..0x1B.
    /* +0x1C */ int     m_refCount;                  // ctor 1 - this class's counter (see header)
    /* +0x20 */ Vec3    m_positionalSpeed;           // 0
    /* +0x2C */ Vec3    m_positionalAcceleration;    // 0
    /* +0x38 */ Vec3    m_localLinearSpeed;          // 0 - impulses to fold in next tick
    /* +0x44 */ Vec3    m_linearIntegrationVelocity; // 0 - the trapezoidal average of the last tick
    /* +0x50 */ float   m_drag;                      // 0
    /* +0x54 */ float   m_mass;                      // 1.0f
    /* +0x58 */ float   m_gravityModifier;           // 1.0f
    /* +0x5C */ float   m_underWater;                // 0 - depth, 0..; 0.1 = fully "in water"
    /* +0x60 */ bool    m_hasSeparatePhysicsUpdate;  // false
    /* +0x64 */ int     m_sleepTimeout;              // 30 (ticks)
    /* +0x68 */ int     m_sleepiness;                // 0 - the countdown, see the header
    /* +0x6C */ Vec3    m_lastPosition;              // 0 - postFrameUpdate's previous position
    /* +0x78 */ Vec3    m_absolutePositionalSpeed;   // 0 - (pos - lastPos) * 30
    /* +0x84 */ float   m_floaterMod;                // 0
    /* +0x88 */ bool    m_byte88;                    // 0 - [UNRESOLVED] cleared by reset(), read by
                                                     //   nothing in this class (a derived-class or
                                                     //   response-side flag)
    /* +0x8C */ int     m_otherCollisionLod;         // 0 (PhysicsNode's ctor makes it 1, Soldier 2)
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_POINTPHYSICSNODE_H
