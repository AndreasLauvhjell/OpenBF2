// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining ClusterNodeManager operations.

#include "ClusterNodeManager.h"

#include "../../AIUtils/checkPointer.h"
#include "../../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes empty node lists and a 313-bucket hash linked to this manager.

ClusterNodeManager::ClusterNodeManager()
    : m_quadTree(0)
    , m_clusterGraph(0)
    , m_i8(0)
    , m_i12(0)
    , m_i16(0)
    , m_i20(0)
    , m_i24(0)
    , m_i28(0)
    , m_i32(0)
    , m_i36(0)
    , m_i40(0)
    , m_freeOCData(0)
    , m_i48(0)
    , m_ocDataHash(checkPointer(new Hash<OCData, ClusterNodeManager, OCData>(kOCDataBuckets)))
    , m_i56(0)
    , m_i60(0)
    , m_i64(0)
    , m_i68(0)
{
    m_ocDataHash->m_nodeManager = this;
}

// [FRONTIER] Cleanup of the hash, buckets, and OCData chain remains unimplemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when ordered node-manager teardown is recovered.
ClusterNodeManager::~ClusterNodeManager()
{
    BF2_AI_FRONTIER("dice::hfe::ai::ClusterNodeManager::~ClusterNodeManager");
}

} // namespace ai
} // namespace hfe
} // namespace dice
