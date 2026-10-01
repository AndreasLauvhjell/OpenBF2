// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining ClusterGraph operations.

#include "ClusterGraph.h"

#include "../../AIUtils/checkPointer.h"
#include "../../../../../IO/File/File.h"

namespace dice {
namespace hfe {
namespace ai {

// Zeroes the six fields at +0x00..+0x14 and the three at +0x24..+0x2C; the two sets
// construct empty (their sentinel allocations are the only calls). is bigger only because the
// SGI node allocator is inlined into it - same source, two inlining decisions.
ClusterGraph::ClusterGraph()
    : m_clusterPolygons(0)
    , m_numberOfClusterPolygons(0)
    , m_clusters(0)
    , m_numberOfClusters(0)
    , m_numberOfPortals(0)
    , m_portals(0)
    , m_array24(0)
    , m_numberOfPortalCosts(0)
    , m_portalCosts(0)
{
}

// clean, then the two sets in reverse declaration order.
ClusterGraph::~ClusterGraph()
{
    clean();
}

// Releases arrays and clears counts, but leaves four freed pointers unchanged.
// Do not clean twice without repopulating those pointers; readFromFile assigns them
// immediately after cleanup. Only the pointer at +0x24 is cleared.
void ClusterGraph::clean()
{
    delete[] m_clusterPolygons;
    m_numberOfClusterPolygons = 0;

    delete[] m_clusters;
    m_numberOfClusters = 0;

    delete[] m_portals;
    m_numberOfPortals = 0;

    delete[] m_array24;
    m_array24 = 0;

    delete[] m_portalCosts;
    m_numberOfPortalCosts = 0;
}

// Loads a version-5 cluster graph. Unsupported versions leave the existing graph unchanged.
void ClusterGraph::readFromFile(io::File& file)
{
    int version = 0;
    file.read(version);

    if (version == 5)
    {
        clean();

        file.read(m_numberOfClusters);
        m_clusters = checkPointer(new Cluster[m_numberOfClusters]);
        file.read(m_clusters, sizeof(Cluster) * m_numberOfClusters);

        file.read(m_numberOfClusterPolygons);
        m_clusterPolygons = checkPointer(new unsigned int[m_numberOfClusterPolygons]);
        file.read(m_clusterPolygons, sizeof(unsigned int) * m_numberOfClusterPolygons);

        file.read(m_numberOfPortals);
        if (m_numberOfPortals)
        {
            m_portals = checkPointer(new ClusterPortal[m_numberOfPortals]);
            file.read(m_portals, sizeof(ClusterPortal) * m_numberOfPortals);
        }
        else
        {
            m_portals = 0;
        }

        file.read(m_numberOfPortalCosts);
        if (m_numberOfPortalCosts)
        {
            m_portalCosts = checkPointer(new float[m_numberOfPortalCosts]);
            file.read(m_portalCosts, sizeof(float) * m_numberOfPortalCosts);
        }
        else
        {
            m_portalCosts = 0;
        }
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice
