// Node pool for navigation-mesh A* searches. Spare nodes form an intrusive free list.
// [UNRESOLVED] Meaning of the separate 1031-entry table.
// [FRONTIER] Remaining search operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the node-pool lifecycle and search operations.

#ifndef AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QTSEARCHNODEMANAGER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QTSEARCHNODEMANAGER_H

namespace dice {
namespace hfe {
namespace ai {

class QuadTree;
class QTSearchNode;

class QTSearchNodeManager
{
public:
    // The eight-byte record of the table at +24. `operator new[]` is called without an array cookie
    // so the element is trivially destructible; the constructor zeroes both of its words. Name and
    // field names [INFERRED] - see the header note, the identity is [UNRESOLVED].
    class CacheEntry
    {
    public:
        CacheEntry()
            : m_w0(0)
            , m_w1(0)
        {
        }

        int m_w0;
        int m_w1;
    };

    // the same prime the search resource'S two hashes use. Written out as a literal in the binary.
    enum { kCacheEntries = 1031 };

    QTSearchNodeManager();
    explicit QTSearchNodeManager(QuadTree* quadTree);
    ~QTSearchNodeManager();                      // , [FRONTIER]

    // layout, on the ELF, read store by store out of the constructor
    /*  +0 */ QuadTree* m_quadTree;      // the one argument the second constructor takes
    /*  +4 */ int m_i4;                  // = 0 [UNRESOLVED]
    /*  +8 */ QTSearchNode* m_nodeCache; // = 0 the intrusive free list `getSizeOfNodeCache` walks
    /* +12 */ int m_i12;                 // = 0 [UNRESOLVED]
    /* +16 */ int m_i16;                 // = 0 [UNRESOLVED]
    /* +20 */ int m_i20;                 // = 0 [UNRESOLVED]
    /* +24 */ CacheEntry* m_table;       // new CacheEntry[kCacheEntries]
    /* +28 */ int m_i28;                 // = 0 [UNRESOLVED]
    /* +32 */ int m_i32;                 // = 0 [UNRESOLVED]
    /* +36 */ int m_i36;                 // = 0 [UNRESOLVED]
    /* +40 */ int m_i40;                 // = 0 `returnReset` clears this one and m_b48
    /* +44 */ int m_i44;                 // = 0 [UNRESOLVED]
    /* +48 */ bool m_b48;                // = false also cleared by `returnReset`
    /* +52 */ int m_i52;                 // = 0 [UNRESOLVED]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_NAVMESH_QTSEARCHNODEMANAGER_H
