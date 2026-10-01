// Owns cover-position search resources and queries.
// [FRONTIER] Remaining queries require the cover-position implementation.

#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_COVERPOSITION_COVERPOSITIONMANAGER_H   // [INFERRED] guard style
#define AI_AIDLL_AI_AISTRATEGICENTITY_COVERPOSITION_COVERPOSITIONMANAGER_H

#include <map>
#include <vector>

#include "../../../../../Math/Vec3.h"           // Vec2 and Vec3 are typedefs, not classes
#include "../../Main/AIPathfindingSearchTypes.h"  // PathfindingEngine, AStarResource, BinarySearchTree, Hash
#include "CPNodeManager.h"

namespace dice {
namespace hfe {
namespace ai {

class CoverPosition;
class CPNode;

// [PLATFORM] The original key is a libstdc++ tree iterator unavailable in MSVC.
// [CHOSEN, not recovered] This iterator substitute preserves current object layout;
// search methods must verify its required semantics before use.
typedef std::map<int, int>::iterator CPSearchKey;

typedef PathfindingEngine<CPNode, CPNodeManager, CPNode, CPSearchKey, std::vector<int> >
        CPPathfindingEngine;
typedef AStarResource<CPNode, CPNodeManager, CPNode, CPSearchKey, std::vector<int> >
        CPAStarResource;

class CoverPositionManager
{
public:
    CoverPositionManager();
    ~CoverPositionManager();                // , not virtual

    CoverPosition* createCoverPosition(const Vec2&, const Vec2&);
    CoverPosition* getLastCoverPosition() const;
    void RandomizeCoverPos(Vec2, Vec2, Vec2&);
    void RandomizeCoverPosAlongEdge(Vec2, Vec2, Vec2&);
    bool isInCoverPosition(Vec2);
    bool getClosestAttackCoverPosition(const Vec2&, const Vec2&, float, float, Vec2&);
    bool getClosestNavCoverPosition(Vec2, Vec2, Vec2&, CoverPosition*&);
    void finaliseCoverPositions();
    void updatePathfinding(double);
    int search(int, int);

    // dice's own capitalisation on the two `Randomize*` methods - initial capital, and the
    // American spelling next to `finaliseCoverPositions`'s British one, in the same class.

    static CoverPositionManager* m_instance;

private:
    // layout, , read from the constructor and confirmed by the destructor
    /*  +0 */ std::vector<CoverPosition*> m_coverPositions;
    // Never touched by the constructor; the destructor frees the inner buffers with a 12-byte
    // stride, so the elements are vectors of a four-byte type. Which type is [UNRESOLVED] - the
    // readers are `search` and `getClosestNavCoverPosition`, both frontiers.
    /* +12 */ std::vector<std::vector<int> > m_v12;
    /* +24 */ CPPathfindingEngine* m_engine;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGICENTITY_COVERPOSITION_COVERPOSITIONMANAGER_H
