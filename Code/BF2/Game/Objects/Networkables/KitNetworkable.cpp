// Registers kits for network ids without replicating state; CreateKitEvent carries their creation data.

#include "Networkables.h"

#include "../../../Core/Ids.h"       // IID_IBase / io::IID_INetworkable / io::IID_INetworkableObject

namespace dice {
namespace hfe {

// The externally visible statistics label is KitObjectNetworkable.
namespace {
Vec3 kitStatsColour()
{
    Vec3 c; c.x = 120.0f; c.y = 10.0f; c.z = 120.0f;
    return c;
}
} // namespace
io::RegistredNetworkStats KitNetworkable::m_stats("KitObjectNetworkable", kitStatsColour());

// create - 0x8779F20, 145 B: outer -> 0; new(0x58); NetworkableBase::NetworkableBase; the
// NetworkableObjectBase stores (+0x40 -1, +0x44 0, +0x48 0, +0x4C -1 - its own ctor's); the two
// vptrs; +0x50 = 1; +0x54 = 0; then queryInterface(iid) and release() (vptr +0x08) on a null
// answer. Same shape as ControlPointNetworkable::create, four bytes smaller (no +0x58 member).
IBase* KitNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    KitNetworkable* self = new KitNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// [INFERRED] from create's inlined stores, exactly like ControlPointNetworkable's ctor.
KitNetworkable::KitNetworkable()
{
    m_refCount  = 1;                                 // +0x50
    m_baselines = 0;                                 // +0x54 and nothing ever allocates it
}

// D2/D1 0x823BD40/0x823BD70, 34 B each: revector both vptrs, then NetworkableBase's chain. NO
// ring delete - compare ControlPointNetworkable's dtor, which frees its +0x54. A kit never
// allocated one, and the ELF body proves the dtor knows it.
KitNetworkable::~KitNetworkable()
{
}

// The quartet - refcount +0x50, the same four bodies every sibling carries.
void  KitNetworkable::addRef() { ++m_refCount; }
int   KitNetworkable::getRef() { return m_refCount; }
int   KitNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;                                 // vptr +0x14 - the deleting destructor
    return r;
}

// IBase / INetworkable -> the primary; INetworkableObject -> the +0x3C base.
void* KitNetworkable::queryInterface(unsigned int iid) const
{
    KitNetworkable* self = const_cast<KitNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// init - 0x823BDF0, 50 B. Two stores and out: the object into NetworkableObjectBase's +0x44, and
// this instance's OWN INetworkableObject interface (the +0x3C sub-object, via queryInterface) into
// INetworkable's +0x18. ControlPointNetworkable::init does the same two plus its typed +0x58 and
// the baseline ring; a kit has neither. Returns true unconditionally.
bool KitNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);                             // +0x44
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // +0x18
    return true;
}

// Leaves the state mask unchanged. Kits have network ids but send no incremental state.
void KitNetworkable::updateStateMask(unsigned int*)
{
}

// Registration intentionally skips NetworkableBase bookkeeping. Removal only clears the network id.
void KitNetworkable::addedToManager()
{
}

void KitNetworkable::removedFromManager()
{
    io::INetworkable::removedFromManager();          // the grandparent's, NOT NetworkableBase's
}

// `mov eax, 1` - write nothing, report success. The kit's whole wire presence is
// the CreateKitEvent that built it; after that there is no state to send.
bool KitNetworkable::getNetUpdate(io::BitStream&, io::BaseLineData*, unsigned int, int)
{
    return true;
}

// / 0x823BE60, 5 B each - both empty.
void KitNetworkable::setNetUpdate(io::BitStream&, float, bool, io::BaseLineData*, int)
{
}

void KitNetworkable::predict(float)
{
}

// `mov eax, 26h` - thirty-eight bits. Non-zero even though this class transmits
// nothing: addDescriptor sums every member's minimum into the descriptor's bit budget, so a kit
// still reserves 38 bits of headroom there. Reproduced; inventing 0 would shrink the descriptor's
// accounting away from retail's.
unsigned int KitNetworkable::getMinimumBitSize() const
{
    return 38;
}

// getAverageUpdateDeltaTime - 0x823C0B0, 59 B. The shared shape (ControlPointNetworkable carries
// the same body over its 36-byte records): no baselines recorded -> 0.0f, else the time span of
// the ring over the update count. For a kit m_count never leaves 0 - updateStateMask is empty
// and nothing else advances the ring - so the first arm always answers. The full body is
// transcribed anyway because it is what the ELF compiled, 40-byte stride and all.
float KitNetworkable::getAverageUpdateDeltaTime() const
{
    if (!m_count)                                    // +0x24 forever 0 here
        return 0.0f;
    return (m_baselines[m_head].time - m_baselines[m_tail].time)     // +0x28 +0x2C, stride 40
           / static_cast<float>(m_count);
}

} // namespace hfe
} // namespace dice
