// Navigation quad-tree node. File loading initializes its stored data.
// [FRONTIER] Generation and debug queries remain incomplete.
// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining QuadTreeNode operations.

#ifndef AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QUADTREENODE_H
#define AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QUADTREENODE_H

#include "../../Main/AIDllTypes.h"

namespace dice {
namespace hfe {
namespace ai {

class MaterialManager;
class QuadTree;

class QuadTreeNode
{
public:
    QuadTreeNode() : m_isLeaf(0), m_polygonCount(0), m_firstPolygon(0) {}
    ~QuadTreeNode() {}                                            // empty

    bool findPolygon(const QuadTree*, const Vec2&, float, const Vec3&, unsigned int&,
                     MaterialManager*, float) const;

    // A branch stores four node-array indices, with zero meaning "no child". A leaf instead uses
    // the byte count and first index to address QuadTree::m_nodePolygons. The two bytes at +0x12
    // remain unread by this query.
    /* +0x00 */ unsigned int   m_children[4];
    /* +0x10 */ unsigned char  m_isLeaf;
    /* +0x11 */ unsigned char  m_polygonCount;
    /* +0x12 */ unsigned char  m_unread12[2];
    /* +0x14 */ unsigned int   m_firstPolygon;
};

typedef char QuadTreeNodeIsTheFileRecord[sizeof(QuadTreeNode) == 24 ? 1 : -1];

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QUADTREENODE_H
