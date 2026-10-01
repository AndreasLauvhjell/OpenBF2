// Selects mobile nodes for integration and counts update calls. Source path [INFERRED].
// postFrameUpdate samples latency compensation once per frame, publishes each node's transform,
// then calls CollisionManager::postFrameUpdate. Preserve that ordering.

#include "PhysicsNodeManager.h"

#include "IPhysicsNode.h"              // The header this file's stubs waited on since
#include "BasicPhysicsSystem.h"         // IPhysicsSystem::update - the first call of every update()
#include "CollisionManager.h"           // collisionManager->postFrameUpdate - postFrameUpdate's tail
#include "LatencyCompensation.h"        // m_isEnabled - sampled once per postFrameUpdate
#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"    // getRootParent
#include "../Game/Objects/ObjectLayoutAccess.h"   // the objects' flag word (+0x04, bit 0)

namespace dice {
namespace hfe {
namespace world {

// Starts with one reference and an empty borrowed-node list.
PhysicsNodeManager::PhysicsNodeManager()
    : m_refCount(1)          // 0x8361744 `mov dword ptr [ebx+4], 1` - one-reference rule
    , m_physicsNodeList()
    , m_updateCount(0)
{
}

// Destroys list storage without deleting or releasing the borrowed physics nodes.
PhysicsNodeManager::~PhysicsNodeManager()
{
}

// IBase quartet. Byte-for-byte the shape Core/IBase.h documents.

// The ELF materialises the new count in eax; IBase declares addRef() void
// and that declaration is frozen, so the value is simply not returned here.
void PhysicsNodeManager::addRef()
{
    ++m_refCount;
}


int PhysicsNodeManager::getRef()
{
    return m_refCount;
}


// Decrements the reference count, retaining the explicit this check before deletion,
// and returns the new count.
int PhysicsNodeManager::release()
{
    int count = --m_refCount;
    if (count == 0 && this != 0)
        delete this;
    return count;
}

// Branchless leaf form - no base to delegate to.
void* PhysicsNodeManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<PhysicsNodeManager*>(this);
    if (iid == IID_IPhysicsNodeManager)
        return const_cast<PhysicsNodeManager*>(this);
    return 0;
}

// [CHOSEN] Returns the node list by reference; the binary alone does not distinguish
// reference from pointer. Any future type correction must update interface and implementation together.
std::list<IPhysicsNode*>& PhysicsNodeManager::getPhysicsNodeList()
{
    return m_physicsNodeList;
}

// Adds mobile nodes not already linked. Membership is tracked on each node, making
// repeated addition idempotent without searching. Static nodes remain unlinked.
void PhysicsNodeManager::addPhysicsNode(IPhysicsNode* node)
{
    if (!node)
        return;
    if (node->m_listPosition != m_physicsNodeList.end())    // already registered
        return;
    if (!node->getIsMobile())                               // the mobile-only gate
        return;

    m_physicsNodeList.insert(m_physicsNodeList.begin(), node);   // push front, as shipped
    node->m_listPosition = m_physicsNodeList.begin();
    node->reset(true);
}

// PhysicsNodeManager::removePhysicsNode(IPhysicsNode*) -
// . Exactly the inverse of addPhysicsNode, O(1) via the iterator the node
// carries - and this reset-to-end() is precisely what keeps IPhysicsNode's destructor abort from
// firing on a properly removed mobile node.

// this is the slot Game/Objects/ObjectTemplate.cpp:377 calls, and it calls the wrong one today.
// See the integrator note at the bottom of this file - that is a live defect in a file this agent
// does not own.
void PhysicsNodeManager::removePhysicsNode(IPhysicsNode* node)
{
    if (!node)
        return;
    if (node->m_listPosition == m_physicsNodeList.end())    // not in the list
        return;

    m_physicsNodeList.erase(node->m_listPosition);
    node->m_listPosition = m_physicsNodeList.end();
}

// Advances the update counter and physics-system timestep before any per-node early return.
// Updates the supplied node or the eligible list. Whole-list updates skip disabled roots/objects,
// separately updated nodes and disabled nodes. The second integer argument is unused.
void PhysicsNodeManager::update(float deltaTime, int /*unused - never read, see above*/, IPhysicsNode* node)
{
    typedef object_detail::LayoutAccess L;
    ++m_updateCount;    // 0x8361A02 `inc dword ptr [esi+0Ch]` - recovered
    static_cast<IPhysicsSystem*>(basicPhysicsSystem)->update(deltaTime);   // +0x18, ds:0x8B2784C

    if (node)                                                   // the single-node path
    {
        if (L::flags(node->getObject()) & 1)                    // +0x2C then +0x04, bit 0
            return;
        if (!node->getEnabled())                                // +0x1C
            return;
        node->updatePhysics(deltaTime);                         // +0xBC
        return;
    }
    for (std::list<IPhysicsNode*>::iterator it = m_physicsNodeList.begin();
         it != m_physicsNodeList.end(); ++it)
    {
        IPhysicsNode* n = *it;
        IObject* object = n->getObject();
        if (L::flags(getRootParent(object)) & 1)                // 0x8361A5x - the root's flags
            continue;
        if (L::flags(object) & 1)
            continue;
        if (n->getHasSeparatePhysicsUpdate())                   // +0x104
            continue;
        if (!n->getEnabled())                                   // +0x1C
            continue;
        n->updatePhysics(deltaTime);                            // +0xBC
    }
}

// Publishes enabled nodes' latency state when the once-per-frame flag permits, then post-frame state.
// Always calls CollisionManager::postFrameUpdate after the node walk.
void PhysicsNodeManager::postFrameUpdate()
{
    typedef object_detail::LayoutAccess L;
    const bool latency = LatencyCompensation::m_isEnabled;      // 0x836196x - once per frame
    for (std::list<IPhysicsNode*>::iterator it = m_physicsNodeList.begin();
         it != m_physicsNodeList.end(); ++it)
    {
        IPhysicsNode* n = *it;
        if (L::flags(n->getObject()) & 1)                       // +0x2C then +0x04, bit 0
            continue;
        if (latency)
            n->updateLatencyState();                            // +0x144
        n->postFrameUpdate();                                   // +0x184
    }
    collisionManager->postFrameUpdate();                        // ds:0x8B27840, +0x38 - the tail call
}

// Returns the original empty console-property list.
std::string PhysicsNodeManager::listProperties()
{
    return std::string();
}

// PhysicsNodeManager::getUpdateCount()  -
int PhysicsNodeManager::getUpdateCount()
{
    return m_updateCount;
}

} // namespace world
} // namespace hfe
} // namespace dice

// Use the complete IPhysicsNode and manager declarations for virtual dispatch.
// Object teardown must remove nodes through the real manager interface.
