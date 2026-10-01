// Shared replication state and four-slot delta-baseline ring. [INFERRED] Source placement.

#include "NetworkableBase.h"

namespace dice {
namespace hfe {
namespace io {

// Calls INetworkable's constructor first, stores its own vptr,
// then eight stores of its own.
NetworkableBase::NetworkableBase()
{
    m_forceFullUpdate = true;   // +0x1C, `mov byte ptr [ebx+1Ch], 1`
    m_unknown20 = 0;            // +0x20
    m_count     = 0;            // +0x24
    m_head      = 0;            // +0x28
    m_tail      = 0;            // +0x2C
    m_updateStartBitPos = 0;    // +0x30
    m_prev      = 0;            // +0x34
    m_next      = 0;            // +0x38  — the last store; a dword here ends at 0x3C == sizeof
}

// The D0 form restores the vptr, calls ~INetworkable, and
// tail-jumps to operator delete. Nothing is freed by hand, which confirms the ring holds indices
// rather than pointers.
NetworkableBase::~NetworkableBase()
{
}

// Clears the ring and re-arms the full-update flag.
void NetworkableBase::reset()
{
    m_forceFullUpdate = true;   // +0x1C — the next update will be absolute, not a delta
    m_count = 0;                // +0x24
    m_head  = 0;                // +0x28
    m_tail  = 0;                // +0x2C
    m_unknown20 = 0;            // +0x20
    m_updateStartBitPos = 0;    // +0x30
    m_prev  = 0;                // +0x34
    m_next  = 0;                // +0x38

    // The only place this class writes below +0x1C: it reaches into the base sub-object and
    // reactivates the ghost. So reset() does two jobs — clear the ring and re-enable replication.
    m_ghostActive = true;       // +0x0C, INetworkable's field
}

// Advances the baseline ring and returns whether the next update must be absolute rather than a delta.
bool NetworkableBase::makeRoomForNextUpdate(bool ghostActive)
{
    bool full = ghostActive;

    // The flag is one-shot: consumed here and cleared, so exactly one absolute update follows a
    // construction or a reset. That is the correctness invariant of the whole scheme — a delta
    // against a baseline the peer never received is unrecoverable.
    if (m_forceFullUpdate)
    {
        m_forceFullUpdate = false;
        full = true;
    }

    // An absolute update invalidates the stored history, so the ring is emptied rather than
    // advanced. Note this discards the baselines without touching m_head/m_tail; they are reset
    // below by the count == 0 branch on the next call through.
    if (full)
        m_count = 0;

    if (m_count == 0)
    {
        // First update after a reset: every index collapses to slot 0.
        m_next = 0;
        m_prev = 0;
        m_head = 0;
        m_tail = 0;
        m_count = 1;
        return full;
    }

    // ring full: the count is clamped to 3 and the tail advances, evicting the oldest baseline.
    // The clamp to 3 rather than 4 is deliberate — the increment at the end of this function then
    // brings it back to 4, so a full ring stays full while the window slides.
    if (m_count == 4)
    {
        m_count = 3;
        m_tail  = (m_tail + 1) & 3;      // `and eax, 3` — mod 4, the depth baked into the code
    }

    m_prev  = m_head;                    // the slot the next delta encodes against
    m_next  = (m_head + 1) & 3;
    m_head  = m_next;
    m_count = m_count + 1;

    return full;
}

// Returns an unrestricted ghost mask; the per-recipient mask determines which fields are sent.
unsigned int NetworkableBase::getGhostStateMask() const
{
    return 0xFFFFFFFFu;
}

// `mov eax, 1 / retn`. Ignores its out-parameter entirely — the reference is
// never written. [UNRESOLVED] whether the 1 is a bool or a status enum; `mov eax,1` is consistent
// with both, and no caller was traced that distinguishes them.
bool NetworkableBase::getBaseLine(BaseLineData&)
{
    return true;
}

} // namespace io
} // namespace hfe
} // namespace dice
