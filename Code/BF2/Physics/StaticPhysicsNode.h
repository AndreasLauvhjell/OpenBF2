// Nonmoving physics node; header path [INFERRED]. Most setters are no-ops and velocity is zero.
// Mass is FLT_MAX and the node is always asleep. Transforms delegate to the object, with previous
// equal to current; parent/root queries follow the object tree. getIsMobile is false, so the manager
// never adds this node to its update list.

#ifndef PHYSICS_STATICPHYSICSNODE_H     // [INFERRED] guard style
#define PHYSICS_STATICPHYSICSNODE_H

#include "IPhysicsNode.h"

namespace dice {
namespace hfe {
namespace world {

class StaticPhysicsNode : public IPhysicsNode
{
public:
    StaticPhysicsNode();
    virtual ~StaticPhysicsNode();                    // trivial -> ~IPhysicsNode (the abort check)

    virtual void  addRef();                          // refcount +0x08
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;   // IBase / IID_IPhysicsNode -> this; else 0

    virtual void  setEnabled(bool);                  // +0x0C, a byte
    virtual bool  getEnabled() const;
    virtual void  setIsDamaged(bool);                // empty
    virtual bool  getIsDamaged() const;              // false
    virtual void  setObject(IObject*);               // +0x10
    virtual IObject* getObject() const;
    virtual void  setParent(IPhysicsNode*);          // empty - the parent is derived, see getParent
    virtual IPhysicsNode* getParent() const;         // object's parent's physics node, or 0
    virtual float getBoundingRadius() const;         // -> m_object (IObject slot 38)
    virtual const Mat4& getAbsoluteTransformation() const;   // -> m_object (slot 33)
    virtual void  setAbsoluteTransformation(const Mat4&);    // -> m_object (slot 35)
    virtual const Mat4& getRelativeTransformation() const;   // -> m_object (slot 58)
    virtual const Vec3& getAbsolutePosition() const;         // -> m_object (slot 27)
    virtual void  setAbsolutePosition(const Vec3&);          // -> m_object (slot 28)
    virtual const Mat4& getPrevTransformation() const;       // -> m_object slot 33 - see header
    virtual void  setPrevTransformation(const Mat4&);        // empty
    virtual void  setOldPosition(const Vec3&);               // empty
    virtual const Vec3& getRelativePosition() const;         // -> m_object (slot 56)
    virtual const Vec3& getPositionalSpeed() const;          // Vec3::getZero
    virtual const Vec3& getRotationalSpeed() const;          // Vec3::getZero
    virtual void  setPositionalSpeed(const Vec3&);           // empty (as are all mutators below)
    virtual void  setRotationalSpeed(const Vec3&);
    virtual void  addPositionalSpeed(const Vec3&);
    virtual void  addRotationalSpeed(const Vec3&);
    virtual void  setPositionalAcceleration(const Vec3&);
    virtual void  setRotationalAcceleration(const Vec3&);
    virtual void  setPositionalFriction(const Vec3&);
    virtual void  setRotationalFriction(const Vec3&);
    virtual void  addSpeedAtAbsolutePosition(const Vec3&, const Vec3&);
    virtual void  addSpeedAtRelativePosition(const Vec3&, const Vec3&);
    virtual void  addAccelerationAtAbsolutePosition(const Vec3&, const Vec3&);
    virtual void  addAccelerationAtRelativePosition(const Vec3&, const Vec3&);
    virtual void  addFrictionAtAbsolutePosition(const Vec3&, const Vec3&);
    virtual void  addPositionalAccelerationToRootParent(const Vec3&);
    virtual void  addRotationalAccelerationAtAbsolutePosition(const Vec3&, const Vec3&);
    virtual const Vec3& getTangentSpeed(const Vec3&);        // Vec3::getZero
    virtual const Vec3& getTangentAcceleration(const Vec3&); // Vec3::getZero
    virtual const Vec3& getPositionalAcceleration() const;   // Vec3::getZero
    virtual const Vec3& getRotationalAcceleration() const;   // Vec3::getZero
    virtual const Vec3& getPositionalFriction() const;       // Vec3::getZero
    virtual const Vec3& getRotationalFriction() const;       // Vec3::getZero
    virtual void  updatePhysics(float);                      // empty - nothing to integrate
    virtual void  setDrag(float);                            // empty
    virtual float getDrag() const;                           // 0.0f
    virtual void  setDragOffset(const Vec3&);                // empty
    virtual const Vec3& getDragOffset() const;               // Vec3::getZero
    virtual void  setDragModifier(const Vec3&);              // empty
    virtual const Vec3& getDragModifier() const;             // Vec3::getZero
    virtual void  setMass(float);                            // empty
    virtual float getMass() const;                           // FLT_MAX - infinite, ds:0x888FFE8
    virtual void  setCenterOfMassOffset(const Vec3&);        // empty
    virtual const Vec3& getCenterOfMassOffset() const;       // Vec3::getZero
    virtual void  setCenterOfCollisionOffset(const Vec3&);   // empty
    virtual const Vec3& getCenterOfCollisionOffset() const;  // Vec3::getZero
    virtual void  setInertiaModifier(const Vec3&);           // empty
    virtual const Vec3& getInertiaModifier() const;          // Vec3::getIdentity - the one
                                                             //   non-zero Vec3 default
    virtual void  setGravityModifier(float);                 // empty
    virtual float getGravityModifier() const;                // 1.0f
    virtual void  setHasSeparatePhysicsUpdate(bool);         // empty
    virtual bool  getHasSeparatePhysicsUpdate() const;       // false
    virtual void  setUnderWater(float);                      // empty
    virtual float getUnderWater() const;                     // 0.0f
    virtual void  setUnderwaterYComponent(float);            // empty
    virtual bool  getIsTreadingWater() const;                // false
    virtual void  reset(bool);                               // empty
    virtual bool  getIsSleeping() const;                     // true - always asleep
    virtual void  setIsSleeping();                           // empty
    virtual void  setSleepiness(int);                        // empty
    virtual int   getSleepiness() const;                     // -1
    virtual void  setIsSleepier();                           // empty
    virtual void  setIsAwake();                              // empty - it cannot be woken
    virtual bool  getIsMobile() const;                       // false - the manager's gate
    virtual bool  getIsPoint() const;                        // false
    virtual void  initLatencyCompensation();                 // empty
    virtual bool  hasLatencyCompensation();                  // false
    virtual void  updateLatencyState();                      // empty
    virtual const Mat4& getLatencyCompensatedTransformation(float, int*);   // = its own
                                                             //   getAbsoluteTransformation (the ELF
                                                             //   dispatches self slot +0x3C)
    virtual const Vec3& getAbsolutePositionalSpeed();        // Vec3::getZero
    virtual IPhysicsNode* getRootNode();                     // root parent's physics node
    virtual void  setHandleCollisionSpeed(float);            // +0x14
    virtual float getHandleCollisionSpeed() const;
    virtual const Vec3& getInverseInertia();                 // Vec3::getZero
    virtual void  addRelativeImpulse(const Vec3&, const Vec3&);   // empty
    virtual void  setFloaterMod(float);                      // empty
    virtual float getFloaterMod() const;                     // 0.0f
    virtual const Vec3& getLinearIntegrationVelocity() const;     // Vec3::getZero
    virtual void  setLinearIntegrationVelocity(const Vec3&);      // empty
    virtual const Vec3& getAngularIntegrationVelocity() const;    // Vec3::getZero
    virtual void  setAngularIntegrationVelocity(const Vec3&);     // empty
    virtual void  setTreeCollisionRadius(float);             // +0x18
    virtual float getTreeCollisionRadius() const;
    virtual void  postFrameUpdate();                         // empty
    virtual void  setLocalLinearSpeed(const Vec3&);          // empty
    virtual const Vec3& getLocalLinearSpeed() const;         // Vec3::getZero
    virtual void  setLocalAngularSpeed(const Vec3&);         // empty
    virtual const Vec3& getLocalAngularSpeed() const;        // Vec3::getZero
    virtual void  setOtherCollisionLod(int);                 // empty
    virtual int   getOtherCollisionLod() const;              // 1
    virtual bool  getShouldGravityCompensateOnContact() const;    // false
    virtual void  setHasContact(bool);                       // empty
    virtual bool  getHasContact() const;                     // false
    virtual void  setIsInAir(bool);                          // empty
    virtual bool  getIsInAir() const;                        // false
    virtual void  setSleepTimeout(int);                      // empty
    virtual void  setIsPlatformPhysics(bool);                // empty - platforms use PhysicsNode
    virtual bool  getIsPlatformPhysics() const;              // false

protected:
    // Derived mobile nodes read enabled/object fields directly but use their own reference
    // counts. This base refcount remains at its initial value for those derived nodes.
    /* +0x08 */ int     m_refCount;                  // ctor 1
    /* +0x0C */ bool    m_enabled;                   // ctor true (byte store)
    /* +0x10 */ IObject* m_object;                   // ctor 0
    /* +0x14 */ float   m_handleCollisionSpeed;      // ctor 1.0f
    /* +0x18 */ float   m_treeCollisionRadius;       // ctor 0
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_STATICPHYSICSNODE_H
