// [INFERRED] Source placement.

#include "AIObject.h"

#include "AITemplate.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)
#include "IAIObjectPlugIn.h"

#include "../../AIDLL/AI/AIKnowledge/BFInformationCourier.h"      // IAIInformationCourier
#include "../../AIDLL/AI/AIKnowledge/BFInformationEnvironment.h"  // IAIInformationEnvironmentModifiers
#include "../../AIDLL/AI/AIKnowledge/InformationFactory.h"        // IInformationFactory
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../SingletonServer.h"                              // BFSingletonServer::m_iInformationFactory

#include "../../../Core/Ids.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IPlayer.h"
#include "../../../Game/Objects/ISoldier.h"
#include "../../../Game/Objects/Game/Soldier.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

// Every use re-reads the roster static, exactly as the image reloads the ds: word before each call.
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

IInformationFactory* informationFactory()
{
    return static_cast<IInformationFactory*>(BFSingletonServer::m_iInformationFactory);
}

} // anonymous namespace

// Initialised data, read as bytes by isLinked.
bool AIObject::m_plugInLinkVec[6] = { false, false, true, false, true, false };

// `movzx eax, m_plugInLinkVec[which]`. No bounds test.
bool AIObject::isLinked(IAIObjectFlags which)
{
    return m_plugInLinkVec[which];
}

// The template is the only parameter; AIObjectReal/AIObjectShared fill in the
// side and the engine object afterwards.
AIObject::AIObject(AITemplate* aiTemplate)
    : m_refCount(1), m_isAllowedByBots(true), m_initLock(0), m_ownSideInGrid(false),
      m_template(aiTemplate)
{
    // +0x10 (side) and +0x18 (object) are not written here - the derived constructors store both.
    m_plugIns[0] = 0;
    m_plugIns[1] = 0;
    m_plugIns[2] = 0;
    m_plugIns[3] = 0;
    m_plugIns[4] = 0;
    m_plugIns[5] = 0;
}

// all six plug-ins, in index order, through their deleting destructors.
AIObject::~AIObject()
{
    if (m_plugIns[0]) delete m_plugIns[0];
    if (m_plugIns[1]) delete m_plugIns[1];
    if (m_plugIns[2]) delete m_plugIns[2];
    if (m_plugIns[3]) delete m_plugIns[3];
    if (m_plugIns[4]) delete m_plugIns[4];
    if (m_plugIns[5]) delete m_plugIns[5];
}

// The shared body of every init: the template learns from its
// first real instance, then each present plug-in initialises, in index order.
void AIObject::IAIObjectInit()
{
    m_template->initFromObject(m_object);
    if (m_plugIns[0]) m_plugIns[0]->init();
    if (m_plugIns[1]) m_plugIns[1]->init();
    if (m_plugIns[2]) m_plugIns[2]->init();
    if (m_plugIns[3]) m_plugIns[3]->init();
    if (m_plugIns[4]) m_plugIns[4]->init();
    if (m_plugIns[5]) m_plugIns[5]->init();
}

void AIObject::aiObjectUpdate()
{
    if (m_plugIns[0]) m_plugIns[0]->update();
    if (m_plugIns[1]) m_plugIns[1]->update();
    if (m_plugIns[2]) m_plugIns[2]->update();
    if (m_plugIns[3]) m_plugIns[3]->update();
    if (m_plugIns[4]) m_plugIns[4]->update();
    if (m_plugIns[5]) m_plugIns[5]->update();
}

// only the mobile plug-in hears about a move (IAIObjectMobile +0x60).
void AIObject::aiObjectPositionChanged()
{
    if (m_plugIns[AIOFMobile])
        static_cast<IAIObjectMobile*>(m_plugIns[AIOFMobile])->positionChanged();
}

// the byte at +0x08.
void AIObject::setIsAllowedByBots(bool allowed)
{
    m_isAllowedByBots = allowed;
}

bool AIObject::getIsAllowedByBots()
{
    return m_isAllowedByBots;
}

// Replaces the plug-in at its id, deleting the old value. An id outside 0..5 leaves
// the incoming plug-in unowned and unfreed.
void AIObject::aiObjectAddPlugIn(IAIObjectPlugIn* plugIn)
{
    switch (plugIn->getId())
    {
    case AIOFArmament:
        if (m_plugIns[0]) delete m_plugIns[0];
        m_plugIns[0] = plugIn;
        break;
    case AIOFControlInfo:
        if (m_plugIns[1]) delete m_plugIns[1];
        m_plugIns[1] = plugIn;
        break;
    case AIOFCover:
        if (m_plugIns[2]) delete m_plugIns[2];
        m_plugIns[2] = plugIn;
        break;
    case AIOFMobile:
        if (m_plugIns[3]) delete m_plugIns[3];
        m_plugIns[3] = plugIn;
        break;
    case AIOFPhysical:
        if (m_plugIns[4]) delete m_plugIns[4];
        m_plugIns[4] = plugIn;
        break;
    case AIOFUnit:
        if (m_plugIns[5]) delete m_plugIns[5];
        m_plugIns[5] = plugIn;
        break;
    default:
        break;
    }
}

// the courier's getInformation (+0x08), tested for non-zero.
bool AIObject::checkIfInfoExists(unsigned int handle)
{
    return informationCourier()->getInformation(handle) != 0;
}

// Creates plug-ins in index order 0, 1, 2, 4, 5, then mobile (3).
// The base implementation ignores side; secondary objects use it.
void AIObject::createInfoPlugins(unsigned int handle, bool real, int /*side*/)
{
    if (m_plugIns[0]) m_plugIns[0]->createInformationPlugIn(handle, real);
    if (m_plugIns[1]) m_plugIns[1]->createInformationPlugIn(handle, real);
    if (m_plugIns[2]) m_plugIns[2]->createInformationPlugIn(handle, real);
    if (m_plugIns[4]) m_plugIns[4]->createInformationPlugIn(handle, real);
    if (m_plugIns[5]) m_plugIns[5]->createInformationPlugIn(handle, real);
    if (m_plugIns[3]) m_plugIns[3]->createInformationPlugIn(handle, real);
}

// Creates real or side-specific known information from the template, attaches its
// plug-ins, and optionally inserts it into the grid. Own-side insertion sets m_ownSideInGrid.
unsigned int AIObject::createInformationInternal(bool real, double time, int side, bool addToGrid)
{
    unsigned int handle;
    if (real)
        handle = informationFactory()->requestInformationReal(
            0xFFFFFFFFu, this, m_template->m_types, m_template->m_basicTemp, time,
            m_template->m_allowedTimeDiff);                                          // +0x18
    else
        handle = informationFactory()->requestInformationKnown(
            0xFFFFFFFFu, this, m_template->m_types, time, m_template->m_degeneration,
            m_template->m_basicTemp, m_template->m_allowedTimeDiff, side);           // +0x1C

    createInfoPlugins(handle, real, side);
    if (addToGrid)
    {
        addInfoToGrid(handle, side);
        if (m_side == side)
            m_ownSideInGrid = true;
    }
    return handle;
}

// Only a primary object enters the grid (secondaries ride on their primary).
void AIObject::addInfoToGrid(unsigned int handle, int side)
{
    if (isPrimary())
        informationModifiers()->addInfo(side, isUnit(), handle);                 // +0x08
}

// The soldier's own pose (Soldier +0x278) mapped one up into the
// AI's SoldierPose: 0 -> SPStanding, 1 -> SPCrouching, 2 -> SPLying, 3 -> SPSwimming, anything
// else and every non-soldier -> SPNotAvailable.
// It reads m_object directly, while getSoldierPoseCameraPosition below asks getObject. Both
// images; reproduced.
SoldierPose AIObject::getSoldierPose() const
{
    world::IObject* const object = m_object;
    if (!const_cast<world::IObjectTemplate*>(object->getTemplate())
             ->queryImplementation(world::CID_SoldierTemplate))                     // +0x50
        return SPNotAvailable;

    switch (static_cast<int>(static_cast<world::Soldier*>(object)->getPose()))      // +0x278
    {
    case 0:  return SPStanding;
    case 1:  return SPCrouching;
    case 2:  return SPLying;
    case 3:  return SPSwimming;
    default: return SPNotAvailable;
    }
}

// Returns the soldier camera offset: standing 0, crouching 1, lying 2, other poses 4.
// Requires a soldier object; the template query result is discarded before access.
const Vec3& AIObject::getSoldierPoseCameraPosition(SoldierPose pose) const
{
    world::IObject* const object = getObject();
    const_cast<world::IObjectTemplate*>(object->getTemplate())
        ->queryImplementation(world::CID_SoldierTemplate);                           // result unused

    int index;
    if (pose == SPCrouching)
        index = 1;
    else if (pose > SPCrouching)
        index = (pose == SPLying) ? 2 : 4;
    else
        index = (pose == SPStanding) ? 0 : 4;
    return static_cast<world::Soldier*>(object)->getPoseCameraPosition(index);      // +0x28C
}

// the "no secondaries" defaults every Secondary and non-Primary keeps.
bool AIObject::hasSecondaryPosition() const
{
    return false;
}

void AIObject::registrateSecondaryPosition(IAIObject* /*secondary*/)
{
}

void AIObject::unregisterSecondaryPosition(IAIObject* /*secondary*/)
{
}

IAIObject* AIObject::getPrimaryObject()
{
    return this;
}

void AIObject::setPrimaryObject(IAIObject* /*primary*/)
{
}

IAIObject* AIObject::getSecondaryObject(int& iterator)
{
    iterator = -1;
    return 0;
}

IAIObject* AIObject::getFirstSecondaryObject(int& iterator)
{
    iterator = -1;
    return 0;
}

const IAIObject* AIObject::getSecondaryObject(int& iterator) const
{
    iterator = -1;
    return 0;
}

const IAIObject* AIObject::getFirstSecondaryObject(int& iterator) const
{
    iterator = -1;
    return 0;
}

int AIObject::getFirstSecondaryObjectIterator() const
{
    return -1;
}

// The same engine object, or two objects under one root.
// A null root on this side answers false before the other root is looked at.
bool AIObject::hasSameParentObject(const IAIObject* other) const
{
    world::IObject* const mine = getObject();
    world::IObject* const theirs = other->getObject();
    if (mine == theirs)
        return true;
    world::IObject* const root = world::getRootParent(mine);
    if (!root)
        return false;
    return root == world::getRootParent(theirs);
}

// No ownership change - see the header note on the destructor.
void AIObject::borrowArmament(IAIObject* from)
{
    if (from)
        m_plugIns[AIOFArmament] = from->getArmament();
}

void AIObject::resetArmament()
{
    m_plugIns[AIOFArmament] = 0;
}

// plain slot reads (ELF getCover precedes getControlInfo in the text).
IAIObjectArmament* AIObject::getArmament() const
{
    return static_cast<IAIObjectArmament*>(m_plugIns[AIOFArmament]);
}

IAIObjectCover* AIObject::getCover() const
{
    return static_cast<IAIObjectCover*>(m_plugIns[AIOFCover]);
}

IAIObjectControlInfo* AIObject::getControlInfo() const
{
    return static_cast<IAIObjectControlInfo*>(m_plugIns[AIOFControlInfo]);
}

IAIObjectMobile* AIObject::getMobile() const
{
    return static_cast<IAIObjectMobile*>(m_plugIns[AIOFMobile]);
}

IAIObjectPhysical* AIObject::getPhysical() const
{
    return static_cast<IAIObjectPhysical*>(m_plugIns[AIOFPhysical]);
}

IAIObjectUnit* AIObject::getUnit() const
{
    return static_cast<IAIObjectUnit*>(m_plugIns[AIOFUnit]);
}

// a switch, not an index - an out-of-range flag answers null.
IAIObjectPlugIn* AIObject::getPlugIn(IAIObjectFlags which) const
{
    switch (which)
    {
    case AIOFArmament:    return m_plugIns[0];
    case AIOFControlInfo: return m_plugIns[1];
    case AIOFCover:       return m_plugIns[2];
    case AIOFMobile:      return m_plugIns[3];
    case AIOFPhysical:    return m_plugIns[4];
    case AIOFUnit:        return m_plugIns[5];
    default:              return 0;
    }
}

void AIObject::lockInit()
{
    ++m_initLock;
}

void AIObject::unlockInit()
{
    --m_initLock;
}

// Initialization is locked during precaching and between level events 2 and 7.
// Objects created while locked defer initialization.
bool AIObject::isLocked()
{
    if (m_initLock || game()->getIsPrecaching())                                    // +0x68
        return true;
    return false;
}

// . 13 is the "no kit" value ISoldier::getKitType itself answers.
int AIObject::getKitType() const
{
    world::ISoldier* const soldier =
        static_cast<world::ISoldier*>(m_object->queryInterface(world::IID_ISoldier));
    if (soldier)
        return soldier->getKitType();                                               // +0x58
    return 13;
}

// Walks UP from the object (m_parent, +0x28) to the first player
// control object, and answers its occupant's id, or -1.
int AIObject::getPlayerId() const
{
    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = m_object; o; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
            o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }
    if (pco)
    {
        world::IPlayer* const player = pco->getOccupingPlayer();                     // +0x10
        if (player)
            return player->getId();                                                 // +0x54
    }
    return -1;
}

// No parent, or a parent that is only a carrier: a ladder, a
// parachute, a zipline or a grappling-hook rope container.
// Each test re-reads m_object->m_parent->m_template and makes its own getClassID call (four
// virtual calls for a rope); reproduced.
bool AIObject::isRootObject() const
{
    if (!m_object->getParent())
        return true;
    if (m_object->getParent()->getTemplate()->getClassID() == world::CID_LadderContainerTemplate)
        return true;
    if (m_object->getParent()->getTemplate()->getClassID() == world::CID_ParachuteTemplate)
        return true;
    if (m_object->getParent()->getTemplate()->getClassID() == world::CID_ZiplineContainerTemplate)
        return true;
    return m_object->getParent()->getTemplate()->getClassID() ==
           world::CID_GrapplingHookRopeContainerTemplate;
}

// A fresh AI object for the wreck, built from the template's
// "<name>_wreck" template (AITemplate::createWreckTemplate) and initialised at once. The caller
// owns it. No wreck template: null.
IAIObject* AIObject::getWreckObject(world::IObject* wreck) const
{
    if (m_template->m_wreckTemplate)
    {
        IAIObject* const object = m_template->m_wreckTemplate->createObject(0, wreck);
        object->init();                                                             // +0x48
        return object;
    }
    return 0;
}

// the engine object's id (+0x10).
unsigned int AIObject::getObjectId() const
{
    return m_object->getObjectId();
}

// AIObject's own IBase over +0x04.
void AIObject::addRef()
{
    ++m_refCount;
}

int AIObject::getRef()
{
    return m_refCount;
}

int AIObject::release()
{
    const int refCount = --m_refCount;
    if (refCount == 0)
        delete this;
    return refCount;
}

// AIObject and the real branch answer IObjectComponent; the shared branch does not (see
// AIObjectShared / AIObjectPrimaryShared / AIObjectSecondaryShared).
void* AIObject::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IAIObject)
        return const_cast<AIObject*>(this);
    if (iid == world::IID_IObjectComponent)
        return const_cast<AIObject*>(this);
    return 0;
}

int AIObject::getSide() const
{
    return m_side;
}

bool AIObject::isArmament() const
{
    return m_plugIns[AIOFArmament] != 0;
}

bool AIObject::isCover() const
{
    return m_plugIns[AIOFCover] != 0;
}

bool AIObject::isPhysical() const
{
    return m_plugIns[AIOFPhysical] != 0;
}

bool AIObject::isMobile() const
{
    return m_plugIns[AIOFMobile] != 0;
}

bool AIObject::isUnit() const
{
    return m_plugIns[AIOFUnit] != 0;
}

// every bit of `flags` present in the template's plug-in mask.
bool AIObject::isSame(int flags) const
{
    return flags == (flags & getFlags());
}

// an empty default; the Primary classes override it.
void AIObject::positionChanged()
{
}

world::IObject* AIObject::getObject() const
{
    return m_object;
}

} // namespace ai
} // namespace hfe
} // namespace dice
