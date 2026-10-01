// Static physics behavior with intentional empty operations and constant results.
// [INFERRED] Source placement beside the header.

#include "StaticPhysicsNode.h"

#include <cfloat>                                   // FLT_MAX - getMass, ds:0x888FFE8

#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/ObjectLayoutAccess.h"

namespace dice {
namespace hfe {
namespace world {

typedef object_detail::LayoutAccess L;

// IPhysicsNode ctor (which claims end() from the manager), then the five fields.
StaticPhysicsNode::StaticPhysicsNode()
{
    m_refCount = 1;                                  // +0x08
    m_enabled = true;                                // +0x0C
    m_object = 0;                                    // +0x10
    m_handleCollisionSpeed = 1.0f;                   // +0x14
    m_treeCollisionRadius = 0.0f;                    // +0x18
}

// Trivial - the base dtor carries the still-listed abort check, which cannot fire here.
StaticPhysicsNode::~StaticPhysicsNode() {}

void StaticPhysicsNode::addRef()  { ++m_refCount; }
int  StaticPhysicsNode::getRef()  { return m_refCount; }
int  StaticPhysicsNode::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

// 32 B, two cases - no IID_IPhysicsNodeManager here, the node and the manager are separate
// interfaces.
void* StaticPhysicsNode::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)         return const_cast<StaticPhysicsNode*>(this);
    if (iid == IID_IPhysicsNode)  return const_cast<StaticPhysicsNode*>(this);
    return 0;
}

void StaticPhysicsNode::setEnabled(bool v)          { m_enabled = v; }
bool StaticPhysicsNode::getEnabled() const          { return m_enabled; }
void StaticPhysicsNode::setIsDamaged(bool)          {}
bool StaticPhysicsNode::getIsDamaged() const        { return false; }
void StaticPhysicsNode::setObject(IObject* o)       { m_object = o; }
IObject* StaticPhysicsNode::getObject() const       { return m_object; }
void StaticPhysicsNode::setParent(IPhysicsNode*)    {}

// 23 B: the parent is not stored - it is the object tree's parent, asked for its node (+0x44).
IPhysicsNode* StaticPhysicsNode::getParent() const
{
    IObject* p = L::parent(m_object);
    return p ? static_cast<IPhysicsNode*>(L::physicsNode(p)) : 0;
}

// The object delegations - a static node's geometry is its object's
// IObject slot 38's declared name is getPhysicBoundingRadius - the node's slot is named
// getBoundingRadius. Two names, one delegation; both are the ELF's.
float StaticPhysicsNode::getBoundingRadius() const                { return m_object->getPhysicBoundingRadius(); }
const Mat4& StaticPhysicsNode::getAbsoluteTransformation() const  { return m_object->getAbsoluteTransformation(); }
void StaticPhysicsNode::setAbsoluteTransformation(const Mat4& m)  { m_object->setAbsoluteTransformation(m); }
const Mat4& StaticPhysicsNode::getRelativeTransformation() const  { return m_object->getRelativeTransformation(); }
const Vec3& StaticPhysicsNode::getAbsolutePosition() const        { return m_object->getAbsolutePosition(); }
void StaticPhysicsNode::setAbsolutePosition(const Vec3& v)        { m_object->setAbsolutePosition(v); }
// the same slot as getAbsoluteTransformation - IObject slot 33 in both bodies. A static
// object's previous transform is its current one, definitionally. Not a copy-paste slip: the two
// functions are separate 27-byte bodies in the ELF, both dispatching +0x84.
const Mat4& StaticPhysicsNode::getPrevTransformation() const      { return m_object->getAbsoluteTransformation(); }
void StaticPhysicsNode::setPrevTransformation(const Mat4&)        {}
void StaticPhysicsNode::setOldPosition(const Vec3&)               {}
const Vec3& StaticPhysicsNode::getRelativePosition() const        { return m_object->getRelativePosition(); }

// The zero/constant surface
const Vec3& StaticPhysicsNode::getPositionalSpeed() const         { return Vec3::getZero(); }
const Vec3& StaticPhysicsNode::getRotationalSpeed() const         { return Vec3::getZero(); }
void StaticPhysicsNode::setPositionalSpeed(const Vec3&)           {}
void StaticPhysicsNode::setRotationalSpeed(const Vec3&)           {}
void StaticPhysicsNode::addPositionalSpeed(const Vec3&)           {}
void StaticPhysicsNode::addRotationalSpeed(const Vec3&)           {}
void StaticPhysicsNode::setPositionalAcceleration(const Vec3&)    {}
void StaticPhysicsNode::setRotationalAcceleration(const Vec3&)    {}
void StaticPhysicsNode::setPositionalFriction(const Vec3&)        {}
void StaticPhysicsNode::setRotationalFriction(const Vec3&)        {}
void StaticPhysicsNode::addSpeedAtAbsolutePosition(const Vec3&, const Vec3&)        {}
void StaticPhysicsNode::addSpeedAtRelativePosition(const Vec3&, const Vec3&)        {}
void StaticPhysicsNode::addAccelerationAtAbsolutePosition(const Vec3&, const Vec3&) {}
void StaticPhysicsNode::addAccelerationAtRelativePosition(const Vec3&, const Vec3&) {}
void StaticPhysicsNode::addFrictionAtAbsolutePosition(const Vec3&, const Vec3&)     {}
void StaticPhysicsNode::addPositionalAccelerationToRootParent(const Vec3&)          {}
void StaticPhysicsNode::addRotationalAccelerationAtAbsolutePosition(const Vec3&, const Vec3&) {}
const Vec3& StaticPhysicsNode::getTangentSpeed(const Vec3&)        { return Vec3::getZero(); }
const Vec3& StaticPhysicsNode::getTangentAcceleration(const Vec3&) { return Vec3::getZero(); }
const Vec3& StaticPhysicsNode::getPositionalAcceleration() const   { return Vec3::getZero(); }
const Vec3& StaticPhysicsNode::getRotationalAcceleration() const   { return Vec3::getZero(); }
const Vec3& StaticPhysicsNode::getPositionalFriction() const       { return Vec3::getZero(); }
const Vec3& StaticPhysicsNode::getRotationalFriction() const       { return Vec3::getZero(); }
void StaticPhysicsNode::updatePhysics(float)                       {}
void StaticPhysicsNode::setDrag(float)                             {}
float StaticPhysicsNode::getDrag() const                           { return 0.0f; }
void StaticPhysicsNode::setDragOffset(const Vec3&)                 {}
const Vec3& StaticPhysicsNode::getDragOffset() const               { return Vec3::getZero(); }
void StaticPhysicsNode::setDragModifier(const Vec3&)               {}
const Vec3& StaticPhysicsNode::getDragModifier() const             { return Vec3::getZero(); }
void StaticPhysicsNode::setMass(float)                             {}
float StaticPhysicsNode::getMass() const                           { return FLT_MAX; }
void StaticPhysicsNode::setCenterOfMassOffset(const Vec3&)         {}
const Vec3& StaticPhysicsNode::getCenterOfMassOffset() const       { return Vec3::getZero(); }
void StaticPhysicsNode::setCenterOfCollisionOffset(const Vec3&)    {}
const Vec3& StaticPhysicsNode::getCenterOfCollisionOffset() const  { return Vec3::getZero(); }
void StaticPhysicsNode::setInertiaModifier(const Vec3&)            {}
const Vec3& StaticPhysicsNode::getInertiaModifier() const          { return Vec3::getIdentity(); }
void StaticPhysicsNode::setGravityModifier(float)                  {}
float StaticPhysicsNode::getGravityModifier() const                { return 1.0f; }
void StaticPhysicsNode::setHasSeparatePhysicsUpdate(bool)          {}
bool StaticPhysicsNode::getHasSeparatePhysicsUpdate() const        { return false; }
void StaticPhysicsNode::setUnderWater(float)                       {}
float StaticPhysicsNode::getUnderWater() const                     { return 0.0f; }
void StaticPhysicsNode::setUnderwaterYComponent(float)             {}
bool StaticPhysicsNode::getIsTreadingWater() const                 { return false; }
void StaticPhysicsNode::reset(bool)                                {}
bool StaticPhysicsNode::getIsSleeping() const                      { return true; }
void StaticPhysicsNode::setIsSleeping()                            {}
void StaticPhysicsNode::setSleepiness(int)                         {}
int  StaticPhysicsNode::getSleepiness() const                      { return -1; }
void StaticPhysicsNode::setIsSleepier()                            {}
void StaticPhysicsNode::setIsAwake()                               {}
bool StaticPhysicsNode::getIsMobile() const                        { return false; }
bool StaticPhysicsNode::getIsPoint() const                         { return false; }
void StaticPhysicsNode::initLatencyCompensation()                  {}
bool StaticPhysicsNode::hasLatencyCompensation()                   { return false; }
void StaticPhysicsNode::updateLatencyState()                       {}

// 21 B: the ELF dispatches self slot +0x3C - i.e. its own getAbsoluteTransformation, virtually.
const Mat4& StaticPhysicsNode::getLatencyCompensatedTransformation(float, int*)
{
    return getAbsoluteTransformation();
}

const Vec3& StaticPhysicsNode::getAbsolutePositionalSpeed()        { return Vec3::getZero(); }

// 27 B: getRootParent(m_object)->m_physicsNode. no null check on the root in the ELF -
// preserved. m_object is set at spawn, and a spawned object always has a root (itself at worst).
IPhysicsNode* StaticPhysicsNode::getRootNode()
{
    return static_cast<IPhysicsNode*>(L::physicsNode(getRootParent(m_object)));
}

void StaticPhysicsNode::setHandleCollisionSpeed(float v)           { m_handleCollisionSpeed = v; }
float StaticPhysicsNode::getHandleCollisionSpeed() const           { return m_handleCollisionSpeed; }
const Vec3& StaticPhysicsNode::getInverseInertia()                 { return Vec3::getZero(); }
void StaticPhysicsNode::addRelativeImpulse(const Vec3&, const Vec3&) {}
void StaticPhysicsNode::setFloaterMod(float)                       {}
float StaticPhysicsNode::getFloaterMod() const                     { return 0.0f; }
const Vec3& StaticPhysicsNode::getLinearIntegrationVelocity() const  { return Vec3::getZero(); }
void StaticPhysicsNode::setLinearIntegrationVelocity(const Vec3&)  {}
const Vec3& StaticPhysicsNode::getAngularIntegrationVelocity() const { return Vec3::getZero(); }
void StaticPhysicsNode::setAngularIntegrationVelocity(const Vec3&) {}
void StaticPhysicsNode::setTreeCollisionRadius(float v)            { m_treeCollisionRadius = v; }
float StaticPhysicsNode::getTreeCollisionRadius() const            { return m_treeCollisionRadius; }
void StaticPhysicsNode::postFrameUpdate()                          {}
void StaticPhysicsNode::setLocalLinearSpeed(const Vec3&)           {}
const Vec3& StaticPhysicsNode::getLocalLinearSpeed() const         { return Vec3::getZero(); }
void StaticPhysicsNode::setLocalAngularSpeed(const Vec3&)          {}
const Vec3& StaticPhysicsNode::getLocalAngularSpeed() const        { return Vec3::getZero(); }
void StaticPhysicsNode::setOtherCollisionLod(int)                  {}
int  StaticPhysicsNode::getOtherCollisionLod() const               { return 1; }
bool StaticPhysicsNode::getShouldGravityCompensateOnContact() const { return false; }
void StaticPhysicsNode::setHasContact(bool)                        {}
bool StaticPhysicsNode::getHasContact() const                      { return false; }
void StaticPhysicsNode::setIsInAir(bool)                           {}
bool StaticPhysicsNode::getIsInAir() const                         { return false; }
void StaticPhysicsNode::setSleepTimeout(int)                       {}
void StaticPhysicsNode::setIsPlatformPhysics(bool)                 {}
bool StaticPhysicsNode::getIsPlatformPhysics() const               { return false; }

} // namespace world
} // namespace hfe
} // namespace dice
