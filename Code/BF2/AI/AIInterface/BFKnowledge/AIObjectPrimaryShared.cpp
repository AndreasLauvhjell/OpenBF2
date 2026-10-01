// [INFERRED] Source placement.

#include "AIObjectPrimaryShared.h"

#include <algorithm>

#include "../../AIDLL/AI/AIKnowledge/BFInformationCourier.h"      // IAIInformationCourier
#include "../../AIDLL/AI/AIKnowledge/BFInformationEnvironment.h"  // IAIInformationEnvironmentModifiers
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"
#include "../../../Game/Objects/IObject.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

IAIInformationCourier* informationCourier()
{
    return static_cast<IAIInformationCourier*>(CommonSingletonServer::m_iaiInformationCourier);
}

IAIInformationEnvironmentModifiers* informationModifiers()
{
    return static_cast<IAIInformationEnvironmentModifiers*>(
        CommonSingletonServer::m_iaiInformationEnvironmentModifiers);
}

} // anonymous namespace

// - PrimaryReal's constructor over the shared base; see that note.
AIObjectPrimaryShared::AIObjectPrimaryShared(AITemplate* aiTemplate, int side,
                                             world::IObject* object)
    : AIObjectShared(aiTemplate, side, object), m_refCount(1), m_secondaryObjects(),
      m_isDestroying(false), m_lastGridPosition{0.0f, -1.0e9f, 0.0f}
{
    m_secondaryObjects.resize(0);
}

// The group goes first (each non-null secondary's engine object drops its
// IID_IAIObject component - PrimaryReal has no null test), then the single handle: out of the grid
// through removeInfo on side 0 if it is in, and freed if the courier still knows it.
AIObjectPrimaryShared::~AIObjectPrimaryShared()
{
    m_isDestroying = true;
    for (std::vector<IAIObject*>::iterator it = m_secondaryObjects.begin();
         it != m_secondaryObjects.end(); ++it)
    {
        if (*it)
            (*it)->getObject()->setComponent(IID_IAIObject, 0);                     // +0x20, +0x58
    }

    if (m_handle != 0xFFFFFFFFu)
    {
        if (m_ownSideInGrid)
            informationModifiers()->removeInfo(0, AIObject::isUnit(), m_handle);    // +0x14
        if (informationCourier()->getInformation(m_handle))                         // +0x08
            informationCourier()->freeInformation(m_handle);                        // +0x10
    }
}

// Initializes the shared object, template, plug-ins, and neutral information before
// gathering the group. Secondary information is absent during the first initialization.
void AIObjectPrimaryShared::init()
{
    if (isLocked())
        return;

    AIObjectShared::init();
    if (m_secondaryObjects.empty())
    {
        traverseObject(m_object->getSibling());                                     // +0x2C
        traverseObject(m_object->getChild());                                       // +0xF0
    }
}

// - PrimaryReal::traverseObject's body; see the note there.
void AIObjectPrimaryShared::traverseObject(world::IObject* object)
{
    while (object)
    {
        IAIObject* const aiObject = static_cast<IAIObject*>(object->getAIComponent());
        if (aiObject)
        {
            if (aiObject->isPrimary())                                              // +0xF8
                return;
            aiObject->setPrimaryObject(this);                                       // +0x100
            registrateSecondaryPosition(aiObject);                                  // +0x10C
        }
        traverseObject(object->getSibling());                                       // +0x2C
        object = object->getChild();                                                // +0xF0
    }
}

bool AIObjectPrimaryShared::isPrimary() const
{
    return true;
}

bool AIObjectPrimaryShared::hasSecondaryPosition() const
{
    return !m_secondaryObjects.empty();
}

void AIObjectPrimaryShared::registrateSecondaryPosition(IAIObject* secondary)
{
    m_secondaryObjects.push_back(secondary);
}

void AIObjectPrimaryShared::unregisterSecondaryPosition(IAIObject* secondary)
{
    if (!m_isDestroying)
    {
        std::vector<IAIObject*>::iterator it =
            std::find(m_secondaryObjects.begin(), m_secondaryObjects.end(), secondary);
        if (it != m_secondaryObjects.end())
            m_secondaryObjects.erase(it);
    }
}

// The secondaries are handed the argument (PrimaryReal re-reads m_side).
void AIObjectPrimaryShared::setSide(int side)
{
    if (m_side != side)
    {
        AIObjectShared::setSide(side);
        for (std::vector<IAIObject*>::iterator it = m_secondaryObjects.begin();
             it != m_secondaryObjects.end(); ++it)
            (*it)->setSide(side);                                                   // +0x128
    }
}

// the iterator protocol; see AIObjectPrimaryReal.cpp.
IAIObject* AIObjectPrimaryShared::getSecondaryObject(int& iterator)
{
    const int index = iterator;
    if (index >= 0 && static_cast<int>(m_secondaryObjects.size()) > index)
    {
        iterator = index + 1;
        return m_secondaryObjects[index];
    }
    iterator = -1;
    return 0;
}

IAIObject* AIObjectPrimaryShared::getFirstSecondaryObject(int& iterator)
{
    if (!m_secondaryObjects.empty())
    {
        iterator = 1;
        return m_secondaryObjects[0];
    }
    iterator = -1;
    return 0;
}

const IAIObject* AIObjectPrimaryShared::getSecondaryObject(int& iterator) const
{
    const int index = iterator;
    if (index >= 0 && static_cast<int>(m_secondaryObjects.size()) > index)
    {
        iterator = index + 1;
        return m_secondaryObjects[index];
    }
    iterator = -1;
    return 0;
}

const IAIObject* AIObjectPrimaryShared::getFirstSecondaryObject(int& iterator) const
{
    if (!m_secondaryObjects.empty())
    {
        iterator = 1;
        return m_secondaryObjects[0];
    }
    iterator = -1;
    return 0;
}

int AIObjectPrimaryShared::getFirstSecondaryObjectIterator() const
{
    return 0;
}

// each. The side argument is ignored - the one handle lives
// on side 0. addInformationToGrid and reInsertInGrid are the same bytes: in if not already in.
void AIObjectPrimaryShared::addInformationToGrid(int /*side*/)
{
    if (m_handle != 0xFFFFFFFFu && !m_ownSideInGrid)
    {
        informationModifiers()->addInfo(0, isUnit(), m_handle);                     // +0x08
        m_ownSideInGrid = true;
    }
}

void AIObjectPrimaryShared::removeFromGrid()
{
    if (m_handle != 0xFFFFFFFFu && m_ownSideInGrid)
    {
        informationModifiers()->removeInfo(0, isUnit(), m_handle);                  // +0x14
        m_ownSideInGrid = false;
    }
}

void AIObjectPrimaryShared::reInsertInGrid()
{
    if (m_handle != 0xFFFFFFFFu && !m_ownSideInGrid)
    {
        informationModifiers()->addInfo(0, isUnit(), m_handle);
        m_ownSideInGrid = true;
    }
}

// Past 0.1 squared metres of movement: the grid entry first, then
// the mobile plug-in (PrimaryReal notifies the plug-in first). Distance in double, as there.
void AIObjectPrimaryShared::positionChanged()
{
    if (!m_ownSideInGrid || m_handle == 0xFFFFFFFFu)
        return;

    const Vec3& position = getPosition();                                           // +0xD4
    const double dx = static_cast<double>(position.x) - m_lastGridPosition.x;
    const double dy = static_cast<double>(position.y) - m_lastGridPosition.y;
    const double dz = static_cast<double>(position.z) - m_lastGridPosition.z;
    if ((dx * dx + dy * dy) + dz * dz > 0.1f)
    {
        m_lastGridPosition = position;
        informationModifiers()->updatePosition(0, isUnit(), m_handle);              // +0x10
        aiObjectPositionChanged();
    }
}

// this level's IBase over +0x40. No IObjectComponent answer.
void AIObjectPrimaryShared::addRef()
{
    ++m_refCount;
}

int AIObjectPrimaryShared::getRef()
{
    return m_refCount;
}

int AIObjectPrimaryShared::release()
{
    const int refCount = --m_refCount;
    if (refCount == 0)
        delete this;
    return refCount;
}

void* AIObjectPrimaryShared::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<AIObjectPrimaryShared*>(this);
    if (iid == IID_IAIObject)
        return const_cast<AIObjectPrimaryShared*>(this);
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
