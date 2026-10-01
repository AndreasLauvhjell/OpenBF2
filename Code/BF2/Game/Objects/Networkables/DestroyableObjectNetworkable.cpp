// Replicates destroyable-object armor through a four-entry baseline ring.
// Static ghosts consume deterministic network ids during map loading, shared by client and server.

#include "Networkables.h"

#include <cmath>

#include "../IObject.h"
#include "../GameLogicComp/ArmorGLComp.h"
#include "../../../IO/BitStream.h"           // the wire pair
#include "../../../Core/Debug.h"             // the attested "must have armor comp" sites
#include "../../../Core/Ids.h"               // the quartet's IIDs
#include "../../../Core/StringUtil.h"        // formatString - the Debug payload, as shipped
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

// THE TU static - the static initialiser at 0x82331A0:
// m_stats (ds:0x8AAE724) <- RegistredNetworkStats("DestroyableObjectNetworkable", Vec3(10, 140, 10))
// - 0x41200000 = 10.0f (x and z), 0x430C0000 = 140.0f (y). A green.
namespace {
Vec3 destroyableStatsColour()
{
    Vec3 c; c.x = 10.0f; c.y = 140.0f; c.z = 10.0f;
    return c;
}
} // namespace
io::RegistredNetworkStats DestroyableObjectNetworkable::m_stats("DestroyableObjectNetworkable",
                                                                destroyableStatsColour());

// Snapshots armor hit points and hit direction and marks changed fields.
// Advances the baseline ring even with no armor; direction uses component epsilon tests.
void DestroyableObjectNetworkable::updateStateMask(unsigned int* stateMask)
{
    m_prev = m_head;                                    // +0x34
    m_head = (m_head + 1) & 3;                          // +0x28
    m_next = m_head;                                    // +0x38

    world::IObjectComponent* const armor = m_object->getArmorComponent();
    if (!armor)
        return;                                         // the ring already moved - see above

    world::ArmorGLComp* const armorComp = static_cast<world::ArmorGLComp*>(armor);
    BaseLine* const ring = m_baselines;

    ring[m_next].hitPoints = armorComp->getHitPoints();                 // vtable +0x3C
    if (ring[m_next].hitPoints != ring[m_prev].hitPoints)
        *stateMask |= 0x01;

    ring[m_next].lastHitDirection = armorComp->getLastHitDirection();   // vtable +0xC0

    const Vec3& next = ring[m_next].lastHitDirection;
    const Vec3& prev = ring[m_prev].lastHitDirection;
    const float eps  = 1.1920929e-7f;
    const bool same  = std::fabs(next.x - prev.x) <= eps
                    && std::fabs(next.y - prev.y) <= eps
                    && std::fabs(next.z - prev.z) <= eps;
    if (!same)
        *stateMask |= 0x02;
}

// create - 0x8779CA0, 145 B (the registry TU's factory, kept beside its class like the others).
// new(0x58); NetworkableBase ctor; the second base's four dwords (+0x40 -1, +0x44 0, +0x48 0,
// +0x4C 1 - NetworkableObjectBase's inlined constructor); both vptrs; +0x50 refCount = 1;
// +0x54 m_baselines = 0; then the standard qi-or-release tail.
IBase* DestroyableObjectNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    DestroyableObjectNetworkable* self = new DestroyableObjectNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// [INFERRED] from create's stores - no separate ELF constructor exists (it is inlined there).
DestroyableObjectNetworkable::DestroyableObjectNetworkable()
{
    m_refCount  = 1;                                 // +0x50
    m_baselines = 0;                                 // +0x54
}

// D2/D1 0x8232450/0x82324A0, 65 B each: revector, delete[] the ring and NULL it, chain.
DestroyableObjectNetworkable::~DestroyableObjectNetworkable()
{
    if (m_baselines)
    {
        delete[] m_baselines;
        m_baselines = 0;
    }
}

void  DestroyableObjectNetworkable::addRef() { ++m_refCount; }
int   DestroyableObjectNetworkable::getRef() { return m_refCount; }
int   DestroyableObjectNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;
    return r;
}

void* DestroyableObjectNetworkable::queryInterface(unsigned int iid) const
{
    DestroyableObjectNetworkable* self = const_cast<DestroyableObjectNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// init - 0x8232540, 159 B. Bind the object (+0x44) and the cached +0x18 interface, then the ring:
// 4 x 24, and the ELF's per-slot loop zeroes +0x00, +0x04 and the Vec3 at +0x0C..+0x14 but skips
// +0x08 so hitPoints starts as heap residue in slots 1..3. Slot 0's hitPoints is then primed
// from the armor component, ceiled - `call ceil` - the only place this class rounds. No reset()
// call and no priming updateStateMask (compare EngineNetworkable::init, which calls reset()).
bool DestroyableObjectNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);                             // +0x44
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // +0x18

    BaseLine* ring = new BaseLine[4];                                            // 0x60 bytes
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time   = 0.0f;
        ring[i].byte04 = false;
        ring[i].lastHitDirection.x = 0.0f;
        ring[i].lastHitDirection.y = 0.0f;
        ring[i].lastHitDirection.z = 0.0f;
        // hitPoints deliberately untouched - the ELF's loop skips +0x08. See the block above.
    }
    m_baselines = ring;                                                          // +0x54
    m_next = 0;                                                                  // +0x38

    world::IObjectComponent* const armor = m_object->getArmorComponent();        // object +0x3C, raw
    if (armor)
    {
        ring[0].hitPoints = static_cast<float>(
            std::ceil(static_cast<world::ArmorGLComp*>(armor)->getHitPoints())); // ceiled here only
    }
    return m_baselines != 0;
}

// Writes a 2-bit mask followed by selected hit points and hit direction.
// Hit points are offset by maximum wreck damage; direction uses compressed-vector encoding.
// Requires armor. Returns false on stream failure; baseline and precision are unused.
bool DestroyableObjectNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                                unsigned int stateMask, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    m_next = m_head;                                                             // the re-sync

    unsigned int mask = stateMask;
    stream.writeBits(&mask, 2);                                                  // both bits, always

    world::ArmorGLComp* const armor =
        static_cast<world::ArmorGLComp*>(m_object->getArmorComponent());         // object +0x3C
    if (armor)
    {
        if (stateMask & 0x1)
        {
            // the inlined BitStream.inl:59 ranged write
            const unsigned int value = static_cast<unsigned int>(
                m_baselines[m_next].hitPoints + armor->getMaxWreckHitPoints());
            const unsigned int lower = static_cast<unsigned int>(
                  static_cast<int>(-armor->getMaxWreckHitPoints())
                + static_cast<int>( armor->getMaxWreckHitPoints()));             // zero, at runtime
            const unsigned int upper = static_cast<unsigned int>(
                  static_cast<int>( armor->getMaxHitPoints())
                + static_cast<int>( armor->getMaxWreckHitPoints()));
            if (value >= lower && value <= upper)
            {
                const unsigned int range = upper - lower;
                unsigned int bits = 0;
                if (range != 0xFFFFFFFFu)
                {
                    do
                        ++bits;
                    while (range + 1 > (1u << bits) - 1);
                }
                unsigned int scratch = value - lower;
                stream.writeBits(&scratch, bits);
            }
            else
            {
                stream.setError(4);                  // reachedEndOfStream = 1, errorCode = 4
                Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
                    << "(fatal error) Value out of bounds val: " << value
                    << " lower " << lower
                    << " upper: " << upper;
            }
        }
        if (stateMask & 0x2)
        {
            const BaseLine& rec = m_baselines[m_next];
            if (stream.hasCompressionVector())
            {
                stream.writeCompressedVector(rec.lastHitDirection, stream.getCompressionVector(),
                                             0.0099999998f);
            }
            else
            {
                unsigned int selector = 0;           // the hand-written selector-0 escape
                stream.writeBits(&selector, 2);
                stream.writeBits(&rec.lastHitDirection.x, 32);
                stream.writeBits(&rec.lastHitDirection.y, 32);
                stream.writeBits(&rec.lastHitDirection.z, 32);
            }
        }
    }
    else
    {
        // attested AND fatal - Debug type 6 aborts on a retail build.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/DestroyableObjectNetworkable.cpp", 110,
              __FUNCTION__) << "must have armor comp";
    }

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// Decodes armor state and applies hit direction before hit points.
// Unsent fields carry forward. Missing armor or a read failure reports a fatal error.
void DestroyableObjectNetworkable::setNetUpdate(io::BitStream& stream, float updateTime,
                                                bool ghostActive, io::BaseLineData* /*baseline*/,
                                                int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    makeRoomForNextUpdate(ghostActive);                                          // return discarded
    if (m_prev != m_next)
        m_baselines[m_next] = m_baselines[m_prev];                               // the six-dword copy
    m_baselines[m_next].time = updateTime;

    unsigned int mask = 0;
    stream.readBits(&mask, 2);

    world::ArmorGLComp* const armor =
        static_cast<world::ArmorGLComp*>(m_object->getArmorComponent());         // object +0x3C
    if (armor)
    {
        if (mask & 0x1)
        {
            const int lower = static_cast<int>(-armor->getMaxWreckHitPoints())
                            + static_cast<int>( armor->getMaxWreckHitPoints());  // zero, at runtime
            const int range = static_cast<int>( armor->getMaxHitPoints())
                            + static_cast<int>( armor->getMaxWreckHitPoints()) - lower;
            unsigned int bits = 0;
            if (range != -1)
            {
                do
                    ++bits;
                while (static_cast<unsigned int>(range) + 1 > (1u << bits) - 1);
            }
            unsigned int raw = 0;
            stream.readBits(&raw, bits);
            // the int-in-a-float-slot store - see the block comment.
            *reinterpret_cast<int*>(&m_baselines[m_next].hitPoints) =
                static_cast<int>(raw) + lower - static_cast<int>(armor->getMaxWreckHitPoints());
        }
        if (mask & 0x2)
        {
            m_baselines[m_next].lastHitDirection =
                stream.readCompressedVector(stream.getCompressionVector(), 0.0099999998f);
        }
    }
    else
    {
        // attested AND fatal - see the block comment.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/DestroyableObjectNetworkable.cpp", 163,
              __FUNCTION__) << "must have armor comp";
    }

    if (stream.hasError())
    {
        // attested AND fatal - Debug type 6 aborts; the clears below are the ELF's dead tail.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/DestroyableObjectNetworkable.cpp", 176,
              __FUNCTION__) << formatString("error reading data");
        stream.clearError();
        return;
    }
    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    if (mask != 0 && armor)
    {
        if (mask & 0x2)
            armor->setLastHitDirection(m_baselines[m_next].lastHitDirection, true);   // +0xBC forced
        if (mask & 0x1)
            armor->setHitPoints(static_cast<float>(
                *reinterpret_cast<const int*>(&m_baselines[m_next].hitPoints)));      // +0x38
    }
}

// empty - a destroyable does not extrapolate.
void DestroyableObjectNetworkable::predict(float)
{
}

// newest minus oldest over the count - the base ring's average update interval.
float DestroyableObjectNetworkable::getAverageUpdateDeltaTime() const
{
    if (m_count == 0)
        return 0.0f;
    return (m_baselines[m_head].time - m_baselines[m_tail].time) / static_cast<float>(m_count);
}

// SIX bits - the two mask bits and change.
unsigned int DestroyableObjectNetworkable::getMinimumBitSize() const
{
    return 6;
}

} // namespace hfe
} // namespace dice
