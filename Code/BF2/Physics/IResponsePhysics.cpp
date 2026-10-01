// Path:     [INFERRED] - beside its header.

#include "IResponsePhysics.h"

#include "CollisionStats.h"             // the block this TU owns - see below
#include "ResponsePhysicsManager.h"     // ResponsePhysicsList

namespace dice {
namespace hfe {
namespace world {

// Owns the zero-initialized shared collision statistics block; queries increment it and
// ResponsePhysicsManager resets it at each frame start.
CollisionStatsData  collisionStatsData_internal = {0};      // zeroed, as the ELF's memset
CollisionStatsData* collisionStatsData = &collisionStatsData_internal;   // ds:0x898BA4C

// vptr plus the three link fields, zeroed.
IResponsePhysics::IResponsePhysics()
{
    m_listPrev = 0;
    m_listNext = 0;
    m_listTag = 0;
}

// 14 B - vptr reset only. no still-listed abort here, unlike IPhysicsNode's destructor: the two
// sides of the physics family made opposite choices about dying while registered, and the response
// side's removeObject is what keeps the lists clean instead.
IResponsePhysics::~IResponsePhysics() {}

// the tag raw, so the caller learns list identity too.
unsigned char IResponsePhysics::isInList() const
{
    return m_listTag;
}

// transcribed branch for branch.
bool IResponsePhysics::pushBackOnList(ResponsePhysicsList& list, unsigned char tag)
{
    if (m_listTag != 0)                    // already on a list - refuse, touch nothing
        return false;

    if (!list.head)                        // empty list: become both ends
    {
        list.head = this;
        list.tail = this;
        m_listNext = 0;
        m_listPrev = 0;
    }
    else
    {
        if (list.tail->m_listNext)         // corruption guard, as shipped: a tail with a next is
        {                                  // broken - zero own links and refuse
            m_listNext = 0;
            m_listPrev = 0;
            return false;
        }
        list.tail->m_listNext = this;      // append after the old tail
        m_listNext = 0;
        m_listPrev = list.tail;
        list.tail = this;
    }

    m_listTag = tag;                       // membership and identity in one byte
    return true;
}

// Unlinks this node from the specified response list. Returns false when head, tail,
// or middle-link guards reject the operation; does not repair inconsistent links.
bool IResponsePhysics::removeFromList(ResponsePhysicsList& list)
{
    if (m_listTag == 0)                    // not on any list - nothing to do
        return false;

    if (list.head == this)                 // the head ----
    {
        if (m_listPrev)                    // a head with a prev is corrupt - refuse
            return false;
        if (list.tail == this)             // the only node: empty the list
        {
            list.head = 0;
            list.tail = 0;
        }
        else                               // promote the next node
        {
            IResponsePhysics* next = m_listNext;
            next->m_listPrev = 0;
            list.head = next;
        }
    }
    else if (list.tail == this || m_listNext == 0)   // the tail, or a node with no next ----
    {
        IResponsePhysics* prev = m_listPrev;
        if (!prev || !list.head)           // no prev, or a list with no head - refuse
            return false;
        prev->m_listNext = 0;
        list.tail = prev;
    }
    else                                   // a middle node ----
    {
        IResponsePhysics* prev = m_listPrev;
        if (!prev)
            return false;
        prev->m_listNext = m_listNext;
        m_listNext->m_listPrev = prev;
    }

    m_listNext = 0;
    m_listPrev = 0;
    m_listTag  = 0;
    return true;
}

} // namespace world
} // namespace hfe
} // namespace dice
