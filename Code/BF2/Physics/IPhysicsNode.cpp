// Shared physics-node interface lifecycle. [INFERRED] Source placement beside its header.

#include "IPhysicsNode.h"

#include <cstdlib>                     // abort() - yes, really; see the destructor

#include "PhysicsNodeManager.h"        // physicsNodeManager, getPhysicsNodeList

namespace dice {
namespace hfe {
namespace world {

extern PhysicsNodeManager* physicsNodeManager;   // ds:0x8B27844, Core/Globals.cpp - attested

// `m_listPosition = end()` = "not in any manager list".
// physicsNodeManager is read unguarded, as shipped - the manager exists from initSingletons on.
IPhysicsNode::IPhysicsNode()
{
    m_listPosition = physicsNodeManager->getPhysicsNodeList().end();
}

// The check is exactly this and nothing else: recompute end(), compare, and
// abort() - not Debug, not a leak, a process abort - if the node is still registered. It can never
// fire for a static node (addPhysicsNode refuses non-mobile nodes, so their iterator never leaves
// end()); for mobile nodes it makes "destroyed while simulating" maximally loud.
IPhysicsNode::~IPhysicsNode()
{
    if (m_listPosition != physicsNodeManager->getPhysicsNodeList().end())
        abort();
}

} // namespace world
} // namespace hfe
} // namespace dice
