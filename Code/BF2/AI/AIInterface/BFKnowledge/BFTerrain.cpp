// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFTerrain operations.

#include "BFTerrain.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods
#include "../../../Physics/HeightmapCluster.h"   // world::IHeightmapCluster - the +0x30 slot, typed

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IAITerrain::IAITerrain()
{
    if (!CommonSingletonServer::m_iaiTerrain)
    {
// Refresh cached interface pointers after singleton registration changes.
        classManager->registerSimpleSingleton(std::string("IAITerrain"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IAITerrain::~IAITerrain()
{
    if (CommonSingletonServer::m_iaiTerrain == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IAITerrain"));
        BFSingletonServer::updateStaticMethods();
    }
}

// Initializes terrain dimensions from the engine terrain and registers the AI terrain adapter.
BFTerrain::BFTerrain()
{
    m_heightmapCluster = classManager->getSingleton(std::string("HeightmapCluster"));

    // No guard - see the block above.
    world::IHeightmapCluster* const cluster =
        static_cast<world::IHeightmapCluster*>(m_heightmapCluster);

    // one number, asked for twice. See the block above.
    m_worldSizeX = cluster->getHeightmapSize();
    m_worldSizeZ = cluster->getHeightmapSize();

    m_worldMinX = static_cast<int>(0.0f - static_cast<float>(m_worldSizeX) * 0.5f);
    m_worldMaxX = m_worldSizeX + m_worldMinX;
    m_worldMinZ = static_cast<int>(0.0f - 0.5f * static_cast<float>(m_worldSizeZ));
    m_worldMaxZ = m_worldSizeZ + m_worldMinZ;
}

BFTerrain::~BFTerrain()
{
}

// Terrain altitude queries forward to the engine terrain at the supplied position.

// slot 2 - one forward, no arithmetic.
float BFTerrain::getHeight(float x, float z) const
{
    return static_cast<world::IHeightmapCluster*>(m_heightmapCluster)
               ->getHeightInWorldCoords(x, z);
}

// slot 3. height above the surface - see the block above. The subtraction is
// an `fsubr` against the Vec3's second word, i.e. `position.y - surface`, in that order.
float BFTerrain::getAltitude(const Vec3& position) const
{
    return position.y - getTerrainAltitude(position);
}

// slot 4 - X and Z, and Y is dropped. A Vec3 goes in and a two-dimensional
// question comes out, which is the same XZ-plane assumption `vec3ToVec2XZ` encodes elsewhere.
float BFTerrain::getTerrainAltitude(const Vec3& position) const
{
    return getTerrainAltitude(position.x, position.z);
}

// slot 5. the ground or the water, whichever is higher.
float BFTerrain::getTerrainAltitude(float x, float z) const
{
    const float height = getHeight(x, z);
    const float water  = getWaterLevel(x, z);

    // `fcmovbe`: the water losing a tie - or an unordered compare - yields the height.
    if (water <= height)
        return height;

    return water;
}

unsigned char BFTerrain::getMaterial(float x, float z) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getMaterial");
}

// ELF, - a constant.
int BFTerrain::getMaterialToGiveDamage() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getMaterialToGiveDamage");
}

bool BFTerrain::isInsideCombatArea(IAIPlayer* player) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::isInsideCombatArea");
}

void BFTerrain::getNormal(float x, float z, Vec3& normal) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getNormal");
}

void BFTerrain::getHeightAndNormal(float x, float z, Vec3& normal, unsigned char& material) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getHeightAndNormal");
}

float BFTerrain::getSlope(float x, float z) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getSlope");
}

// slot 12 - the second half of `getTerrainAltitude`, and the same one-line
// forward shape as `getHeight`. `IHeightmapCluster` slot 38.
float BFTerrain::getWaterLevel(float x, float z) const
{
    return static_cast<world::IHeightmapCluster*>(m_heightmapCluster)
               ->getWaterLevelInWorldCoords(x, z);
}

float BFTerrain::getSeaWaterLevel() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getSeaWaterLevel");
}

bool BFTerrain::findTerrainPoint(const Vec3& from, const Vec3& to, float step, Vec3& hit) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::findTerrainPoint");
}

// ELF, - a bare member load. All ten of these are.
// The six world-bounds getters - eleven bytes each.
// They return exactly what the constructor computed and nothing else: no clamping, no
// recomputation, no null check on the heightmap cluster. The whole of the AI's world extent is
// six member reads. The four `getPrimaryWorld*` getters below are not these - they go back
// through the cluster to the primary heightmap every call, and floor the mins while ceiling the
// maxes. Different question, different answer.
int BFTerrain::getWorldSizeX() const
{
    return m_worldSizeX;
}

int BFTerrain::getWorldSizeZ() const
{
    return m_worldSizeZ;
}

int BFTerrain::getWorldMinX() const
{
    return m_worldMinX;
}

int BFTerrain::getWorldMinZ() const
{
    return m_worldMinZ;
}

int BFTerrain::getWorldMaxX() const
{
    return m_worldMaxX;
}

int BFTerrain::getWorldMaxZ() const
{
    return m_worldMaxZ;
}

int BFTerrain::getPrimaryWorldMinX() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getPrimaryWorldMinX");
}

int BFTerrain::getPrimaryWorldMinZ() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getPrimaryWorldMinZ");
}

int BFTerrain::getPrimaryWorldMaxX() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getPrimaryWorldMaxX");
}

int BFTerrain::getPrimaryWorldMaxZ() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getPrimaryWorldMaxZ");
}

bool BFTerrain::intersectTerrain(const Vec3& from, const Vec3& to, Vec3& hit, float& distance, Vec3& normal) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::intersectTerrain");
}

bool BFTerrain::getTriangle(const Vec3& position, Vec3& a, Vec3& b, Vec3& c, Vec3& normal, unsigned char& material) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getTriangle");
}

bool BFTerrain::getTriangleNeighbour(const Vec3& a, const Vec3& b, const Vec3& c, unsigned int edge, Vec3& na, Vec3& nb, Vec3& nc, Vec3& normal, unsigned char& material) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getTriangleNeighbour");
}

int BFTerrain::getAllTerrainPolygons(std::pair<Vec2, Vec2> bounds, std::vector<Vec3>& vertices, std::vector<std::pair<int, int> >& edges, std::list<triple<int, int, int> >& triangles) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getAllTerrainPolygons");
}

void BFTerrain::getAllTerrainPolygonsWithinBoundary(std::pair<Vec2, Vec2> bounds, const Vec2& min, const Vec2& max, std::vector<Vec3>& vertices, std::vector<std::pair<int, int> >& edges, std::list<triple<int, int, int> >& triangles) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFTerrain::getAllTerrainPolygonsWithinBoundary");
}

} // namespace ai
} // namespace hfe
} // namespace dice
