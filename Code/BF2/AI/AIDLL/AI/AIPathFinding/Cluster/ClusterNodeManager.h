// Node pool for cluster-graph A* searches, including an intrusive free list for open/closed records.
// returnReset is intentionally empty.
// [FRONTIER] Remaining search operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the node-pool lifecycle and search operations.

#ifndef AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERNODEMANAGER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERNODEMANAGER_H

#include "../../Main/AIPathfindingSearchTypes.h"      // Hash

namespace dice {
namespace hfe {
namespace ai {

class ClusterGraph;
class QuadTree;
class ClusterNode;

class ClusterNodeManager
{
public:
    // The open/closed record. Its only recovered field is the free-list link at +8, which
    // `recycleOCData` writes, so the type stays incomplete here rather than guessing a size.
    // Attested as a nested type by the mangled names of `getNewOCData`, `recycleOCData` and
    // `HashIterator<ClusterNodeManager::OCData, ...>`.
    class OCData;

    // the same prime the cluster search resource'S two hashes use. A literal in the binary.
    enum { kOCDataBuckets = 313 };

    ClusterNodeManager();
    ~ClusterNodeManager();       // , [FRONTIER]

    // layout, on the ELF, read store by store out of the constructor
    /*  +0 */ QuadTree* m_quadTree;          // = 0; `setQuadTree` writes it
    /*  +4 */ ClusterGraph* m_clusterGraph;  // = 0; `setClusterGraph` writes it
    /*  +8 */ int m_i8;                      // = 0 [UNRESOLVED]
    /* +12 */ int m_i12;                     // = 0 [UNRESOLVED]
    /* +16 */ int m_i16;                     // = 0 [UNRESOLVED]
    /* +20 */ int m_i20;                     // = 0 [UNRESOLVED]
    /* +24 */ int m_i24;                     // = 0 [UNRESOLVED]
    /* +28 */ int m_i28;                     // = 0 [UNRESOLVED]
    /* +32 */ int m_i32;                     // = 0 [UNRESOLVED]
    /* +36 */ int m_i36;                     // = 0 [UNRESOLVED]
    /* +40 */ int m_i40;                     // = 0 [UNRESOLVED]
    /* +44 */ OCData* m_freeOCData;          // = 0 the intrusive free list `recycleOCData` pushes to
    /* +48 */ int m_i48;                     // = 0 [UNRESOLVED]
    /* +52 */ Hash<OCData, ClusterNodeManager, OCData>* m_ocDataHash;   // new Hash(313), then ->this
    /* +56 */ int m_i56;                     // = 0 [UNRESOLVED]
    /* +60 */ int m_i60;                     // = 0 [UNRESOLVED]
    /* +64 */ int m_i64;                     // = 0 [UNRESOLVED]
    /* +68 */ int m_i68;                     // = 0 [UNRESOLVED]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERNODEMANAGER_H
