// Physics tree traversal and nearby-object wake helpers. Header path [INFERRED].
// wakeUpObjects filters root response objects and handles ropes specially; wakeUpAllObjects always
// requires objects above the supplied position. Both reuse function-static candidate vectors.
// Recursive sleep setters visit the object node, sibling subtrees and then the child chain.

#ifndef PHYSICS_PHYSICSUTIL_H     // [INFERRED] guard style
#define PHYSICS_PHYSICSUTIL_H

#include "../Math/Vec3.h"

#include "../Core/WeakPtr.h"
#include "../Scene/Object/ObjectPredicator.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;

void wakeUpObjects(const Vec3& position, float radius, const IObject* exclude, bool onlyAbove);
void wakeUpAllObjects(const Vec3& position, float radius, const IObject* exclude);
void setSleepinessRecursive(IObject* object, int sleepiness);
void setSleepTimeoutRecursive(IObject* object, int timeout);

// Filters WheelEffectBundleTemplate objects for drive-wheel queries.
// findDriveWheels assigns one or two outputs only when enough wheels exist. getWheelMaterials reads
// current ground material and optional object material; no wheel yields -2/0.
class WheelObjectPredicator : public ObjectPredicator
{
public:
    virtual bool         predicate(IObject* object);
    virtual unsigned int getFlags() const { return 0; }
    virtual bool         performCulling(IObject*) { return false; }
};

bool findDriveWheels(WeakPtr<IObject>* first, WeakPtr<IObject>* second, IObject* root);
int  getWheelMaterials(const WeakPtr<IObject>& wheel, int* outSurface);

} // namespace world

// Disables physics recursively. With includePlayerControlObjects false, traversal stops at
// player-control children so vehicle changes do not affect passengers' independent vehicles.
// These helpers belong to dice::hfe; their grouping in this header is a placement choice.
void disablePhysics(world::IObject* object, bool includePlayerControlObjects);
void reenablePhysics(world::IObject* object, bool includePlayerControlObjects);
void resetPhysics(world::IObject* object, bool includePlayerControlObjects);

// , and it is one comparison: the object's update frequency
// type (slot +0xC0) equal to 10. That resolves the magic 10 this file already flagged: the
// disable path sets frequency type 10 and the enable path sets 0, so the "type" doubles as the
// physics-disabled flag and there is no separate bit. Soldier::addItem is its first written caller.
bool isPhysicsDisabled(world::IObject* object);

// The on/off pair over one shared walk. Both wrappers and the 280-byte walk were read in full;
// togglePhysicsOnOff switches the physics node and response component, clears carried surface
// speed, changes update-frequency type (0 on / 10 off), then recurses with the PCO child filter.
void turnPhysicsOn(world::IObject* object, bool includePlayerControlObjects);
void turnPhysicsOff(world::IObject* object, bool includePlayerControlObjects);
void togglePhysicsOnOff(world::IObject* object, bool includePlayerControlObjects,
                        bool turnOn);

// Updates an object subtree with deltaTime first in the interface. When control-object
// descent is disabled, stops at child player-control objects, soldiers, and parachutes
// because they receive separate update passes.
void performHandleUpdate(float deltaTime, world::IObject* object, unsigned int flags,
                         bool includeControlObjects);

// Yaw stabilization applies to rotational bundles and requires valid child templates.
// Mobile-physics updates use post-order and skip whole non-mobile subtrees; handle and
// collision updates use pre-order. Preserve shared child/occupant filters and repeated queries.
void performYawStabilisation(world::IObject* object);

void performMobilePhysicsUpdate(float deltaTime, world::IObject* object,
                                bool includeControlObjects);

// Collision updates are pre-order and gate only the current object on response/flag 0x100.
// Handle updates likewise gate only the object; mobile-physics updates are post-order and a
// nonmobile node prunes the entire subtree. Preserve those distinct traversal contracts.
void performMobileCollisionUpdate(float deltaTime, world::IObject* object,
                                  bool includeControlObjects);

} // namespace hfe
} // namespace dice

#endif // PHYSICS_PHYSICSUTIL_H
