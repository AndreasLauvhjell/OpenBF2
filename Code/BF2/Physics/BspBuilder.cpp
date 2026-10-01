// Path: [INFERRED] Physics/BspBuilder.cpp; see BspBuilder.h.
#include "BspBuilder.h"
#include "CollisionGeometry.h"
#include "../Math/TriangleToAABBIntersection.h"
#include <algorithm>
#include <cstddef>
#include <cmath>
#include <float.h>
#include <iostream>
#include <limits>
#include <new>
#include <set>

namespace dice { namespace hfe {
namespace world {

#if defined(_M_IX86)
static_assert(sizeof(BspBuilderFace) == 72, "retail x86 builder face size");
static_assert(offsetof(BspBuilderFace, index) == 0x1C, "retail x86 original face index");
static_assert(offsetof(BspBuilderFace, clippedAway) == 0x38, "retail x86 clipped flag");
static_assert(offsetof(BspBuilderFace, normal) == 0x3C, "retail x86 face normal");
static_assert(sizeof(BspSimpleNode) == 32, "retail x86 simple node size");
#endif
static_assert(sizeof(unsigned int) == 4, "BSP node and original face indices are u32");

// ELF 83945E0 / PE 6C4450; empty sets retain the inverted FLT_MAX box.
void AABB::fitPointSet(const Vec3* points, unsigned int count)
{
    mins.x = mins.y = mins.z = FLT_MAX;
    maxs.x = maxs.y = maxs.z = -FLT_MAX;
    for (unsigned int i = 0; i < count; ++i)
        for (int axis = 0; axis < 3; ++axis)
        {
            const float value = (&points[i].x)[axis];
            if (value < (&mins.x)[axis]) (&mins.x)[axis] = value;
            if (value > (&maxs.x)[axis]) (&maxs.x)[axis] = value;
        }
}

// ELF 8390380 / PE 6C6410. The logarithm really is log(n/log(2)), not log(n)/log(2).
BspBuilder::BspBuilder(const std::vector<Vec3>& vertices,
                       const std::vector<CollisionFace>& faces, int mode)
    : m_vertices(vertices), m_faces(0), m_root(0),
      m_minFaces(~0u), m_maxFaces(0), m_totalFaces(0), m_nonemptyLeaves(0),
      m_minDepth(~0u), m_deepestDepth(0), m_totalDepth(0), m_depthSamples(0),
      m_leafCount(0), m_emptyLeaves(0), m_numFaceReferences(0)
{
    if (mode)
    {
        m_minImprovement = 0.05f;
        m_maxDepth = static_cast<unsigned int>(static_cast<int>(
            logf(static_cast<float>(faces.size()) / logf(2.0f)) * 1.7f + 3.0f) + 1);
    }
    else
    {
        m_minImprovement = 0.1f;
        m_maxDepth = static_cast<unsigned int>(static_cast<int>(
            logf(static_cast<float>(faces.size()) / logf(2.0f)) * 1.2f + 2.0f) + 1);
    }
    m_leafThreshold = 6;
    m_smallNodeSize = 16;
    m_maxLeafSize = 255;
    m_epsilon = m_coplanarEpsilon = 0.00001f;
    m_faces = new BspBuilderFace[faces.size()];
    for (unsigned int i = 0; i < faces.size(); ++i)
    {
        BspBuilderFace& face = m_faces[i];
        face.face = &faces[i];
        // Both images zero-extend these three shorts (CollisionFace's API spelling is signed).
        const Vec3 triangle[3] = {vertices[static_cast<unsigned short>(faces[i].v0)],
            vertices[static_cast<unsigned short>(faces[i].v1)],
            vertices[static_cast<unsigned short>(faces[i].v2)]};
        face.bounds.fitPointSet(triangle, 3);
        face.index = i;
        const Vec3 a = {triangle[2].x - triangle[1].x, triangle[2].y - triangle[1].y,
                        triangle[2].z - triangle[1].z};
        const Vec3 b = {triangle[1].x - triangle[0].x, triangle[1].y - triangle[0].y,
                        triangle[1].z - triangle[0].z};
        face.normal.x = a.y * b.z - a.z * b.y;
        face.normal.y = a.z * b.x - a.x * b.z;
        face.normal.z = a.x * b.y - a.y * b.x;
        const float lengthSquared = face.normal.x * face.normal.x + face.normal.y * face.normal.y
                                  + face.normal.z * face.normal.z;
        // PE 45BFA0 is Vec3 normalization, despite the IDB's stale 'sqrtf' name.
        if (fabsf(lengthSquared - 1.0f) >= FLT_EPSILON)
        {
            if (fabsf(lengthSquared) < FLT_EPSILON)
                face.normal.x = face.normal.y = face.normal.z = 0.0f;
            else
            {
                const float inverse = 1.0f / sqrtf(lengthSquared);
                face.normal.x *= inverse;
                face.normal.y *= inverse;
                face.normal.z *= inverse;
            }
        }
    }
    m_root = new BspBuilderNode;
    m_root->bounds.fitPointSet(vertices.data(), static_cast<unsigned int>(vertices.size()));
    m_root->faces.resize(faces.size(), 0);
    for (unsigned int i = 0; i < faces.size(); ++i)
        m_root->faces[i] = &m_faces[i];
    build(*m_root, 1);
    if (!m_root->children[0] && !m_root->children[1])
    {
        BspBuilderNode* root = new BspBuilderNode;
        root->bounds = m_root->bounds;
        root->axis = 0;
        root->split = root->bounds.maxs.x;
        root->children[0] = m_root;
        m_root = root;
        BspBuilderNode* right = new BspBuilderNode;
        right->bounds = m_root->bounds;
        right->bounds.mins.x = right->bounds.maxs.x;
        m_root->children[1] = right;
    }
    m_nodes.reserve(m_leafCount);
    m_faceReferences.reserve(m_numFaceReferences);
    buildSimpleNode(*m_root);
}

// ELF 8390C30 / PE 6C5670: scalar deallocation of the trivial-destructor face array
// is a shipped mismatch, retained explicitly (no array cookie for this type).
BspBuilder::~BspBuilder()
{
    ::operator delete(m_faces);
    deleteNode(m_root);
}

const Vec3& BspBuilder::getMins() const { return m_root->bounds.mins; }
const Vec3& BspBuilder::getMaxs() const { return m_root->bounds.maxs; }
const std::vector<BspSimpleNode>& BspBuilder::getNodes() const { return m_nodes; }
unsigned int BspBuilder::getNumFaceReferences() const { return m_numFaceReferences; }

// ELF 8390CF0 / PE 6C56D0. Both reserves above are required: nodes retain pointers
// into m_faceReferences, and recursion retains a reference into m_nodes.
unsigned int BspBuilder::buildSimpleNode(BspBuilderNode& node)
{
    const BspSimpleNode blank = {};
    m_nodes.push_back(blank);
    const unsigned int index = static_cast<unsigned int>(m_nodes.size()) - 1;
    BspSimpleNode& result = m_nodes.back();
    result.split = node.split;
    result.axis = node.axis;
    for (unsigned int side = 0; side < 2; ++side)
    {
        BspBuilderNode& child = *node.children[side];
        if (child.children[0] || child.children[1])
        {
            result.faceCount[side] = 0;
            result.faceIndices[side] = 0;
            result.children[side] = buildSimpleNode(child);
        }
        else
        {
            result.faceCount[side] = static_cast<unsigned int>(child.faces.size());
            if (result.faceCount[side])
            {
                result.faceIndices[side] = m_faceReferences.data() + m_faceReferences.size();
                for (unsigned int i = 0; i < result.faceCount[side]; ++i)
                    m_faceReferences.push_back(child.faces[i]->index);
            }
            else result.faceIndices[side] = 0;
            result.children[side] = 0;
        }
    }
    return index;
}

// The comparator is inlined into ELF 8394010 / PE 6C7E70. Equal minima have no
// secondary key; keep std::sort, not a stable sort or an invented index tie-break.
struct AxisSort
{
    int axis;
    bool operator()(const BspBuilderFace* a, const BspBuilderFace* b) const
    { return (&a->bounds.mins.x)[axis] < (&b->bounds.mins.x)[axis]; }
};

// ELF 8390F20 / PE 6C5860: ascending axes/planes, strict cost improvement, left
// coplanar placement considered first, then depth-first left/right recursion.
void BspBuilder::build(BspBuilderNode& node, unsigned int depth)
{
    const unsigned int count = static_cast<unsigned int>(node.faces.size());
    if (count <= m_leafThreshold) { createLeaf(node, depth); return; }
    if (depth >= m_maxDepth)
    {
        if (count <= m_maxLeafSize) { createLeaf(node, depth); return; }
        std::cout << "Attempting to terminate recursion but leaf can't hold all the elements. "
                  << "Recursing again, current depth: " << depth << std::endl;
    }
    int bestAxis = -1;
    float bestSplit = 0.0f;
    int coplanarSide = 0;
    AABB bestLeft, bestRight;
    const float targetCost = (1.0f - m_minImprovement) * computeCost(node.bounds, count);
    float bestCost = std::numeric_limits<float>::infinity();
    unsigned int bestLeftCount = count, bestRightCount = count;
    for (int axis = 0; axis < 3; ++axis)
    {
        std::sort(node.faces.begin(), node.faces.end(), AxisSort{axis});
        std::vector<float> planes;
        planes.reserve(2 * node.faces.size());
        for (unsigned int i = 0; i < node.faces.size(); ++i)
        {
            planes.push_back((&node.faces[i]->bounds.mins.x)[axis]);
            planes.push_back((&node.faces[i]->bounds.maxs.x)[axis]);
        }
        std::sort(planes.begin(), planes.end());
        std::set<BspBuilderFace*> active;
        unsigned int scanned = 0;
        float split = planes[0] - 1.0f;
        for (unsigned int i = 0; i < planes.size(); ++i)
        {
            while (i < planes.size() && planes[i] == split) ++i;
            if (i == planes.size()) break;
            split = planes[i];
            if (split < (&node.bounds.mins.x)[axis] || split > (&node.bounds.maxs.x)[axis]) continue;
            AABB left = node.bounds, right = node.bounds;
            (&left.maxs.x)[axis] = split;
            (&right.mins.x)[axis] = split;
            while (scanned < count && split > (&node.faces[scanned]->bounds.mins.x)[axis] - m_epsilon)
                active.insert(node.faces[scanned++]);
            std::vector<BspBuilderFace*> candidates;
            candidates.reserve(count);
            for (std::set<BspBuilderFace*>::iterator it = active.begin(); it != active.end(); )
            {
                BspBuilderFace* face = *it;
                if (split > (&face->bounds.maxs.x)[axis] + m_epsilon) active.erase(it++);
                else { candidates.push_back(face); ++it; }
            }
            const unsigned int outsideRight = count - scanned;
            const unsigned int outsideLeft = scanned - static_cast<unsigned int>(candidates.size());
            std::vector<BspBuilderFace*> leftFaces, rightFaces, coplanar;
            leftFaces.reserve(count); rightFaces.reserve(count); coplanar.reserve(count);
            partition(candidates, node.bounds, left, leftFaces, right, rightFaces, axis, split, coplanar);
            const int sideCount = coplanar.empty() ? 1 : 2;
            for (int side = 0; side < sideCount; ++side)
            {
                const unsigned int leftCount = outsideLeft + static_cast<unsigned int>(leftFaces.size())
                    + (side == 0 ? static_cast<unsigned int>(coplanar.size()) : 0);
                const unsigned int rightCount = outsideRight + static_cast<unsigned int>(rightFaces.size())
                    + (side == 1 ? static_cast<unsigned int>(coplanar.size()) : 0);
                const float leftCost = computeCost(left, leftCount);
                const float cost = leftCost + computeCost(right, rightCount);
                if (cost < bestCost)
                {
                    bestCost = cost; bestLeft = left; bestRight = right;
                    bestAxis = axis; bestSplit = split; coplanarSide = side;
                    bestLeftCount = leftCount; bestRightCount = rightCount;
                }
            }
        }
    }
    const bool improved = targetCost > bestCost;
    const bool bothReduced = bestLeftCount < count && bestRightCount < count;
    // Retail evaluates this before testing bestAxis == -1; no new validity guard.
    const bool interior = bestSplit > (&node.bounds.mins.x)[bestAxis]
                       && (&node.bounds.maxs.x)[bestAxis] > bestSplit;
    if (count <= m_smallNodeSize && !improved) bestAxis = -1;
    if (count <= m_maxLeafSize && !improved && !bothReduced && !interior) bestAxis = -1;
    if (bestAxis == -1) { createLeaf(node, depth); return; }
    node.axis = bestAxis;
    node.split = bestSplit;
    node.children[0] = new BspBuilderNode;
    node.children[1] = new BspBuilderNode;
    std::vector<BspBuilderFace*> coplanar;
    node.children[0]->faces.reserve(count);
    node.children[1]->faces.reserve(count);
    coplanar.reserve(count);
    partition(node.faces, node.bounds, bestLeft, node.children[0]->faces,
              bestRight, node.children[1]->faces, node.axis, node.split, coplanar);
    for (unsigned int i = 0; i < coplanar.size(); ++i)
        node.children[coplanarSide]->faces.push_back(coplanar[i]);
    node.children[0]->bounds = bestLeft;
    node.children[1]->bounds = bestRight;
    build(*node.children[0], depth + 1);
    build(*node.children[1], depth + 1);
}

// ELF 8392810 / PE 6C5210. Parent is an actual, unused parameter in this version.
void BspBuilder::partition(const std::vector<BspBuilderFace*>& faces, const AABB&,
    const AABB& left, std::vector<BspBuilderFace*>& leftFaces,
    const AABB& right, std::vector<BspBuilderFace*>& rightFaces,
    int axis, float split, std::vector<BspBuilderFace*>& coplanar)
{
    for (unsigned int i = 0; i < faces.size(); ++i)
    {
        const AABB& bounds = faces[i]->bounds;
        if (fabsf((&bounds.mins.x)[axis] - split) < m_coplanarEpsilon
            && fabsf((&bounds.maxs.x)[axis] - split) < m_coplanarEpsilon)
        {
            coplanar.push_back(faces[i]);
            continue;
        }
        AABB testLeft = left, testRight = right;
        for (int a = 0; a < 3; ++a)
        {
            const float low = (&bounds.mins.x)[a];
            if (fabsf((&bounds.maxs.x)[a] - low) < m_epsilon)
            {
                if (low >= (&left.mins.x)[a] - m_epsilon && (&left.maxs.x)[a] + m_epsilon >= low)
                { (&testLeft.mins.x)[a] = -1e9f; (&testLeft.maxs.x)[a] = 1e9f; }
                if (low >= (&right.mins.x)[a] - m_epsilon && (&right.maxs.x)[a] + m_epsilon >= low)
                { (&testRight.mins.x)[a] = -1e9f; (&testRight.maxs.x)[a] = 1e9f; }
            }
        }
        const bool intersectsLeft = bounds.maxs.x > testLeft.mins.x && testLeft.maxs.x > bounds.mins.x
            && bounds.maxs.y > testLeft.mins.y && testLeft.maxs.y > bounds.mins.y
            && bounds.maxs.z > testLeft.mins.z && testLeft.maxs.z > bounds.mins.z;
        const bool intersectsRight = bounds.maxs.x > testRight.mins.x && testRight.maxs.x > bounds.mins.x
            && bounds.maxs.y > testRight.mins.y && testRight.maxs.y > bounds.mins.y
            && bounds.maxs.z > testRight.mins.z && testRight.maxs.z > bounds.mins.z;
        if (intersectsLeft && triangleToAABBIntersection(testLeft.mins, testLeft.maxs,
            m_vertices[static_cast<unsigned short>(faces[i]->face->v0)],
            m_vertices[static_cast<unsigned short>(faces[i]->face->v1)],
            m_vertices[static_cast<unsigned short>(faces[i]->face->v2)]))
            leftFaces.push_back(faces[i]);
        if (intersectsRight && triangleToAABBIntersection(testRight.mins, testRight.maxs,
            m_vertices[static_cast<unsigned short>(faces[i]->face->v0)],
            m_vertices[static_cast<unsigned short>(faces[i]->face->v1)],
            m_vertices[static_cast<unsigned short>(faces[i]->face->v2)]))
            rightFaces.push_back(faces[i]);
    }
}

// ELF 8392CA0; inlined in PE 6C5912 / 6C5E57. No factor of two.
float BspBuilder::computeCost(const AABB& bounds, unsigned int count)
{
    const float x = bounds.maxs.x - bounds.mins.x;
    const float y = bounds.maxs.y - bounds.mins.y;
    const float z = bounds.maxs.z - bounds.mins.z;
    // Use the standalone ELF helper's product order. PE inlines and reassociates
    // differently at its initial-cost and candidate-cost sites; see evidence notes.
    return (x * z + x * y + z * y) * static_cast<float>(count);
}


void BspBuilder::createLeaf(const BspBuilderNode& node, unsigned int depth)
{
    ++m_leafCount;
    if (!node.faces.empty())
    {
        const unsigned int count = static_cast<unsigned int>(node.faces.size());
        m_totalFaces += count;
        if (count > m_maxFaces) m_maxFaces = count;
        if (count < m_minFaces) m_minFaces = count;
        ++m_nonemptyLeaves;
        m_totalDepth += depth;
        if (depth > m_deepestDepth) m_deepestDepth = depth;
        if (depth < m_minDepth) m_minDepth = depth;
        ++m_depthSamples;
        m_numFaceReferences += static_cast<unsigned int>(node.faces.size());
    }
    else ++m_emptyLeaves;
}

// ELF 8392D70 / PE 6C5610: post-order destruction, left before right.
void BspBuilder::deleteNode(BspBuilderNode* node)
{
    if (node)
    {
        for (int side = 0; side < 2; ++side) deleteNode(node->children[side]);
        delete node;
    }
}
} } }
