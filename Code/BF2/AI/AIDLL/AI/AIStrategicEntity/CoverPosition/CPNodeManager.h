// Node manager for cover-position searches.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_COVERPOSITION_CPNODEMANAGER_H     // [INFERRED] guard style
#define AI_AIDLL_AI_AISTRATEGICENTITY_COVERPOSITION_CPNODEMANAGER_H

#include <vector>

namespace dice {
namespace hfe {
namespace ai {

class CPNode;

class CPNodeManager
{
public:
    CPNodeManager();
    ~CPNodeManager();           // , not virtual

    void init();
    void reset();

private:
    // layout, , every word read from the 54-byte constructor
    // The element size of the vector is two bytes: the destructor computes its count as
    // `(end_of_storage - start) >> 1` and frees `2 * count`. What the shorts are is [UNRESOLVED].
    /*  +0 */ std::vector<unsigned short> m_v0;
    // The head of a singly linked chain of nodes threaded through `CPNode + 4`; the destructor
    // unlinks and deletes every one of them.
    /* +12 */ CPNode* m_nodes;      // [INFERRED] name
    /* +16 */ int m_i16;            // = -1 [UNRESOLVED] - the only non-zero the constructor writes
    /* +20 */ int m_i20;            // = 0 [UNRESOLVED]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGICENTITY_COVERPOSITION_CPNODEMANAGER_H
