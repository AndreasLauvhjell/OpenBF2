// Cluster record used by the navigation graph. Loading initializes its stored data.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTER_H
#define AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTER_H

namespace dice {
namespace hfe {
namespace ai {

class Cluster
{
public:
    Cluster() {}      // empty
    ~Cluster() {}     // empty

    /* +0x00 */ unsigned char  m_unread00[8];         // [UNRESOLVED] - see the note above
    /* +0x08 */ unsigned short m_firstPortal;         // getClusterPortal's base
    /* +0x0A */ unsigned short m_numberOfPortals;     // getClusterPortal's stride
    /* +0x0C */ unsigned char  m_unread0C[4];         // index 2 (+0x0E): map-part walkability state
};

typedef char ClusterIsTheFileRecord[sizeof(Cluster) == 16 ? 1 : -1];

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPATHFINDING_CLUSTER_CLUSTER_H
