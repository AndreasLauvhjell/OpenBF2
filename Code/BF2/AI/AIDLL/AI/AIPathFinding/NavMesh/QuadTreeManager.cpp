// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining QuadTreeManager operations.

#include "QuadTreeManager.h"

#include <cstdlib>                                       // std::abort - the ELF's island trap

#include "QuadTree.h"
#include "../Cluster/ClusterGraph.h"
#include "../../AIUtils/checkPointer.h"
#include "../../Main/AIDllFrontier.h"
#include "../../Main/CommonSingletonServer.h"
#include "../../../../AIInterface/BFKnowledge/BFGame.h"  // IAIGame::getLevelPath
#include "../../../../../IO/File/File.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes an empty quad-tree collection and its island metadata.
QuadTreeManager::QuadTreeManager()
{
    // m_numberOfIslands deliberately not initialised - the binary steps over it.
}

// [FRONTIER] Manager destruction requires the remaining quad-tree and debug-mesh cleanup.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding QuadTreeManager operation.
QuadTreeManager::~QuadTreeManager()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::~QuadTreeManager");
}

// Returns the selected island or cluster graph. The caller must supply a valid island index.
QuadTree* QuadTreeManager::getIsland(unsigned int island) const
{
    return m_islands[island].first;
}

ClusterGraph* QuadTreeManager::getClusterGraph(unsigned int island) const
{
    return m_islands[island].second;
}

unsigned int QuadTreeManager::getNumberOfIslands() const
{
    return static_cast<unsigned int>(m_islands.size());
}

// Deletes both halves of every island's pair (each `delete` null-tests for itself), then
// `resize(0)` - the shape in both images: g++ 3.x's size test with erase / zero-length
// _M_fill_insert, and MSVC's size test with an erase that only moves `end` back. Not clear: the
// vector keeps its capacity in both. Called first by loadQuadTrees, so a reload frees the old
// islands before the new files are even opened - a reload that then fails to open leaves the map
// with no islands at all.
void QuadTreeManager::destroyQuadTrees()
{
    for (std::vector<std::pair<QuadTree*, ClusterGraph*> >::iterator it = m_islands.begin();
         it != m_islands.end(); ++it)
    {
        delete it->first;
        delete it->second;
    }
    m_islands.resize(0);
}

// Deletes the matching quad tree while retaining its island entry and nulling its pointer.
void QuadTreeManager::destroyQuadTree(QuadTree* quadTree)
{
    std::vector<std::pair<QuadTree*, ClusterGraph*> >::iterator it = m_islands.begin();
    for (; it != m_islands.end(); ++it)
    {
        if (it->first == quadTree)
            break;
    }

    if (it != m_islands.end())
    {
        delete it->first;
        it->first = 0;
    }
}

// Loads navigation meshes for a map into the managed quad trees.
void QuadTreeManager::loadQuadTrees(const std::string& name)
{
    destroyQuadTrees();

    const std::string levelPath =
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getLevelPath();

    io::File quadTreeFile;
    if (!quadTreeFile.open(levelPath + "/AIPathFinding/" + name + ".qtr", io::FILEMODE_READ,
                           static_cast<io::FileAccess>(0), 0))
        return;

    io::File clusterFile;
    if (!clusterFile.open(levelPath + "/AIPathFinding/" + name + ".clb", io::FILEMODE_READ,
                          static_cast<io::FileAccess>(0), 0))
        return;

    quadTreeFile.read(m_numberOfIslands);
    if (m_numberOfIslands > 1)
    {
        // [PLATFORM] ELF: signed `cmp edi, 1 / jg` at 0x30AF0A, `call abort` at 0x30B385. PE: signed
        // `cmp [ebx], 2 / jl` at , `int 3` at - which falls through into the
        // island loop if a debugger resumes it.
#if defined(_MSC_VER)
        __debugbreak();
#else
        std::abort();
#endif
    }

    for (unsigned int island = 0; island < static_cast<unsigned int>(m_numberOfIslands); ++island)
    {
        QuadTree* const quadTree = checkPointer(new QuadTree(island));
        quadTree->readFromFile(quadTreeFile);

        ClusterGraph* const clusterGraph = checkPointer(new ClusterGraph);
        clusterGraph->readFromFile(clusterFile);

        m_islands.push_back(std::make_pair(quadTree, clusterGraph));
    }

    for (unsigned int island = 0; island < static_cast<unsigned int>(m_numberOfIslands); ++island)
    {
        for (unsigned short cluster = 0;
             cluster < m_islands[island].second->getNumberOfClusters();
             ++cluster)
        {
        }
    }

    clusterFile.close();
    quadTreeFile.close();
}

// [FRONTIER] Remaining operations require quad-tree, cluster-graph, GTS or debug-mesh support.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void QuadTreeManager::saveQuadTrees(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::saveQuadTrees");
}

void QuadTreeManager::loadGTSData(const std::string&, int, int, int, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::loadGTSData");
}

void QuadTreeManager::recalculatePortalCosts(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::recalculatePortalCosts");
}

void QuadTreeManager::draw()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::draw");
}

void QuadTreeManager::undraw()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::undraw");
}

void QuadTreeManager::drawCluster()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::drawCluster");
}

void QuadTreeManager::undrawCluster()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::undrawCluster");
}

void QuadTreeManager::drawClusterNeighbourIndices(const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::drawClusterNeighbourIndices");
}

void QuadTreeManager::buildDebugMeshes(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::buildDebugMeshes");
}

void QuadTreeManager::buildDebugClusterMeshes(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::buildDebugClusterMeshes");
}

void QuadTreeManager::loadGTSDebugMeshes(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::loadGTSDebugMeshes");
}

void QuadTreeManager::unloadDebugMeshes()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::unloadDebugMeshes");
}

void QuadTreeManager::unloadDebugClusterMeshes()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::unloadDebugClusterMeshes");
}

void QuadTreeManager::triangulateDebugMeshes(std::list<DebugMesh*>&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::triangulateDebugMeshes");
}

// Visit islands in vector order and return the first
// whose tree finds a polygon. The polygon output is whatever that tree wrote; a total miss leaves it
// untouched and returns UINT_MAX.
unsigned int QuadTreeManager::findPolygon(const Vec3& point, unsigned int& polygon,
                                          MaterialManager* materials, float allowedHeightDiff)
{
    for (unsigned int island = 0; island < m_islands.size(); ++island)
    {
        if (m_islands[island].first->findPolygon(point, polygon, materials, allowedHeightDiff))
            return island;
    }

    return 0xFFFFFFFFu;
}

// Reuse a known polygon/island only when neither is the
// sentinel and the point still belongs to it; otherwise run the full scan. The island is deliberately
// not bounds-checked before indexing the vector.
unsigned int QuadTreeManager::findPolygon(const Vec3& point, unsigned int& polygon,
                                          unsigned int knownPolygon, unsigned int knownIsland,
                                          MaterialManager* materials, float allowedHeightDiff)
{
    if (knownIsland == 0xFFFFFFFFu || knownPolygon == 0xFFFFFFFFu
        || !m_islands[knownIsland].first->isPointInPolygon(
            point, knownPolygon, allowedHeightDiff, materials))
        return findPolygon(point, polygon, materials, allowedHeightDiff);

    polygon = knownPolygon;
    return knownIsland;
}

void QuadTreeManager::printPolygonsInNode(const Vec3&, unsigned int&, MaterialManager*, float) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::printPolygonsInNode");
}

unsigned short QuadTreeManager::getCluster(unsigned int, unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::getCluster");
}

void QuadTreeManager::getClusterPortals(unsigned int, unsigned short, unsigned short,
                                        std::list<ClusterPortal*>&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::getClusterPortals");
}

void QuadTreeManager::getPolygonsInRadius(std::list<unsigned int>&, unsigned int, unsigned int,
                                          const Vec3&, float) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::getPolygonsInRadius");
}

void QuadTreeManager::getPolygonsInTriangle(std::list<unsigned int>&, unsigned int, unsigned int,
                                            const Vec3&, const Vec3&, const Vec3&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::getPolygonsInTriangle");
}

// Forward to every island in ascending order, re-reading the vector
// size on each pass. Runtime map parts use the ordinary float-vertex path (`useDoubles=false`).
void QuadTreeManager::initializeDestroyableMapPart(int mapPartId, bool initiallyWalkable,
                                                   const Vec3& minimum, const Vec3& maximum,
                                                   const Mat4& transformation)
{
    for (unsigned int island = 0; island < m_islands.size(); ++island)
        m_islands[island].first->initializeDestroyableMapPart(
            mapPartId, initiallyWalkable, minimum, maximum, transformation,
            m_islands[island].second, false);
}

void QuadTreeManager::destroyMapPart(int mapPartId)
{
    for (unsigned int island = 0; island < m_islands.size(); ++island)
        m_islands[island].first->destroyMapPart(mapPartId, m_islands[island].second);
}

void QuadTreeManager::restoreMapPart(int mapPartId)
{
    for (unsigned int island = 0; island < m_islands.size(); ++island)
        m_islands[island].first->restoreMapPart(mapPartId, m_islands[island].second);
}

void QuadTreeManager::resetOriginalMaterialOfDestroyableMapPart(int mapPartId)
{
    for (unsigned int island = 0; island < m_islands.size(); ++island)
        m_islands[island].first->resetOriginalMaterialOfDestroyableMapPart(
            mapPartId, m_islands[island].second);
}

void QuadTreeManager::resetDestroyableMapParts()
{
    for (unsigned int island = 0; island < m_islands.size(); ++island)
        m_islands[island].first->resetDestroyableMapParts(m_islands[island].second);
}

void QuadTreeManager::validateNeighbourRelations()
{
    BF2_AI_FRONTIER("dice::hfe::ai::QuadTreeManager::validateNeighbourRelations");
}

} // namespace ai
} // namespace hfe
} // namespace dice
