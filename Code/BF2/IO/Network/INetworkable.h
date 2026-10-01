// Replication interface with interleaved virtual declarations; preserve their order. [INFERRED] Header placement.
// ID zero denotes an unregistered object. init stores its queried object interface.

#ifndef IO_NETWORK_INETWORKABLE_H     // [INFERRED] guard style
#define IO_NETWORK_INETWORKABLE_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace io {

// sizeof == 0x1C (28).
class INetworkableObject;   // the replicated object's interface, held at +0x18
class INetworkableInfo;     // per-type replication policy, held at +0x08
struct BaseLineData;        // [UNRESOLVED] layout; name is present in mangled signatures
class BitStream;

// sizeof == 0x1C (28).
class INetworkable : public ::dice::hfe::IBase
{
public:
    INetworkable();
    virtual ~INetworkable();                              // vptr +0x10/+0x14 (ELF D1/D0)

    virtual bool init(void*) = 0;                         // vptr +0x18
    virtual void reset();                                 // +0x1C
    virtual void addedToManager();                        // +0x20
    virtual void removedFromManager();                    // +0x24
    virtual unsigned int getGhostStateMask() const = 0;   // vptr +0x28
    virtual void updateStateMask(unsigned int*) = 0;      // vptr +0x2C
    virtual bool getNetUpdate(BitStream&, BaseLineData*,
                              unsigned int stateMask, int precision) = 0;     // vptr +0x30
    virtual void setNetUpdate(BitStream&, float updateTime, bool ghostActive,
                              BaseLineData*, int precision) = 0;              // vptr +0x34
    virtual bool getBaseLine(BaseLineData&) = 0;          // vptr +0x38
    virtual void setGhostActive(bool);                    // +0x3C
    virtual unsigned int getMinimumBitSize() const;       // +0x40
    virtual float getAverageUpdateDeltaTime() const;      // +0x44

    // Sets the policy pointer and copies its priority when non-null. A null pointer leaves priority unchanged. [INFERRED] Name.
    void setNetworkableInfo(INetworkableInfo* info);

// Network IDs are read directly by the server and network manager.
public:
    /* +0x04 */ unsigned short m_networkableId;   // 0 = not registered with the NetworkManager
// The network manager shares access to policy and object-entry fields.
    /* +0x08 */ INetworkableInfo* m_networkableInfo; // Per-type replication policy.
    // GhostManager reads this state directly to identify first updates.
public:
    /* +0x0C */ bool           m_ghostActive;     // default true
protected:
// Object initialization may override this instance's replication priority directly.
public:
    /* +0x10 */ float          m_basePriority; // This instance's copy of the type's base priority.
    // Replication-authority state used by server registration and collision resolution.
public:
    /* +0x14 */ bool           m_isInNetwork;     // [CHOSEN name from the IDB label] default true
public:                                           // (see the +0x08 note - NetworkManager's raw reads)
    /* +0x18 */ INetworkableObject* m_networkableObject;  // set by init() via queryInterface
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif
