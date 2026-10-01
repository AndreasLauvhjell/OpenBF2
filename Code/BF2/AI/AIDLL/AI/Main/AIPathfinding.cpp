// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIPathfinding operations.

#include "AIPathfinding.h"

#include <algorithm>     // std::remove - deleteMap uses the erase-remove idiom
#include <cstddef>

#include "../AIPathFinding/Cluster/ClusterNodeManager.h"   // init builds one per cluster search
#include "../AIPathFinding/NavMesh/QuadTree.h"             // MapContainer polygon vertex query
#include "../AIPathFinding/NavMesh/QTSearchNodeManager.h"  // init builds one per local search
#include "../AIStrategicEntity/AIStrategicObjectManager.h" // loadQuadTrees' tail calls initAreas
#include "../AIUtils/checkPointer.h"
#include "AIDllFrontier.h"
#include "AIMineManager.h"               // loadQuadTrees / loadGTSData call IAIMineManager::init
#include "AISettingsInterfaces.h"        // IAISettings::getVehicle / getVehicleMaterialManager
#include "CommonSingletonServer.h"
#include "../../../../Core/ConsoleObjectMacro.h"  // the aiPathfinding.* bindings at the foot

namespace dice {
namespace hfe {
namespace ai {

typedef PathfindingEngine<QTSearchNode, QTSearchNodeManager, QTSearchNode,
                          unsigned short, QTPath> LocalEngine;
typedef PathfindingEngine<ClusterNode, ClusterNodeManager, ClusterNode,
                          unsigned short, ClusterNodePath> ClusterEngine;

// The pool's records. Both element types are attested in full by the `_M_insert_aux` symbols the
// two `push_back`s in `init` call.
typedef AStarResource<QTSearchNode, QTSearchNodeManager, QTSearchNode,
                      unsigned short, QTPath> LocalResource;
typedef AStarResource<ClusterNode, ClusterNodeManager, ClusterNode,
                      unsigned short, ClusterNodePath> ClusterResource;

// Allocates the pathfinding engines, search resources and debug paths.
AIPathfinding::AIPathfinding()
    : m_activatePathfindingDebugTool(false)
    , m_pathfindingDebugToolVehicleId(0)
    , m_searchPointToSet(0)
    , m_debugPath(0)
    , m_debugClusterPath(0)
    , m_d36(-1000.0)
    , m_clusterSearch(false)
    , m_engine(0)
    , m_clusterEngine(0)
    , m_activeMap(0)
    , m_traceMapIndex(-666)
    , m_nextDestroyableMapPartId(0)
    , m_maxWaterDepth(0.5f)
    , m_locking(false)
{
    m_debugPath = new QTPath();
    checkPointer(m_debugPath);

    m_debugClusterPath = new ClusterNodePath();
    checkPointer(m_debugClusterPath);

    m_engine = new LocalEngine();
    checkPointer(m_engine);

    m_clusterEngine = new ClusterEngine();
    checkPointer(m_clusterEngine);

    // `new Vec3` with the parentheses. `Vec3` is a POD in this tree and the binary zeroes all
    // three floats after the allocation, which is value-initialisation, not default.
    m_searchPoint[0] = new Vec3();
    checkPointer(m_searchPoint[0]);

    m_searchPoint[1] = new Vec3();
    checkPointer(m_searchPoint[1]);
}

// Resets debug paths before deleting engines, paths, maps, and search points.
// Preserves the explicit null check before each map deletion.
AIPathfinding::~AIPathfinding()
{
    reset();

    delete m_engine;
    delete m_clusterEngine;
    delete m_debugPath;
    delete m_debugClusterPath;
    delete m_searchPoint[0];
    delete m_searchPoint[1];

    for (std::vector<MapContainer*>::iterator i = m_maps.begin(); i != m_maps.end(); ++i)
    {
        if (*i)
            delete *i;
    }
}

// Resets pathfinding debug state. Loaded maps and in-flight searches remain active.
int AIPathfinding::reset()
{
    m_activatePathfindingDebugTool = false;
    m_d36 = -1000.0;
    m_searchPointToSet = 0;

    m_debugPath->m_nodes.resize(0);
    m_debugPath->m_i0 = 0;
    m_debugPath->m_i20 = 0;

    m_debugClusterPath->m_nodes.resize(0);
    m_debugClusterPath->m_i0 = 0;

    m_locking = false;
    return 1;
}

// Requests a cooperative pause. Poll isLockComplete() until both search pools are idle.
// This is not a thread synchronization primitive.
void AIPathfinding::lock()
{
    m_locking = true;
}

void AIPathfinding::unlock()
{
    m_locking = false;
}

bool AIPathfinding::isLocking() const
{
    return m_locking;
}

bool AIPathfinding::isLockComplete() const
{
    if (m_engine->m_inFlight.size() != 0)
        return false;
    if (m_clusterEngine->m_inFlight.size() != 0)
        return false;
    return true;
}

// Map-name lookup is case-insensitive. Negative stored IDs are treated as missing.
int AIPathfinding::getNumberOfMaps() const
{
    return static_cast<int>(m_maps.size());
}

bool AIPathfinding::isMapUsed(int id) const
{
    if (id < 0)
        return false;
    return static_cast<unsigned int>(m_maps.size()) > static_cast<unsigned int>(id);
}

// Returns the stored map name directly for a nonnegative id; the caller must ensure
// it is below the map count. Negative ids use the fallback name.
const std::string& AIPathfinding::getMapNameFromId(int id) const
{
    if (id >= 0)
        return m_maps[id]->m_name;

    m_returnedName.assign("No Map");
    return m_returnedName;
}

int AIPathfinding::getMapIdFromName(const std::string& name) const
{
    const std::map<std::string, int, NoCaseStringCompare>::const_iterator i = m_mapIds.find(name);
    if (i == m_mapIds.end())
        return -1;
    return i->second;
}

void AIPathfinding::setActiveMap(const std::string& name)
{
    const std::map<std::string, int, NoCaseStringCompare>::const_iterator i = m_mapIds.find(name);
    if (i == m_mapIds.end() || i->second < 0)
        m_activeMap = 0;
    else
        m_activeMap = m_maps[i->second];
}

const std::string& AIPathfinding::getActiveMap() const
{
    if (m_activeMap)
        return m_activeMap->getName();

    m_returnedName.assign("* NO ACTIVE MAPPROPERTIES *");
    return m_returnedName;
}

// 666 is the "NO TRACE MAP" sentinel, not -1, and it is written here on every miss.
void AIPathfinding::setTraceMap(const std::string& name)
{
    const std::map<std::string, int, NoCaseStringCompare>::const_iterator i = m_mapIds.find(name);
    if (i == m_mapIds.end() || i->second < 0)
        m_traceMapIndex = -666;
    else
        m_traceMapIndex = i->second;
}

// the literal has no space before its closing star - "* NO ACTIVE TRACE MAP*" - unlike its
// neighbour above. Copied exactly; it is a shipped string a console user can see.
const std::string& AIPathfinding::getTraceMap() const
{
    if (m_traceMapIndex == -666)
    {
        m_returnedName.assign("* NO ACTIVE TRACE MAP*");
        return m_returnedName;
    }
    return m_maps[m_traceMapIndex]->getName();
}

int AIPathfinding::getTraceMapIndex() const
{
    return m_traceMapIndex;
}

IAIPathfindingSetup::IMapContainer* AIPathfinding::getActiveMapContainer() const
{
    return m_activeMap;
}

// no bounds check and no guard - it walks whatever is in the list and calls the non-virtual
// `MapContainer::init` on each.
void AIPathfinding::initMaps()
{
    for (std::vector<MapContainer*>::iterator i = m_maps.begin(); i != m_maps.end(); ++i)
        (*i)->init();
}

// Debug vehicle selection uses the vehicle IDs and names in AI settings.
bool AIPathfinding::getActivatePathfindingDebugTool() const
{
    return m_activatePathfindingDebugTool;
}

void AIPathfinding::setPathfindingDebugToolVehicle(const std::string& name)
{
    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    m_pathfindingDebugToolVehicleId = settings->getVehicle(name);
}

void* AIPathfinding::getPathfindingDebugToolVehicle() const
{
    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    return const_cast<std::string*>(&settings->getVehicle(m_pathfindingDebugToolVehicleId));
}

void AIPathfinding::setPathfindingDebugToolVehicleId(int id)
{
    m_pathfindingDebugToolVehicleId = id;
}

void AIPathfinding::setClusterSearch(bool on)
{
    m_clusterSearch = on;
}

bool AIPathfinding::getClusterSearch() const
{
    return m_clusterSearch;
}

int AIPathfinding::getSearchPointToSet() const
{
    return m_searchPointToSet;
}

QTPath* AIPathfinding::getDebugPath() const
{
    return m_debugPath;
}

ClusterNodePath* AIPathfinding::getDebugClusterPath() const
{
    return m_debugClusterPath;
}

// unchecked, and the array has exactly two elements. `searchPointSelected` subscripts it with
// `m_searchPointToSet`, which that function is careful to keep at 0 or 1; nothing here enforces it.
Vec3* AIPathfinding::getSearchPoint(int index) const
{
    return m_searchPoint[index];
}

void AIPathfinding::setMaxWaterDepth(float depth)
{
    m_maxWaterDepth = depth;
}

float AIPathfinding::getMaxWaterDepth() const
{
    return m_maxWaterDepth;
}

// Queries navigation using the selected map and vehicle-specific material rules.
bool AIPathfinding::getPolygon(const Vec3& position, int map, unsigned int& polygon,
                               unsigned int& quadTree, int vehicleId) const
{
    if (map < 0 || getNumberOfMaps() <= map)
        return false;

    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    MaterialManager* const materials = settings->getVehicleMaterialManager(vehicleId);

    return m_maps[map]->getPolygon(position, polygon, quadTree, materials);
}

void AIPathfinding::getPolygonsInRadius(std::list<unsigned int>& polygons, int map,
                                        unsigned int quadTree, unsigned int polygon,
                                        const Vec3& centre, float radius) const
{
    if (map < 0 || getNumberOfMaps() <= map)
        return;

    m_maps[map]->getPolygonsInRadius(polygons, quadTree, polygon, centre, radius);
}

void AIPathfinding::getPolygonsInTriangle(std::list<unsigned int>& polygons, int map,
                                          unsigned int quadTree, unsigned int polygon,
                                          const Vec3& corner0, const Vec3& corner1,
                                          const Vec3& corner2) const
{
    if (map < 0 || getNumberOfMaps() <= map)
        return;

    m_maps[map]->getPolygonsInTriangle(polygons, quadTree, polygon, corner0, corner1, corner2);
}

// no bounds check - see the block above.
void AIPathfinding::setMaterial(int map, unsigned int quadTree, unsigned int polygon,
                                unsigned char material) const
{
    m_maps[map]->setMaterial(quadTree, polygon, material);
}

// no bounds check.
bool AIPathfinding::getMaterial(int map, unsigned int quadTree, unsigned int polygon,
                                unsigned char& material) const
{
    return m_maps[map]->getMaterial(quadTree, polygon, material);
}

// no bounds check, and it draws unconditionally in the shipped server build.
void AIPathfinding::drawPolygon(int map, unsigned int quadTree, unsigned int polygon,
                                float* colour, float duration) const
{
    m_maps[map]->drawPolygon(quadTree, polygon, colour, duration);
}

// Requires a valid map index. Validation checks the vehicle vector while cost lookup
// indexes the other vector; the caller must also ensure that lookup index is valid.
bool AIPathfinding::isValidClusterType(int map, unsigned int type) const
{
    return type < m_maps[map]->m_clusterCostVehicles.size();
}

int AIPathfinding::getClusterCostType(int map, unsigned int type) const
{
    return m_maps[map]->m_clusterCostTypes[type];
}

// AIPathfinding::MapContainer - the trivial accessors.
const std::string& AIPathfinding::MapContainer::getName()
{
    return m_name;
}

int AIPathfinding::MapContainer::getMinPolygonsInLeaf()
{
    return m_minPolygonsInLeaf;
}

void AIPathfinding::MapContainer::setMinPolygonsInLeaf(int n)
{
    m_minPolygonsInLeaf = n;
}

int AIPathfinding::MapContainer::getMaxDepthInTree()
{
    return m_maxDepthInTree;
}

void AIPathfinding::MapContainer::setMaxDepthInTree(int n)
{
    m_maxDepthInTree = n;
}

// Owns a navigation map with defaults of four polygons per leaf and tree depth eight.
AIPathfinding::MapContainer::MapContainer(const std::string& name, int id)
    : m_id(id)
    , m_name(name)
    , m_properties(0)
    , m_minPolygonsInLeaf(4)
    , m_maxDepthInTree(8)
    , m_quadTreeManager(0)
{
    m_properties = new MapProperties();
    checkPointer(m_properties);

    m_quadTreeManager = new QuadTreeManager();
    checkPointer(m_quadTreeManager);
}

// Two deletes; the two vectors and the name string are the compiler's own work. The quad-tree
// manager goes first and the properties second, which is the reverse of the order the constructor
// built them - and both are `delete` with a null test the binary spells out even though `delete`
// already handles null.
AIPathfinding::MapContainer::~MapContainer()
{
    delete m_quadTreeManager;
    delete m_properties;
}

// Slope setters accept degrees; getters return radians.
float AIPathfinding::MapContainer::getMaxSlope()
{
    return m_properties->getMaxSlope();
}

// Converts the supplied slope from degrees to radians. The corresponding getter
// returns radians, so console input and output use different units.
void AIPathfinding::MapContainer::setMaxSlope(float degrees)
{
    m_properties->setMaxSlope(0.017453292f * degrees);
}

void AIPathfinding::MapContainer::setHeadClearance(float clearance)
{
    m_properties->setHeadClearance(clearance);
}

float AIPathfinding::MapContainer::getHeadClearance()
{
    return m_properties->getHeadClearance();
}

void AIPathfinding::MapContainer::setRadius(float radius)
{
    m_properties->setRadius(radius);
}

float AIPathfinding::MapContainer::getRadius()
{
    return m_properties->getRadius();
}

void AIPathfinding::MapContainer::setAllowedHeightDiff(float diff)
{
    m_properties->setAllowedHeightDiff(diff);
}

// A direct member read - see the block above.
float AIPathfinding::MapContainer::getAllowedHeightDiff()
{
    return m_properties->m_allowedHeightDiff;
}

// Forwards map operations to QuadTreeManager. Build/load operations use m_name as the
// file stem for quad-tree and GTS mesh data.
void AIPathfinding::MapContainer::draw()
{
    m_quadTreeManager->draw();
}

void AIPathfinding::MapContainer::undraw()
{
    m_quadTreeManager->undraw();
}

void AIPathfinding::MapContainer::drawCluster()
{
    m_quadTreeManager->drawCluster();
}

void AIPathfinding::MapContainer::undrawCluster()
{
    m_quadTreeManager->undrawCluster();
}

void AIPathfinding::MapContainer::drawClusterNeighbourIndices(const Vec3& position)
{
    m_quadTreeManager->drawClusterNeighbourIndices(position);
}

void AIPathfinding::MapContainer::loadGTSDebugMeshes()
{
    m_quadTreeManager->loadGTSDebugMeshes(m_name);
}

void AIPathfinding::MapContainer::buildDebugMeshes()
{
    m_quadTreeManager->buildDebugMeshes(m_name);
}

void AIPathfinding::MapContainer::unloadDebugMeshes()
{
    m_quadTreeManager->unloadDebugMeshes();
}

void AIPathfinding::MapContainer::buildDebugClusterMeshes()
{
    m_quadTreeManager->buildDebugClusterMeshes(m_name);
}

void AIPathfinding::MapContainer::unloadDebugClusterMeshes()
{
    m_quadTreeManager->unloadDebugClusterMeshes();
}

void AIPathfinding::MapContainer::saveQuadTrees()
{
    m_quadTreeManager->saveQuadTrees(m_name);
}

// Loads navigation data and supplies its dimensions to the mine manager.
void AIPathfinding::MapContainer::loadQuadTrees()
{
    m_quadTreeManager->loadQuadTrees(m_name);

    const unsigned int islands = m_quadTreeManager->getNumberOfIslands();
    IAIMineManager* const mines =
        static_cast<IAIMineManager*>(CommonSingletonServer::m_iAIMineManager);
    mines->init(static_cast<IAIMineManager::MineType>(m_name.compare("Infantry") != 0 ? 1 : 0),
                islands);
}

void AIPathfinding::MapContainer::loadGTSData(int a, float b)
{
    // the tree parameters GO in depth-then-leaf order, which is the reverse of the order they are
    // declared and initialised in. From the binary; do not tidy.
    m_quadTreeManager->loadGTSData(m_name, m_maxDepthInTree, m_minPolygonsInLeaf, a, b);

    const unsigned int islands = m_quadTreeManager->getNumberOfIslands();
    IAIMineManager* const mines =
        static_cast<IAIMineManager*>(CommonSingletonServer::m_iAIMineManager);
    mines->init(static_cast<IAIMineManager::MineType>(m_name.compare("Infantry") != 0 ? 1 : 0),
                islands);
}

// Appends a vehicle name only. init populates the parallel cost-type vector later.
void AIPathfinding::MapContainer::addVehicleForClusterCost(const std::string& vehicle)
{
    m_clusterCostVehicles.push_back(vehicle);
}

void AIPathfinding::MapContainer::recalculatePortalCosts()
{
    m_quadTreeManager->recalculatePortalCosts(m_id);
}

void AIPathfinding::MapContainer::init()
{
    // Build the map-local vehicle
    // numbering used by cluster costs. The settings pointer is deliberately unguarded.
    IAISettings* const settings =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);

    std::map<int, int> vehicleIndices;
    int vehicleIndex = 0;
    for (std::vector<std::string>::iterator i = m_clusterCostVehicles.begin();
         i != m_clusterCostVehicles.end(); ++i)
    {
        // insert makes duplicate global ids first-wins; the sequential index still advances.
        vehicleIndices.insert(std::make_pair(settings->getVehicle(*i), vehicleIndex));
        ++vehicleIndex;
    }

    const std::vector<int>& settingsClusterCostTypes = settings->getClusterCostTypes();
    m_clusterCostTypes.resize(settingsClusterCostTypes.size());
    for (unsigned int i = 0; i < m_clusterCostTypes.size(); ++i)
    {
        // Retail dereferences find without comparing it to end. Preserve that missing guard.
        m_clusterCostTypes[i] = vehicleIndices.find(settingsClusterCostTypes[i])->second;
    }
}

bool AIPathfinding::MapContainer::isValidPosition(const Vec3&, MaterialManager*, bool) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::isValidPosition");
}

bool AIPathfinding::MapContainer::isValidPosition(const Vec3&, MaterialManager*, unsigned int,
                                                  bool) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::isValidPosition");
}

bool AIPathfinding::MapContainer::isValidPosition(const Vec3&, MaterialManager*, unsigned int,
                                                  unsigned int, bool) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::isValidPosition");
}

// Resolves polygon then island outputs. Always writes the full-width island result;
// on failure it is UINT_MAX and the polygon output is unchanged. The vertex overload
// appends without clearing, and known ids are validated before reuse.
bool AIPathfinding::MapContainer::getPolygon(std::vector<const Vec3*>& vertices, Vec3& position,
                                             unsigned int& polygon, unsigned int& island,
                                             MaterialManager* materials) const
{
    island = m_quadTreeManager->findPolygon(
        position, polygon, materials, m_properties->m_allowedHeightDiff);
    return getPolygon(vertices, polygon, island);
}

bool AIPathfinding::MapContainer::getPolygon(std::vector<const Vec3*>& vertices,
                                             unsigned int polygon, unsigned int island) const
{
    if (island == 0xFFFFFFFFu || polygon == 0xFFFFFFFFu)
        return false;

    m_quadTreeManager->getIsland(island)->getVerticiesOfPolygon(vertices, polygon);
    return true;
}

bool AIPathfinding::MapContainer::getPolygon(const Vec3& position, unsigned int& polygon,
                                             unsigned int& island,
                                             MaterialManager* materials) const
{
    island = m_quadTreeManager->findPolygon(
        position, polygon, materials, m_properties->m_allowedHeightDiff);
    return island != 0xFFFFFFFFu && polygon != 0xFFFFFFFFu;
}

bool AIPathfinding::MapContainer::getPolygon(const Vec3& position, unsigned int& polygon,
                                             unsigned int& island, unsigned int knownPolygon,
                                             unsigned int knownIsland,
                                             MaterialManager* materials) const
{
    island = m_quadTreeManager->findPolygon(position, polygon, knownPolygon, knownIsland,
                                            materials, m_properties->m_allowedHeightDiff);
    return island != 0xFFFFFFFFu && polygon != 0xFFFFFFFFu;
}

void AIPathfinding::MapContainer::getPolygonsInRadius(std::list<unsigned int>&, unsigned int,
                                                      unsigned int, const Vec3&, float) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::getPolygonsInRadius");
}

void AIPathfinding::MapContainer::getPolygonsInTriangle(std::list<unsigned int>&, unsigned int,
                                                        unsigned int, const Vec3&, const Vec3&,
                                                        const Vec3&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::getPolygonsInTriangle");
}

bool AIPathfinding::MapContainer::getPolygonFromIsland(const Vec3&, unsigned int&, unsigned int&,
                                                       MaterialManager*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::getPolygonFromIsland");
}

bool AIPathfinding::MapContainer::getMaterial(const Vec3&, MaterialManager*, unsigned char&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::getMaterial");
}

// UINT_MAX in either index is the only rejected input. Other
// out-of-range values retain retail's unchecked island and polygon indexing.
bool AIPathfinding::MapContainer::getMaterial(unsigned int quadTree, unsigned int polygon,
                                               unsigned char& material) const
{
    if (quadTree == 0xFFFFFFFFu || polygon == 0xFFFFFFFFu)
    {
        material = 0;
        return false;
    }

    material = m_quadTreeManager->getIsland(quadTree)->getMaterialOfPolygon(polygon);
    return true;
}

bool AIPathfinding::MapContainer::setMaterial(unsigned int, unsigned int, unsigned char)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::setMaterial");
}

void AIPathfinding::MapContainer::getPolygonCentre(Vec3&, unsigned int, unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::getPolygonCentre");
}

void AIPathfinding::MapContainer::getEdge(Vec3&, Vec3&, unsigned int, unsigned int,
                                          unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::getEdge");
}

bool AIPathfinding::MapContainer::trace(unsigned int&, unsigned int&, const Vec3&, const Vec3&,
                                        Vec3&, unsigned int&, int, float, MaterialManager*,
                                        unsigned char, unsigned char*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::trace");
}

bool AIPathfinding::MapContainer::traceKnown(unsigned int, unsigned int, const Vec3&, const Vec3&,
                                             Vec3&, unsigned int&, int, float, MaterialManager*,
                                             unsigned char, unsigned char*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::traceKnown");
}

bool AIPathfinding::MapContainer::getLineIntersectionWithPolygon(Vec3&, unsigned int, unsigned int,
                                                                 const Vec3&, const Vec3&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::getLineIntersectionWithPolygon");
}

void AIPathfinding::MapContainer::drawPolygon(unsigned int, unsigned int, float*, float) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::drawPolygon");
}

// Sample `limit` equally spaced points from
// `from` through `to`, returning the first point accepted by the ordinary polygon lookup. Retail
// advances `closest` once more after the final failed sample, so failure can leave it beyond `to`;
// preserve that observable quirk. The output ids are cleared before the zero-limit return, polygon
// first and island second, and receive the successful lookup in that same order.
bool AIPathfinding::MapContainer::findClosestValidPolygon(unsigned int& quadTree,
                                                          unsigned int& polygon, Vec3& closest,
                                                          const Vec3& from, const Vec3& to,
                                                          unsigned int limit,
                                                          MaterialManager* materials) const
{
    Vec3 step;
    step.x = to.x - from.x;
    step.y = to.y - from.y;
    step.z = to.z - from.z;

    if (limit > 1)
    {
        const float inverse = 1.0f / static_cast<float>(limit - 1);
        step.x *= inverse;
        step.y *= inverse;
        step.z *= inverse;
    }

    closest = from;
    polygon = 0xFFFFFFFFu;
    quadTree = 0xFFFFFFFFu;

    if (limit == 0)
        return false;

    unsigned int sample = 0;
    unsigned int foundPolygon;
    unsigned int foundIsland;
    while (!getPolygon(closest, foundPolygon, foundIsland, materials))
    {
        ++sample;
        closest.x += step.x;
        closest.y += step.y;
        closest.z += step.z;
        if (sample >= limit)
            return false;
    }

    polygon = foundPolygon;
    quadTree = foundIsland;
    return true;
}

void AIPathfinding::MapContainer::updateClusterPortalCosts(
    unsigned int&, triple<unsigned short, unsigned short, unsigned short>&, unsigned int,
    unsigned short)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::MapContainer::updateClusterPortalCosts");
}

// AIPathfinding, the unwritten half

// Resets destroyable map parts in every map's quad-tree manager.
void AIPathfinding::resetDestroyableMapParts()
{
    for (unsigned int i = 0; i < m_maps.size(); ++i)
        m_maps[i]->m_quadTreeManager->resetDestroyableMapParts();
}

int AIPathfinding::localSearch(int, int, unsigned int, unsigned int, const Vec3&, unsigned int,
                               const Vec3&, int, QTPath*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::localSearch");
}

int AIPathfinding::clusterSearch(int, int, unsigned int, unsigned int, const Vec3&, unsigned int,
                                 const Vec3&, int, ClusterNodePath*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::clusterSearch");
}

int AIPathfinding::internalLocalSearch(int, int, unsigned int, unsigned int, const Vec3&,
                                       unsigned int, const Vec3&, int, QTPath*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::internalLocalSearch");
}

bool AIPathfinding::isValidPosition(int, int, const Vec3&, bool) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::isValidPosition");
}

bool AIPathfinding::isValidPosition(int, int, const Vec3&, unsigned int, bool) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::isValidPosition");
}

bool AIPathfinding::isValidPosition(int, int, const Vec3&, unsigned int, unsigned int, bool) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::isValidPosition");
}

int AIPathfinding::getPolygon(std::vector<const Vec3*>&, int, const Vec3&, int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getPolygon");
}

int AIPathfinding::getPolygon(std::vector<const Vec3*>&, int, const Vec3&, unsigned int&,
                              unsigned int&, int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getPolygon");
}

int AIPathfinding::getPolygon(std::vector<const Vec3*>&, int, unsigned int, unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getPolygon");
}

// This is the saved-position variant used by the information
// courier. The two hint values are forwarded unchanged; the lower seeded search decides whether
// they can be reused or whether it must fall back to a normal spatial search. As in the adjacent
// overload, retail leaves both output references untouched when the map index is invalid and does
// not guard the settings singleton.
bool AIPathfinding::getPolygon(const Vec3& position, int map, unsigned int& polygon,
                               unsigned int& quadTree, unsigned int knownPolygon,
                               unsigned int knownQuadTree, int vehicleId) const
{
    if (map < 0 || getNumberOfMaps() <= map)
        return false;

    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    MaterialManager* const materials = settings->getVehicleMaterialManager(vehicleId);

    return m_maps[map]->getPolygon(position, polygon, quadTree, knownPolygon, knownQuadTree,
                                   materials);
}

int AIPathfinding::getPolygonFromIsland(const Vec3&, int, unsigned int&, unsigned int, int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getPolygonFromIsland");
}

bool AIPathfinding::getMaterial(const Vec3&, int, int, unsigned char&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getMaterial");
}

void* AIPathfinding::getQuadTree(int, unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getQuadTree");
}

void* AIPathfinding::getClusterGraph(int, unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getClusterGraph");
}

void AIPathfinding::getPolygonCentre(Vec3&, int, unsigned int, unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getPolygonCentre");
}

void AIPathfinding::getEdge(Vec3&, Vec3&, int, unsigned int, unsigned int, unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getEdge");
}

bool AIPathfinding::getLineIntersectionWithPolygon(Vec3&, int, unsigned int, unsigned int,
                                                   const Vec3&, const Vec3&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getLineIntersectionWithPolygon");
}

int AIPathfinding::getClusterOfPolygon(int, unsigned int, unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getClusterOfPolygon");
}

bool AIPathfinding::areNeighbouringClusters(int, unsigned int, unsigned short,
                                            unsigned short) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::areNeighbouringClusters");
}

void AIPathfinding::updateClusterPortalCosts(unsigned int&,
                                             triple<unsigned short, unsigned short,
                                                    unsigned short>&, int, unsigned int,
                                             unsigned short)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::updateClusterPortalCosts");
}

bool AIPathfinding::trace(int, const Vec3&, const Vec3&, Vec3&, unsigned int&, int, int, float,
                          unsigned char, unsigned char*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::trace");
}

bool AIPathfinding::trace(unsigned int, unsigned int, int, const Vec3&, const Vec3&, Vec3&,
                          unsigned int&, int, int, float, unsigned char, unsigned char*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::trace");
}

bool AIPathfinding::trace(BotNavigation*, bool, const Vec3&, Vec3&, unsigned int&, int, int, float,
                          unsigned char, unsigned char*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::trace");
}

// The
// vehicle's material table (IAISettings slot 44, +0xB0 in both images), then the non-virtual
// MapContainer::findClosestValidPolygon on `m_maps[map]`.
// unguarded - no sign or bounds test on the map index (getPolygon above has one; this has none)
// so the -666 "no trace map" sentinel would index before the vector. Both images. Retail always has
// a trace map: `aiPathfinding.traceMap Infantry` (AIPathFinding.ai), bound at the foot of this file.
bool AIPathfinding::findClosestValidPolygon(unsigned int& quadTree, unsigned int& polygon,
                                            Vec3& closest, int map, const Vec3& from,
                                            const Vec3& to, unsigned int limit,
                                            int vehicleId) const
{
    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    MaterialManager* const materials = settings->getVehicleMaterialManager(vehicleId);   // +0xB0
    return m_maps[map]->findClosestValidPolygon(quadTree, polygon, closest, from, to, limit,
                                                materials);
}

bool AIPathfinding::acquireResource(unsigned int&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::acquireResource");
}

void AIPathfinding::releaseResource(unsigned int&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::releaseResource");
}

bool AIPathfinding::acquireClusterResource(unsigned int&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::acquireClusterResource");
}

void AIPathfinding::releaseClusterResource(unsigned int&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::releaseClusterResource");
}

// Search-record mutations remain unimplemented: status, iteration/depth limits,
// additional goals, allowed clusters, and smoothing must use the confirmed record layout.
void AIPathfinding::setMaxIterationsForResource(unsigned int&, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::setMaxIterationsForResource");
}

void AIPathfinding::setMaxSearchDepthForResource(unsigned int&, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::setMaxSearchDepthForResource");
}

void AIPathfinding::setAdditionalGoals(unsigned int, const std::set<unsigned int>*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::setAdditionalGoals");
}

void AIPathfinding::setAllowedClusters(unsigned int, const std::set<unsigned short>*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::setAllowedClusters");
}

void AIPathfinding::setUsePathTopologySmoothing(unsigned int, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::setUsePathTopologySmoothing");
}

int AIPathfinding::getStatus(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getStatus");
}

void AIPathfinding::setStatus(unsigned int, astar::Status)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::setStatus");
}

void AIPathfinding::updateQuadTree(unsigned int, int, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::updateQuadTree");
}

float AIPathfinding::calculateCostOfPath(unsigned int, int, unsigned int, const QTPath&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::calculateCostOfPath");
}

// Splits the time budget between cluster and polygon searches by queue length.
// With both queues empty, the cluster share is NaN and no cluster work runs.
void AIPathfinding::updatePathfinding(double budget)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double deadline = timer->getUpdateTime() + budget;
    const unsigned int clusters = static_cast<unsigned int>(m_clusterEngine->m_inFlight.size());
    const unsigned int polygons = static_cast<unsigned int>(m_engine->m_inFlight.size());
    m_clusterEngine->update(
        budget * (static_cast<double>(clusters) / (static_cast<double>(polygons) + clusters)));
    m_engine->update(deadline - timer->getUpdateTime());
}

// Assigns an ID to a destroyable map part and propagates its lifecycle to all navigation meshes.
void AIPathfinding::initializeDestroyableMapPart(int& mapPartId, bool walkable, const Vec3& minCorner,
                                                 const Vec3& maxCorner, const Mat4& transformation)
{
    mapPartId = m_nextDestroyableMapPartId++;
    for (unsigned int i = 0; i < m_maps.size(); ++i)
        m_maps[i]->m_quadTreeManager->initializeDestroyableMapPart(mapPartId, walkable, minCorner,
                                                                  maxCorner, transformation);
}

void AIPathfinding::destroyMapPart(int mapPartId)
{
    for (unsigned int i = 0; i < m_maps.size(); ++i)
        m_maps[i]->m_quadTreeManager->destroyMapPart(mapPartId);
}

void AIPathfinding::restoreMapPart(int mapPartId)
{
    for (unsigned int i = 0; i < m_maps.size(); ++i)
        m_maps[i]->m_quadTreeManager->restoreMapPart(mapPartId);
}

void AIPathfinding::resetOriginalMaterialOfDestroyableMapPart(int mapPartId)
{
    for (unsigned int i = 0; i < m_maps.size(); ++i)
        m_maps[i]->m_quadTreeManager->resetOriginalMaterialOfDestroyableMapPart(mapPartId);
}

void AIPathfinding::searchPointSelected(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::searchPointSelected");
}

bool AIPathfinding::tryPath(const Vec3&, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::tryPath");
}

bool AIPathfinding::tryClusterPath(const Vec3&, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::tryClusterPath");
}

void AIPathfinding::setActivatePathfindingDebugTool(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::setActivatePathfindingDebugTool");
}

void* AIPathfinding::getEquipmentTypeFromClusterCostType(int, unsigned short) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::getEquipmentTypeFromClusterCostType");
}

// IAIPathfindingSetup, the half that is now one body shorter

// Allocates the search-resource pool: one resource per four bots, with a minimum of two.
void AIPathfinding::init()
{
    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);

    for (int i = 0; i < settings->getMaxNBots() || i < 8; i += 4)
    {
        // the navigation-mesh search
        LocalResource* const resource =
            checkPointer(new LocalResource(checkPointer(new QTSearchNodeManager())));
        resource->m_maxIterations = 30000;
        m_engine->m_searches.push_back(resource);

        resource->m_i16      = 0;
        resource->m_openSize = 21;
        resource->m_hashSize = 1031;
        resource->m_open   = checkPointer(new BinarySearchTree<QTSearchNode, QTSearchNode>(21));
        resource->m_closed =
            checkPointer(new Hash<QTSearchNode, QTSearchNodeManager, QTSearchNode>(1031));
        resource->m_cache  =
            checkPointer(new Hash<QTSearchNode, QTSearchNodeManager, QTSearchNode>(1031));
        resource->m_closed->m_nodeManager = resource->m_nodeManager;
        resource->m_cache->m_nodeManager  = resource->m_nodeManager;

        m_engine->m_available.push_back(
            static_cast<unsigned int>(m_engine->m_searches.size() - 1));

        // the cluster-graph search
        ClusterResource* const clusterResource =
            checkPointer(new ClusterResource(checkPointer(new ClusterNodeManager())));
        clusterResource->m_maxIterations = 5000;
        m_clusterEngine->m_searches.push_back(clusterResource);

        clusterResource->m_i16      = 0;
        clusterResource->m_openSize = 13;
        clusterResource->m_hashSize = 313;
        clusterResource->m_open   = checkPointer(new BinarySearchTree<ClusterNode, ClusterNode>(13));
        clusterResource->m_closed =
            checkPointer(new Hash<ClusterNode, ClusterNodeManager, ClusterNode>(313));
        clusterResource->m_cache  =
            checkPointer(new Hash<ClusterNode, ClusterNodeManager, ClusterNode>(313));
        clusterResource->m_closed->m_nodeManager = clusterResource->m_nodeManager;
        clusterResource->m_cache->m_nodeManager  = clusterResource->m_nodeManager;

        m_clusterEngine->m_available.push_back(
            static_cast<unsigned int>(m_clusterEngine->m_searches.size() - 1));
    }
}

// Creates and selects a named map. Duplicate names fail and clear the active selection.
bool AIPathfinding::createMap(const std::string& name)
{
    if (m_mapIds.find(name) != m_mapIds.end())
    {
        m_activeMap = 0;
        return false;
    }

    const int id = static_cast<int>(m_maps.size());

    m_activeMap = new MapContainer(name, id);
    checkPointer(m_activeMap);

    m_mapIds.insert(std::make_pair(name, id));
    m_maps.push_back(m_activeMap);
    return true;
}

// Deletes the active map. The supplied map argument does not select the map to remove.
bool AIPathfinding::deleteMap(const std::string& name)
{
    const std::map<std::string, int, NoCaseStringCompare>::iterator i = m_mapIds.find(name);
    if (i == m_mapIds.end())
        return false;

    m_mapIds.erase(i);

    // `m_activeMap` - see above. Not `m_maps[i->second]`.
    m_maps.erase(std::remove(m_maps.begin(), m_maps.end(), m_activeMap), m_maps.end());

    m_activeMap = 0;
    return true;
}

void AIPathfinding::saveQuadTrees()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::saveQuadTrees");
}

// Loads quad trees and notifies the strategic layer after loading.
void AIPathfinding::loadQuadTrees()
{
    for (std::vector<MapContainer*>::iterator i = m_maps.begin(); i != m_maps.end(); ++i)
        (*i)->loadQuadTrees();

    AIStrategicObjectManager::getInstance()->initAreas();
}

void AIPathfinding::loadGTSData()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::loadGTSData");
}

bool AIPathfinding::exportGTSMeshes() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::exportGTSMeshes");
}

bool AIPathfinding::exportGTSMeshesWithinBoundary(const Vec2&, const Vec2&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::exportGTSMeshesWithinBoundary");
}

// 18,942 bytes in retail - the offline level compiler. Nothing on a server route reaches it.
bool AIPathfinding::createGTSFiles(std::list<void*>&, std::list<void*>&,
                                   std::map<const IAIObject*, void*>&, std::list<void*>&,
                                   IAITerrain*, unsigned char) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::createGTSFiles");
}

// Returns virtual getName results joined by newlines, with no trailing newline.
std::string AIPathfinding::getMapList()
{
    std::string list;

    for (std::vector<MapContainer*>::iterator i = m_maps.begin(); i != m_maps.end(); ++i)
    {
        if (!list.empty())
            list.append("\n" + (*i)->getName());
        else
            list.append((*i)->getName());
    }

    return list;
}

void AIPathfinding::drawDynamicAIObjects() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPathfinding::drawDynamicAIObjects");
}

// Console bindings for pathfinding configuration and map loading.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

// a free function of the generated console TU: the setup's active map, or null
// without a setup. Every `map.*` binding reads through it, and tests it.
IAIPathfindingSetup::IMapContainer* getActiveMapContainer()
{
    if (!IAIPathfindingSetup::instance)
        return 0;
    return IAIPathfindingSetup::instance->getActiveMapContainer();         // +0x50
}

namespace {

const char* const kAiPathfindingConsoleTag = "aiPathfinding";

// No null test on the instance: the binding's isObjectActive is the guard, as in the executor.
bool consoleAiPathfindingCreateMap(const std::string& name)
{
    return IAIPathfindingSetup::instance->createMap(name);                // +0x10
}

} // anonymous namespace

BF2_CONSOLE_OBJECT("AIPathFinding", 2, 0)
BF2_CONSOLE_METHOD_1_RET_ACTIVE(createMap, consoleAiPathfindingCreateMap, bool,
                                IAIPathfindingSetup::instance != 0, std::string,
                                kAiPathfindingConsoleTag)
BF2_CONSOLE_PROPERTY_FN(activeMap, std::string, IAIPathfindingSetup::instance != 0,
                        IAIPathfindingSetup::instance->getActiveMap(),
                        IAIPathfindingSetup::instance->setActiveMap, kAiPathfindingConsoleTag)
BF2_CONSOLE_PROPERTY_FN(traceMap, std::string, IAIPathfindingSetup::instance != 0,
                        IAIPathfindingSetup::instance->getTraceMap(),
                        IAIPathfindingSetup::instance->setTraceMap, kAiPathfindingConsoleTag)
BF2_CONSOLE_PROPERTY_FN_NAMED("map.maxSlope", float, getActiveMapContainer() != 0,
                              getActiveMapContainer()->getMaxSlope(),
                              getActiveMapContainer()->setMaxSlope, kAiPathfindingConsoleTag)
BF2_CONSOLE_PROPERTY_FN_NAMED("map.headClearance", float, getActiveMapContainer() != 0,
                              getActiveMapContainer()->getHeadClearance(),
                              getActiveMapContainer()->setHeadClearance, kAiPathfindingConsoleTag)
BF2_CONSOLE_PROPERTY_FN_NAMED("map.radius", float, getActiveMapContainer() != 0,
                              getActiveMapContainer()->getRadius(),
                              getActiveMapContainer()->setRadius, kAiPathfindingConsoleTag)
BF2_CONSOLE_PROPERTY_FN_NAMED("map.allowedHeightDiff", float, getActiveMapContainer() != 0,
                              getActiveMapContainer()->getAllowedHeightDiff(),
                              getActiveMapContainer()->setAllowedHeightDiff, kAiPathfindingConsoleTag)

// map.addVehicleForClusterCost is family 1a (BF2_CONSOLE_METHOD_1_ACTIVE's shape: type 0, exactly one
// argument, "void", a real availability test) under a console spelling with a dot in it, which that
// macro cannot take. [CHOSEN] A one-shot local macro with the named head: everything expands inside
// one macro invocation, so `BF2_CC_NAME`'s __LINE__ agrees between the class and its constructor (a
// hand-written class would not). Core/ConsoleObjectMacro.h is another front's live file and is not
// touched for one binding.
#define BF2_AIPATHFINDING_CONSOLE_METHOD_1_ACTIVE_NAMED(fnNameLiteral, target, activeExpr, A0, listTag) \
namespace {                                                                                        \
class BF2_CC_NAME : public ::dice::hfe::ConsoleObjectBaseImpl                                      \
{                                                                                                  \
    BF2_CC_HEAD_NAMED(fnNameLiteral, 0, 1, 1, "void", true)                                        \
            m_argTypes[0] = #A0;                                                                   \
    BF2_CC_TAIL(1, listTag)                                                                        \
    virtual bool isObjectActive() const { return (activeExpr); }                                   \
    virtual void executeObjectMethod()                                                             \
    {                                                                                              \
        if (m_argCount != 1) return;                                                               \
        m_hasReturnValue = false;                                                                  \
        target(BF2_CC_ARG(A0, 0));                                                                 \
    }                                                                                              \
    virtual void setArgFromString(int index, const std::string& value)                             \
    {                                                                                              \
        BF2_CC_CONVERT(A0, 1)                                                                      \
    }                                                                                              \
    BF2_CC_NO_RETURN_AS_STRING                                                                     \
    BF2_CC_COMMON_HOOKS                                                                            \
};                                                                                                 \
static BF2_CC_NAME BF2_CC_INSTANCE;                                                                \
}

BF2_AIPATHFINDING_CONSOLE_METHOD_1_ACTIVE_NAMED("map.addVehicleForClusterCost",
                                                getActiveMapContainer()->addVehicleForClusterCost,
                                                getActiveMapContainer() != 0, std::string,
                                                kAiPathfindingConsoleTag)
#undef BF2_AIPATHFINDING_CONSOLE_METHOD_1_ACTIVE_NAMED

BF2_CONSOLE_PROPERTY_FN(maxWaterDepth, float, IAIPathfindingSetup::instance != 0,
                        IAIPathfindingSetup::instance->getMaxWaterDepth(),
                        IAIPathfindingSetup::instance->setMaxWaterDepth, kAiPathfindingConsoleTag)

// Handed to the console registry by AIModule::init, beside the `ai`, `aiSettings` and
// `aiStrategicArea` lists.
ConsoleObjectList& aiPathfindingConsoleObjects()
{
    return consoleBindingList(kAiPathfindingConsoleTag);
}

} // namespace ai
} // namespace hfe
} // namespace dice
