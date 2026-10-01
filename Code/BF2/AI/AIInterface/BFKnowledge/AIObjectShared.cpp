// [INFERRED] Source placement.

#include "AIObjectShared.h"

#include "AITemplate.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)
#include "BFTimer.h"                                         // IBFTimer::getGameTime
#include "IAIObjectPlugIn.h"

#include "../../AIDLL/AI/AIKnowledge/BFInformationCourier.h"      // IAIInformationCourier
#include "../../AIDLL/AI/AIKnowledge/BFInformationEnvironment.h"  // IAIInformationEnvironmentModifiers
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"
#include "../../../Game/IGameLogic.h"                        // gameLogic->getObjectSpawner
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../Game/Objects/ObjectLayoutAccess.h"        // IObject +0x44, the physics node
#include "../../../Physics/IPhysicsNode.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

IAIGame* game() { return static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame); }

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

AIObjectShared::AIObjectShared(AITemplate* aiTemplate, int side, world::IObject* object)
    : AIObject(aiTemplate), m_refCount(1), m_handle(0xFFFFFFFFu)
{
    m_side = side;
    m_object = object;
}

AIObjectShared::~AIObjectShared()
{
}

// The shared body, then - outside precaching - the one information, on the
// neutral side and into the grid. A parachute's AI object leaves the grid again at once.
void AIObjectShared::init()
{
    IAIObjectInit();
    if (!game()->getIsPrecaching())                                                 // +0x68
    {
        createInformation(0, true);                                                 // +0xAC
        if (m_object->getTemplate()->getClassID() == world::CID_ParachuteTemplate)
            removeFromGrid();                                                       // +0xC8
    }
}

void AIObjectShared::update()
{
    aiObjectUpdate();
}

// - the engine object's own absolute state.
const Mat4& AIObjectShared::getTransformation() const
{
    return m_object->getAbsoluteTransformation();                                   // +0x84
}

const Vec3& AIObjectShared::getPosition() const
{
    return m_object->getAbsolutePosition();                                         // +0x6C
}

float AIObjectShared::getBoundingRadius()
{
    return m_object->getPhysicBoundingRadius();                                     // +0x98
}

int AIObjectShared::getFlags() const
{
    return m_template->m_plugInMask;
}

// the engine template's name, twice.
const std::string& AIObjectShared::getName() const
{
    return m_object->getTemplate()->getName();                                      // +0x2C
}

const std::string& AIObjectShared::getObjectTypeName() const
{
    return m_object->getTemplate()->getName();
}

const std::string& AIObjectShared::getAIName() const
{
    return m_template->m_name;
}

// The side names nothing here except the side a re-creation is made for.
unsigned int AIObjectShared::forceGetInformation(int side, bool addToGrid)
{
    if (!informationCourier()->getInformation(m_handle))                            // +0x08
        createInformation(side, addToGrid);                                         // +0xAC
    return m_handle;
}

unsigned int AIObjectShared::getInformation(int /*side*/)
{
    return m_handle;
}

// Deletes shared-side information, removing an entered primary from the grid first.
// Unlike the real-object path, leaves m_ownSideInGrid set.
void AIObjectShared::deleteInformation(int /*side*/)
{
    if (m_handle != 0xFFFFFFFFu)
    {
        if (m_ownSideInGrid && isPrimary())                                         // +0xF8
            informationModifiers()->removeInfo(0, isUnit(), m_handle);              // +0x14
        if (informationCourier()->getInformation(m_handle))                         // +0x08
            informationCourier()->freeInformation(m_handle);                        // +0x10
        m_handle = 0xFFFFFFFFu;
    }
}

void AIObjectShared::deleteAllInformation()
{
    if (m_handle != 0xFFFFFFFFu)
    {
        if (m_ownSideInGrid && isPrimary())
            informationModifiers()->removeInfo(0, isUnit(), m_handle);
        if (informationCourier()->getInformation(m_handle))
            informationCourier()->freeInformation(m_handle);
        m_handle = 0xFFFFFFFFu;
    }
}

// A new plug-in throws the information away (it is rebuilt with the plug-in's
// half on the next createInformation).
void AIObjectShared::addPlugIn(IAIObjectPlugIn* plugIn)
{
    aiObjectAddPlugIn(plugIn);
    deleteAllInformation();                                                         // +0xC0
}

unsigned int AIObjectShared::createInformation(int side, bool addToGrid)
{
    return createInformation(timer()->getGameTime(), side, addToGrid);             // +0xB0
}

// Always a real information. A mobile object's courier
// polygon is updated before the information exists (AIObjectReal does it after).
unsigned int AIObjectShared::createInformation(double time, int side, bool addToGrid)
{
    const unsigned int handle = m_handle;
    if (!checkIfInfoExists(handle))
    {
        if (getMobile())                                                            // +0x90
            informationCourier()->updateCurrentPolygon(this);                       // +0x38
        m_handle = createInformationInternal(true, time, side, addToGrid);
        return m_handle;
    }
    return handle;
}

// The side only; a unit's possession clock goes to -1000.0 through getPlugIn(5).
void AIObjectShared::setSide(int side)
{
    if (m_side != side)
    {
        m_side = side;
        IAIObjectUnit* const unit = static_cast<IAIObjectUnit*>(getPlugIn(AIOFUnit));   // +0x78
        if (unit)
            unit->setLastPlayerPossesionTime(-1000.0);                              // +0x64
    }
}

bool AIObjectShared::setPosition(const Vec3& position, int side)
{
    if (side != m_side)
        return false;
    m_object->setAbsolutePosition(position);                                        // +0x70
    return true;
}

bool AIObjectShared::setTransformation(const Mat4& transformation, int side)
{
    if (side != m_side)
        return false;
    m_object->setAbsoluteTransformation(transformation);                            // +0x8C
    return true;
}

// - the same body as AIObjectReal's; see the note there on unordered compares.
void AIObjectShared::striveForRotationalSpeed(int axis, float target, float step)
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

void AIObjectShared::setObject(world::IObject* object)
{
    m_object = object;
}

bool AIObjectShared::isOfType(InformationType type) const
{
    return ((m_template->m_types >> type) & 1) != 0;
}

// The spawner id is asked and discarded; no null test on the control object.
void AIObjectShared::releaseObjectFromSpawner()
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object);
    pco->getObjectSpawnerId();                                                      // +0xBC, unguarded
}

bool AIObjectShared::getObjectSpawnerHolding() const
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object);
    if (pco)
        return pco->getObjectSpawnerHolding();                                      // +0xC4
    return false;
}

// - AIObjectReal's walk without its fatal diagnostic.
IAIObject* AIObjectShared::getObjectSpawner() const
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object);
    if (!pco)
        return 0;
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

// - with the parent walk (see the header).
int AIObjectShared::getPlayerControlObjectId()
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object);
    if (pco)
        return pco->getPcoId();                                                     // +0x30
    return 0;
}

// (emitted in the PrimaryShared TU).
unsigned int AIObjectShared::getHandle(int /*side*/) const
{
    return m_handle;
}

// this level's IBase over +0x38.
void AIObjectShared::addRef()
{
    ++m_refCount;
}

int AIObjectShared::getRef()
{
    return m_refCount;
}

int AIObjectShared::release()
{
    const int refCount = --m_refCount;
    if (refCount == 0)
        delete this;
    return refCount;
}

// No IObjectComponent answer on the shared branch.
void* AIObjectShared::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<AIObjectShared*>(this);
    if (iid == IID_IAIObject)
        return const_cast<AIObjectShared*>(this);
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
