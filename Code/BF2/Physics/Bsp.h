// Collision-mesh acceleration structures. Header path [INFERRED]; shared declarations are chosen.
// BspV8 uses a k-d tree of 16-byte nodes and 24-byte face records. A leaf holds at most 255 faces.
// Per-query stamps prevent reporting a face twice when it belongs to several leaves.
// Loaded and built trees use V8; legacy query arms remain [FRONTIER] where marked.
// SearchContext carries query inputs and result storage through the traversal.

#ifndef PHYSICS_BSP_H     // [INFERRED] guard style
#define PHYSICS_BSP_H

#include <vector>

#include "../Math/LocalBoundingBox.h"
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {

namespace io { class IStream; }

namespace world {

struct CollisionFace;                            // Physics/CollisionGeometry.h - 8-byte face
class  BspV8;

// Legacy 40-byte BSP node, also defining the face record used by BspV8.
// Stores a plane point at +0x08 and normal at +0x14.
struct BspNode
{
    struct Face                                  // 24 B - see the header block
    {
        int            lastQueryStamp;           // +0x00
        Vec3           normal;                   // +0x04
        unsigned short material;                 // +0x10
        unsigned short v0, v1, v2;               // +0x12 / +0x14 / +0x16
    };

    int      faceCount;                          // +0x00
    Face**   faces;                              // +0x04 - Bsp::allocateNewFaceList's new Face*[n]
    Vec3     planePoint;                         // +0x08 (IDB "pathAndName"/"floatC"/"byte10")
    Vec3     planeNormal;                        // +0x14 (IDB "float14"/"versionToLoad"/"float1C")
    BspNode* childNode1;                         // +0x20
    BspNode* childNode2;                         // +0x24

    BspNode();                                   // all zero
    void destroy();                              // delete[] faces, recurse
    void addFace(Face* face);                    // faces[faceCount++] = face

    // The legacy walkers - all read the TU-static search context Bsp's wrappers fill (Bsp.cpp).
    bool getCollidingFaces(bool frontFaces) const;   // (an empty tree finds
                                                     //   nothing - the only case a server sees)
    bool getCollidingFacesInsideCapsule() const;
    bool getCollidingFacesInsideSphere() const;      // [FRONTIER]
    bool getAllFaces(bool) const;                    // [FRONTIER]
    void printInfo(int depth);                       // ELF 0x838E820 / PE 0x6C0FA0; non-const
    // [FRONTIER] Legacy closest-face traversal requires its shared Context.
    // [FRONTIER] Legacy tree building and persistence remain unimplemented.
};

// LOD-owned BSP wrapper holding the V8 tree, legacy root, and bookkeeping.
// Destruction delegates to destroy.
struct Bsp
{
    unsigned char m_flag0;                       // +0x00 ctor 0 [UNRESOLVED semantics] - destroy()
                                                 //   skips the legacy node teardown when it is set
    BspNode  bspNode;                            // +0x04..+0x2B - the legacy root
    int      m_numFaces;                         // +0x2C ctor 0 - findFaceIndex's bound,
                                                 //   getApproxBspSize's "6 words per face"
    int      m_numNodes;                         // +0x30 ctor 0 - allocateNewBspNode ++
    void*    m_pool34;                           // +0x34 ctor 0 [UNRESOLVED] delete[]'d by destroy
    void*    m_pool38;                           // +0x38 ctor 0 [UNRESOLVED] delete[]'d by destroy
    int      m_faceListSlots;                    // +0x3C ctor 0 - allocateNewFaceList +=
    int      _unresolved40[3];                   // +0x40..+0x4B - not written by the ctor
    int      m_totalFaceRefs;                    // +0x4C ctor 0 - allocateNewBspNode +=
    BspV8*   bspV8;                              // +0x50 ctor 0 - load()/build() new it

    Bsp();
    ~Bsp();
    void destroy();                              // frees the V8, the legacy tree and the pools

    // `bspV8 = new BspV8; bspV8->load(stream, verts, faces)`. no reuse check -
    // a second load on the same Bsp leaks the first tree, as shipped.
    bool load(io::IStream* stream, const std::vector<Vec3>* vertices,
              const std::vector<CollisionFace>* faces);
    // if no V8 yet, new one and BspV8::build; else return true. Only the tool
    // path (CollisionMeshTemplate::save) calls it. BspV8 packs the typed BspBuilder output.
    bool build(const std::vector<Vec3>* vertices, const std::vector<CollisionFace>* faces, int a4);
    bool save(io::IStream* stream) const;        // [FRONTIER] tool path

    // The queries the CollisionMeshTemplate distance family calls
    // V8: BspV8::getCollidingFaces(out, p0, p1, radius 0, backFaces = !front).
    // `frontFaces` selects which side is reported: true -> faces whose normal opposes the segment
    // (dot < 0), false -> the others. Never both. Returns V8's "found any"; the legacy arm
    // returns true unconditionally, as shipped.
    bool getCollidingFaces(std::vector<const BspNode::Face*>& out, const Vec3& p0, const Vec3& p1,
                           bool frontFaces) const;
    // V8: BspV8::getCollidingFaces(out, p0, p1, radius, backFaces = false) -
    // the swept-sphere / capsule gather (a zero-length segment becomes the sphere walk).
    bool getCollidingFacesInsideCapsule(std::vector<const BspNode::Face*>& out, const Vec3& p0,
                                        const Vec3& p1, float radius) const;
    // V8: BspV8::getClosestFace(outPoint, outT, outPlaneDist, p0, dir,
    // noFacingFilter, backFaces, firstHit). The box and vertex vector are the legacy arm's
    // inputs only. Returns the face hit, or null.
    const BspNode::Face* getClosestFace(Vec3* outPoint, float* outT, float* outPlaneDist,
                                        const Vec3& p0, const Vec3& dir,
                                        const LocalBoundingBox& box, const std::vector<Vec3>& verts,
                                        bool noFacingFilter, bool backFaces, bool firstHit) const;

    // (face - &v8faces[0]) / 24 - the index of a V8 face record. Assumes a V8.
    int getFaceIndex(const BspNode::Face* face) const;
    // linear search of the first m_numFaces V8 records; a miss logs
    // Debug(6, "Physics/Bsp.cpp", 289) << "notfound" and returns 0. Tool path (BspNode::save).
    unsigned int findFaceIndex(const BspNode::Face* face) const;
    int  getApproxBspSize() const;               // 4*(refs + 6*faces + 10*nodes) + 84
    void printInfo();                            // > BspNode::printInfo(root, 0)

    // The legacy allocators (the tool/legacy builder's) - written, they are tiny.
    BspNode* allocateNewBspNode(std::vector<BspNode::Face*>& faces);
    BspNode::Face** allocateNewFaceList(int count);

    static int m_totalCheckedFaces;              // the legacy arms `+=` their result
                                                 //   counts into it (the TU's first static; its
                                                 //   global-constructor symbol is keyed to it)
};

// BSP V8 collision tree with owned face, node, and leaf-index vectors. Node zero is
// the root. Borrows the LOD vertex vector, which load/build must bind before queries.
// The query counter stamps faces to avoid repeated processing.
class BspV8
{
public:
    struct Node                                  // 16 B - see the header block
    {
        float        split;
        unsigned int flags;
        unsigned int child[2];
    };

    // The per-walk context. Offsets are the ELF's stack layout (getCollidingFaces 0x8394F10 and
    // getClosestFace 0x8395100 both build one).
    struct SearchContext
    {
        int   collectAll;                        // +0x00  1: gather every face (copyCollidingFaces)
                                                 //        0: track the closest (findClosestFace)
        bool  done;                              // +0x04  set by findClosestFace when firstHit
        Vec3  origin;                            // +0x08
        Vec3  dir;                               // +0x14  ray walk: p1 - p0; closest: the direction
        float epsilon;                           // +0x20  1e-4 / |dir| (0.001 for the sphere walk)
        float radius;                            // +0x24
        bool  backFaces;                         // +0x28  faces to report: false -> dot(dir,n) < 0
                                                 //        (front); true -> the others. findClosest
                                                 //        also negates the normal it tests when set
        bool  noFacingFilter;                    // +0x29  skip the dot-product prefilter (the
                                                 //        sphere walk sets it: report all faces);
                                                 //        also passed to checkFaceAndEdgeCollision
        bool  firstHit;                          // +0x2A  findClosestFace stops at the first
                                                 //        accepted face (sets done)
        std::vector<const BspNode::Face*>* result;   // +0x2C  collectAll's sink
        const BspNode::Face* closestFace;        // +0x30  \  findClosestFace's outputs; the
        Vec3  closestPoint;                      // +0x34   | caller pre-sets closestT = FLT_MAX
        float closestT;                          // +0x40   | (checkFaceAndEdgeCollision's along-
        float closestPlaneDist;                  // +0x44  /  segment value: |d| * (t - 1) <= 0)
    };

    BspV8();
    ~BspV8();

    // the file format, right after the Lod's own data (Physics/
    // CollisionMeshTemplate.cpp's parser hands the stream here at the '1' flag byte):
    //   m_vertices = verts; copyFaces(faces)   (the 24-byte records are rebuilt, not stored)
    //   12 B min, 12 B max
    //   u32 nodeCount; nodeCount * 16 B nodes
    //   u32 indexCount; indexCount * 2 B face indices
    // always returns true.
    bool load(io::IStream* stream, const std::vector<Vec3>* vertices,
              const std::vector<CollisionFace>* faces);
    // ELF 83948F0 / PE 6C38A0: BspBuilder does the work; this packs its native-pointer
    // builder nodes into the 16-byte Nodes above and u16 leaf indices. Rebuild appends to
    // m_faceIndices without clearing it, as shipped. Tool path (CollisionMeshTemplate::save).
    bool build(const std::vector<Vec3>* vertices, const std::vector<CollisionFace>* faces, int a4);
    bool save(io::IStream* stream) const;        // [FRONTIER] tool path

    // one Face per CollisionFace: indices + material copied, normal =
    // normalize(cross(v2 - v1, v1 - v0)) - renormalised only if |len^2 - 1| >= FLT_EPSILON, zeroed
    // if len^2 < FLT_EPSILON - stamp 0.
    void copyFaces(const std::vector<CollisionFace>* faces);

    // the gather. Bumps the query counter, then: |p1 - p0|^2 <= 1.42e-14
    // (a point) -> sphere walk (sphereIntersectsAABB gate, getSphereCollidingFaces); else the
    // swept-ray walk (rayRadiusIntersectsAABB gate for the [tMin, tMax] window, then the recursive
    // getCollidingFaces(node, tMin, tMax)). Returns "out is non-empty" (the whole vector, not
    // just this call's additions - the caller clears it first).
    bool getCollidingFaces(std::vector<const BspNode::Face*>& out, const Vec3& p0, const Vec3& p1,
                           float radius, bool backFaces) const;
    // the RAY cast. Empty tree -> null. |dir|^2 <= 1.42e-14 -> null. Otherwise
    // the aabb window (clamped to [0,1]) and the closest-mode walk; on a hit the three optional
    // outs are written. Does not bump the query counter (stamps are unused in this mode).
    const BspNode::Face* getClosestFace(Vec3* outPoint, float* outT, float* outPlaneDist,
                                        const Vec3& p0, const Vec3& dir,
                                        bool noFacingFilter, bool backFaces, bool firstHit) const;

    const std::vector<BspNode::Face>& getFaceVector() const { return m_faces; }

private:
    // The walkers - Physics/BspV8.cpp documents each.
    bool getCollidingFaces(const Node& node, float tMin, float tMax, SearchContext* ctx) const;
    bool getSphereCollidingFaces(const Node& node, SearchContext* ctx) const;
    bool copyCollidingFaces(SearchContext* ctx, unsigned int start, unsigned int count) const;
    bool findClosestFace(const Node& node, unsigned int side, SearchContext* ctx,
                         unsigned int start, unsigned int count) const;

    mutable std::vector<BspNode::Face> m_faces;  // +0x00 - mutable: the const walkers stamp faces
    Vec3                         m_min;          // +0x0C
    Vec3                         m_max;          // +0x18
    std::vector<Node>            m_nodes;        // +0x24
    std::vector<unsigned short>  m_faceIndices;  // +0x30
    const std::vector<Vec3>*     m_vertices;     // +0x3C - not owned
    mutable int                  m_queryCounter; // +0x40 - the ELF bumps it through a const this
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_BSP_H
