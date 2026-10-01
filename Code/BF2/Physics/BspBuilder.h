// Native-pointer BSP builder. [INFERRED] Source path and member names follow the
// adjacent attested BSP implementation; do not pack its pointer or STL members.
#ifndef PHYSICS_BSPBUILDER_H
#define PHYSICS_BSPBUILDER_H

#include <vector>
#include "../Math/Vec3.h"

namespace dice { namespace hfe { namespace world {
struct CollisionFace;

struct AABB
{
    Vec3 mins, maxs;
    AABB() : mins{0.0f, 0.0f, 0.0f}, maxs{0.0f, 0.0f, 0.0f} {}
    void fitPointSet(const Vec3* points, unsigned int count);
};

struct BspBuilderFace
{
    const CollisionFace* face;                  // x86 +00, borrowed
    AABB bounds;                                // +04
    unsigned int index;                         // +1C, original face index
    AABB clippedBounds;                         // +20, used by the unneeded build2 arm
    bool clippedAway;                           // +38, build2 stores a byte; ctor leaves it unset
    Vec3 normal;                                // +3C
    BspBuilderFace() : normal{0.0f, 0.0f, 0.0f} {}
};

struct BspBuilderNode
{
    AABB bounds;
    std::vector<BspBuilderFace*> faces;
    BspBuilderNode* children[2];
    int axis;
    float split;
    BspBuilderNode() : children{0, 0}, axis(-1), split(0.0f) {}
};

struct BspSimpleNode
{
    float split;
    int axis;
    unsigned int children[2];
    unsigned int faceCount[2];
    const unsigned int* faceIndices[2];
};

class BspBuilder
{
public:
    BspBuilder(const std::vector<Vec3>& vertices, const std::vector<CollisionFace>& faces, int mode);
    ~BspBuilder();
    const Vec3& getMins() const;
    const Vec3& getMaxs() const;
    const std::vector<BspSimpleNode>& getNodes() const;
    unsigned int getNumFaceReferences() const;

private:
    void build(BspBuilderNode& node, unsigned int depth);
    unsigned int buildSimpleNode(BspBuilderNode& node);
    void partition(const std::vector<BspBuilderFace*>& faces, const AABB& parent,
                   const AABB& left, std::vector<BspBuilderFace*>& leftFaces,
                   const AABB& right, std::vector<BspBuilderFace*>& rightFaces,
                   int axis, float split, std::vector<BspBuilderFace*>& coplanar);
    float computeCost(const AABB& bounds, unsigned int count);
    void createLeaf(const BspBuilderNode& node, unsigned int depth);
    void deleteNode(BspBuilderNode* node);

    const std::vector<Vec3>& m_vertices;
    BspBuilderFace* m_faces;
    BspBuilderNode* m_root;
    std::vector<BspSimpleNode> m_nodes;
    std::vector<unsigned int> m_faceReferences;
    unsigned int m_maxDepth;
    float m_minImprovement;
    unsigned int m_maxLeafSize;
    unsigned int m_smallNodeSize;
    unsigned int m_leafThreshold;
    float m_epsilon;
    float m_coplanarEpsilon;
    // Two four-word statistics blocks: minimum, maximum, sum, sample count.
    unsigned int m_minFaces, m_maxFaces, m_totalFaces, m_nonemptyLeaves;
    unsigned int m_minDepth, m_deepestDepth, m_totalDepth, m_depthSamples;
    unsigned int m_leafCount, m_emptyLeaves, m_numFaceReferences;
    // Original member extent: ELF 0x6C, PE 0x74 due to vector-layout differences.
    // [FRONTIER] build2/classification/clipping helpers remain outside the constructor path.
};
} } }
#endif
