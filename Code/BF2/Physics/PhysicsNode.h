// Rigid body for vehicles and platforms, extending point-mass integration with angular dynamics.
// Physics parenting follows the object tree; parented bodies report zero local speed.
// The previous transform initially resolves to the object's current transform until explicitly saved.
// Owns separate 16-entry position and rotation latency rings; full reset rewinds both.
// The concrete class uses its own reference counter independently of its bases.

#ifndef PHYSICS_PHYSICSNODE_H     // [INFERRED] guard style
#define PHYSICS_PHYSICSNODE_H

#include "PointPhysicsNode.h"
#include "../Math/Quat.h"

namespace dice {
namespace hfe {
namespace world {

class PhysicsNode : public PointPhysicsNode
{
public:
    PhysicsNode();
    virtual ~PhysicsNode();                          // 74 B - delete[] the two latency rings

    virtual void  addRef();                          // the +0x90 counter
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    virtual IPhysicsNode* getParent() const;         // m_object->parent's node, or 0
    virtual void  setAbsoluteTransformation(const Mat4&);    // -> m_object (slot 35)
    virtual void  setAbsolutePosition(const Vec3&);          // -> m_object (slot 28)
    virtual const Mat4& getPrevTransformation() const;       // +0x1D4, or the object's current
    virtual void  setPrevTransformation(const Mat4&);        // stores +0x1D4, clears the flag
    virtual void  setOldPosition(const Vec3&);               // +0x148
    virtual const Vec3& getPositionalSpeed() const;          // zero while parented
    virtual const Vec3& getRotationalSpeed() const;          // zero while parented
    virtual void  setRotationalSpeed(const Vec3&);           // +0x94, snap
    virtual void  addRotationalSpeed(const Vec3&);           // snap
    virtual void  setRotationalAcceleration(const Vec3&);    // +0xA0, snap
    virtual void  setPositionalFriction(const Vec3&);        // +0xC4, snap
    virtual void  setRotationalFriction(const Vec3&);        // +0xD0, snap
    virtual void  addSpeedAtAbsolutePosition(const Vec3&, const Vec3&);
    virtual void  addSpeedAtRelativePosition(const Vec3&, const Vec3&);
    virtual void  addAccelerationAtAbsolutePosition(const Vec3&, const Vec3&);
    virtual void  addAccelerationAtRelativePosition(const Vec3&, const Vec3&);
    virtual void  addFrictionAtAbsolutePosition(const Vec3&, const Vec3&);        // written
    virtual void  addPositionalAccelerationToRootParent(const Vec3&);
    virtual void  addRotationalAccelerationAtAbsolutePosition(const Vec3&, const Vec3&);
    virtual const Vec3& getTangentSpeed(const Vec3&);
    virtual const Vec3& getTangentAcceleration(const Vec3&);
    virtual const Vec3& getRotationalAcceleration() const;   // +0xA0
    virtual const Vec3& getPositionalFriction() const;       // +0xC4
    virtual const Vec3& getRotationalFriction() const;       // +0xD0
    virtual void  updatePhysics(float dt);
    virtual void  setDragOffset(const Vec3&);                // +0xDC
    virtual const Vec3& getDragOffset() const;
    virtual void  setDragModifier(const Vec3&);              // +0xE8
    virtual const Vec3& getDragModifier() const;
    virtual void  setCenterOfMassOffset(const Vec3&);        // +0xF8
    virtual const Vec3& getCenterOfMassOffset() const;
    virtual void  setCenterOfCollisionOffset(const Vec3&);   // +0x104
    virtual const Vec3& getCenterOfCollisionOffset() const;
    virtual void  setInertiaModifier(const Vec3&);           // +0x110
    virtual const Vec3& getInertiaModifier() const;
    virtual void  setGravityModifier(float);                 // +0x58 (the base field, re-overridden)
    virtual void  reset(bool full);                          // 291 B - see the header
    virtual bool  getIsPoint() const;                        // false
    virtual void  initLatencyCompensation();                 // 120 B - the two rings
    virtual bool  hasLatencyCompensation();                  // positions ring != 0
    virtual void  updateLatencyState();
    virtual const Mat4& getLatencyCompensatedTransformation(float, int*);   // written
    virtual IPhysicsNode* getRootNode();                     // getRootParent(m_object)'s node
    virtual const Vec3& getInverseInertia();
    virtual void  addRelativeImpulse(const Vec3&, const Vec3&);   // written
    virtual const Vec3& getAngularIntegrationVelocity() const;    // +0xB8
    virtual void  setAngularIntegrationVelocity(const Vec3&);
    virtual void  setLocalAngularSpeed(const Vec3&);         // +0xAC (no snap)
    virtual const Vec3& getLocalAngularSpeed() const;
    virtual bool  getShouldGravityCompensateOnContact() const;    // Caches the result; -1 means not computed.
    virtual void  setHasContact(bool);                       // +0x228
    virtual bool  getHasContact() const;
    virtual void  setIsInAir(bool);                          // +0x229
    virtual bool  getIsInAir() const;
    virtual void  setIsPlatformPhysics(bool);                // +0x215
    virtual bool  getIsPlatformPhysics() const;

    // Non-virtual.
    PhysicsNode* getTopFixParent();                          // see the header
    float getDragMod(float underWater);                      // 1 - u + 25u
    void updatePositionalDragSimple(float, float);
    void updateRotationalDragSimple(float, float);
    void updatePositionalDragAdvanced(float, float, float, float);
    void updateRotationalDragAdvanced(float, float, float, float);
    void updatePositionalPhysics(float dt);
    void updateRotationalPhysics(float dt);

protected:
    // Original rigid-body layout. [UNRESOLVED] Fields retain unknown meanings where integrator
    // accesses alone do not establish their roles.
    /* +0x090 */ int     m_refCount;                 // ctor 1
    /* +0x094 */ Vec3    m_rotationalSpeed;
    /* +0x0A0 */ Vec3    m_rotationalAcceleration;
    /* +0x0AC */ Vec3    m_localAngularSpeed;
    /* +0x0B8 */ Vec3    m_angularIntegrationVelocity;
    /* +0x0C4 */ Vec3    m_positionalFriction;
    /* +0x0D0 */ Vec3    m_rotationalFriction;
    /* +0x0DC */ Vec3    m_dragOffset;
    /* +0x0E8 */ Vec3    m_dragModifier;
    /* +0x0F4 */ int     m_dwordF4;                  // 0, zeroed by reset - the friction-mean sample count (addFrictionAtAbsolutePosition)
    /* +0x0F8 */ Vec3    m_centerOfMassOffset;
    /* +0x104 */ Vec3    m_centerOfCollisionOffset;
    /* +0x110 */ Vec3    m_inertiaModifier;          // (1, 1, 1)
    /* +0x11C */ bool    m_fixToParent;              // true - [CHOSEN] name: getTopFixParent's gate
    /* +0x120 */ Vec3*   m_latencyPositions;         // 0; Vec3[16] once initLatencyCompensation ran
    /* +0x124 */ Quat*   m_latencyRotations;         // 0; Quat[16]
    /* +0x128 */ int     m_latencyIndex;             // 0; the ring cursor, mod 16
    /* +0x12C */ Vec3    m_vec12C;                   // 0; getTangentSpeed scratch/result
    /* +0x138 */ Vec3    m_vec138;                   // 0; getTangentAcceleration scratch/result
    /* +0x144 */ bool    m_byte144;                  // 0 [UNRESOLVED]
    /* +0x148 */ Vec3    m_oldPosition;              // setOldPosition's target
    /* +0x154 */ Vec3    m_vec154;                   // 0 [UNRESOLVED]
    /* +0x160 */ Vec3    m_vec160[5];                // 0 [UNRESOLVED] - five, by the ctor's loop
    /* +0x19C */ int     m_dword19C;                 // 0 [UNRESOLVED]
    /* +0x1A0 */ Vec3    m_vec1A0;                   // 0 [UNRESOLVED]
    /* +0x1AC */ Vec3    m_vec1AC[3];                // 0 [UNRESOLVED] - three, by the ctor's loop
    /* +0x1D0 */ int     m_dword1D0;                 // 0 [UNRESOLVED]
    /* +0x1D4 */ Mat4    m_prevTransformation;       // identity
    /* +0x214 */ bool    m_prevTransformationIsObjects;   // true - [CHOSEN] name: getPrev returns
                                                     //   the object's current one while set
    /* +0x215 */ bool    m_isPlatformPhysics;        // false
    /* +0x216 */ bool    m_inverseInertiaValid;      // One-shot inverse-inertia cache flag, initially false.
    /* +0x218 */ Vec3    m_inverseInertia;           // 0 - the cache itself, 3 / (the collision
                                                     //   box's other two squared extents) per axis
    /* +0x224 */ int     m_gravityCompensateOnContact;    // -1 = not computed; the 253-B getter's cache
    /* +0x228 */ bool    m_hasContact;               // false
    /* +0x229 */ bool    m_isInAir;                  // true
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_PHYSICSNODE_H
