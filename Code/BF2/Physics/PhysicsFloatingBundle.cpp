// Floating-bundle forces and physics integration. [INFERRED] Source path.

#include "PhysicsFloatingBundle.h"

#include "../Core/Ids.h"
#include "BasicPhysicsSystem.h"                                  // basicPhysicsSystem->getGravity
#include "HeightmapCluster.h"                                    // getWaterLevelInWorldCoords (+0x98)
#include "ICollisionMesh.h"                                      // hasLod / getBoundingBox
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/ObjectLayoutAccess.h"                  // the root's +0x40 / +0x44
#include "../Game/Objects/RotationalBundleLeaves.h"              // FloatingBundleTemplate's fields
#include "../Math/LocalBoundingBox.h"                            // the lod-1 footprint

namespace dice {
namespace hfe {
namespace world {

extern IBase* basicPhysicsSystem;                                // ds:0x8B2784C - Core/Globals.cpp
extern IBase* heightmapCluster;                                  // ds:0x8ABAF24 - Core/Globals.cpp

// PhysicsNode(); the vptr; count 1; the three own words 0; the template.
PhysicsFloatingBundle::PhysicsFloatingBundle(const FloatingBundleTemplate* floatingTemplate, const IObject*)
    : PhysicsNode()
    , m_refCount(1)                                                                  // +0x22C
    , m_template(floatingTemplate)                                                   // +0x230
    , m_floaterValue(0.0f)                                                           // +0x234
    , m_float238(0.0f)
    , m_float23C(0.0f)
{
}

PhysicsFloatingBundle::~PhysicsFloatingBundle() {}                                   // 0x82AA580: ~PhysicsNode

// The quartet - 0x82AA5A0 / 0x82AA5C0 / 0x82AA5D0 / 0x82AA610.
void PhysicsFloatingBundle::addRef()  { ++m_refCount; }
int  PhysicsFloatingBundle::getRef()  { return m_refCount; }
int  PhysicsFloatingBundle::release()
{
    const int left = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = left;
    if (last)
        delete this;                                                                 // vslot +0x14
    return left;
}

void* PhysicsFloatingBundle::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IPhysicsNode)
        return const_cast<PhysicsFloatingBundle*>(this);
    return 0;
}


// Applies buoyancy and vertical water drag to the root at this float's position; dt is unused.
// Requires a parent, follows root sleepiness and wakes the parent for active depth controls.
// Submersion is negative: getDragMod receives [-1,0), so its drag term can change sign.
// Preserve that arithmetic and repeated root queries.
void PhysicsFloatingBundle::updatePhysics(float)                                     // dt is unread
{
    if (m_floaterValue != 0.0f)                                                      // +0x234
        getParent()->setIsAwake();                                                   // +0x34 -> +0x130

    if (m_float23C != 0.0f)                                                          // +0x23C
    {
        m_float238 = m_float23C + m_float238;
        getParent()->setIsAwake();
    }

    if (!getRootNode())                                                              // +0x150
        return;
    setSleepiness(getRootNode()->getSleepiness());                                    // +0x124 <- +0x128
    if (getIsSleeping())                                                             // +0x11C
        return;

    const Vec3& here = getAbsolutePosition();                                        // +0x48
    const float water = static_cast<IHeightmapCluster*>(heightmapCluster)
                            ->getWaterLevelInWorldCoords(here.x, here.z);            // +0x98
    const float hull = m_template->m_hullHeight;                                     // template +0x218
    const float fraction = (here.y - hull - water + m_float238) / hull;

    float blend = -(hull * fraction - m_floaterValue);
    if (blend >= 1.0f) blend = 1.0f;
    if (!(blend >= 0.0f)) blend = 0.0f;

    if (fraction >= 0.0f)                                                            // dry - nothing to do
        return;

    float depth = fraction;
    if (depth < -1.0f) depth = -1.0f;

    const float lift = (1.0f - blend) * m_template->m_floatMinLift                   // template +0x220
                     + blend * m_template->m_floatMaxLift;                           // template +0x21C
    const float buoyancy = static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity()   // +0x20
                         * (-depth * lift) / -9.8199997f;

    IObject* rootObject = getRootParent(m_object);
    if (!rootObject)
        return;

    const Vec3 at = getAbsolutePosition();                                           // read again, as compiled

    ICollisionMesh* mesh = static_cast<ICollisionMesh*>(
        object_detail::LayoutAccess::collisionMesh(rootObject));                     // object +0x40
    if (!mesh || !mesh->hasLod(1))                                                   // slot 11
        return;

    LocalBoundingBox* box = const_cast<LocalBoundingBox*>(
        static_cast<const LocalBoundingBox*>(mesh->getBoundingBox(1)));              // slot 22
    if (box->m_derivedDataDirty)
        box->calcRadius();
    const float footprint = box->m_size.z * box->m_size.x;

    IPhysicsNode* rootNode = static_cast<IPhysicsNode*>(
        object_detail::LayoutAccess::physicsNode(rootObject));                       // object +0x44

    const float verticalSpeed = rootNode->getTangentSpeed(at).y;                     // +0xA4
    const float area = getDragMod(depth) * 10.0f * footprint;                        // negative arg
    float drag = rootNode->getDrag() * area / rootNode->getMass();                   // +0xC4, +0xDC

    static const float maxDragModInWater = 2.0f;                                     // updatePhysics(float)::maxDragModInWater
    if (-maxDragModInWater > drag)     drag = -maxDragModInWater;
    else if (drag > maxDragModInWater) drag = maxDragModInWater;

    Vec3 force;
    force.x = 0.0f;
    force.y = drag * verticalSpeed + buoyancy;
    force.z = 0.0f;
    rootNode->addAccelerationAtAbsolutePosition(at, force);                          // +0x90
}

} // namespace world
} // namespace hfe
} // namespace dice
