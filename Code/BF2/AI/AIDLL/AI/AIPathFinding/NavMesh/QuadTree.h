// Navigation quad tree with file loading, polygon lookup and destroyable-map-part state.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

#ifndef AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QUADTREE_H
#define AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QUADTREE_H

#include <vector>

#include "../../Main/AIDllTypes.h"      // Vec2, Vec3, DrawPrimitive
#include "QuadTreeNode.h"

namespace dice {
namespace hfe {

namespace io { class File; }

namespace ai {

class ClusterGraph;
class MaterialManager;

// Polygon - one navigation-mesh polygon, 20 bytes on disk and in memory. The array is constructed
// before the read overwrites it: the ELF zeroes +8..+19 per element, the PE runs a per-element
// constructor (0x10168E40, 14 B) - the normal below. In retail that zeroing is Vec3's own
// constructor (PE 0x10011130 zeroes x, y, z); this tree's Math::Vec3 is a plain aggregate, so the
// constructor here writes it out. Field names [INFERRED] from their readers.
class Polygon
{
public:
    Polygon() { m_normal.x = 0.0f; m_normal.y = 0.0f; m_normal.z = 0.0f; }

    /* +0x00 */ unsigned char  m_numberOfVertices;  // bounds every corner / edge index (0x3068B0, 0x307560)
    /* +0x01 */ unsigned char  m_material;          // get/setMaterialOfPolygon (0x3081A0 / 0x3081C0)
    /* +0x02 */ unsigned short m_cluster;           // ClusterGraph::getClusterId (0x314E10)
    /* +0x04 */ unsigned int   m_firstIndex;        // into m_vertexIndices and m_neighbours
    /* +0x08 */ Vec3           m_normal;            // getNormalOfPolygon returns its address (0x307760)
};

typedef char PolygonIsTheFileRecord[sizeof(Polygon) == 20 ? 1 : -1];

// DestroyableMapPart - one per process-wide map-part id and per island. The vector holds every
// polygon with at least one vertex inside the part's object-local boundary. The three trailing
// bytes are paired-layout evidence: ELF +0x0C/+0x0D/+0x0E and PE +0x10/+0x11/+0x12, after the
// ports' different vector layouts. Names are [INFERRED]; behavior is recovered below.
class DestroyableMapPart
{
public:
    std::vector<unsigned int> m_polygons;           // [INFERRED] name
    unsigned char m_originalMaterial;               // [INFERRED] name
    bool m_initiallyWalkable;                        // [INFERRED] name
    bool m_destroyed;                                // [INFERRED] name
};

// [INFERRED] source name. readGTSDataFromFileDoubles allocates exactly 24 bytes per element and
// getPolygonsWithinBoundaryDoubles reads three consecutive doubles from each one.
struct DoubleVec3
{
    double x;
    double y;
    double z;
};

typedef char DoubleVec3IsTheFileRecord[sizeof(DoubleVec3) == 24 ? 1 : -1];

class QuadTree
{
public:
    explicit QuadTree(unsigned int island);
    ~QuadTree();

    void clean();
    void readFromFile(io::File& file);
    void undraw() const;

    bool findPolygon(const Vec3&, unsigned int&, MaterialManager*, float) const;
    bool isPointInPolygon(const Vec3&, unsigned int, float, MaterialManager*) const;
    void getVerticiesOfPolygon(std::vector<const Vec3*>&, unsigned int) const;
    unsigned char getMaterialOfPolygon(unsigned int polygon) const;

    void initializeDestroyableMapPart(int, bool, const Vec3&, const Vec3&, const Mat4&,
                                      ClusterGraph*, bool);
    void destroyMapPart(int, ClusterGraph*);
    void restoreMapPart(int, ClusterGraph*);
    void resetDestroyableMapParts(ClusterGraph*);
    void resetOriginalMaterialOfDestroyableMapPart(int, ClusterGraph*);

private:
    friend class QuadTreeNode;

    bool withinPolygon(Polygon*, const Vec3&, float&, float) const;
    bool pointWithinPolygonEdges(Polygon*, const Vec3&) const;
    void getPolygonsWithinBoundary(const Vec3&, const Vec3&, const Mat4&,
                                   std::vector<unsigned int>&) const;
    void getPolygonsWithinBoundaryDoubles(const Vec3&, const Vec3&, const Mat4&,
                                          std::vector<unsigned int>&) const;

    // Left-hand offsets are the ELF's (sizeof 196; `operator new(0xC4)` in loadQuadTrees).
    // [UNRESOLVED] roles for the four vectors, the two byte arrays and the four doubles: only the
    // GTS generation path (not written) fills them; the constructor empties them, clean() frees the
    // arrays and the destructor the vectors.
    /* +0x00 */ std::vector<Vec3>         m_vector00;          // 12-byte elements, trivial destructor
    /* +0x0C */ std::vector<unsigned int> m_vector0C;
    /* +0x18 */ std::vector<unsigned int> m_vector18;
    /* +0x24 */ std::vector<unsigned int> m_vector24;
    /* +0x30 */ unsigned char             m_unwritten30[12];   // no constructor, clean or destructor touches it
    /* +0x3C */ Vec2           m_min;                // [INFERRED] the island's bounds - Dalian Infantry
    /* +0x44 */ Vec2           m_max;                //   (-351.75, -466.75) .. (382.65, 340.53)
    /* +0x4C */ float          m_size;               // [INFERRED] the root square's edge - 807.28 there, the larger extent
    /* +0x50 */ unsigned int   m_island;             // the constructor's argument
    /* +0x54 */ unsigned int   m_numberOfNodes;
    /* +0x58 */ unsigned int   m_numberOfPolygons;   // getPolygon's bound (0x3069A0)
    /* +0x5C */ unsigned int   m_numberOfNodePolygons;
    /* +0x60 */ unsigned int   m_numberOfVertices;
    /* +0x64 */ unsigned int   m_numberOfVertexIndices;
    /* +0x68 */ unsigned int   m_numberOfNeighbours;
    /* +0x6C */ QuadTreeNode*  m_nodes;
    /* +0x70 */ Polygon*       m_polygons;
    /* +0x74 */ unsigned int*  m_nodePolygons;       // [INFERRED] the leaves' polygon lists (71,907 entries on Dalian Infantry)
    /* +0x78 */ Vec3*          m_vertices;
    /* +0x7C */ unsigned int*  m_vertexIndices;
    /* +0x80 */ unsigned int*  m_neighbours;
    /* +0x84 */ DoubleVec3*    m_array84;            // GTS-only double-precision vertex array
    /* +0x88 */ unsigned char* m_array88;
    /* +0x8C */ double         m_double8C;
    /* +0x94 */ double         m_double94;
    /* +0x9C */ double         m_double9C;
    /* +0xA4 */ double         m_doubleA4;
    /* +0xAC */ double         m_doubleAC;            // GTS-computed maximum X/Z extent; ctor skips it
    /* +0xB4 */ std::vector<DestroyableMapPart*> m_destroyableMapParts;
    // [CHOSEN] mutable: the ELF's undraw is const (`_ZNK`) and still hands this to
    // IDPManager::deleteMesh by reference, which nulls it.
    /* +0xC0 */ mutable DrawPrimitive* m_debugMesh;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QUADTREE_H
