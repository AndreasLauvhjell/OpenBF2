// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFPhysics operations.

#include "BFPhysics.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IAIPhysics::IAIPhysics()
{
    if (!CommonSingletonServer::m_iaiPhysics)
        classManager->registerSimpleSingleton(std::string("IAIPhysics"), this);
}

IAIPhysics::~IAIPhysics()
{
    if (CommonSingletonServer::m_iaiPhysics == this)
        classManager->unregisterSimpleSingleton(std::string("IAIPhysics"));
}

BFPhysics::BFPhysics()
{
}

BFPhysics::~BFPhysics()
{
}

// - world::basicPhysicsSystem (ds:) vtable +0x20.
// The whole class is a one-call forwarder; four bytes, one virtual.
float BFPhysics::getGravity() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFPhysics::getGravity");
}

} // namespace ai
} // namespace hfe
} // namespace dice
