// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-07-knowledge] Complete and validate the remaining AIObjectPhysical operations.

#include "AIObjectPhysical.h"

#include "AIFrontier.h"
#include "AITemplatePhysical.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)

#include "../IAIEventCourier.h"                              // event_retrievePathfindingMapId & co.
#include "../SingletonServer.h"                              // BFSingletonServer
#include "../../AIDLL/AI/AIKnowledge/InformationFactory.h"        // IInformationFactory
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"
#include "../../../Game/Objects/DestroyableObjectTemplate.h" // usedByAi / initiallyWalkable / corners
#include "../../../Game/Objects/GameLogicComp/IArmorGLComp.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../Game/Objects/IPlayer.h"
#include "../../../Game/Objects/ObjectLayoutAccess.h"
#include "../../../Math/LocalBoundingBox.h"
#include "../../../Physics/ICollisionMesh.h"
#include "../../../Physics/IPhysicsNode.h"
#include "../../../Physics/IResponsePhysics.h"
#include "../../../Physics/PhysicsUtil.h"                    // hfe::resetPhysics
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

IAIGame* game() { return static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame); }

IInformationFactory* informationFactory()
{
    return static_cast<IInformationFactory*>(BFSingletonServer::m_iInformationFactory);
}

world::ICollisionMesh* collisionMesh(world::IObject* object)
{
    return static_cast<world::ICollisionMesh*>(world::object_detail::LayoutAccess::collisionMesh(object));
}

world::IResponsePhysics* responsePhysics(world::IObject* object)
{
    return static_cast<world::IResponsePhysics*>(
        world::object_detail::LayoutAccess::responsePhysics(object));
}

} // anonymous namespace

// . +0x0C is left unwritten.
AIObjectPhysical::AIObjectPhysical(IAIObject* object, AITemplatePhysical* aiTemplate)
    : IAIObjectPhysical(object), m_template(aiTemplate), m_physicsNode(0), m_mapPosId(-1)
{
}

// A pathfinding-relevant object gives its map entry back.
AIObjectPhysical::~AIObjectPhysical()
{
    if (m_template->m_affectsPathfinding)
        BFSingletonServer::m_iaiEventCourier->event_pathfindingMapIdRestoreOriginalMaterials(
            m_mapPosId, m_object);                                                  // +0xC4
}

int AIObjectPhysical::getId() const
{
    return AIOFPhysical;
}

// - the factory's three-argument request under this plug-in's own id.
void AIObjectPhysical::createInformationPlugIn(unsigned int handle, bool real)
{
    informationFactory()->requestInformationPlugin(handle, getId(), real);         // +0x20
}

// The template's +0x04 is copied before the template initialises (see the
// header), the physics node is fetched, the template learns from its first instance, and a
// `usedByAi` destroyable object - outside precaching - asks the AI for a pathfinding map id from its
// template's walkability and corners (DestroyableObjectTemplate +0x189, +0x18C, +0x198) and
// announces itself as a bridge.
// The template is cast to DestroyableObjectTemplate unchecked - only such templates can set
// m_affectsPathfinding (AITemplatePhysical::initFromObject).
void AIObjectPhysical::init()
{
    m_maxHealth = m_template->m_maxHealth;
    m_physicsNode = static_cast<world::IPhysicsNode*>(
        world::object_detail::LayoutAccess::physicsNode(m_object->getObject()));   // +0x44
    if (!m_template->m_initialized)
        m_template->initFromObject(m_object);

    if (m_template->m_affectsPathfinding && !game()->getIsPrecaching())             // +0x68
    {
        const world::DestroyableObjectTemplate* const objectTemplate =
            static_cast<const world::DestroyableObjectTemplate*>(m_object->getObject()->getTemplate());
        const Mat4& transformation = m_object->getTransformation();                 // +0xDC
        BFSingletonServer::m_iaiEventCourier->event_retrievePathfindingMapId(
            m_mapPosId, objectTemplate->initiallyWalkable,
            *reinterpret_cast<const Vec3*>(objectTemplate->minCorner),
            *reinterpret_cast<const Vec3*>(objectTemplate->maxCorner), transformation);   // +0xB8
        BFSingletonServer::m_iaiEventCourier->event_bridgeCreated(m_object);        // +0xD0
    }
}

// empty.
void AIObjectPhysical::update()
{
}

// Returns 1,000,000 when there is no armour component.
float AIObjectPhysical::getHealth()
{
    world::IArmorGLComp* const armor =
        static_cast<world::IArmorGLComp*>(m_object->getObject()->getArmorComponent());   // +0x3C
    if (armor)
        return armor->getHitPoints();                                               // +0x3C
    return 1000000.0f;
}

// / - template reads.
float AIObjectPhysical::getMaxHealth() const
{
    return m_template->m_maxHealth;
}

float AIObjectPhysical::getMass() const
{
    return m_template->m_mass;
}

float AIObjectPhysical::getVolume() const
{
    return m_template->m_volume;
}

float AIObjectPhysical::getBestArmour() const
{
    return m_template->m_bestArmour;
}

int AIObjectPhysical::getStrType() const
{
    return m_template->m_strType;
}

// a constant, whatever the direction.
float AIObjectPhysical::getArmour(const Vec3& /*direction*/) const
{
    return 1.0f;
}

// an object without collision LOD 1 calls abort - retail's own, not a
// frontier. Otherwise the collision mesh's LOD-1 box.
const LocalBoundingBox* AIObjectPhysical::getLocalBoundingBox() const
{
    if (!m_object->getObject()->hasCollisionLod(1))                                 // +0xCC
        std::abort();
    return collisionMesh(m_object->getObject())->getBoundingBox(1);                 // +0x58
}

// `new BoundingBox` (0x60 B, eight zeroed corners), initWithLocalBoundingBox(
// getLocalBoundingBox), transform(object->getAbsoluteTransformation), checkPointer'd. The
// BoundingBox class is not in this tree.
BoundingBox* AIObjectPhysical::getCurrentBoundingBox()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIObjectPhysical::getCurrentBoundingBox (BoundingBox)");
}

// the same with the caller's transformation.
BoundingBox* AIObjectPhysical::getBoundingBox(const Mat4& /*transformation*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIObjectPhysical::getBoundingBox (BoundingBox)");
}

// The collision mesh's LOD-1 radius, 0 without that LOD.
float AIObjectPhysical::getRadius() const
{
    if (!m_object->getObject()->hasCollisionLod(1))                                 // +0xCC
        return 0.0f;
    return collisionMesh(m_object->getObject())->getBoundingRadius(1);              // +0x54
}

// The box centre, recomputed first when stale - through a const pointer, as
// retail does (the derived data is cached state).
Vec3 AIObjectPhysical::getSphereLocalOffset() const
{
    LocalBoundingBox* const box = const_cast<LocalBoundingBox*>(getLocalBoundingBox());   // +0x30
    if (box->m_derivedDataDirty)
        box->calcRadius();
    return box->m_centre;
}

// the radius about the centre.
float AIObjectPhysical::getSmallestRadius() const
{
    LocalBoundingBox* const box = const_cast<LocalBoundingBox*>(getLocalBoundingBox());
    if (box->m_derivedDataDirty)
        box->calcRadius();
    return box->m_radiusFromCentre;
}

// the LOD-1 box's longer horizontal side split into equal circles along its middle
// (pushed into `out` as Vec2), the step returned. Read; not yet transcribed.
float AIObjectPhysical::getSubSpheres2d(std::vector<Vec2>& /*out*/) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIObjectPhysical::getSubSpheres2d");
}

bool AIObjectPhysical::isTouchingTerrain(Vec3& out) const
{
    return responsePhysics(m_object->getObject())->getIsIntersectingTerrain(out);   // +0x7C
}

// false while the physics node sleeps; a soldier answers the negation of a raw
// byte at +0xFA of its response physics (the soldier physics' ground flag - no class for it in this
// tree); anything else asks the node's getIsInAir (+0x1B0).
bool AIObjectPhysical::isInAir() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIObjectPhysical::isInAir (soldier response physics +0xFA)");
}

// strictly above zero (`setnbe` after fucomip - unordered is false).
bool AIObjectPhysical::isInWater() const
{
    return m_physicsNode->getUnderWater() > 0.0f;                                   // +0x10C
}

// The engine object is asked for and discarded; the answer is always 0.
int AIObjectPhysical::getMaterial() const
{
    m_object->getObject();
    return 0;
}

// The occupying player's man-down clock, when it has a
// control object, an occupant, and that occupant is down. The occupant is asked for twice.
float AIObjectPhysical::getManDownTimeLeft() const
{
    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        m_object->getObject()->queryInterface(world::IID_IPlayerControlObject));
    if (!pco || !pco->getOccupingPlayer())                                          // +0x10
        return 0.0f;
    world::IPlayer* const player = pco->getOccupingPlayer();
    if (!player->getIsManDown())                                                    // +0x100
        return 0.0f;
    return player->getManDownTimeLeft();                                            // +0xF8
}

// the ArmorGLCompTemplate component's +0x110 accessor (not declared in this tree)
// 1.0 without the component.
float AIObjectPhysical::getMaxManDownTime() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIObjectPhysical::getMaxManDownTime (ArmorGLCompTemplate +0x110)");
}

// status 1 on the armour component.
bool AIObjectPhysical::getManDown() const
{
    world::IArmorGLComp* const armor =
        static_cast<world::IArmorGLComp*>(m_object->getObject()->getArmorComponent());
    if (armor)
        return armor->getStatus() == 1;                                             // +0x5C
    return false;
}

bool AIObjectPhysical::doesAffectPathfinding() const
{
    return m_template->m_affectsPathfinding;
}

int AIObjectPhysical::getMapPosId() const
{
    return m_mapPosId;
}

// Only the owning side may switch; the
// physics node and the response physics of the node's object are switched together, and an enable
// resets the object's physics (hfe::resetPhysics, player control objects included). Both answer
// true for the owning side whether or not anything changed.
bool AIObjectPhysical::enablePhysics(int side)
{
    if (side != m_object->getSide())                                                // +0x12C
        return false;
    if (!hasPhysicsEnabled())                                                       // +0x7C
    {
        m_physicsNode->setEnabled(true);                                            // +0x18
        responsePhysics(m_physicsNode->getObject())->setEnabled(true);              // +0x2C, +0x18
        resetPhysics(m_object->getObject(), true);
    }
    return true;
}

bool AIObjectPhysical::disablePhysics(int side)
{
    if (side != m_object->getSide())
        return false;
    if (hasPhysicsEnabled())
    {
        m_physicsNode->setEnabled(false);
        responsePhysics(m_physicsNode->getObject())->setEnabled(false);
    }
    return true;
}

bool AIObjectPhysical::hasPhysicsEnabled()
{
    return m_physicsNode->getEnabled();                                             // +0x1C
}

} // namespace ai
} // namespace hfe
} // namespace dice
