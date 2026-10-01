// Per-connection ghost record, allocated and managed by GhostManager.
// [INFERRED] Header path; fields recovered from the dedicated ELF.
// m_ghostId indexes the wire-ID pool; m_networkableId identifies the engine object.
// Negative creation priority marks a static ghost, exempt from ordinary relevance sweeping.

#ifndef GAME_COMMON_GHOSTOBJECT_H             // [INFERRED] guard style
#define GAME_COMMON_GHOSTOBJECT_H

#include <cstddef>                            // offsetof - see the asserts below

namespace dice {
namespace hfe {

namespace io { struct NetworkableDescriptor; }   // IO/Network/NetworkManager.h - by pointer only

class GhostObject
{
public:
    // attested by the three recovered readers
    /* +0x00 */ unsigned short m_ghostId;          // the wire id and the m_ghostObjects index
    /* +0x02 */ unsigned short m_word02;           // [UNRESOLVED] and never initialised
    /* +0x04 */ int            m_ghostableState;   // 1 = live, 3 = gone; see above
    /* +0x08 */ unsigned short m_networkableId;    // the m_ghostHash key
    /* +0x0A */ unsigned short m_word0A;           // [UNRESOLVED] and never initialised

    // Dirty-state mask: zero means fully sent and all ones request a full update.
    // Successful writes save and clear it; packet loss restores the saved bits.
    // [UNRESOLVED] Internal division of the twelve words.
    /* +0x0C */ int            m_stateMask[12];

    // attested: ghostObject stores its `descriptor` argument here (`mov [edi+3Ch], ecx`, 0x80956B6)
    // and compares against it to detect a descriptor swap.
    /* +0x3C */ io::NetworkableDescriptor* m_descriptor;

    // THE static-ghost exemption - see the header note. Set once, at creation, to
    // `priority < 0.0f`; read only by resetGhostStates. Name [INFERRED] from that behaviour.
    /* +0x40 */ bool           m_isStaticGhost;
    /* +0x41 */ unsigned char  m_pad41[3];

    /* +0x44 */ float          m_priority;         // 10000.0f at birth and on the way out,
                                                   // Send-priority accumulator: starts at zero, gains priority each update, and
                                                   // resets after a successful send. removeOldGhosts accepts only values <= 0;
                                                   // NaN therefore prevents removal.
    /* +0x48 */ float          m_accumulatedPriority;
    /* +0x4C */ int            m_dword4C;          // [UNRESOLVED] and never initialised
    // A pending flag, NOT A property - see the header note. Set by creation and by
    // addGhostable; cleared by writeData, writeControlObjectState and transmit. Name [INFERRED]
    // from that set/clear pattern.
    /* +0x50 */ bool           m_ghostablePending;
    // seeded false AT creation AND `inc`'d BY transmit (0x809839A). A per-ghost byte counter that
    // wraps at 256; [UNRESOLVED] what it counts. Flagged here for whoever writes transmit.
    /* +0x51 */ unsigned char  m_byte51;
    /* +0x52 */ unsigned char  m_pad52[2];
};

// Scalar-only x86 layout is reproducible; native x64 widens the pointer while
// preserving wire-state widths.
#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] Native in-memory record, never a packed packet. GhostManager
// allocates with new GhostObject and encodes IDs/masks separately; only m_descriptor widens.
static_assert(sizeof(GhostObject) == 0x60, "native x64 ghost record");
static_assert(offsetof(GhostObject, m_ghostId) == 0x00, "fixed ghost ID prefix");
static_assert(offsetof(GhostObject, m_ghostableState) == 0x04, "fixed ghost state prefix");
static_assert(offsetof(GhostObject, m_networkableId) == 0x08, "fixed networkable ID prefix");
static_assert(offsetof(GhostObject, m_stateMask) == 0x0C, "fixed state mask prefix");
static_assert(offsetof(GhostObject, m_descriptor) == 0x40, "native descriptor pointer alignment");
static_assert(offsetof(GhostObject, m_isStaticGhost) == 0x48, "native static-ghost flag");
static_assert(offsetof(GhostObject, m_priority) == 0x4C, "native ghost priority");
static_assert(offsetof(GhostObject, m_accumulatedPriority) == 0x50, "native accumulated priority");
static_assert(offsetof(GhostObject, m_ghostablePending) == 0x58, "native pending-ghost flag");
#else
    typedef char GhostObjectLayoutCheck[
        (sizeof(GhostObject) == 0x54                            &&
         offsetof(GhostObject, m_ghostId)         == 0x00       &&
         offsetof(GhostObject, m_ghostableState)  == 0x04       &&
         offsetof(GhostObject, m_networkableId)   == 0x08       &&
         offsetof(GhostObject, m_stateMask)       == 0x0C       &&
         offsetof(GhostObject, m_descriptor)      == 0x3C       &&
         offsetof(GhostObject, m_isStaticGhost)   == 0x40       &&
         offsetof(GhostObject, m_ghostablePending) == 0x50      &&
         offsetof(GhostObject, m_priority)        == 0x44       &&
         offsetof(GhostObject, m_accumulatedPriority) == 0x48) ? 1 : -1];
#endif
static_assert(sizeof(GhostObject::m_ghostId) == 2 && sizeof(GhostObject::m_networkableId) == 2,
              "ghost and networkable IDs remain 16-bit");
static_assert(sizeof(GhostObject::m_stateMask) == 48, "ghost dirty masks remain twelve 32-bit words");

// Orders the send max-heap by accumulated priority so unsent ghosts gain precedence.
// Unordered floating-point comparisons return false.
struct GhostLessComp
{
    bool operator()(const GhostObject* a, const GhostObject* b) const
    {
        return a->m_accumulatedPriority < b->m_accumulatedPriority;
    }
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_GHOSTOBJECT_H
