// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining QTSearchNodeManager operations.

#include "QTSearchNodeManager.h"

#include "../../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Constructors share initialization except for the optional quad-tree pointer.
// Keep both overloads; observed stores do not establish member declaration order.

QTSearchNodeManager::QTSearchNodeManager()
    : m_quadTree(0)
    , m_i4(0)
    , m_nodeCache(0)
    , m_i12(0)
    , m_i16(0)
    , m_i20(0)
    , m_table(new CacheEntry[kCacheEntries])
    , m_i28(0)
    , m_i32(0)
    , m_i36(0)
    , m_i40(0)
    , m_i44(0)
    , m_b48(false)
    , m_i52(0)
{
}

QTSearchNodeManager::QTSearchNodeManager(QuadTree* quadTree)
    : m_quadTree(quadTree)
    , m_i4(0)
    , m_nodeCache(0)
    , m_i12(0)
    , m_i16(0)
    , m_i20(0)
    , m_table(new CacheEntry[kCacheEntries])
    , m_i28(0)
    , m_i32(0)
    , m_i36(0)
    , m_i40(0)
    , m_i44(0)
    , m_b48(false)
    , m_i52(0)
{
}

// [FRONTIER] Node-manager table and cache-chain teardown remains unimplemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when all owned-resource cleanup is recovered.
QTSearchNodeManager::~QTSearchNodeManager()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QTSearchNodeManager::~QTSearchNodeManager");
}

} // namespace ai
} // namespace hfe
} // namespace dice
