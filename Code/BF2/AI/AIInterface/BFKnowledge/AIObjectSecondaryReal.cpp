// [INFERRED] Source placement.

#include "AIObjectSecondaryReal.h"

#include "IAIObjectPlugIn.h"

#include "../SingletonServer.h"                              // BFSingletonServer::m_iInformationFactory
#include "../../AIDLL/AI/AIKnowledge/BFInformationCourier.h"      // IAIInformationCourier
#include "../../AIDLL/AI/AIKnowledge/InformationFactory.h"        // IInformationFactory
#include "../../AIDLL/AI/Main/AISettingsInterfaces.h"             // IAISettings::getNSides
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

IAISettings* settings() { return static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings); }

IAIInformationCourier* informationCourier()
{
    return static_cast<IAIInformationCourier*>(CommonSingletonServer::m_iaiInformationCourier);
}

} // anonymous namespace

AIObjectSecondaryReal::AIObjectSecondaryReal(AITemplate* aiTemplate, int side,
                                             world::IObject* object)
    : AIObjectReal(aiTemplate, side, object), m_refCount(1), m_primaryObject(0)
{
}

AIObjectSecondaryReal::AIObjectSecondaryReal(AITemplate* aiTemplate, int side,
                                             world::IObject* object, IAIObject* primary)
    : AIObjectReal(aiTemplate, side, object), m_refCount(1), m_primaryObject(primary)
{
}

void AIObjectSecondaryReal::init()
{
    if (!isLocked())
        AIObjectReal::init();
}

// Leaves its primary's list, then frees every handle the courier still knows.
// Nothing is taken out of the grid - a secondary never enters it.
AIObjectSecondaryReal::~AIObjectSecondaryReal()
{
    if (m_primaryObject)
        m_primaryObject->unregisterSecondaryPosition(this);                         // +0x110

    const int count = settings()->getNSides() + 1;                                  // +0x40
    for (int side = 0; side < count; ++side)
    {
        if (m_handles[side] != 0xFFFFFFFFu &&
            informationCourier()->getInformation(m_handles[side]))                  // +0x08
            informationCourier()->freeInformation(m_handles[side]);                 // +0x10
    }
}

// A secondary still on its primary's side hands the change UP (the
// primary re-teams, and re-teams its secondaries - this one included); one that has drifted onto
// another side re-teams alone. No null test on the primary.
void AIObjectSecondaryReal::setSide(int side)
{
    if (m_side != side)
    {
        if (m_primaryObject->getSide() == m_side)                                   // +0x12C
            m_primaryObject->setSide(side);                                         // +0x128
        else
            AIObjectReal::setSide(side);
    }
}

bool AIObjectSecondaryReal::isPrimary() const
{
    return false;
}

IAIObject* AIObjectSecondaryReal::getPrimaryObject()
{
    return m_primaryObject;
}

void AIObjectSecondaryReal::setPrimaryObject(IAIObject* primary)
{
    m_primaryObject = primary;
}

// Creates plug-ins in factory index order. Cover (2) and physical (4) reuse the primary
// side handle when absent. Requires a primary object and caches its factory pointer.
void AIObjectSecondaryReal::createInfoPlugins(unsigned int handle, bool real, int side)
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

// this level's IBase over +0x48.
void AIObjectSecondaryReal::addRef()
{
    ++m_refCount;
}

int AIObjectSecondaryReal::getRef()
{
    return m_refCount;
}

int AIObjectSecondaryReal::release()
{
    const int refCount = --m_refCount;
    if (refCount == 0)
        delete this;
    return refCount;
}

void* AIObjectSecondaryReal::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IAIObject)
        return const_cast<AIObjectSecondaryReal*>(this);
    if (iid == world::IID_IObjectComponent)
        return const_cast<AIObjectSecondaryReal*>(this);
    return 0;
}

// - a secondary has no grid entry of its own.
void AIObjectSecondaryReal::addInformationToGrid(int /*side*/)
{
}

void AIObjectSecondaryReal::removeFromGrid()
{
}

void AIObjectSecondaryReal::reInsertInGrid()
{
}

} // namespace ai
} // namespace hfe
} // namespace dice
