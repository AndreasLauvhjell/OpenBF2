// [INFERRED] Source placement.

#include "AIObjectPrimaryReal.h"

#include <algorithm>

#include "AITemplate.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)

#include "../../AIDLL/AI/AIKnowledge/BFInformationCourier.h"      // IAIInformationCourier
#include "../../AIDLL/AI/AIKnowledge/BFInformationEnvironment.h"  // IAIInformationEnvironmentModifiers
#include "../../AIDLL/AI/Main/AISettingsInterfaces.h"             // IAISettings::getNSides
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

IAIGame* game() { return static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame); }

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

} // anonymous namespace

// The vector, the destroying flag and the (0, -1e9, 0) sentinel
// in declaration order, then `resize(0)` - retail's own no-op on an empty vector (GCC's inline
// resize: erase on a shrink, _M_fill_insert otherwise; ).
AIObjectPrimaryReal::AIObjectPrimaryReal(AITemplate* aiTemplate, int side, world::IObject* object)
    : AIObjectReal(aiTemplate, side, object), m_refCount(1), m_secondaryObjects(),
      m_isDestroying(false), m_lastGridPosition{0.0f, -1.0e9f, 0.0f}
{
    m_secondaryObjects.resize(0);
}

// Marks destruction before detaching secondary AI components, removes live side-grid
// entries, then frees information. An unused own-side entry is skipped.
// The destructor uses the statically selected isUnit implementation.
AIObjectPrimaryReal::~AIObjectPrimaryReal()
{
    m_isDestroying = true;
    for (std::vector<IAIObject*>::iterator it = m_secondaryObjects.begin();
         it != m_secondaryObjects.end(); ++it)
    {
        (*it)->getObject()->setComponent(IID_IAIObject, 0);                         // +0x20, +0x58
    }

    const int count = settings()->getNSides() + 1;                                  // +0x40
    for (int side = 0; side < count; ++side)
    {
        if (m_handles[side] != 0xFFFFFFFFu)
        {
            const bool known = informationCourier()->getInformation(m_handles[side]) != 0;   // +0x08
            if (side != m_side || m_ownSideInGrid)
                informationModifiers()->destroyRemoveInfo(side, AIObject::isUnit(), m_handles[side]);  // +0x1C
            if (known)
                informationCourier()->freeInformation(m_handles[side]);             // +0x10
        }
    }
}

// Initializes the primary and its secondary objects. Does nothing while initialization is locked.
void AIObjectPrimaryReal::init()
{
    if (isLocked())
        return;

    if (m_secondaryObjects.empty())
    {
        traverseObject(m_object->getSibling());                                     // +0x2C
        traverseObject(m_object->getChild());                                       // +0xF0
    }

    AIObjectReal::init();
    if (game()->getIsPrecaching())                                                  // +0x68
        return;

    if (m_side)
    {
        createInformation(m_side, true);                                            // +0xAC
        for (std::vector<IAIObject*>::iterator it = m_secondaryObjects.begin();
             it != m_secondaryObjects.end(); ++it)
            (*it)->createInformation(m_side, true);
    }
    else if (!isOfType(ITSoldier))                                                  // +0xA4
    {
        createInformation(1, true);
        createInformation(2, true);
        m_ownSideInGrid = true;
        for (std::vector<IAIObject*>::iterator it = m_secondaryObjects.begin();
             it != m_secondaryObjects.end(); ++it)
        {
            (*it)->createInformation(1, true);
            (*it)->createInformation(2, true);
        }
    }

    if (m_object->getTemplate()->getClassID() == world::CID_ParachuteTemplate)
        removeFromGrid();                                                           // +0xC8
}

// The object's own re-teaming, then every secondary follows the side as it now
// stands (re-read per call).
void AIObjectPrimaryReal::setSide(int side)
{
    if (m_side != side)
    {
        AIObjectReal::setSide(side);
        for (std::vector<IAIObject*>::iterator it = m_secondaryObjects.begin();
             it != m_secondaryObjects.end(); ++it)
            (*it)->setSide(m_side);                                                 // +0x128
    }
}

bool AIObjectPrimaryReal::isPrimary() const
{
    return true;
}

bool AIObjectPrimaryReal::hasSecondaryPosition() const
{
    return !m_secondaryObjects.empty();
}

// push_back.
void AIObjectPrimaryReal::registrateSecondaryPosition(IAIObject* secondary)
{
    m_secondaryObjects.push_back(secondary);
}

// find and erase - unless the primary itself is being destroyed.
void AIObjectPrimaryReal::unregisterSecondaryPosition(IAIObject* secondary)
{
    if (!m_isDestroying)
    {
        std::vector<IAIObject*>::iterator it =
            std::find(m_secondaryObjects.begin(), m_secondaryObjects.end(), secondary);
        if (it != m_secondaryObjects.end())
            m_secondaryObjects.erase(it);
    }
}

// Along a sibling chain and down each child: every engine object
// with an AI component that is not a primary becomes this object's secondary. Meeting another
// primary ends the whole walk at once (a return, not a skip) - its own subtree and every sibling
// still to come are left alone. The AI component is IObject +0x34, read directly.
void AIObjectPrimaryReal::traverseObject(world::IObject* object)
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

// the iterator protocol the DLL walks secondaries with: the iterator is the
// next index; -1 (and a null object) at the end. The const overloads are the same bytes.
IAIObject* AIObjectPrimaryReal::getSecondaryObject(int& iterator)
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

const IAIObject* AIObjectPrimaryReal::getSecondaryObject(int& iterator) const
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

IAIObject* AIObjectPrimaryReal::getFirstSecondaryObject(int& iterator)
{
    if (!m_secondaryObjects.empty())
    {
        iterator = 1;
        return m_secondaryObjects[0];
    }
    iterator = -1;
    return 0;
}

const IAIObject* AIObjectPrimaryReal::getFirstSecondaryObject(int& iterator) const
{
    if (!m_secondaryObjects.empty())
    {
        iterator = 1;
        return m_secondaryObjects[0];
    }
    iterator = -1;
    return 0;
}

int AIObjectPrimaryReal::getFirstSecondaryObjectIterator() const
{
    return 0;
}

void AIObjectPrimaryReal::addInformationToGrid(int side)
{
    if (m_handles[side] != 0xFFFFFFFFu)
    {
        addInfoToGrid(m_handles[side], side);
        if (side == m_side)
            m_ownSideInGrid = true;
    }
}

// Per side with a handle: the courier is asked about it (result
// discarded), then the own side leaves through removeInfo - only if it is in the grid, and the flag
// drops - and every other side through safeRemoveInfo.
void AIObjectPrimaryReal::removeFromGrid()
{
    const int count = settings()->getNSides() + 1;
    for (int side = 0; side < count; ++side)
    {
        if (m_handles[side] != 0xFFFFFFFFu)
        {
            informationCourier()->getInformation(m_handles[side]);                  // result unused
            if (side == m_side)
            {
                if (m_ownSideInGrid)
                {
                    informationModifiers()->removeInfo(m_side, isUnit(), m_handles[side]);   // +0x14
                    m_ownSideInGrid = false;
                }
            }
            else
            {
                informationModifiers()->safeRemoveInfo(side, isUnit(), m_handles[side]);    // +0x18
            }
        }
    }
}

void AIObjectPrimaryReal::reInsertInGrid()
{
    const int count = settings()->getNSides() + 1;
    for (int side = 0; side < count; ++side)
    {
        if (m_handles[side] != 0xFFFFFFFFu)
            addInformationToGrid(side);                                             // +0xC4
    }
}

// Updates an entered own-side grid only after squared movement exceeds 0.1.
// Both side iterations use m_side, so a neutral object updates side zero twice.
// Unordered distances do not trigger an update.
void AIObjectPrimaryReal::positionChanged()
{
    if (!m_ownSideInGrid)
        return;

    const Vec3& position = getPosition();                                           // +0xD4
    const double dx = static_cast<double>(position.x) - m_lastGridPosition.x;
    const double dy = static_cast<double>(position.y) - m_lastGridPosition.y;
    const double dz = static_cast<double>(position.z) - m_lastGridPosition.z;
    if (!((dx * dx + dy * dy) + dz * dz > 0.1f))
        return;

    m_lastGridPosition = position;
    aiObjectPositionChanged();
    for (int side = 1; side <= 2; ++side)
    {
        if ((m_side == 0 || m_side == side) && m_handles[side] != 0xFFFFFFFFu)
            informationModifiers()->updatePosition(m_side, isUnit(), m_handles[m_side]);   // +0x10
    }
}

// this level's IBase over +0x48.
void AIObjectPrimaryReal::addRef()
{
    ++m_refCount;
}

int AIObjectPrimaryReal::getRef()
{
    return m_refCount;
}

int AIObjectPrimaryReal::release()
{
    const int refCount = --m_refCount;
    if (refCount == 0)
        delete this;
    return refCount;
}

void* AIObjectPrimaryReal::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IAIObject)
        return const_cast<AIObjectPrimaryReal*>(this);
    if (iid == world::IID_IObjectComponent)
        return const_cast<AIObjectPrimaryReal*>(this);
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
