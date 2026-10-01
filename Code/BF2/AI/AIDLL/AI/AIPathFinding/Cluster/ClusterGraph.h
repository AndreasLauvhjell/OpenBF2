// Navigation cluster graph with file loading and cleanup.
// [FRONTIER] Remaining graph queries and mesh generation require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

#ifndef AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERGRAPH_H
#define AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERGRAPH_H

#include <set>

#include "../../Main/AIDllTypes.h"      // Vec3
#include "Cluster.h"

namespace dice {
namespace hfe {

namespace io { class File; }

namespace ai {

class QuadTree;

// Portal record used by the cluster graph and its serialized format.
class ClusterPortal
{
public:
    ClusterPortal() { m_position.x = 0.0f; m_position.y = 0.0f; m_position.z = 0.0f; }

    /* +0x00 */ Vec3          m_position;          // [INFERRED] name
    /* +0x0C */ unsigned char m_unread0C[8];       // [UNRESOLVED] - see the note above
};

typedef char ClusterPortalIsTheFileRecord[sizeof(ClusterPortal) == 20 ? 1 : -1];

class ClusterGraph
{
public:
    ClusterGraph();
    ~ClusterGraph();

    void clean();
    void readFromFile(io::File& file);

    // [CHOSEN name] Returns the stored cluster count used by QuadTreeManager.
    unsigned int getNumberOfClusters() const { return m_numberOfClusters; }

private:
    friend class QuadTree;
    /* +0x00 */ unsigned int*  m_clusterPolygons;           // [INFERRED] polygon indices grouped by cluster
    /* +0x04 */ unsigned int   m_numberOfClusterPolygons;   // (Dalian Infantry: 18,512, every polygon once)
    /* +0x08 */ Cluster*       m_clusters;                  // getClusterPortal
    /* +0x0C */ unsigned int   m_numberOfClusters;          // loadQuadTrees' empty walk bound
    /* +0x10 */ unsigned int   m_numberOfPortals;
    /* +0x14 */ ClusterPortal* m_portals;                   // getClusterPortal
    /* +0x18 */ std::set<unsigned int>   m_set18;          // [UNRESOLVED] roles for both sets and
    /* +0x24 */ unsigned char*           m_array24;        // the +0x24 array: only the GTS
                                                             // generation path (not written) fills
                                                             // them; clean frees and nulls +0x24
    /* +0x28 */ unsigned int   m_numberOfPortalCosts;
    // float: the PE allocates it through its own checkPointer instantiation (, not the
    // uint one at the +0x00 list uses), and the shipped values are plainly floats
    // (Dalian Infantry opens 31.67, 24.51, 14.41, 56.92). Name [INFERRED] from
    // QuadTreeManager::recalculatePortalCosts -> ClusterGraph::calculatePortalCosts.
    /* +0x2C */ float*         m_portalCosts;
    /* +0x30 */ std::set<unsigned short> m_set30;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTERGRAPH_H
