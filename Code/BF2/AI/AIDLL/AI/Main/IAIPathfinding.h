// Pathfinding query interface used by bots. Preserve virtual declaration order.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_MAIN_IAIPATHFINDING_H
#define AI_AIDLL_AI_MAIN_IAIPATHFINDING_H

#include <list>
#include <set>
#include <string>
#include <vector>

#include "AIDllTypes.h"

namespace dice {
namespace hfe {
namespace ai {

// Pathfinding helper types belong to dice::hfe::ai.

// Three-value aggregate; <unsigned, unsigned, Vec3> occupies 20 bytes.
// [INFERRED] Member names extend std::pair naming.
template <class A, class B, class C>
struct triple
{
    A first;
    B second;
    C third;
};

namespace astar {
// `setStatus`'s second argument and `getStatus`'s result. [UNRESOLVED] enumerators.
enum Status : int;
} // namespace astar

// The path/graph types the search slots hand back and forth.
class BotNavigation;
class ClusterNodePath;
class QTPath;

class IAIPathfinding
{
public:
    IAIPathfinding();                    // ELF 0x3E68C0, 46 B - unconditional `instance = this`
    virtual ~IAIPathfinding();           // ELF 0x3E6920, 42 B - unconditional `instance = 0`

    // Preserve const qualifiers and virtual declaration order across the pathfinding interface.
    virtual int getTraceMapIndex() const = 0;
    virtual int reset() = 0;
    // Resets destroyable map parts; returns no value.
    virtual void resetDestroyableMapParts() = 0;
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual bool isLocking() const = 0;
    virtual bool isLockComplete() const = 0;
    virtual int localSearch(int, int, unsigned int, unsigned int, const Vec3&, unsigned int, const Vec3&, int, QTPath *) = 0;
    virtual int clusterSearch(int, int, unsigned int, unsigned int, const Vec3&, unsigned int, const Vec3&, int, ClusterNodePath *) = 0;
    virtual bool isValidPosition(int, int, const Vec3&, bool) const = 0;
    virtual bool isValidPosition(int, int, const Vec3&, unsigned int, bool) const = 0;
    virtual bool isValidPosition(int, int, const Vec3&, unsigned int, unsigned int, bool) const = 0;
    // Appends pointers to const vertices through a mutable output vector.
    virtual int getPolygon(std::vector<const Vec3*>&, int, const Vec3&, int) const = 0;
    virtual int getPolygon(std::vector<const Vec3*>&, int, const Vec3&, unsigned int&, unsigned int&, int) const = 0;
    virtual int getPolygon(std::vector<const Vec3*>&, int, unsigned int, unsigned int) const = 0;
    // Returns whether a polygon was found; invalid map indices return false.
    // [INFERRED] The second overload's bool result follows the first overload's contract.
    virtual bool getPolygon(const Vec3&, int, unsigned int&, unsigned int&, int) const = 0;
    virtual bool getPolygon(const Vec3&, int, unsigned int&, unsigned int&, unsigned int, unsigned int, int) const = 0;
    virtual void getPolygonsInRadius(std::list<unsigned int>&, int, unsigned int, unsigned int, const Vec3&, float) const = 0;
    virtual void getPolygonsInTriangle(std::list<unsigned int>&, int, unsigned int, unsigned int, const Vec3&, const Vec3&, const Vec3&) const = 0;
    virtual int getPolygonFromIsland(const Vec3&, int, unsigned int&, unsigned int, int) const = 0;
    virtual bool getMaterial(const Vec3&, int, int, unsigned char&) const = 0;
    virtual bool getMaterial(int, unsigned int, unsigned int, unsigned char&) const = 0;
    virtual void setMaterial(int, unsigned int, unsigned int, unsigned char) const = 0;   // const, and mutates
    virtual void* getQuadTree(int, unsigned int) const = 0;
    virtual void* getClusterGraph(int, unsigned int) const = 0;
    virtual void getPolygonCentre(Vec3&, int, unsigned int, unsigned int) const = 0;
    virtual void getEdge(Vec3&, Vec3&, int, unsigned int, unsigned int, unsigned int) const = 0;
    virtual bool getLineIntersectionWithPolygon(Vec3&, int, unsigned int, unsigned int, const Vec3&, const Vec3&) const = 0;
    virtual void drawPolygon(int, unsigned int, unsigned int, float *, float) const = 0;   // const, and draws
    virtual int getClusterOfPolygon(int, unsigned int, unsigned int) const = 0;
    virtual bool areNeighbouringClusters(int, unsigned int, unsigned short, unsigned short) const = 0;
    virtual void updateClusterPortalCosts(unsigned int&, triple<unsigned short, unsigned short, unsigned short>&, int, unsigned int, unsigned short) = 0;
    virtual bool trace(int, const Vec3&, const Vec3&, Vec3&, unsigned int&, int, int, float, unsigned char, unsigned char *) const = 0;
    virtual bool trace(unsigned int, unsigned int, int, const Vec3&, const Vec3&, Vec3&, unsigned int&, int, int, float, unsigned char, unsigned char *) const = 0;
    virtual bool trace(BotNavigation *, bool, const Vec3&, Vec3&, unsigned int&, int, int, float, unsigned char, unsigned char *) const = 0;
    virtual bool findClosestValidPolygon(unsigned int&, unsigned int&, Vec3&, int, const Vec3&, const Vec3&, unsigned int, int) const = 0;
    virtual bool acquireResource(unsigned int&, int) = 0;
    virtual void releaseResource(unsigned int&, int) = 0;
    virtual bool acquireClusterResource(unsigned int&, int) = 0;
    virtual void releaseClusterResource(unsigned int&, int) = 0;
    virtual void setMaxIterationsForResource(unsigned int&, unsigned int) = 0;
    virtual void setMaxSearchDepthForResource(unsigned int&, unsigned int) = 0;
    virtual void setAdditionalGoals(unsigned int, const std::set<unsigned int>*) = 0;
    virtual void setAllowedClusters(unsigned int, const std::set<unsigned short>*) = 0;
    virtual void setUsePathTopologySmoothing(unsigned int, bool) = 0;
    virtual int getStatus(unsigned int) const = 0;
    virtual void setStatus(unsigned int, astar::Status) = 0;
    virtual void updateQuadTree(unsigned int, int, unsigned int) = 0;
    virtual float calculateCostOfPath(unsigned int, int, unsigned int, const QTPath&) const = 0;
    // return type resolved by reading the body: it hands back a reference to the caller-visible
    // scratch string on `AIPathfinding`, either the map's own name or the literal "No Map".
    virtual const std::string& getMapNameFromId(int) const = 0;
    virtual int getMapIdFromName(const std::string&) const = 0;
    virtual int getNumberOfMaps() const = 0;
    virtual bool isMapUsed(int) const = 0;
    virtual void updatePathfinding(double) = 0;
    // Initializes the map part and writes its id through the reference parameter; returns no value.
    virtual void initializeDestroyableMapPart(int&, bool, const Vec3&, const Vec3&, const Mat4&) = 0;
    virtual void destroyMapPart(int) = 0;
    virtual void restoreMapPart(int) = 0;
    virtual void resetOriginalMaterialOfDestroyableMapPart(int) = 0;
    virtual void searchPointSelected(bool) = 0;
    virtual bool tryPath(const Vec3&, const Vec3&) = 0;
    virtual bool tryClusterPath(const Vec3&, const Vec3&) = 0;
    virtual void setClusterSearch(bool) = 0;
    virtual bool getClusterSearch() const = 0;
    virtual void setActivatePathfindingDebugTool(bool) = 0;
    virtual bool getActivatePathfindingDebugTool() const = 0;
    virtual void setPathfindingDebugToolVehicle(const std::string&) = 0;
    virtual void* getPathfindingDebugToolVehicle() const = 0;
    virtual void setPathfindingDebugToolVehicleId(int) = 0;
    virtual int getClusterCostType(int, unsigned int) const = 0;

    // ds:0x4CE8A0-ish, plain .bss. last wins - see the note at the top.
    static IAIPathfinding* instance;
};

// Map setup interface for loading, selecting, saving and inspecting navigation maps.
class IAIPathfindingSetup
{
public:
    // Per-map setup and debug interface for build parameters, drawing, and tree persistence.
    // Polygon and trace queries belong to the concrete MapContainer and are nonvirtual.
    class IMapContainer
    {
    public:
        IMapContainer();
        virtual ~IMapContainer();                                       // slots 0, 1

        virtual const std::string& getName() = 0;                       //  2
        virtual int getMinPolygonsInLeaf() = 0;                         //  3
        virtual void setMinPolygonsInLeaf(int) = 0;                     //  4
        virtual int getMaxDepthInTree() = 0;                            //  5
        virtual void setMaxDepthInTree(int) = 0;                        //  6
        virtual float getMaxSlope() = 0;                                //  7
        virtual void setMaxSlope(float) = 0;                            //  8
        virtual void setHeadClearance(float) = 0;                       //  9
        virtual float getHeadClearance() = 0;                           // 10
        virtual void setRadius(float) = 0;                              // 11
        virtual float getRadius() = 0;                                  // 12
        virtual void setAllowedHeightDiff(float) = 0;                   // 13
        virtual float getAllowedHeightDiff() = 0;                       // 14
        virtual void draw() = 0;                                        // 15
        virtual void undraw() = 0;                                      // 16
        virtual void drawCluster() = 0;                                 // 17
        virtual void undrawCluster() = 0;                               // 18
        virtual void drawClusterNeighbourIndices(const Vec3&) = 0;      // 19
        virtual void loadGTSDebugMeshes() = 0;                          // 20
        virtual void buildDebugMeshes() = 0;                            // 21
        virtual void unloadDebugMeshes() = 0;                           // 22
        virtual void buildDebugClusterMeshes() = 0;                     // 23
        virtual void unloadDebugClusterMeshes() = 0;                    // 24
        virtual void saveQuadTrees() = 0;                               // 25
        virtual void loadQuadTrees() = 0;                               // 26
        virtual void loadGTSData(int, float) = 0;                       // 27
        virtual void addVehicleForClusterCost(const std::string&) = 0;  // 28
        virtual void recalculatePortalCosts() = 0;                      // 29
    };

    IAIPathfindingSetup();
    virtual ~IAIPathfindingSetup();                                     // slots 0, 1

    virtual void init() = 0;                                            //  2
    // `int`, not `void` - it has to match `IAIPathfinding::reset` exactly, because a single
    // `AIPathfinding::reset` override satisfies both bases and C++ forbids a return-type
    // difference there. The body materialises `1`, so non-void is the recovered reading.
    virtual int reset() = 0;                                            //  3  also IAIPathfinding slot 3
    // Map creation and deletion report success. getMapList returns its string by value;
    // preserve the ABI's hidden return-object parameter.
    virtual bool createMap(const std::string&) = 0;                     //  4
    virtual bool deleteMap(const std::string&) = 0;                     //  5
    virtual void setActiveMap(const std::string&) = 0;                  //  6
    virtual const std::string& getActiveMap() const = 0;                //  7
    virtual void setTraceMap(const std::string&) = 0;                   //  8
    virtual const std::string& getTraceMap() const = 0;                 //  9
    virtual void saveQuadTrees() = 0;                                   // 10
    virtual void loadQuadTrees() = 0;                                   // 11
    virtual void loadGTSData() = 0;                                     // 12
    virtual bool exportGTSMeshes() const = 0;                           // 13
    virtual bool exportGTSMeshesWithinBoundary(const Vec2&, const Vec2&) const = 0;  // 14
    virtual std::string getMapList() = 0;                               // 15  not const, unlike its neighbours
    virtual void setMaxWaterDepth(float) = 0;                           // 16
    virtual float getMaxWaterDepth() const = 0;                         // 17
    virtual void drawDynamicAIObjects() const = 0;                      // 18
    virtual void initMaps() = 0;                                        // 19
    virtual IMapContainer* getActiveMapContainer() const = 0;           // 20

    // bss, last wins, same as IAIPathfinding::instance above.
    static IAIPathfindingSetup* instance;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_IAIPATHFINDING_H