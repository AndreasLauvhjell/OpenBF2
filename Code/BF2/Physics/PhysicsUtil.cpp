// Object-subtree physics traversal and wheel helpers. [INFERRED] Source path.

#include "PhysicsUtil.h"

#include <vector>

#include "IPhysicsNode.h"
#include "IResponsePhysics.h"      // setSurfacePositionalSpeed - disablePhysics
#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IObjectTemplate.h"
#include "../Game/Objects/ObjectLayoutAccess.h"
#include "../Game/Objects/Fx/WheelEffectBundle.h"    // getWheelMaterials: m_material
#include "../Game/Objects/Game/RopeLink.h"           // RopeLink::setAwake
#include "../Game/Objects/Game/RotationalBundle.h"  // performYawStabilisation: applyStabilizedRotation
#include "../Scene/Object/ObjectManager.h"           // objectManager (0x8B1B920), getObjectsFromGrid
#include "../Scene/Object/ObjectPredicator.h"        // ObjectFlagPredicator
#include "../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // getOccupingObject - performHandleUpdate
#include "PhysicsNodeManager.h"                       // the +0x24 node update
#include "ResponsePhysicsManager.h"                   // the +0x20 per-object collision step

namespace dice {
namespace hfe {
namespace world {

// ds:0x8B27844, Core/Globals.cpp - attested. Declared per-TU, as ModuleRegistry.cpp,
// GameServer.cpp and SimpleObjectTemplate.cpp already do.
extern PhysicsNodeManager* physicsNodeManager;
extern ResponsePhysicsManager* responsePhysicsManager;   // ds:0x8B27848

typedef object_detail::LayoutAccess L;

static inline IPhysicsNode* nodeOf(IObject* object)
{
    return static_cast<IPhysicsNode*>(L::physicsNode(object));
}

// The function-local static vector is the ELF's (0x8ABA5E8, guarded, atexit).
void wakeUpObjects(const Vec3& position, float radius, const IObject* exclude, bool onlyAbove)
{
    static std::vector<IObject*> objects;
    objects.resize(0);
    ObjectFlagPredicator predicator(0x1000100u);
    objectManager->getObjectsFromGrid(position, radius, objects, &predicator);

    const unsigned int count = static_cast<unsigned int>(objects.size());
    for (unsigned int i = 0; i < count; ++i)
    {
        IObject* object = objects[i];
        if (object == exclude)
            continue;
        if (object->getTemplate()->getClassID() == CID_RopeLinkTemplate)
        {
            static_cast<RopeLink*>(object)->setAwake();
        }
        else if (object)
        {
            if (!onlyAbove || object->getAbsolutePosition().y > position.y)
            {
                IPhysicsNode* node = nodeOf(object);
                if (node)
                    node->setIsAwake();
            }
        }
    }
}

// Everything in the sphere (flags 0), only what lies above the position.
void wakeUpAllObjects(const Vec3& position, float radius, const IObject* exclude)
{
    static std::vector<IObject*> objects;
    objects.resize(0);
    ObjectFlagPredicator predicator(0u);
    objectManager->getObjectsFromGrid(position, radius, objects, &predicator);

    const unsigned int count = static_cast<unsigned int>(objects.size());
    for (unsigned int i = 0; i < count; ++i)
    {
        IObject* object = objects[i];
        if (object == exclude || !object)
            continue;
        if (object->getAbsolutePosition().y > position.y)
        {
            IPhysicsNode* node = nodeOf(object);
            if (node)
                node->setIsAwake();
        }
    }
}

// node, then the sibling chain (recursively), then down the child chain (loop).
void setSleepinessRecursive(IObject* object, int sleepiness)
{
    do
    {
        IPhysicsNode* node = nodeOf(object);
        if (node)
            node->setSleepiness(sleepiness);
        IObject* sibling = L::sibling(object);
        if (sibling)
            setSleepinessRecursive(sibling, sleepiness);
        object = object->getChild();
    }
    while (object);
}

// the same walk, setSleepTimeout.
void setSleepTimeoutRecursive(IObject* object, int timeout)
{
    do
    {
        IPhysicsNode* node = nodeOf(object);
        if (node)
            node->setSleepTimeout(timeout);
        IObject* sibling = L::sibling(object);
        if (sibling)
            setSleepTimeoutRecursive(sibling, timeout);
        object = object->getChild();
    }
    while (object);
}


// Selects objects whose template is a wheel-effect bundle.
bool WheelObjectPredicator::predicate(IObject* object)
{
    return object->getTemplate()->getClassID() == CID_WheelEffectBundleTemplate;   // template +0x10
}

bool findDriveWheels(WeakPtr<IObject>* first, WeakPtr<IObject>* second, IObject* root)
{
    std::vector<IObject*> wheels;
    WheelObjectPredicator predicator;
    getChilds(root, wheels, &predicator, 0);
    if (second)
    {
        if (wheels.size() <= 1)
            return false;
        *first  = wheels[0]->getWeakPtr();                                    // IObject +0x1C
        *second = wheels[1]->getWeakPtr();
        return true;
    }
    if (wheels.empty())
        return false;
    *first = wheels[0]->getWeakPtr();
    return true;
}

int getWheelMaterials(const WeakPtr<IObject>& wheel, int* outSurface)
{
    IObject* w = wheel.get();
    if (!w)
    {
        if (outSurface)
            *outSurface = 0;
        return -2;
    }
    if (outSurface)
        *outSurface = w->getMaterial();                                       // IObject slot 52 (+0xD0)
    // Reads the typed wheel-effect material so compiler-specific member offsets remain correct.
    return static_cast<const WheelEffectBundle*>(w)->m_material;
}

} // namespace world

// Parks physics in separate-update mode, clears response surface speed and selects update-frequency value 10.
// Recurses child-first; the player-control option filters children, not the current object.
void disablePhysics(world::IObject* object, bool includePlayerControlObjects)
{
    typedef world::object_detail::LayoutAccess L;

    world::IPhysicsNode* node =
        static_cast<world::IPhysicsNode*>(L::physicsNode(object));  // +0x44
    if (node)
        node->setHasSeparatePhysicsUpdate(true);                   // IPhysicsNode +0x100

    world::IResponsePhysics* response = static_cast<world::IResponsePhysics*>(
        object->queryComponent(world::IID_IResponsePhysics, world::IID_IResponsePhysics));   // slot 23
    if (response)
    {
        Vec3 stop;
        stop.x = 0.0f; stop.y = 0.0f; stop.z = 0.0f;
        response->setSurfacePositionalSpeed(stop);                 // IResponsePhysics +0x88
    }

    object->setUpdateFrequencyType(
        static_cast<world::UpdateFreqencyType>(10));               // slot 49 - see the note

    for (world::IObject* child = object->getChild(); child; child = L::sibling(child))
    {
        if (!includePlayerControlObjects &&
            child->queryInterface(world::IID_IPlayerControlObject))
            continue;
        disablePhysics(child, includePlayerControlObjects);
    }
}


// Restores normal frequency 0 and clears separate-update state and response surface speed.
// With includePlayerControlObjects false, skips player-control children while continuing siblings.
void reenablePhysics(world::IObject* object, bool includePlayerControlObjects)
{
    typedef world::object_detail::LayoutAccess L;

    world::IPhysicsNode* node =
        static_cast<world::IPhysicsNode*>(L::physicsNode(object));  // +0x44
    if (node)
        node->setHasSeparatePhysicsUpdate(false);                  // IPhysicsNode +0x100 false

    world::IResponsePhysics* response = static_cast<world::IResponsePhysics*>(
        object->queryComponent(world::IID_IResponsePhysics, world::IID_IResponsePhysics));   // slot 23
    if (response)
    {
        Vec3 stop;
        stop.x = 0.0f; stop.y = 0.0f; stop.z = 0.0f;
        response->setSurfacePositionalSpeed(stop);                 // IResponsePhysics +0x88
    }

    object->setUpdateFrequencyType(
        static_cast<world::UpdateFreqencyType>(0));                // slot 49 0, not 10

    for (world::IObject* child = object->getChild(); child; child = L::sibling(child))
    {
        if (!includePlayerControlObjects &&
            child->queryInterface(world::IID_IPlayerControlObject))
            continue;
        reenablePhysics(child, includePlayerControlObjects);
    }
}

// Tests update-frequency value 10, the physics-disabled state.
// resetPhysics below recursively resets nodes and response physics through the object's fields;
// unlike the enable/disable helpers, it does not query the response interface.
bool isPhysicsDisabled(world::IObject* object)
{
    return object->getUpdateFrequencyType() == static_cast<world::UpdateFreqencyType>(10);
}

void resetPhysics(world::IObject* object, bool includePlayerControlObjects)
{
    typedef world::object_detail::LayoutAccess L;

    world::IPhysicsNode* node =
        static_cast<world::IPhysicsNode*>(L::physicsNode(object));  // +0x44
    if (node)
        node->reset(true);                                         // IPhysicsNode +0x118

    world::IResponsePhysics* response =
        static_cast<world::IResponsePhysics*>(L::responsePhysics(object));   // +0x48, the field
    if (response)
        response->reset();                                         // IResponsePhysics +0x20

    for (world::IObject* child = object->getChild(); child; child = L::sibling(child))
    {
        if (!includePlayerControlObjects &&
            child->queryInterface(world::IID_IPlayerControlObject))
            continue;
        resetPhysics(child, includePlayerControlObjects);
    }
}

// Enables or parks node and response interfaces and updates separate-tick state.
// Always clears surface speed; preserves child order and optional control-object exclusion.
void turnPhysicsOn(world::IObject* object, bool includePlayerControlObjects)
{
    togglePhysicsOnOff(object, includePlayerControlObjects, true);
}

void turnPhysicsOff(world::IObject* object, bool includePlayerControlObjects)
{
    togglePhysicsOnOff(object, includePlayerControlObjects, false);
}

void togglePhysicsOnOff(world::IObject* object, bool includePlayerControlObjects, bool turnOn)
{
    typedef world::object_detail::LayoutAccess L;

    world::IPhysicsNode* const node =
        static_cast<world::IPhysicsNode*>(L::physicsNode(object));
    if (node)
    {
        node->setEnabled(turnOn);                         // +0x18
        node->setHasSeparatePhysicsUpdate(!turnOn);       // +0x100
    }

    world::IResponsePhysics* const response = static_cast<world::IResponsePhysics*>(
        object->queryComponent(world::IID_IResponsePhysics, world::IID_IResponsePhysics));
    if (response)
    {
        response->setEnabled(turnOn);                     // +0x18
        Vec3 zero;
        zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        response->setSurfacePositionalSpeed(zero);        // +0x88
    }

    object->setUpdateFrequencyType(
        static_cast<world::UpdateFreqencyType>(turnOn ? 0 : 10));

    for (world::IObject* child = object->getChild(); child; child = L::sibling(child))
    {
        if (!includePlayerControlObjects &&
            child->queryInterface(world::IID_IPlayerControlObject))
            continue;
        togglePhysicsOnOff(child, includePlayerControlObjects, turnOn);
    }
}


// Updates this subtree in pre-order. Flag 0x800 enables the current object's update;
// disabled children prune their whole subtree. includeControlObjects controls descent into
// player-control objects, soldiers and parachutes. Vehicle occupants that are soldiers also update.
// Requires an acyclic hierarchy; preserve the repeated player-control interface query.
void performHandleUpdate(float deltaTime, world::IObject* object, unsigned int flags,
                         bool includeControlObjects)
{
    if (!object)
        return;

    if (object->getFlags() & 0x800)                                    // IObject +0x04
        object->handleUpdate(deltaTime, flags);                        // slot 44

    if (object->queryInterface(world::IID_IPlayerControlObject) &&
        object->getTemplate()->getClassID() != world::CID_SoldierTemplate)
    {
        world::IPlayerControlObject* pco = static_cast<world::IPlayerControlObject*>(
            object->queryInterface(world::IID_IPlayerControlObject));   // asked twice
        if (pco)
        {
            world::IObject* occupant = pco->getOccupingObject();        // PCO +0x14
            if (occupant &&
                occupant->getTemplate()->getClassID() == world::CID_SoldierTemplate)
                performHandleUpdate(deltaTime, occupant, flags, includeControlObjects);
        }
    }

    for (world::IObject* child = object->getChild();                    // slot 60
         child;
         child = world::object_detail::LayoutAccess::sibling(child))    // +0x2C
    {
        if (child->getFlags() & 1)
            continue;                                                   // disabled - skip the subtree

        if (!includeControlObjects)
        {
            const unsigned int classId = child->getTemplate()->getClassID();
            if (classId == world::CID_PlayerControlObjectTemplate ||
                classId == world::CID_SoldierTemplate ||
                classId == world::CID_ParachuteTemplate)
                continue;                                               // updated in its own right
        }

        performHandleUpdate(deltaTime, child, flags, includeControlObjects);
    }
}


// Updates mobile physics in post-order through PhysicsNodeManager.
// A nonmobile node prunes its whole subtree; a node-less parent still visits children.
// Disabled/control-object child filters match the other subtree-update helpers.
void performMobilePhysicsUpdate(float deltaTime, world::IObject* object, bool includeControlObjects)
{
    if (!object)
        return;

    world::IPhysicsNode* node = static_cast<world::IPhysicsNode*>(
        world::object_detail::LayoutAccess::physicsNode(object));      // IObject +0x44

    if (node && !node->getIsMobile())                                  // +0x134 - the gate
        return;

    for (world::IObject* child = object->getChild();                   // slot 60
         child;
         child = world::object_detail::LayoutAccess::sibling(child))   // +0x2C
    {
        if (child->getFlags() & 1)
            continue;                                                  // disabled - skip the subtree

        if (!includeControlObjects)
        {
            const unsigned int classId = child->getTemplate()->getClassID();
            if (classId == world::CID_PlayerControlObjectTemplate ||
                classId == world::CID_SoldierTemplate ||
                classId == world::CID_ParachuteTemplate)
                continue;                                              // simulated in its own right
        }

        performMobilePhysicsUpdate(deltaTime, child, includeControlObjects);
    }

    // last, not first - see the post-order note above.
    if (node)
        world::physicsNodeManager->update(deltaTime, 0, node);         // +0x24
}


// Runs collision updates in pre-order, passing each object to ResponsePhysicsManager.
// The current object needs response physics and flag 0x100 (name [UNRESOLVED]); failing that test
// still visits its children. Disabled children prune the subtree; includeControlObjects controls
// descent into player-control objects, soldiers and parachutes.
void performMobileCollisionUpdate(float deltaTime, world::IObject* object,
                                  bool includeControlObjects)
{
    if (!object)
        return;

    // first, unlike its mobile-physics twin.
    if (world::object_detail::LayoutAccess::responsePhysics(object) &&      // IObject +0x48
        (object->getFlags() & 0x100))                                       // bit 8 - see the note
        world::responsePhysicsManager->update(deltaTime, object);           // +0x20

    for (world::IObject* child = object->getChild();                        // slot 60
         child;
         child = world::object_detail::LayoutAccess::sibling(child))        // +0x2C
    {
        if (child->getFlags() & 1)
            continue;                                                       // disabled - whole subtree

        if (!includeControlObjects)
        {
            const unsigned int classId = child->getTemplate()->getClassID();
            if (classId == world::CID_PlayerControlObjectTemplate ||
                classId == world::CID_SoldierTemplate ||
                classId == world::CID_ParachuteTemplate)
                continue;                                                   // stepped in its own right
        }

        performMobileCollisionUpdate(deltaTime, child, includeControlObjects);
    }
}

// Applies per-step yaw stabilization to the local control object when present.
void performYawStabilisation(world::IObject* object)
{
    typedef world::object_detail::LayoutAccess L;

    if (!object)
        return;

    // OFHasUpdates (bit 11) - the `test byte [obj+5], 8`. RotationalBundle.h records that a
    // bundle only takes this flag when its template auto-rotates on enter or rotates continuously,
    // so the plain turrets and hubs are skipped here for free.
    if (object->getFlags() & 0x800u)
    {
        if (const world::IObjectTemplate* const objectTemplate = object->getTemplate())   // +0x14
        {
            if (objectTemplate->getClassID() == world::CID_RotationalBundleTemplate)      // 38013
                static_cast<world::RotationalBundle*>(object)->applyStabilizedRotation();  // ELF +0x198
        }
    }

    for (world::IObject* child = object->getChild();                     // ELF +0xF0 / PE +0xEC
         child;
         child = L::sibling(child))                                      // +0x2C
    {
        if (child->getFlags() & 1u)
            continue;                                                    // OFIsDisabled - whole subtree

        // not null-tested here, unlike the object's own template above. Shipped asymmetry.
        const unsigned int classId = child->getTemplate()->getClassID();
        if (classId == world::CID_PlayerControlObjectTemplate ||
            classId == world::CID_SoldierTemplate ||
            classId == world::CID_ParachuteTemplate)
            continue;                                                    // stepped in its own right

        performYawStabilisation(child);
    }
}

} // namespace hfe
} // namespace dice
