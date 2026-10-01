// [INFERRED] Source placement.

#include "AIObjectSecondaryShared.h"

#include "IAIObjectPlugIn.h"

#include "../SingletonServer.h"                              // BFSingletonServer::m_iInformationFactory
#include "../../AIDLL/AI/AIKnowledge/BFInformationCourier.h"      // IAIInformationCourier
#include "../../AIDLL/AI/AIKnowledge/InformationFactory.h"        // IInformationFactory
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

IAIInformationCourier* informationCourier()
{
    return static_cast<IAIInformationCourier*>(CommonSingletonServer::m_iaiInformationCourier);
}

} // anonymous namespace

AIObjectSecondaryShared::AIObjectSecondaryShared(AITemplate* aiTemplate, int side,
                                                 world::IObject* object)
    : AIObjectShared(aiTemplate, side, object), m_refCount(1), m_primaryObject(0)
{
}

AIObjectSecondaryShared::AIObjectSecondaryShared(AITemplate* aiTemplate, int side,
                                                 world::IObject* object, IAIObject* primary)
    : AIObjectShared(aiTemplate, side, object), m_refCount(1), m_primaryObject(primary)
{
}

void AIObjectSecondaryShared::init()
{
    if (!isLocked())
        AIObjectShared::init();
}

// Requires a primary object. Unregisters this secondary, then looks up the handle
// even when zero and frees it only if the courier still knows it.
AIObjectSecondaryShared::~AIObjectSecondaryShared()
{
    m_primaryObject->unregisterSecondaryPosition(this);                             // +0x110
    if (informationCourier()->getInformation(m_handle))                             // +0x08
        informationCourier()->freeInformation(m_handle);                            // +0x10
}

// - AIObjectSecondaryReal::createInfoPlugins's body; see the note there.
void AIObjectSecondaryShared::createInfoPlugins(unsigned int handle, bool real, int side)
{
    IInformationFactory* const factory =
        static_cast<IInformationFactory*>(BFSingletonServer::m_iInformationFactory);

    if (m_plugIns[AIOFArmament])
        factory->requestInformationPlugin(handle, AIOFArmament, real);              // +0x20
    if (m_plugIns[AIOFControlInfo])
        factory->requestInformationPlugin(handle, AIOFControlInfo, real);
    if (m_plugIns[AIOFCover])
        factory->requestInformationPlugin(handle, AIOFCover, real);
    else if (m_primaryObject->getCover())                                           // +0x8C
        factory->requestInformationPlugin(m_primaryObject->getInformation(side),    // +0xB8
                                          handle, AIOFCover, real);                 // +0x24
    if (m_plugIns[AIOFMobile])
        factory->requestInformationPlugin(handle, AIOFMobile, real);
    if (m_plugIns[AIOFPhysical])
        factory->requestInformationPlugin(handle, AIOFPhysical, real);
    else if (m_primaryObject->getPhysical())                                        // +0x94
        factory->requestInformationPlugin(m_primaryObject->getInformation(side),
                                          handle, AIOFPhysical, real);
    if (m_plugIns[AIOFUnit])
        factory->requestInformationPlugin(handle, AIOFUnit, real);
}

// The shared side store, then the primary follows unconditionally (no null test).
void AIObjectSecondaryShared::setSide(int side)
{
    if (m_side != side)
    {
        AIObjectShared::setSide(side);
        m_primaryObject->setSide(side);                                             // +0x128
    }
}

bool AIObjectSecondaryShared::isPrimary() const
{
    return false;
}

IAIObject* AIObjectSecondaryShared::getPrimaryObject()
{
    return m_primaryObject;
}

void AIObjectSecondaryShared::setPrimaryObject(IAIObject* primary)
{
    m_primaryObject = primary;
}

// this level's IBase over +0x40. No IObjectComponent answer.
void AIObjectSecondaryShared::addRef()
{
    ++m_refCount;
}

int AIObjectSecondaryShared::getRef()
{
    return m_refCount;
}

int AIObjectSecondaryShared::release()
{
    const int refCount = --m_refCount;
    if (refCount == 0)
        delete this;
    return refCount;
}

void* AIObjectSecondaryShared::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<AIObjectSecondaryShared*>(this);
    if (iid == IID_IAIObject)
        return const_cast<AIObjectSecondaryShared*>(this);
    return 0;
}

// - empty.
void AIObjectSecondaryShared::addInformationToGrid(int /*side*/)
{
}

void AIObjectSecondaryShared::removeFromGrid()
{
}

void AIObjectSecondaryShared::reInsertInGrid()
{
}

} // namespace ai
} // namespace hfe
} // namespace dice
