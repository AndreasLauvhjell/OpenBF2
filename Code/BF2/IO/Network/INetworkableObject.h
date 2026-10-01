// Secondary object-facing interface of a networkable. Header path [INFERRED].
// queryInterface must return the adjusted subobject pointer; descriptors are keyed by that pointer.
// setOwnerID and predict follow IBase, with no new virtuals in NetworkableObjectBase.
// The replicated object pointer's placement on this interface is [CHOSEN]; extra base fields remain
// [UNRESOLVED]. Preserve field and method order across both interface views.

#ifndef IO_NETWORK_INETWORKABLEOBJECT_H     // [INFERRED] guard style
#define IO_NETWORK_INETWORKABLEOBJECT_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world { class IObject; }
namespace io {

class INetworkableObject : public ::dice::hfe::IBase
{
public:
    virtual void setOwnerID(int ownerId) { m_ownerId = ownerId; }   // +0x10
    virtual void predict(float dt) = 0;                             // 5 (+0x14)

    /* +0x04 */ int             m_ownerId;      // ctor -1 (in every recovered class ctor)
    /* +0x08 */ world::IObject* m_object;       // the replicated object - init() stores it, the
                                                //   NetworkManager reads it raw
};

// sizeof 0x14 - the two extra dwords are ctor-initialised (0, -1) and otherwise unread so far.
class NetworkableObjectBase : public INetworkableObject
{
public:
    NetworkableObjectBase()
    {
        m_ownerId = -1;
        m_object  = 0;
        m_dword0C = 0;
        m_dword10 = -1;
    }
    // Prediction state: the keep-awake latch is separate from the state stamp.
    // Stamp values are -1 after a received update, 0 snapped, 1 extrapolated and 2 interpolated. [INFERRED] Names.
    /* +0x0C */ int m_dword0C;      // ctor 0  - the keep-awake latch
    /* +0x10 */ int m_dword10;      // ctor -1 - the predict-state stamp
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_INETWORKABLEOBJECT_H
