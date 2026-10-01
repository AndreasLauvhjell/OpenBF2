// [INFERRED] Source placement.

#include "AITemplatePhysical.h"

#include "AIObjectPhysical.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)
#include "IAIObject.h"

#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"
#include "../../../Game/Objects/DestroyableObjectTemplate.h" // usedByAi (+0x188)
#include "../../../Game/Objects/GameLogicComp/IArmorGLComp.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../Game/Objects/ObjectLayoutAccess.h"        // IObject +0x40, the collision mesh
#include "../../../Math/LocalBoundingBox.h"
#include "../../../Physics/ICollisionMesh.h"
#include "../../../Physics/IPhysicsNode.h"

namespace dice {
namespace hfe {
namespace ai {

// , - every field zero, in field order.
AITemplatePhysical::AITemplatePhysical()
    : m_maxHealth(0.0f), m_mass(0.0f), m_volume(0.0f), m_bestArmour(0.0f), m_radius(0.0f),
      m_strType(0), m_initialized(false), m_affectsPathfinding(false)
{
}

// - the vptr store and, for , the delete. Nothing else.
AITemplatePhysical::~AITemplatePhysical()
{
}

// createPlugIn - , : `new AIObjectPhysical(object, this)` (0x18 B) through
// checkPointer. The AITemplate argument is unused.
IAIObjectPlugIn* AITemplatePhysical::createPlugIn(IAIObject* object, AITemplate* /*aiTemplate*/)
{
    return checkPointer(new AIObjectPhysical(object, this));
}

// Initializes physical properties from the first live object outside precaching.
void AITemplatePhysical::initFromObject(IAIObject* object)
{
    if (m_initialized ||
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getIsPrecaching())     // +0x68
        return;

    world::IArmorGLComp* armor = 0;
    for (world::IObject* o = object->getObject(); o; o = o->getParent())
    {
        armor = static_cast<world::IArmorGLComp*>(o->getArmorComponent());          // +0x3C
        if (armor)
            break;
    }
    if (armor)
    {
        if (object->isOfType(ITObstacle))                                           // +0xA4
            m_maxHealth = armor->getMaxWreckHitPoints();                            // +0x88
        else
            m_maxHealth = armor->getMaxHitPoints();                                 // +0x84
    }
    else
    {
        m_maxHealth = 1000000.0f;
    }

    world::IPhysicsNode* const physics = static_cast<world::IPhysicsNode*>(
        object->getObject()->queryComponent(world::IID_IPhysicsNode, world::IID_IPhysicsNode));
    if (physics)
        m_mass = physics->getMass();                                                // +0xDC

    if (object->getObject()->hasCollisionLod(1) == 1)                               // +0xCC
    {
        world::ICollisionMesh* const mesh = static_cast<world::ICollisionMesh*>(
            world::object_detail::LayoutAccess::collisionMesh(object->getObject()));   // +0x40
        const LocalBoundingBox* const box = mesh->getBoundingBox(1);                // +0x58
        Vec3 size;                                                                  // dead, as shipped
        size.x = box->m_max.x - box->m_min.x;
        size.y = box->m_max.y - box->m_min.y;
        size.z = box->m_max.z - box->m_min.z;
        static_cast<void>(size);
        m_volume = 1.0f;
        m_radius = mesh->getBoundingRadius(1);                                      // +0x54
    }

    if (object->getObject()->getTemplate()->getClassID() == world::CID_DestroyableObjectTemplate)
        m_affectsPathfinding = static_cast<const world::DestroyableObjectTemplate*>(
                                   object->getObject()->getTemplate())->usedByAi;
    m_initialized = true;
}

// `mov eax, 4`.
int AITemplatePhysical::getId()
{
    return 4;
}

} // namespace ai
} // namespace hfe
} // namespace dice
