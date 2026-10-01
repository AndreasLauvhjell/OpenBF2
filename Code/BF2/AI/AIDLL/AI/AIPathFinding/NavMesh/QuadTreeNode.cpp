// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining QuadTreeNode operations.

#include "QuadTreeNode.h"

#include "QuadTree.h"
#include "../Material/MaterialManager.h"

namespace dice {
namespace hfe {
namespace ai {

// A branch selects exactly one quadrant. Child index zero means absent; non-zero indices address
// QuadTree::m_nodes directly. Equality belongs to the positive-X / positive-Z halves.
// A leaf examines its byte-counted slice of m_nodePolygons. It rejects downward/vertical polygon
// normals and negative material costs, then keeps the candidate with the smallest squared vertical
// projection distance. Equal distances retain the first polygon. The output is untouched when no
// candidate succeeds. `materials` is deliberately unguarded on the leaf path.
bool QuadTreeNode::findPolygon(const QuadTree* tree, const Vec2& minimum, float size,
                               const Vec3& point, unsigned int& polygon,
                               MaterialManager* materials, float allowedHeightDiff) const
{
    if (!m_isLeaf)
    {
        const float half = size * 0.5f;
        const float middleX = minimum.x + half;
        const float middleZ = minimum.y + half;

        unsigned int child;
        Vec2 childMinimum;

        if (middleX > point.x)
        {
            childMinimum.x = minimum.x;
            if (middleZ > point.z)
            {
                childMinimum.y = minimum.y;
                child = m_children[0];
            }
            else
            {
                childMinimum.y = middleZ;
                child = m_children[1];
            }
        }
        else
        {
            childMinimum.x = middleX;
            if (middleZ > point.z)
            {
                childMinimum.y = minimum.y;
                child = m_children[2];
            }
            else
            {
                childMinimum.y = middleZ;
                child = m_children[3];
            }
        }

        if (!child)
            return false;

        return tree->m_nodes[child].findPolygon(tree, childMinimum, half, point, polygon,
                                                materials, allowedHeightDiff);
    }

    Polygon* best = 0;
    unsigned int bestPolygon = 0;
    float bestDistance = 100000000.0f;

    for (unsigned int i = 0; i < m_polygonCount; ++i)
    {
        const unsigned int candidateIndex = tree->m_nodePolygons[m_firstPolygon + i];
        Polygon* const candidate = &tree->m_polygons[candidateIndex];

        // Both shipped comparisons are rejection tests. Their unordered (NaN) case therefore
        // continues into the geometry test: reject normal.y <= 0, then reject cost < 0.
        if (!(candidate->m_normal.y <= 0.0f)
            && !(materials->getMaterialCost(candidate->m_material) < 0.0f))
        {
            float distanceSquared = 0.0f;
            if (tree->withinPolygon(candidate, point, distanceSquared, allowedHeightDiff)
                && distanceSquared < bestDistance)
            {
                bestDistance = distanceSquared;
                best = candidate;
                bestPolygon = candidateIndex;
            }
        }
    }

    if (!best)
        return false;

    polygon = bestPolygon;
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
