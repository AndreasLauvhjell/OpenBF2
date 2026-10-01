// Shared physics-node interface. Header path [INFERRED]; virtual declaration order is ABI-sensitive.
// The stored list iterator identifies membership in PhysicsNodeManager's mobile-node list.
// The manager must exist before node construction. Remove a listed node before destruction:
// the destructor aborts if the iterator is not the manager's end sentinel.

#ifndef PHYSICS_IPHYSICSNODE_H     // [INFERRED] guard style
#define PHYSICS_IPHYSICSNODE_H

#include <list>

#include "../Core/IBase.h"
#include "../Math/Vec3.h"
#include "../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;

class IPhysicsNode : public IBase
{
public:
    IPhysicsNode();                                  // see the header block
    // +0x10 / +0x14. Declared here (with a body) so GCC's rule places it; the abort check lives
    // in the .cpp.
    virtual ~IPhysicsNode();

    // +0x18.. the 106 added slots, in ELF vtable order
    virtual void  setEnabled(bool) = 0;                                        // +0x18
    virtual bool  getEnabled() const = 0;                                      // +0x1C
    virtual void  setIsDamaged(bool) = 0;                                      // +0x20
    virtual bool  getIsDamaged() const = 0;                                    // +0x24
    virtual void  setObject(IObject*) = 0;                                     // +0x28
    virtual IObject* getObject() const = 0;                                    // +0x2C
    virtual void  setParent(IPhysicsNode*) = 0;                                // +0x30
    virtual IPhysicsNode* getParent() const = 0;                               // +0x34
    virtual float getBoundingRadius() const = 0;                               // +0x38
    virtual const Mat4& getAbsoluteTransformation() const = 0;                 // +0x3C
    virtual void  setAbsoluteTransformation(const Mat4&) = 0;                  // +0x40
    virtual const Mat4& getRelativeTransformation() const = 0;                 // +0x44
    virtual const Vec3& getAbsolutePosition() const = 0;                       // +0x48
    virtual void  setAbsolutePosition(const Vec3&) = 0;                        // +0x4C
    virtual const Mat4& getPrevTransformation() const = 0;                     // +0x50
    virtual void  setPrevTransformation(const Mat4&) = 0;                      // +0x54
    virtual void  setOldPosition(const Vec3&) = 0;                             // +0x58
    virtual const Vec3& getRelativePosition() const = 0;                       // +0x5C
    virtual const Vec3& getPositionalSpeed() const = 0;                        // +0x60
    virtual const Vec3& getRotationalSpeed() const = 0;                        // +0x64
    virtual void  setPositionalSpeed(const Vec3&) = 0;                         // +0x68
    virtual void  setRotationalSpeed(const Vec3&) = 0;                         // +0x6C
    virtual void  addPositionalSpeed(const Vec3&) = 0;                         // +0x70
    virtual void  addRotationalSpeed(const Vec3&) = 0;                         // +0x74
    virtual void  setPositionalAcceleration(const Vec3&) = 0;                  // +0x78
    virtual void  setRotationalAcceleration(const Vec3&) = 0;                  // +0x7C
    virtual void  setPositionalFriction(const Vec3&) = 0;                      // +0x80
    virtual void  setRotationalFriction(const Vec3&) = 0;                      // +0x84
    virtual void  addSpeedAtAbsolutePosition(const Vec3&, const Vec3&) = 0;    // +0x88
    virtual void  addSpeedAtRelativePosition(const Vec3&, const Vec3&) = 0;    // +0x8C
    virtual void  addAccelerationAtAbsolutePosition(const Vec3&, const Vec3&) = 0;   // +0x90
    virtual void  addAccelerationAtRelativePosition(const Vec3&, const Vec3&) = 0;   // +0x94
    virtual void  addFrictionAtAbsolutePosition(const Vec3&, const Vec3&) = 0; // +0x98
    virtual void  addPositionalAccelerationToRootParent(const Vec3&) = 0;      // +0x9C
    virtual void  addRotationalAccelerationAtAbsolutePosition(const Vec3&, const Vec3&) = 0; // +0xA0
    // NON-const, per the mangling (_ZN, not _ZNK) - unusual for getters, preserved.
    virtual const Vec3& getTangentSpeed(const Vec3&) = 0;                      // +0xA4
    virtual const Vec3& getTangentAcceleration(const Vec3&) = 0;               // +0xA8
    virtual const Vec3& getPositionalAcceleration() const = 0;                 // +0xAC
    virtual const Vec3& getRotationalAcceleration() const = 0;                 // +0xB0
    virtual const Vec3& getPositionalFriction() const = 0;                     // +0xB4
    virtual const Vec3& getRotationalFriction() const = 0;                     // +0xB8
    virtual void  updatePhysics(float dt) = 0;                                 // +0xBC - the slot
                                                                               //   the manager's
                                                                               //   update loop calls
    virtual void  setDrag(float) = 0;                                          // +0xC0
    virtual float getDrag() const = 0;                                         // +0xC4
    virtual void  setDragOffset(const Vec3&) = 0;                              // +0xC8
    virtual const Vec3& getDragOffset() const = 0;                             // +0xCC
    virtual void  setDragModifier(const Vec3&) = 0;                            // +0xD0
    virtual const Vec3& getDragModifier() const = 0;                           // +0xD4
    virtual void  setMass(float) = 0;                                          // +0xD8
    virtual float getMass() const = 0;                                         // +0xDC
    virtual void  setCenterOfMassOffset(const Vec3&) = 0;                      // +0xE0
    virtual const Vec3& getCenterOfMassOffset() const = 0;                     // +0xE4
    virtual void  setCenterOfCollisionOffset(const Vec3&) = 0;                 // +0xE8
    virtual const Vec3& getCenterOfCollisionOffset() const = 0;                // +0xEC
    virtual void  setInertiaModifier(const Vec3&) = 0;                         // +0xF0
    virtual const Vec3& getInertiaModifier() const = 0;                        // +0xF4
    virtual void  setGravityModifier(float) = 0;                               // +0xF8
    virtual float getGravityModifier() const = 0;                              // +0xFC
    virtual void  setHasSeparatePhysicsUpdate(bool) = 0;                       // +0x100
    virtual bool  getHasSeparatePhysicsUpdate() const = 0;                     // +0x104
    virtual void  setUnderWater(float) = 0;                                    // +0x108
    virtual float getUnderWater() const = 0;                                   // +0x10C
    virtual void  setUnderwaterYComponent(float) = 0;                          // +0x110
    virtual bool  getIsTreadingWater() const = 0;                              // +0x114
    virtual void  reset(bool) = 0;                                             // +0x118
    virtual bool  getIsSleeping() const = 0;                                   // +0x11C
    virtual void  setIsSleeping() = 0;                                         // +0x120
    virtual void  setSleepiness(int) = 0;                                      // +0x124
    virtual int   getSleepiness() const = 0;                                   // +0x128
    virtual void  setIsSleepier() = 0;                                         // +0x12C
    virtual void  setIsAwake() = 0;                                            // +0x130
    virtual bool  getIsMobile() const = 0;                                     // +0x134 - the gate
                                                                               //   addPhysicsNode
                                                                               //   filters on
    virtual bool  getIsPoint() const = 0;                                      // +0x138
    virtual void  initLatencyCompensation() = 0;                               // +0x13C
    virtual bool  hasLatencyCompensation() = 0;                                // +0x140 (non-const)
    virtual void  updateLatencyState() = 0;                                    // +0x144
    virtual const Mat4& getLatencyCompensatedTransformation(float, int*) = 0;  // +0x148
    virtual const Vec3& getAbsolutePositionalSpeed() = 0;                      // +0x14C (non-const)
    virtual IPhysicsNode* getRootNode() = 0;                                   // +0x150
    virtual void  setHandleCollisionSpeed(float) = 0;                          // +0x154
    virtual float getHandleCollisionSpeed() const = 0;                         // +0x158
    virtual const Vec3& getInverseInertia() = 0;                               // +0x15C (non-const)
    virtual void  addRelativeImpulse(const Vec3&, const Vec3&) = 0;            // +0x160
    virtual void  setFloaterMod(float) = 0;                                    // +0x164
    virtual float getFloaterMod() const = 0;                                   // +0x168
    virtual const Vec3& getLinearIntegrationVelocity() const = 0;              // +0x16C
    virtual void  setLinearIntegrationVelocity(const Vec3&) = 0;               // +0x170
    virtual const Vec3& getAngularIntegrationVelocity() const = 0;             // +0x174
    virtual void  setAngularIntegrationVelocity(const Vec3&) = 0;              // +0x178
    virtual void  setTreeCollisionRadius(float) = 0;                           // +0x17C
    virtual float getTreeCollisionRadius() const = 0;                          // +0x180
    virtual void  postFrameUpdate() = 0;                                       // +0x184
    virtual void  setLocalLinearSpeed(const Vec3&) = 0;                        // +0x188
    virtual const Vec3& getLocalLinearSpeed() const = 0;                       // +0x18C
    virtual void  setLocalAngularSpeed(const Vec3&) = 0;                       // +0x190
    virtual const Vec3& getLocalAngularSpeed() const = 0;                      // +0x194
    virtual void  setOtherCollisionLod(int) = 0;                               // +0x198
    virtual int   getOtherCollisionLod() const = 0;                            // +0x19C
    virtual bool  getShouldGravityCompensateOnContact() const = 0;             // +0x1A0
    virtual void  setHasContact(bool) = 0;                                     // +0x1A4
    virtual bool  getHasContact() const = 0;                                   // +0x1A8
    virtual void  setIsInAir(bool) = 0;                                        // +0x1AC
    virtual bool  getIsInAir() const = 0;                                      // +0x1B0
    virtual void  setSleepTimeout(int) = 0;                                    // +0x1B4
    virtual void  setIsPlatformPhysics(bool) = 0;                              // +0x1B8
    virtual bool  getIsPlatformPhysics() const = 0;                            // +0x1BC

protected:
    // See the header block. addPhysicsNode/removePhysicsNode manipulate it directly, exactly as
    // the ELF does; the friendship is the [CHOSEN] access mechanism (dice's own access route is
    // not observable from the binary).
    friend class PhysicsNodeManager;
    /* +0x04 */ std::list<IPhysicsNode*>::iterator m_listPosition;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_IPHYSICSNODE_H
