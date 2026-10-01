// Replicates spring position and suspension state using four baselines.
// Parented objects use relative position; roots use absolute position.

#include "Networkables.h"

#include <cmath>

#include "../IObject.h"
#include "../ObjectLayoutAccess.h"           // native IObject physics/response component fields
#include "../Game/Spring.h"                  // Spring::getEngine (+0x180) -
#include "../Game/EngineBase.h"              // EngineBase::getEngineType (+0x1AC) - typed
#include "../../../Physics/IResponsePhysics.h"   // getPermanentGrip (+0x80) - the +0x48 field's type
#include "../../../Physics/PhysicsSpring.h"       // native m_deflection / m_wheelSpin payload
#include "../../../Math/Mat4.h"
#include "../../../IO/BitStream.h"           // the wire pair
#include "../../../Core/Debug.h"             // setNetUpdate's attested "error reading data" site
#include "../../../Core/Ids.h"               // the quartet's IIDs
#include "../../../Core/StringUtil.h"        // formatString - the Debug payload, as shipped
#include "../../Utility/Vars.h"              // the TU's dead con-var handle - see below

namespace dice {
namespace hfe {

// [CHOSEN, not recovered] Native-port contract: the baseline contains only scalar wire state.
// PhysicsSpring's native pointers/base layout can grow without changing this record or its codec.
static_assert(sizeof(SpringNetworkable::BaseLine) == 40, "spring baseline retains its fixed-width stride");

// Registers the spring precision console variable for compatibility; its value is unused.
namespace {
int* g_networkSpringFullPrecision = Vars::getInt("network-spring-fullPrecision", 0);   // ds:0x8AAE954

Vec3 springStatsColour()
{
    Vec3 c; c.x = 50.0f; c.y = 50.0f; c.z = 50.0f;
    return c;
}
} // namespace
io::RegistredNetworkStats SpringNetworkable::m_stats("SpringNetworkable", springStatsColour());
int SpringNetworkable::globalUseSpringFix = 1;

// Creates the requested interface and rejects aggregation.
// [CHOSEN] The constructor clears m_flag54 before init/reset writes it.
IBase* SpringNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    SpringNetworkable* self = new SpringNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// [INFERRED] from create's inlined stores, exactly like ControlPointNetworkable's ctor.
SpringNetworkable::SpringNetworkable()
{
    m_refCount  = 1;                                 // +0x50
    m_flag54    = false;                             // +0x54 [CHOSEN] - see create's note
    m_baselines = 0;                                 // +0x58
}

// D2/D1 0x824C0B0/0x824C100, 65 B each: revector both vptrs, delete[] the ring when there is one
// (and null it), then NetworkableBase's chain.
SpringNetworkable::~SpringNetworkable()
{
    if (m_baselines)
    {
        delete[] m_baselines;
        m_baselines = 0;
    }
}

// The quartet - refcount +0x50, the same four bodies every sibling carries.
void  SpringNetworkable::addRef() { ++m_refCount; }
int   SpringNetworkable::getRef() { return m_refCount; }
int   SpringNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;                                 // vptr +0x14 - the deleting destructor
    return r;
}

// IBase / INetworkable -> the primary; INetworkableObject -> the +0x3C base.
void* SpringNetworkable::queryInterface(unsigned int iid) const
{
    SpringNetworkable* self = const_cast<SpringNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// init - 0x824C1A0, 150 B. Bind (+0x44 and the cached +0x18 interface, the same two stores every
// sibling makes), then the ring: new[] 4 x 40 bytes, and per record the ELF zeroes exactly time
// (+0x00), byte04 (+0x04), position (+0x0C..+0x14) and springVector (+0x18..+0x20) - NOT pad08 and
// NOT trailing (+0x24), which start as heap residue. Then reset() and one updateStateMask into a
// discarded local mask - both through the vptr, so an override would be honoured - and true.
// The ELF's local mask is uninitialised; updateStateMask only ORs bits in and the value dies
// here, so the zero seed below is unobservable.
bool SpringNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);                             // +0x44
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // +0x18

    BaseLine* ring = new BaseLine[4];                                            // 0xA0 bytes
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time   = 0.0f;
        ring[i].byte04 = false;
        ring[i].position.x = 0.0f; ring[i].position.y = 0.0f; ring[i].position.z = 0.0f;
        ring[i].springVector.x = 0.0f; ring[i].springVector.y = 0.0f; ring[i].springVector.z = 0.0f;
        // pad08 and trailing deliberately NOT zeroed - the ELF's loop skips them.
    }
    m_baselines = ring;                                                          // +0x58
    if (!ring)
        return false;

    reset();                                         // vptr +0x1C
    unsigned int scratch = 0;
    updateStateMask(&scratch);                       // vptr +0x2C - result discarded, as shipped
    return true;
}

// Enables the trailing spring channel only for engine-type bit 7 with permanent-grip bit 3.
void SpringNetworkable::reset()
{
    m_flag54 = false;                                // +0x54

    world::IObject* const engine =
        static_cast<world::Spring*>(m_object)->getEngine();          // ELF +0x180
    if (!engine)
        return;

    if ((static_cast<unsigned int>(
             static_cast<world::EngineBase*>(engine)->getEngineType()) & 0x80u) == 0)   // ELF +0x1AC
        return;

    world::IResponsePhysics* const grip = static_cast<world::IResponsePhysics*>(
        world::object_detail::LayoutAccess::responsePhysics(m_object));
    if (!grip)
        return;
    if ((grip->getPermanentGrip() & 8u) != 0)                        // ELF +0x80
        m_flag54 = true;
}

// Snapshots spring position, suspension vector and optional grip value.
// Only position changes set the dirty bit; the other values travel with that change.
// The ring advances even when no physics spring is attached.
void SpringNetworkable::updateStateMask(unsigned int* stateMask)
{
    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;

    const world::PhysicsSpring* const spring = static_cast<world::PhysicsSpring*>(
        static_cast<world::IPhysicsNode*>(world::object_detail::LayoutAccess::physicsNode(m_object)));
    if (!spring)
        return;                                     // the ring already moved - see above

    BaseLine* const ring = m_baselines;

    const Mat4& transform = m_object->getParent() ? m_object->getRelativeTransformation()
                                                  : m_object->getAbsoluteTransformation();
    ring[m_next].position.x = transform.m[12];
    ring[m_next].position.y = transform.m[13];
    ring[m_next].position.z = transform.m[14];

    const Vec3& next = ring[m_next].position;
    const Vec3& prev = ring[m_prev].position;
    const float eps  = 1.1920929e-7f;
    const bool same  = std::fabs(next.x - prev.x) <= eps
                    && std::fabs(next.y - prev.y) <= eps
                    && std::fabs(next.z - prev.z) <= eps;
    if (!same)
        *stateMask |= 0x01;                         // the only bit this class ever sets

    // snapshotted but never compared - see the note above.
    ring[m_next].springVector.x = spring->m_deflection.x;
    ring[m_next].springVector.y = spring->m_deflection.y;
    ring[m_next].springVector.z = spring->m_deflection.z;

    float trailing = 0.0f;
    world::IResponsePhysics* const grip = static_cast<world::IResponsePhysics*>(
        world::object_detail::LayoutAccess::responsePhysics(m_object));
    if (m_flag54 && grip && (grip->getPermanentGrip() & 8u) != 0)
        trailing = spring->m_wheelSpin;
    ring[m_next].trailing = trailing;
}

// Writes the movement bit and selected absolute spring state.
// Requires a mask using only bit 0; baseline and precision are unused. Returns false on stream failure.
bool SpringNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                     unsigned int stateMask, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    m_next = m_head;                                                             // the re-sync

    unsigned int mask = stateMask;
    stream.writeBits(&mask, 1);                                                  // the low bit

    if (stateMask)                                                               // the whole dword
    {
        const BaseLine& rec = m_baselines[m_next];

        Vec3 zero; zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        stream.writeHighCompressedVector(rec.position, zero, 0.0020000001f);
        zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;                             // memset'd again
        stream.writeHighCompressedVector(rec.springVector, zero, 0.0020000001f);

        world::IResponsePhysics* const grip = static_cast<world::IResponsePhysics*>(
            world::object_detail::LayoutAccess::responsePhysics(m_object));
        if (m_flag54)
        {
            bool canQuantise = false;
            if (grip)
                canQuantise = (grip->getPermanentGrip() & 8u) != 0;   // ELF +0x80 - typed
            if (canQuantise)
            {
                bool inRange = false;
                if (m_flag54 && std::fabs(rec.trailing) < 256.0)                 // flag re-tested
                {
                    unsigned int one = 1;
                    stream.writeBits(&one, 1);
                    inRange = true;
                }
                else
                {
                    unsigned int zeroBit = 0;
                    stream.writeBits(&zeroBit, 1);
                }
                if (inRange)
                {
                    double t = (static_cast<double>(rec.trailing) / 256.0 + 1.0) * 0.5;
                    if (t < 0.0)      t = 0.0;
                    else if (t > 1.0) t = 1.0;
                    unsigned int q = static_cast<unsigned int>(t * 65535.0);     // truncation
                    stream.writeBits(&q, 16);
                }
            }
        }
    }

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// Decodes spring state, applying the grip value immediately when present.
// Accepted updates apply position and suspension state when the spring fix is enabled.
// Read failure reports a fatal error.
void SpringNetworkable::setNetUpdate(io::BitStream& stream, float updateTime, bool ghostActive,
                                     io::BaseLineData* /*baseline*/, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    world::PhysicsSpring* const spring = static_cast<world::PhysicsSpring*>(
        static_cast<world::IPhysicsNode*>(world::object_detail::LayoutAccess::physicsNode(m_object)));

    unsigned int present = 0;
    const bool roomForNextUpdate = makeRoomForNextUpdate(ghostActive);
    if (m_prev != m_next)
        m_baselines[m_next] = m_baselines[m_prev];                               // the ten-dword copy
    m_baselines[m_next].time = updateTime;

    stream.readBits(&present, 1);
    if (present)
    {
        BaseLine& rec = m_baselines[m_next];

        Vec3 zero; zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        rec.position = stream.readHighCompressedVector(zero, 0.0020000001f);
        zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        rec.springVector = stream.readHighCompressedVector(zero, 0.0020000001f);

        world::IResponsePhysics* const grip = static_cast<world::IResponsePhysics*>(
            world::object_detail::LayoutAccess::responsePhysics(m_object));
        if (m_flag54)
        {
            if (grip)
            {
                if ((grip->getPermanentGrip() & 8u) != 0)            // ELF +0x80 - typed
                {
                    unsigned int inRange = 0;
                    stream.readBits(&inRange, 1);
                    if (inRange == 1)
                    {
                        unsigned int q = 0;
                        stream.readBits(&q, 16);
                        const float t = static_cast<float>(q / 65535.0);
                        double f = static_cast<double>(t) + static_cast<double>(t) - 1.0;
                        if (!(std::fabs(f) > 3.0518044e-05))                     // one quantum
                            f = 0.0;
                        rec.trailing = static_cast<float>(f * 256.0);
                        spring->m_wheelSpin = rec.trailing;                     // applied NOW
                    }
                }
            }
        }
    }

    if (stream.hasError())
    {
        // attested AND fatal - Debug type 6 aborts; the clears below are the ELF's dead tail.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/SpringNetworkable.cpp", 261, __FUNCTION__)
            << formatString("error reading data");
        stream.clearError();
        return;
    }
    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    if (roomForNextUpdate && present && spring)
    {
        if (globalUseSpringFix)
        {
            const BaseLine& rec = m_baselines[m_next];
            m_object->setRelativePosition(rec.position);                         // slot 57 +0xE4
            spring->m_deflection.x = rec.springVector.x;
            spring->m_deflection.y = rec.springVector.y;
            spring->m_deflection.z = rec.springVector.z;
        }
    }
}

// / 0x824C090, 5 B each - both empty.
void SpringNetworkable::predict(float)
{
}

void SpringNetworkable::printSpringInfo(world::PhysicsSpring*)
{
}

// `sub esp, 148h` then straight out - a debug body compiled away to a dead stack
// frame. Empty here; the 328-byte frame is not behaviour.
void SpringNetworkable::dumpState(int)
{
}

// 0x30 - forty-eight bits of descriptor headroom.
unsigned int SpringNetworkable::getMinimumBitSize() const
{
    return 48;
}

// `xor eax, eax` - a spring overrides the ghost-state mask to zero.
unsigned int SpringNetworkable::getGhostStateMask() const
{
    return 0;
}

} // namespace hfe
} // namespace dice
