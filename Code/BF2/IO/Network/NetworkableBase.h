// Replication base with a four-slot baseline ring; changing its depth changes the protocol. [INFERRED] Header placement.
// Construction and reset force an absolute update; reset reactivates the ghost.
// [UNRESOLVED] Original getBaseLine return type beyond its constant-one body.

#ifndef IO_NETWORK_NETWORKABLEBASE_H     // [INFERRED] guard style
#define IO_NETWORK_NETWORKABLEBASE_H

#include "INetworkable.h"

namespace dice {
namespace hfe {
namespace io {

// sizeof == 0x3C (60). Ring depth is fixed at 4. abstract - eight pure virtual slots.
// INetworkable occupies +0x00..+0x1C, which is why this class's own fields start at +0x1C.
class NetworkableBase : public INetworkable
{
public:
    NetworkableBase();
    virtual ~NetworkableBase();

    virtual void reset();                   // 0x8465F50, vptr +0x1C - overrides INetworkable

    // not virtual - it appears nowhere in the vtable. The ring is base-class mechanism, not policy.
    // Returns true when the update being made room for must be absolute rather than a delta.
    bool makeRoomForNextUpdate(bool ghostActive);

    // These override pure INetworkable policy hooks at their inherited slots. The other eight pure
    // slots remain abstract here and are implemented by each concrete networkable.
    virtual unsigned int getGhostStateMask() const;      // +0x28
    virtual bool         getBaseLine(BaseLineData&);     // +0x38

protected:
    /* +0x1C */ bool m_forceFullUpdate;      // ctor and reset set this true
    /* +0x20 */ int  m_unknown20;            // [UNRESOLVED] - reset only, never in the update path
    /* +0x24 */ int  m_count;                // valid baselines, 0..4
    /* +0x28 */ int  m_head;                 // newest slot, mod 4
    /* +0x2C */ int  m_tail;                 // oldest slot, mod 4
    /* +0x30 */ unsigned int m_updateStartBitPos;   // (was m_unknown30):
                                             //   ControlPointNetworkable's getNetUpdate and
                                             //   setNetUpdate open by storing the stream's current
                                             //   bit cursor here and close by subtracting it from
                                             //   the cursor to credit their RegistredNetworkStats -
                                             //   the "bits this update" scratch. Base-owned so every
                                             //   subclass's wire pair can use it; reset zeroes it
    /* +0x34 */ int  m_prev;                 // slot to delta against
    /* +0x38 */ int  m_next;                 // slot to write next
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif
