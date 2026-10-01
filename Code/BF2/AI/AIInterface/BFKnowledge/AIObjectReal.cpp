
#include "AIObjectReal.h"

#include "AITemplate.h"
#include "BFTimer.h"                                         // IBFTimer::getGameTime
#include "IAIObjectPlugIn.h"

#include "../../AIDLL/AI/AIKnowledge/BFInformationCourier.h"      // IAIInformationCourier
#include "../../AIDLL/AI/AIKnowledge/BFInformationEnvironment.h"  // IAIInformationEnvironmentModifiers
#include "../../AIDLL/AI/Main/AISettingsInterfaces.h"             // IAISettings::getNSides
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Game/IGameLogic.h"                        // gameLogic->getObjectSpawner
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../Game/Objects/ObjectLayoutAccess.h"      // IObject +0x44, the physics node
#include "../../../Physics/IPhysicsNode.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

IAISettings* settings() { return static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings); }

IAIInformationCourier* informationCourier()
{
    return static_cast<IAIInformationCourier*>(CommonSingletonServer::m_iaiInformationCourier);
}

IAIInformationEnvironmentModifiers* informationModifiers()
{
    return static_cast<IAIInformationEnvironmentModifiers*>(
        CommonSingletonServer::m_iaiInformationEnvironmentModifiers);
}

IBFTimer* timer() { return static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer); }

// The walk every spawner/holding query shares: up the m_parent chain (+0x28) to the first object
// that answers IID_IPlayerControlObject.
world::IPlayerControlObject* findPlayerControlObject(world::IObject* object)
{
    for (world::IObject* o = object; o; o = o->getParent())
    {
        world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
            o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            return pco;
    }
    return 0;
}

} // anonymous namespace

// The side count is asked once (the loop bound is held in a register).
AIObjectReal::AIObjectReal(AITemplate* aiTemplate, int side, world::IObject* object)
    : AIObject(aiTemplate), m_refCount(1)
{
    m_side = side;
    m_object = object;
    const int count = settings()->getNSides() + 1;                                  // +0x40
    for (int i = 0; i < count; ++i)
        m_handles[i] = 0xFFFFFFFFu;
}

// - nothing of its own.
AIObjectReal::~AIObjectReal()
{
}

// tail calls into the shared bodies.
void AIObjectReal::init()
{
    IAIObjectInit();
}

void AIObjectReal::update()
{
    aiObjectUpdate();
}

// - the engine object's own absolute state.
const Vec3& AIObjectReal::getPosition() const
{
    return m_object->getAbsolutePosition();                                         // +0x6C
}

const Mat4& AIObjectReal::getTransformation() const
{
    return m_object->getAbsoluteTransformation();                                   // +0x84
}

float AIObjectReal::getBoundingRadius()
{
    return m_object->getPhysicBoundingRadius();                                     // +0x98
}

// the template's plug-in mask.
int AIObjectReal::getFlags() const
{
    return m_template->m_plugInMask;
}

// are the same : the engine template's name.
const std::string& AIObjectReal::getName() const
{
    return m_object->getTemplate()->getName();                                      // +0x2C
}

const std::string& AIObjectReal::getObjectTypeName() const
{
    return m_object->getTemplate()->getName();
}

// the AI template's name.
const std::string& AIObjectReal::getAIName() const
{
    return m_template->m_name;
}

// Recreates on a handle the courier no longer knows, then returns the slot
// as it now stands.
unsigned int AIObjectReal::forceGetInformation(int side, bool addToGrid)
{
    if (!informationCourier()->getInformation(m_handles[side]))                     // +0x08
        createInformation(side, addToGrid);                                         // +0xAC
    return m_handles[side];
}

// No bounds test.
unsigned int AIObjectReal::getInformation(int side)
{
    return m_handles[side];
}

// A primary's own-side entry leaves the grid through
// removeInfo, another side's through destroyRemoveInfo; the handle is freed if the courier still
// knows it, and the slot is cleared. The own-side grid flag drops whenever the own side is named
// even when there was nothing to delete.
void AIObjectReal::deleteInformation(int side)
{
    if (m_handles[side] != 0xFFFFFFFFu)
    {
        if (isPrimary())
        {
            if (side == m_side)
                informationModifiers()->removeInfo(side, isUnit(), m_handles[side]);          // +0x14
            else
                informationModifiers()->destroyRemoveInfo(side, isUnit(), m_handles[side]);   // +0x1C
        }
        if (informationCourier()->getInformation(m_handles[side]))                  // +0x08
            informationCourier()->freeInformation(m_handles[side]);                 // +0x10
        m_handles[side] = 0xFFFFFFFFu;
    }
    if (side == m_side)
        m_ownSideInGrid = false;
}

// The side count is asked once.
void AIObjectReal::deleteAllInformation()
{
    const int count = settings()->getNSides() + 1;
    for (int side = 0; side < count; ++side)
        deleteInformation(side);                                                    // +0xBC
}

void AIObjectReal::addPlugIn(IAIObjectPlugIn* plugIn)
{
    aiObjectAddPlugIn(plugIn);
}

// the timer's game time (+0x08, a double on st0) into the three-argument slot.
unsigned int AIObjectReal::createInformation(int side, bool addToGrid)
{
    return createInformation(timer()->getGameTime(), side, addToGrid);             // +0xB0
}

// Nothing happens while the courier still knows the handle.
// A new handle is real for the owning side, and for every side while the object is neutral; a mobile
// object then has the courier update its current navigation polygon (+0x38).
unsigned int AIObjectReal::createInformation(double time, int side, bool addToGrid)
{
    const unsigned int handle = m_handles[side];
    if (!checkIfInfoExists(handle))
    {
        const bool real = (m_side == side || m_side == 0);
        m_handles[side] = createInformationInternal(real, time, side, addToGrid);
        if (getMobile())
            informationCourier()->updateCurrentPolygon(this);                       // +0x38
        return m_handles[side];
    }
    return handle;
}

// Changes side and rebuilds the information visible to each side.
void AIObjectReal::setSide(int side)
{
    const int oldSide = m_side;
    if (oldSide == side)
        return;

    informationCourier()->isRealInformation(m_handles[oldSide]);                    // +0x0C
    informationCourier()->saveCurrentPolygon(this);                                 // +0x34

    if (oldSide == 0)
    {
        const int other = (side == 1) + 1;
        deleteInformation(other);                                                   // +0xBC
        m_side = side;
        if (isOfType(ITSoldier))                                                    // +0xA4
        {
            createInformation(side, true);                                          // +0xAC
            m_ownSideInGrid = true;
        }
        else
        {
            createInformation(other, true);
        }
    }
    else
    {
        int other;
        if (side != 0)
        {
            other = (side == 1) + 1;
            deleteInformation(other);
            deleteInformation(side);
            m_side = side;
            createInformation(side, true);
        }
        else
        {
            other = (m_side == 1) + 1;
            deleteInformation(other);
            m_side = 0;
        }
        createInformation(other, true);
        m_ownSideInGrid = true;
    }

    informationCourier()->saveCurrentPolygon(0);
    IAIObjectUnit* const unit = getUnit();                                          // +0x98
    if (unit)
        unit->setLastPlayerPossesionTime(-1000.0);                                  // +0x64
}

// only the owning side may move the object.
bool AIObjectReal::setPosition(const Vec3& position, int side)
{
    if (side != m_side)
        return false;
    m_object->setAbsolutePosition(position);                                        // +0x70
    return true;
}

bool AIObjectReal::setTransformation(const Mat4& transformation, int side)
{
    if (side != m_side)
        return false;
    m_object->setAbsoluteTransformation(transformation);                            // +0x8C
    return true;
}

// Steps each rotational-speed component toward its target by at most maxStep.
// Does nothing without a node; unordered targets take the <= branch in double precision.
void AIObjectReal::striveForRotationalSpeed(int axis, float target, float step)
{
    world::IPhysicsNode* const physics = static_cast<world::IPhysicsNode*>(
        world::object_detail::LayoutAccess::physicsNode(m_object));                // +0x44
    if (!physics)
        return;

    Vec3 speed = physics->getRotationalSpeed();                                     // +0x64
    float& current = (&speed.x)[axis];
    double result;
    if (!(static_cast<double>(target) > current))
    {
        result = static_cast<double>(current) - step;
        if (!(result > target))
            result = target;
    }
    else
    {
        result = static_cast<double>(current) + step;
        if (result > target)
            result = target;
    }
    current = static_cast<float>(result);
    physics->setRotationalSpeed(speed);                                             // +0x6C
}

void AIObjectReal::setObject(world::IObject* object)
{
    m_object = object;
}

// the type bit in the template's type mask.
bool AIObjectReal::isOfType(InformationType type) const
{
    return ((m_template->m_types >> type) & 1) != 0;
}

// Requires a player-control object. Reads its spawner id, then emits the shipped fatal
// "Does not work" diagnostic.
void AIObjectReal::releaseObjectFromSpawner()
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object);
    pco->getObjectSpawnerId();                                                      // +0xBC, unguarded
    Debug(DebugTypeError, "AI/AIInterface/BFKnowledge/AIObjectReal.cpp", 399, "") << "Does not work!";
}

bool AIObjectReal::getObjectSpawnerHolding() const
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object);
    if (pco)
        return pco->getObjectSpawnerHolding();                                      // +0xC4
    return false;
}

// Emits the shipped fatal diagnostic when a player-control object is present before
// attempting to resolve its spawner AI component.
IAIObject* AIObjectReal::getObjectSpawner() const
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object);
    if (!pco)
        return 0;

    Debug(DebugTypeError, "AI/AIInterface/BFKnowledge/AIObjectReal.cpp", 433, "") << "Does not work!";
    world::IObject* const spawner =
        gameLogic->getObjectSpawner(static_cast<unsigned int>(pco->getObjectSpawnerId()));   // +0x1E8
    if (!spawner)
        return 0;
    world::IPlayerControlObject* const spawnerPco = findPlayerControlObject(spawner);
    if (!spawnerPco)
        return 0;
    world::IObject* const spawnerObject =
        static_cast<world::IObject*>(spawnerPco->queryInterface(world::IID_IObject));
    return static_cast<IAIObject*>(spawnerObject->getAIComponent());
}

unsigned int AIObjectReal::getHandle(int side) const
{
    return m_handles[side];
}

// only the object itself is asked - no walk up the parent chain, unlike
// AIObjectShared's copy. Reproduced.
int AIObjectReal::getPlayerControlObjectId()
{
    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        m_object->queryInterface(world::IID_IPlayerControlObject));
    if (pco)
        return pco->getPcoId();                                                     // +0x30
    return 0;
}

// this level's IBase over +0x38.
void AIObjectReal::addRef()
{
    ++m_refCount;
}

int AIObjectReal::getRef()
{
    return m_refCount;
}

int AIObjectReal::release()
{
    const int refCount = --m_refCount;
    if (refCount == 0)
        delete this;
    return refCount;
}

void* AIObjectReal::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IAIObject)
        return const_cast<AIObjectReal*>(this);
    if (iid == world::IID_IObjectComponent)
        return const_cast<AIObjectReal*>(this);
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
