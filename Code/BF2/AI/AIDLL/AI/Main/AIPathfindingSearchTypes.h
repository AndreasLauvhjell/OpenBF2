// Shared path containers, node records and reusable A* search resources.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIPathfindingSearchTypes operations.

#ifndef AI_AIDLL_AI_MAIN_AIPATHFINDINGSEARCHTYPES_H
#define AI_AIDLL_AI_MAIN_AIPATHFINDINGSEARCHTYPES_H

#include <list>
#include <utility>
#include <vector>

#include "IAIPathfinding.h"     // ai::triple, and Vec3 through AIDllTypes.h
#include "AIDllFrontier.h"      // AStarResource::update's named stop
#include "CommonSingletonServer.h"                           // PathfindingEngine::update's clock
#include "../../../AIInterface/BFKnowledge/BFTimer.h"

namespace dice {
namespace hfe {
namespace ai {

// Navigation path result with polygon positions and traversal state.
class QTPath
{
public:
    QTPath()
        : m_i0(0)
        , m_sentinel16(-1)
        , m_i20(0)
    {
        // The vector is default-constructed (three null pointers in retail) by the member above
        // being absent from this list - the binary's four zero stores at +0 and +4..+15 are that
        // and nothing more.
    }

    /*  +0 */ int m_i0;                                       // = 0 [UNRESOLVED]
    /*  +4 */ std::vector<triple<unsigned int, unsigned int, Vec3> > m_nodes;
    /* +16 */ int m_sentinel16;                               // = -1 [UNRESOLVED]
    /* +20 */ int m_i20;                                      // = 0 [UNRESOLVED]
};

// Cluster-level path result and traversal state.
class ClusterNodePath
{
public:
    ClusterNodePath()
        : m_i0(0)
    {
        // m_gap4 deliberately not initialised - the binary steps over it. See above.
    }

    /*  +0 */ int m_i0;                                       // = 0 [UNRESOLVED]
    /*  +4 */ char m_gap4[12];                                // never written
    /* +16 */ std::vector<std::pair<unsigned short, unsigned short> > m_nodes;
};

// Reusable A* node pools, hash tables and search state.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding AIPathfindingSearchTypes operation.

// Allocated only by `BinarySearchTree`'s constructor, which writes two of
// its four words: `+0 = 0` and `+8 = <the previous free-list head>`. The other two are whatever the
// allocator returned. A classic node shape, but which word is left/right/parent is [UNRESOLVED]
// until `insert`/`merge`/`flatten` are read.
template <class T>
class BSTNode
{
public:
    BSTNode()
        : m_i0(0)
    {
        // +4 and +12 deliberately not initialised - the constructor at writes only +0 and
        // +8, and +8 is the free-list link the tree sets after construction.
    }

    /*  +0 */ int m_i0;              // = 0 [UNRESOLVED]
    /*  +4 */ int m_i4;              // never written by the constructor
    /*  +8 */ BSTNode<T>* m_next;    // the free-list link [INFERRED]
    /* +12 */ int m_i12;             // never written by the constructor
};

// Search-node tree and reusable node storage.
template <class Node, class SearchNode>
class BinarySearchTree
{
public:
    explicit BinarySearchTree(int n)
        : m_root(0)
        , m_free(0)
        , m_i12(0)
    {
        // m_i8 deliberately not initialised - see above.
        for (int i = n; i > 0; --i)
        {
            BSTNode<SearchNode>* const node = new BSTNode<SearchNode>();
            node->m_next = m_free;
            m_free = node;
        }
    }

    /*  +0 */ BSTNode<SearchNode>* m_root;   // [INFERRED]
    /*  +4 */ BSTNode<SearchNode>* m_free;   // the pre-allocated pool, proven by the loop
    /*  +8 */ int m_i8;                      // never written by the constructor [UNRESOLVED]
    /* +12 */ int m_i12;                     // = 0 [UNRESOLVED]
};

// Bucketed search-node lookup with a shared node manager.
template <class Node, class NodeManager, class SearchNode>
class Hash
{
public:
    explicit Hash(unsigned int n)
        : m_self0((Hash*)this)
        , m_i12(0)
        , m_i16(0)
        , m_self1((Hash*)this)
        , m_buckets(new int[3 * n + 3])
        , m_i28(0)
        , m_i32(0)
        , m_capacity((int)n)
        , m_nodeManager(0)
    {
        // m_i0 and m_i4 deliberately not initialised - see above.
        // n + 1 entries of three words each, cleared from n down through zero.
        for (unsigned int i = 0; i <= n; ++i)
        {
            m_buckets[3 * i]     = 0;
            m_buckets[3 * i + 1] = 0;
            m_buckets[3 * i + 2] = 0;
        }
    }

    /*  +0 */ int m_i0;                  // never written by the constructor
    /*  +4 */ int m_i4;                  // never written by the constructor
    /*  +8 */ Hash* m_self0;             // = this (an intrusive list sentinel)
    /* +12 */ int m_i12;                 // = 0
    /* +16 */ int m_i16;                 // = 0
    /* +20 */ Hash* m_self1;             // = this
    /* +24 */ int* m_buckets;            // operator new[](3 * n + 3) ints, all zeroed
    /* +28 */ int m_i28;                 // = 0
    /* +32 */ int m_i32;                 // = 0
    /* +36 */ int m_capacity;            // = n
    /* +40 */ NodeManager* m_nodeManager;// = 0, written by the owner right after construction
};

// State for one in-flight A* search. Its node manager and hash tables belong to the search resource.
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
class AStarResource
{
public:
    explicit AStarResource(NodeManager* nodeManager)
        : m_status(0)
        , m_maxIterations(0)
        , m_maxSearchDepth(-1)
        , m_i16(0)
        , m_openSize(0)
        , m_hashSize(0)
        , m_i28(-1)
        , m_nodeManager(nodeManager)
        , m_i36(0)
        , m_open(0)
        , m_closed(0)
        , m_i48(0)
        , m_i52(0)
        , m_i56(0)
        , m_cache(0)
        , m_i64(0)
    {
        // m_i4 deliberately not initialised - see above.
    }

    // One A* step against a deadline. ELF: ClusterNode , QTSearchNode
    // CPNode , all `...AStarResource...6updateEd`. The
    // result is discarded by PathfindingEngine::update. A named stop until the search is read;
    // the definition follows the class.
    void update(double deadline);

    /*  +0 */ int m_status;          // = 0 `PathfindingEngine::getStatus` reads this word
    /*  +4 */ int m_i4;              // never written
    /*  +8 */ int m_maxIterations;   // = 0 [INFERRED] - `setMaxIterationsForResource` writes +8
    /* +12 */ int m_maxSearchDepth;  // = -1 [INFERRED] - `setMaxSearchDepthForResource` writes +12
    /* +16 */ int m_i16;             // = 0, set by the owner after the push
    /* +20 */ int m_openSize;        // = the int handed to BinarySearchTree's constructor
    /* +24 */ int m_hashSize;        // = the int handed to both Hash constructors
    /* +28 */ int m_i28;             // = -1 [UNRESOLVED]
    /* +32 */ NodeManager* m_nodeManager;
    /* +36 */ int m_i36;             // = 0 [UNRESOLVED]
    /* +40 */ BinarySearchTree<Node, SearchNode>* m_open;
    /* +44 */ Hash<Node, NodeManager, SearchNode>* m_closed;   // [INFERRED] which is which
    /* +48 */ int m_i48;             // = 0 [UNRESOLVED]
    /* +52 */ int m_i52;             // = 0 [UNRESOLVED]
    /* +56 */ int m_i56;             // = 0 [UNRESOLVED]
    /* +60 */ Hash<Node, NodeManager, SearchNode>* m_cache;    // [INFERRED]
    /* +64 */ int m_i64;             // = 0 [UNRESOLVED]
};

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Defined here because every instantiation needs it; reached only when a search is in flight.
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
void AStarResource<Node, NodeManager, SearchNode, Key, Path>::update(double)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AStarResource::update");
}
// Pathfinding engine and its pool of reusable search resources.
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
class PathfindingEngine
{
public:
    PathfindingEngine() {}

    // [INFERRED] Steps active searches round-robin until now + budget, at most once per
    // search per call, moving each processed search from front to back.
    void update(double budget);

    /*  +0 */ std::vector<AStarResource<Node, NodeManager, SearchNode, Key, Path>*> m_searches;
    /* +12 */ std::list<unsigned int> m_available;    // [INFERRED] the free list
    /* +16 */ std::list<unsigned int> m_inFlight;     // [INFERRED] - isLockComplete waits on this one
};

// The timer is fetched once; the in-flight count once, before the pass (libstdc++'s O(n) size).
// The walk keeps its own iterator but always erases the front and re-appends its id, which is the
// same element on every step because each processed id moves to the back. The deadline test comes
// before the count test in both images.
template <class Node, class NodeManager, class SearchNode, class Key, class Path>
void PathfindingEngine<Node, NodeManager, SearchNode, Key, Path>::update(double budget)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double deadline = timer->getUpdateTime() + budget;
    const unsigned int count = static_cast<unsigned int>(m_inFlight.size());
    unsigned int processed = 0;
    std::list<unsigned int>::iterator it = m_inFlight.begin();
    while (deadline > timer->getUpdateTime() && processed < count)
    {
        ++processed;
        const unsigned int id = *it;
        m_searches[id]->update(deadline);
        ++it;
        m_inFlight.erase(m_inFlight.begin());
        m_inFlight.insert(m_inFlight.end(), id);
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_AIPATHFINDINGSEARCHTYPES_H
