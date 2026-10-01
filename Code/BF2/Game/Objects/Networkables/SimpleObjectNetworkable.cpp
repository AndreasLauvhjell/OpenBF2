// Replicates object transforms, physics, armor and specialized vehicle state.
// Optional acknowledged baselines delta-compress position and primary physics vectors.

#include "Networkables.h"

#include <cmath>
#include <cstdio>                                    // the BF2_TRACE_GHOST lines
#include <cstdlib>                                   // std::getenv / std::atoi - the trace's arming
#include <cstring>                                   // memcmp - the quaternion's bitwise diff

#include "../IObject.h"
#include "../IObjectTemplate.h"                      // getClassID - the supply/parachute forks
#include "../ObjectLayoutAccess.h"                   // LayoutAccess::objectTemplate - the CID forks
#include "../Game/SupplyObject.h"                    // setNetUpdate's supply-landing apply
#include "../Game/Parachute.h"                       // native trigger-message snapshot
#include "../GameLogicComp/IArmorGLComp.h"           // the hit-points channel's four armor calls
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../../IO/BitStream.h"
#include "../../../IO/Network/NetworkManager.h"      // StatePrecisionFull, the times, byte5C
#include "../../../IO/Network/NetworkableInfo.h"     // getPredictionMode - predict's mode fork
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"                       // the IIDs and the two template CIDs
#include "../../../Core/StringUtil.h"                // formatString - the Debug payload, as shipped
#include "../../../Core/System.h"                    // g_system->getExactTime - the timing probe
#include "../../../Math/Mat4.h"
#include "../../../Physics/IPhysicsNode.h"           // the typed physics calls - see the +0x44 note
#include "../../../Physics/PhysicsUtil.h"            // world::setSleepinessRecursive - predict

namespace dice {
namespace hfe {

// Defined in PlayerControlObjectNetworkable.cpp (its Debug site attests the placement); this class
// calls the same free function for its weapon indices.
int getWeaponIndex(const std::vector<world::IObject*>& weapons, world::IObject* weapon);

// Replication statistics and a zero reference for absolute compressed vectors.
namespace {
Vec3 g_zeroReference;                                // ds:0x8AAE8F8 - .bss, stays zero

Vec3 simpleStatsColour()
{
    Vec3 c; c.x = 20.0f; c.y = 200.0f; c.z = 50.0f;
    return c;
}

// Returns the object's physics interface for typed dispatch across platform virtual layouts.
inline world::IPhysicsNode* physicsNodeOf(world::IObject* object)
{
    return static_cast<world::IPhysicsNode*>(world::object_detail::LayoutAccess::physicsNode(object));
}

// Returns the object's cached armor-component interface.
inline world::IArmorGLComp* armorCompOf(world::IObject* object)
{
    return static_cast<world::IArmorGLComp*>(world::object_detail::LayoutAccess::armorComponent(object));
}

inline bool nodeIsMobile(const world::IPhysicsNode* node)
{
    return node->getIsMobile();                      // ELF +0x134
}

// The transform every snapshot reads - a child replicates its relative transformation, a root its
// absolute one, the same fork SpringNetworkable documents.
inline const Mat4& snapshotTransformOf(const world::IObject* object)
{
    return object->getParent() ? object->getRelativeTransformation()
                               : object->getAbsoluteTransformation();
}

// The apply-side mirror: build the matrix, then hand it to the matching setter (slots 59 / 35).
inline void applyTransform(world::IObject* object, const Quat& rotation, const Vec3& position)
{
    Mat4 m = Mat4::getIdentity();
    rotation.toMat(m);
    m.m[12] = position.x;
    m.m[13] = position.y;
    m.m[14] = position.z;
    if (object->getParent())
        object->setRelativeTransformation(m);        // slot 59 vtable +0xEC
    else
        object->setAbsoluteTransformation(m);        // slot 35 vtable +0x8C
}

} // namespace

io::RegistredNetworkStats SimpleObjectNetworkable::m_stats("SimpleObjectNetworkable",
                                                           simpleStatsColour());

// Initializes time, flags, vectors, and identity rotations.
// exactTime and fields from hitPoints onward remain uninitialized until a snapshot writes them.
SimpleObjectNetworkable::UPDATE::UPDATE()
{
    time   = 0.0f;
    byte04 = false;
    position.x = 0.0f; position.y = 0.0f; position.z = 0.0f;
    rotation.x = 0.0f; rotation.y = 0.0f; rotation.z = 0.0f; rotation.w = 1.0f;
    quat28.x = 0.0f; quat28.y = 0.0f; quat28.z = 0.0f; quat28.w = 1.0f;
    vec38.x = 0.0f; vec38.y = 0.0f; vec38.z = 0.0f;
    vec44.x = 0.0f; vec44.y = 0.0f; vec44.z = 0.0f;
    vec50.x = 0.0f; vec50.y = 0.0f; vec50.z = 0.0f;
    vec5C.x = 0.0f; vec5C.y = 0.0f; vec5C.z = 0.0f;
    vec68.x = 0.0f; vec68.y = 0.0f; vec68.z = 0.0f;
    vec74.x = 0.0f; vec74.y = 0.0f; vec74.z = 0.0f;
    vec80.x = 0.0f; vec80.y = 0.0f; vec80.z = 0.0f;
    vec8C.x = 0.0f; vec8C.y = 0.0f; vec8C.z = 0.0f;
}

// create - 0x8779920, 145 B: outer -> 0; new(0x5C); the usual factory shape; +0x50 = 1, +0x58 = 0.
// +0x54 (m_predictTime) is left unwritten by create - predict() is its writer. Zeroed in the
// ctor here [CHOSEN], the house pattern.
IBase* SimpleObjectNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    SimpleObjectNetworkable* self = new SimpleObjectNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

SimpleObjectNetworkable::SimpleObjectNetworkable()
{
    m_refCount    = 1;                               // +0x50
    m_predictTime = 0.0f;                            // +0x54 [CHOSEN] - see create's note
    m_records     = 0;                               // +0x58
}

// D2/D1 0x8241250/0x82412A0, 65 B each: revector, delete[] the ring and NULL it, chain.
SimpleObjectNetworkable::~SimpleObjectNetworkable()
{
    if (m_records)
    {
        delete[] m_records;
        m_records = 0;
    }
}

void  SimpleObjectNetworkable::addRef() { ++m_refCount; }
int   SimpleObjectNetworkable::getRef() { return m_refCount; }
int   SimpleObjectNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;
    return r;
}

void* SimpleObjectNetworkable::queryInterface(unsigned int iid) const
{
    SimpleObjectNetworkable* self = const_cast<SimpleObjectNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// init - 0x8241340, 138 B. Bind, then the ring: new[] 4 x 188 - the ELF calls UPDATE::UPDATE on
// each record explicitly, which new[] of a defaulted struct does here too - then reset() through
// the vptr and true.
bool SimpleObjectNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);                             // +0x44
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // +0x18

    UPDATE* ring = new UPDATE[4];                                                // 0x2F0 bytes
    m_records = ring;                                                            // +0x58
    if (!ring)
        return false;
    reset();                                         // vptr +0x1C
    return true;
}

// Resets the base and seeds record zero from the live object without normalizing its quaternion.
// Copies awake physics vectors and rounded armor health; replicationState is cleared last.
void SimpleObjectNetworkable::reset()
{
    io::NetworkableBase::reset();
    m_next = 0;                                                                  // +0x38

    UPDATE& rec0 = m_records[0];
    const Mat4& transform = snapshotTransformOf(m_object);
    rec0.rotation.fromMat(transform);
    rec0.position.x = transform.m[12];
    rec0.position.y = transform.m[13];
    rec0.position.z = transform.m[14];

    world::IPhysicsNode* const node = physicsNodeOf(m_object);
    if (node && nodeIsMobile(node))
    {
        rec0.vec38 = node->getPositionalSpeed();         // ELF +0x60
        rec0.vec44 = node->getPositionalAcceleration();  // ELF +0xAC
        rec0.vec5C = node->getRotationalSpeed();         // ELF +0x64
        rec0.vec68 = node->getRotationalAcceleration();  // ELF +0xB0
        rec0.vec50 = node->getPositionalFriction();      // ELF +0xB4
        rec0.vec74 = node->getRotationalFriction();      // ELF +0xB8

        rec0.byte9C = node->getIsSleeping();             // ELF +0x11C

        if (world::object_detail::LayoutAccess::objectTemplate(m_object)->getClassID()
                == world::CID_SupplyObjectTemplate)
        {
            // reads one byte, not the adjacent padding or a retail object offset.
            rec0.supplyB0 = static_cast<world::SupplyObject*>(m_object)->m_suppliesLanded;
        }
    }

    world::IArmorGLComp* const armor = armorCompOf(m_object);
    if (armor)
    {
        rec0.hitPoints = static_cast<int>(std::ceil(armor->getHitPoints()));     // cvttss2si
    }

    rec0.replicationState = 0;                       // unconditional, the function's last store
}

// Snapshots object state and marks changes across 19 channels.
// Quaternion bytes compare exactly; vectors use component epsilon tests. Two local-speed vectors share bit 14.
void SimpleObjectNetworkable::updateStateMask(unsigned int* stateMask)
{
    if (m_object->getFlags() & 1)
        return;                                      // before the ring moves

    m_prev = m_head;                                 // +0x34
    m_head = (m_head + 1) & 3;                       // +0x28
    m_next = m_head;                                 // +0x38

    UPDATE& next = m_records[m_next];
    UPDATE& prev = m_records[m_prev];
    const float eps = 1.1920929e-7f;

    // bit 0: the quaternion, normalised then memcmp'd
    next.rotation.fromMat(snapshotTransformOf(m_object));
    {
        Quat& q = next.rotation;
        const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        if (len >= eps)
        {
            q.w /= len; q.x /= len; q.y /= len; q.z /= len;
        }
        else
        {
            q.w = 1.0f; q.x = 0.0f; q.y = 0.0f; q.z = 0.0f;
        }
    }
    if (std::memcmp(&next.rotation, &prev.rotation, 0x10) != 0)
        *stateMask |= 0x1u;

    // bit 1: the position
    {
        const Mat4& transform = snapshotTransformOf(m_object);
        next.position.x = transform.m[12];
        next.position.y = transform.m[13];
        next.position.z = transform.m[14];
        const bool same = std::fabs(next.position.x - prev.position.x) <= eps
                       && std::fabs(next.position.y - prev.position.y) <= eps
                       && std::fabs(next.position.z - prev.position.z) <= eps;
        if (!same)
            *stateMask |= 0x2u;
    }

    // the physics block
    world::IPhysicsNode* const node = physicsNodeOf(m_object);
    if (node && nodeIsMobile(node))
    {
        typedef const Vec3& (world::IPhysicsNode::*ChannelGetter)() const;
        struct Channel { ChannelGetter getter; int fieldOffset; unsigned int bit; };
        // ELF order: +0x60->vec38 (bit 2), +0xAC->vec44 (bit 5), +0x64->vec5C (bit 3),
        // +0xB0->vec68 (bit 6), +0xB4->vec50 (bit 8), +0xB8->vec74 (bit 9),
        // +0x18C->vec80 (bit 14), +0x194->vec8C (bit 14 again).
        static const Channel channels[8] = {
            { &world::IPhysicsNode::getPositionalSpeed,        0x38, 0x0004u },   // ELF +0x60
            { &world::IPhysicsNode::getPositionalAcceleration, 0x44, 0x0020u },   // ELF +0xAC
            { &world::IPhysicsNode::getRotationalSpeed,        0x5C, 0x0008u },   // ELF +0x64
            { &world::IPhysicsNode::getRotationalAcceleration, 0x68, 0x0040u },   // ELF +0xB0
            { &world::IPhysicsNode::getPositionalFriction,     0x50, 0x0100u },   // ELF +0xB4
            { &world::IPhysicsNode::getRotationalFriction,     0x74, 0x0200u },   // ELF +0xB8
            { &world::IPhysicsNode::getLocalLinearSpeed,       0x80, 0x4000u },   // ELF +0x18C
            { &world::IPhysicsNode::getLocalAngularSpeed,      0x8C, 0x4000u },   // ELF +0x194
        };
        for (int i = 0; i < 8; ++i)
        {
            const Vec3& src = (node->*channels[i].getter)();
            Vec3& dst = *reinterpret_cast<Vec3*>(
                reinterpret_cast<char*>(&next) + channels[i].fieldOffset);
            const Vec3& old = *reinterpret_cast<const Vec3*>(
                reinterpret_cast<const char*>(&prev) + channels[i].fieldOffset);
            dst = src;
            const bool same = std::fabs(dst.x - old.x) <= eps
                           && std::fabs(dst.y - old.y) <= eps
                           && std::fabs(dst.z - old.z) <= eps;
            if (!same)
                *stateMask |= channels[i].bit;
        }

        next.byte9C = node->getIsSleeping();             // ELF +0x11C
        if (prev.byte9C != next.byte9C)
            *stateMask |= 0x80u;

        if (world::object_detail::LayoutAccess::objectTemplate(m_object)->getClassID()
                == world::CID_SupplyObjectTemplate)
        {
            // the live latch supplies bit 15 after the first collision.
            next.supplyB0 = static_cast<world::SupplyObject*>(m_object)->m_suppliesLanded;
            if (prev.supplyB0 != next.supplyB0)
                *stateMask |= 0x8000u;
        }
    }

    // bit 4: the ceil'd hit points
    world::IArmorGLComp* const armor = armorCompOf(m_object);
    if (armor)
    {
        next.hitPoints = static_cast<int>(std::ceil(armor->getHitPoints()));
        if (next.hitPoints != prev.hitPoints)
            *stateMask |= 0x10u;
    }

    // bit 10: the replication byte
    next.replicationState = static_cast<world::SimpleObject*>(m_object)->m_replicationState;
    if (next.replicationState != prev.replicationState)
        *stateMask |= 0x400u;

    // the PCO specials
    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = m_object; o; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
                  o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }
    if (pco)
    {
        if (world::object_detail::LayoutAccess::objectTemplate(m_object)->getClassID()
                == world::CID_ParachuteTemplate)
        {
            next.parachuteAC = static_cast<world::Parachute*>(m_object)->m_triggerMessage;
            if (next.parachuteAC != prev.parachuteAC)
                *stateMask |= 0x2000u;
        }
        else
        {
            static const unsigned int slotBits[3] = { 0x800u, 0x1000u, 0x20000u };
            for (int slot = 0; slot < 3; ++slot)
            {
                if (pco->getActiveWeapon(slot))          // called twice, as in the sibling
                {
                    const std::vector<world::IObject*>& weapons =
                        *static_cast<const std::vector<world::IObject*>*>(pco->getWeapons(slot));
                    next.weaponIndex[slot] = getWeaponIndex(weapons, pco->getActiveWeapon(slot));
                }
                else
                {
                    next.weaponIndex[slot] = -1;
                }
                if (next.weaponIndex[slot] != prev.weaponIndex[slot])
                    *stateMask |= slotBits[slot];
            }

            // Ammo status occupies the range 0..15.
            next.pcoB4 = pco->getAmmoStatus();           // ELF +0x14C
            if (next.pcoB4 != prev.pcoB4)
                *stateMask |= 0x10000u;

            next.isDucking = pco->getIsDucking() ? 1 : 0;
            if (next.isDucking != prev.isDucking)
                *stateMask |= 0x40000u;
        }
    }
}

// The in-TU ranged-write pair - the same IO/BitStream.inl copies every user carries.
namespace {
unsigned int bitsToHold(unsigned int n)
{
    unsigned int bits = 0;
    do
        ++bits;
    while ((1u << bits) - 1u < n);
    return bits;
}

void rangedOutOfBounds(io::BitStream& stream, int value, int lower, int upper)
{
    stream.setError(4);
    Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
        << "(fatal error) Value out of bounds val: " << value
        << " lower " << lower
        << " upper: " << upper;
}
} // namespace

// [TEMP] Traces ghost masks and per-channel bit positions. BF2_TRACE_GHOST selects a net id; -1 traces all.
// [RETIRE-WHEN: ghost-wire-verified] Remove after validating the affected ghost wire format against retail.
namespace {
int ghostTraceTarget()
{
    static int target = -2;                          // -2 = unparsed, -1 = all, 0 = off
    if (target == -2)
    {
        const char* env = std::getenv("BF2_TRACE_GHOST");
        target = env ? std::atoi(env) : 0;
    }
    return target;
}
} // namespace

bool SimpleObjectNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* baseline,
                                           unsigned int stateMask, int precision)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30

    const int traceTarget = ghostTraceTarget();
    const bool trace = traceTarget == -1
                    || (traceTarget > 0 && traceTarget == static_cast<int>(m_networkableId));
    if (trace)
        std::fprintf(stderr, "[TRACE %u] SONW mask=%05X prec=%d baseline=%d start=%u\n",
                     m_networkableId, stateMask, precision, baseline ? 1 : 0,
                     m_updateStartBitPos);
#define BF2_SONW_TRACE(tag) \
    do { if (trace) std::fprintf(stderr, "[TRACE %u]   %-10s -> +%u\n", m_networkableId, tag, \
                                 stream.getBitPosition() - m_updateStartBitPos); } while (0)

    unsigned int mask = stateMask;
    stream.writeBits(&mask, 19);
    BF2_SONW_TRACE("mask19");

    const UPDATE& rec = m_records[m_next];
    world::IPhysicsNode* const comp = physicsNodeOf(m_object);

    if (baseline)
    {
        // the baseline-delta arm
        unsigned char* const base = reinterpret_cast<unsigned char*>(baseline);
        const unsigned short cursor = *reinterpret_cast<const unsigned short*>(base + 0x400);
        const Vec3* const ref = reinterpret_cast<const Vec3*>(base + cursor * 2);

        if (stateMask & 0x2)
            stream.writeCompressedVector(rec.position, ref[0], 0.00050000002f);
        if (comp && nodeIsMobile(comp))
        {
            if (stateMask & 0x4)
                stream.writeCompressedVector(rec.vec38, ref[1], 0.001f);
            if (stateMask & 0x8)
                stream.writeCompressedVector(rec.vec5C, ref[2], 0.001f);
        }
        *reinterpret_cast<unsigned short*>(base + 0x400) =
            static_cast<unsigned short>(cursor + 36);                            // always advances
        BF2_SONW_TRACE("baseArm");
    }
    else
    {
        // the absolute arm
        if (stateMask & 0x2)
        {
            if (stream.hasCompressionVector())
            {
                stream.writeCompressedVector(rec.position, stream.getCompressionVector(),
                                             0.00050000002f);
            }
            else
            {
                unsigned int selector = 0;           // the hand-written selector-0 escape
                stream.writeBits(&selector, 2);
                stream.writeBits(&rec.position.x, 32);
                stream.writeBits(&rec.position.y, 32);
                stream.writeBits(&rec.position.z, 32);
            }
        }
        if (comp && nodeIsMobile(comp))
        {
            if (stateMask & 0x4)
                stream.writeCompressedVector(rec.vec38, g_zeroReference, 0.001f);
                BF2_SONW_TRACE("vec38abs");
            if (io::StatePrecisionFull != precision)
            {
                if (stateMask & 0x8)
                    stream.writeCompressedVector2(rec.vec5C, g_zeroReference, 0.0099999998f);
            }
            else
            {
                if (stateMask & 0x8)
                    stream.writeCompressedVector(rec.vec5C, g_zeroReference, 0.001f);
            }
        }
    }

    // the common tail
    if (stateMask & 0x1)
        stream.writeUnitQuaternion(rec.rotation, 14);
    BF2_SONW_TRACE("quat14");

    if (comp && nodeIsMobile(comp))
    {
        if (stateMask & 0x4000)
            stream.writeCompressedVector(rec.vec80, g_zeroReference, 0.001f);
        if (stateMask & 0x4000)                      // the same bit, twice - as compiled
            stream.writeCompressedVector(rec.vec8C, g_zeroReference, 0.001f);
        if (stateMask & 0x20)
            stream.writeCompressedVector(rec.vec44, g_zeroReference, 0.000099999997f);
        if (stateMask & 0x40)
            stream.writeCompressedVector(rec.vec68, g_zeroReference, 0.000099999997f);
        if (stateMask & 0x100)
            stream.writeCompressedVector(rec.vec50, g_zeroReference, 0.000099999997f);
        if (stateMask & 0x200)
            stream.writeCompressedVector(rec.vec74, g_zeroReference, 0.000099999997f);
        if (stateMask & 0x80)
        {
            unsigned int bit = rec.byte9C ? 1u : 0u;
            stream.writeBits(&bit, 1);
        }
        if (stateMask & 0x8000)
        {
            unsigned int bit = rec.supplyB0 ? 1u : 0u;
            stream.writeBits(&bit, 1);
        }
    }
    BF2_SONW_TRACE("mobileBlk");

    world::IArmorGLComp* const armor = armorCompOf(m_object);
    if (armor)
    {
        if (stateMask & 0x10)
        {
            // Encode health across the live and wreck bands; preserve repeated virtual range queries.
            const int value = rec.hitPoints
                + static_cast<int>(armor->getMaxWreckHitPoints());
            const int lower = static_cast<int>(-armor->getMaxWreckHitPoints())
                + static_cast<int>(armor->getMaxWreckHitPoints());
            const int upper = static_cast<int>(armor->getMaxHitPoints())
                + static_cast<int>(armor->getMaxWreckHitPoints());
            if (value >= lower && value <= upper)
            {
                const unsigned int span = static_cast<unsigned int>(upper - lower);
                unsigned int bits = 0;
                if (span != 0xFFFFFFFFu)             // the .inl's -1 guard
                {
                    do
                        ++bits;
                    while (span + 1 > (1u << bits) - 1u);
                }
                unsigned int out = static_cast<unsigned int>(value - lower);
                stream.writeBits(&out, bits);
            }
            else
            {
                rangedOutOfBounds(stream, value, lower, upper);
            }
        }
    }
    BF2_SONW_TRACE("armor10");

    if (stateMask & 0x400)
    {
        unsigned int state = rec.replicationState;
        stream.writeBits(&state, 2);
    }
    BF2_SONW_TRACE("repl400");

    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = m_object; o; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
                  o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }
    if (pco)
    {
        if (world::object_detail::LayoutAccess::objectTemplate(m_object)->getClassID()
                == world::CID_ParachuteTemplate)
        {
            if (stateMask & 0x2000)
            {
                const int value = rec.parachuteAC;
                if (value <= 7)
                {
                    unsigned int out = static_cast<unsigned int>(value);
                    stream.writeBits(&out, bitsToHold(8));                       // 4 bits
                }
                else
                {
                    rangedOutOfBounds(stream, value, 0, 7);
                }
            }
        }
        else
        {
            static const unsigned int slotBits[3] = { 0x800u, 0x1000u, 0x20000u };
            for (int slot = 0; slot < 3; ++slot)
            {
                if (stateMask & slotBits[slot])
                {
                    const int value = rec.weaponIndex[slot] + 1;
                    if (value <= 8)
                    {
                        unsigned int out = static_cast<unsigned int>(value);
                        stream.writeBits(&out, bitsToHold(9));                   // 4 bits
                    }
                    else
                    {
                        rangedOutOfBounds(stream, value, 0, 8);
                    }
                }
            }
            if (stateMask & 0x10000)
            {
                const int value = rec.pcoB4;
                if (value <= 15)
                {
                    unsigned int out = static_cast<unsigned int>(value);
                    stream.writeBits(&out, bitsToHold(16));                      // 4 bits
                }
                else
                {
                    rangedOutOfBounds(stream, value, 0, 15);
                }
            }
            if (stateMask & 0x40000)
            {
                unsigned int bit = rec.isDucking ? 1u : 0u;
                stream.writeBits(&bit, 1);
            }
        }
    }

    BF2_SONW_TRACE("pcoBlk");
#undef BF2_SONW_TRACE

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// Decodes object state and applies control-object fields before transform and physics.
// Ring acceptance gates transform/physics only; armor, replication and supply state apply independently.
void SimpleObjectNetworkable::setNetUpdate(io::BitStream& stream, float updateTime,
                                           bool ghostActive, io::BaseLineData* baseline,
                                           int precision)
{
    world::IPhysicsNode* const comp = physicsNodeOf(m_object);
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30

    const bool roomForNextUpdate = makeRoomForNextUpdate(ghostActive);
    if (m_prev != m_next)
        m_records[m_next] = m_records[m_prev];                                   // all 188 bytes
    m_records[m_next].time = updateTime;
    if (io::networkManager->getByte5C())
        m_records[m_next].exactTime = static_cast<float>(g_system->getExactTime()); // the timing probe

    unsigned int mask = 0;
    stream.readBits(&mask, 19);

    UPDATE& rec = m_records[m_next];

    if (baseline)
    {
        unsigned char* const base = reinterpret_cast<unsigned char*>(baseline);
        const unsigned short cursor = *reinterpret_cast<const unsigned short*>(base + 0x400);
        const Vec3* const ref = reinterpret_cast<const Vec3*>(base + cursor * 2);

        if (mask & 0x2)
            rec.position = stream.readCompressedVector(ref[0], 0.00050000002f);
        if (comp && nodeIsMobile(comp))
        {
            if (mask & 0x4)
                rec.vec38 = stream.readCompressedVector(ref[1], 0.001f);
            if (mask & 0x8)
                rec.vec5C = stream.readCompressedVector(ref[2], 0.001f);
        }
        *reinterpret_cast<unsigned short*>(base + 0x400) =
            static_cast<unsigned short>(cursor + 36);
    }
    else
    {
        if (mask & 0x2)
            rec.position = stream.readCompressedVector(stream.getCompressionVector(),
                                                       0.00050000002f);
        if (comp && nodeIsMobile(comp))
        {
            if (mask & 0x4)
                rec.vec38 = stream.readCompressedVector(g_zeroReference, 0.001f);
            if (io::StatePrecisionFull != precision)
            {
                if (mask & 0x8)
                    rec.vec5C = stream.readCompressedVector2(g_zeroReference, 0.0099999998f);
            }
            else
            {
                if (mask & 0x8)
                    rec.vec5C = stream.readCompressedVector(g_zeroReference, 0.001f);
            }
        }
    }

    if (mask & 0x1)
        rec.rotation = stream.readUnitQuaternion(14);

    world::IPhysicsNode* const compLate = physicsNodeOf(m_object);   // the ELF re-caches it here
    if (compLate && nodeIsMobile(compLate))
    {
        if (mask & 0x4000)
            rec.vec80 = stream.readCompressedVector(g_zeroReference, 0.001f);
        if (mask & 0x4000)                           // the same bit, twice - as compiled
            rec.vec8C = stream.readCompressedVector(g_zeroReference, 0.001f);
        if (mask & 0x20)
            rec.vec44 = stream.readCompressedVector(g_zeroReference, 0.000099999997f);
        if (mask & 0x40)
            rec.vec68 = stream.readCompressedVector(g_zeroReference, 0.000099999997f);
        if (mask & 0x100)
            rec.vec50 = stream.readCompressedVector(g_zeroReference, 0.000099999997f);
        if (mask & 0x200)
            rec.vec74 = stream.readCompressedVector(g_zeroReference, 0.000099999997f);
        if (mask & 0x80)
        {
            unsigned int bit = 0;
            stream.readBits(&bit, 1);
            rec.byte9C = (bit == 1);
        }
        if (mask & 0x8000)
        {
            unsigned int bit = 0;
            stream.readBits(&bit, 1);
            rec.supplyB0 = (bit == 1) ? 1 : 0;
        }
    }

    world::IArmorGLComp* const armor = armorCompOf(m_object);
    if (armor)
    {
        if (mask & 0x10)
        {
            // four getMaxWreckHitPoints fetches and one getMaxHitPoints on this side too.
            const int lower = static_cast<int>(-armor->getMaxWreckHitPoints())
                + static_cast<int>(armor->getMaxWreckHitPoints());
            const int upper = static_cast<int>(armor->getMaxHitPoints())
                + static_cast<int>(armor->getMaxWreckHitPoints());
            const unsigned int span = static_cast<unsigned int>(upper - lower);
            unsigned int bits = 0;
            if (span != 0xFFFFFFFFu)
            {
                do
                    ++bits;
                while (span + 1 > (1u << bits) - 1u);
            }
            unsigned int q = 0;
            stream.readBits(&q, bits);
            rec.hitPoints = static_cast<int>(q) + lower
                - static_cast<int>(armor->getMaxWreckHitPoints());
        }
    }

    if (mask & 0x400)
    {
        unsigned int state = 0;
        stream.readBits(&state, 2);
        rec.replicationState = static_cast<unsigned char>(state);
    }

    world::IPlayerControlObject* pco = 0;
    for (world::IObject* o = m_object; o; o = o->getParent())
    {
        pco = static_cast<world::IPlayerControlObject*>(
                  o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            break;
    }
    const bool isParachute = pco
        && world::object_detail::LayoutAccess::objectTemplate(m_object)->getClassID()
               == world::CID_ParachuteTemplate;
    if (pco)
    {
        if (isParachute)
        {
            if (mask & 0x2000)
            {
                unsigned int value = 0;
                stream.readBits(&value, bitsToHold(8));                          // 4 bits
                rec.parachuteAC = static_cast<int>(value);
            }
        }
        else
        {
            static const unsigned int slotBits[3] = { 0x800u, 0x1000u, 0x20000u };
            for (int slot = 0; slot < 3; ++slot)
            {
                if (mask & slotBits[slot])
                {
                    unsigned int value = 0;
                    stream.readBits(&value, bitsToHold(9));                      // 4 bits
                    rec.weaponIndex[slot] = static_cast<int>(value) - 1;
                }
            }
            if (mask & 0x10000)
            {
                unsigned int value = 0;
                stream.readBits(&value, bitsToHold(16));                         // 4 bits
                rec.pcoB4 = static_cast<int>(value);
            }
            if (mask & 0x40000)
            {
                unsigned int bit = 0;
                stream.readBits(&bit, 1);
                rec.isDucking = (bit == 1) ? 1 : 0;
            }
        }
    }

    if (stream.hasError())
    {
        // attested AND fatal - type 6 aborts; the clears are the ELF's dead tail.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/SimpleObjectNetworkable.cpp", 523,
              __FUNCTION__)
            << formatString("error reading data");
        stream.clearError();
        return;
    }
    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    // Apply parachute trigger state through its typed interface; animation propagation occurs on its own update.
    if (isParachute)
    {
        if ((mask & 0x2000)
            && world::object_detail::LayoutAccess::objectTemplate(m_object)->getClassID()
                   == world::CID_ParachuteTemplate)
        {
            static_cast<world::Parachute*>(m_object)->setTriggerMessage(rec.parachuteAC);
        }
    }
    else
    {
        static const unsigned int slotBits[3] = { 0x800u, 0x1000u, 0x20000u };
        for (int slot = 0; slot < 3; ++slot)
        {
            if ((mask & slotBits[slot]) && pco && rec.weaponIndex[slot] != -1)
            {
                const std::vector<world::IObject*>& weapons =
                    *static_cast<const std::vector<world::IObject*>*>(pco->getWeapons(slot));
                // the bounds check PlayerControlObjectNetworkable's apply does not have.
                if (rec.weaponIndex[slot] < static_cast<int>(weapons.size()))
                    pco->setActiveWeapon(weapons[rec.weaponIndex[slot]], slot);
            }
        }
        if ((mask & 0x10000) && pco)
            pco->setAmmoStatus(rec.pcoB4);           // ELF +0x150 - see the mask side
        if ((mask & 0x40000) && pco)
            pco->setIsDucking(rec.isDucking != 0);
    }

    // apply, part 2: the object state, behind makeRoom
    if (roomForNextUpdate)
    {
        applyTransform(m_object, rec.rotation, rec.position);

        if (comp && nodeIsMobile(comp))
        {
            comp->setPositionalSpeed(rec.vec38);                     // ELF +0x68
            if (mask & 0x20)
                comp->setPositionalAcceleration(rec.vec44);          // ELF +0x78
            if (mask & 0x8)
                comp->setRotationalSpeed(rec.vec5C);                 // ELF +0x6C
            if (mask & 0x40)
                comp->setRotationalAcceleration(rec.vec68);          // ELF +0x7C
            if (mask & 0x100)
                comp->setPositionalFriction(rec.vec50);              // ELF +0x80
            if (mask & 0x200)
                comp->setRotationalFriction(rec.vec74);              // ELF +0x84
            if (mask & 0x4000)
            {
                comp->setLocalLinearSpeed(rec.vec80);                // ELF +0x188
                comp->setLocalAngularSpeed(rec.vec8C);               // ELF +0x190
            }
            if ((mask & 0x80) && !m_isInNetwork)     // gated on the network flag being clear
            {
                if (rec.byte9C)
                    comp->setSleepiness(-1);                         // ELF +0x124
                else
                    comp->setIsAwake();                              // ELF +0x130
            }
        }
        m_dword10 = -1;                              // the predict-state reset - see the base header
    }

    // apply, part 3: armor / replication byte / supply, ungated
    if (armor)
    {
        if (mask & 0x10)
            armor->setHitPoints(static_cast<float>(rec.hitPoints));              // cvtsi2ss
    }
    if (mask & 0x400)
    {
        static_cast<world::SimpleObject*>(m_object)->m_replicationState = rec.replicationState;
    }
    if ((mask & 0x8000)
        && world::object_detail::LayoutAccess::objectTemplate(m_object)->getClassID()
               == world::CID_SupplyObjectTemplate)
    {
        static_cast<world::SupplyObject*>(m_object)->setSuppliesLanded(rec.supplyB0 != 0);
    }
}

// Predicts the object transform at now and reconciles physics sleep state.
// Linear mode derives velocity from records, then interpolates or extrapolates within configured limits.
// Pending full updates suppress prediction.
void SimpleObjectNetworkable::predict(float now)
{
    world::IPhysicsNode* const comp = physicsNodeOf(m_object);
    m_next = m_head;                                 // before every gate

    if (m_forceFullUpdate)                           // +0x1C
        return;

    g_system->getExactTime();                        // a dead call - result discarded, as compiled

    if (comp && nodeIsMobile(comp))
    {
        if (m_records[m_next].byte9C || m_dword0C)   // the keep-awake latch - see the base header
        {
            if (comp->getSleepiness() != -1)                         // ELF +0x128
                world::setSleepinessRecursive(m_object, -1);
        }
        else
        {
            comp->setIsAwake();                                      // ELF +0x130
        }
        (void)comp->getAbsolutePosition();           // ELF +0x48 - a dead fetch, as compiled
    }

    int mode = static_cast<int>(m_networkableInfo->getPredictionMode());   // vptr +0x24
    if (mode == 1 && io::networkManager->getGlobalPredictionMode() == 2)
        mode = 2;                                    // the global kill switch

    if (mode != 1)
    {
        if (mode)
            return;                                  // mode 2 (or anything else): nothing

        // PMNone: snap and zero
        const UPDATE& rec = m_records[m_next];
        applyTransform(m_object, rec.rotation, rec.position);
        if (comp && nodeIsMobile(comp))
        {
            Vec3 zero;
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setPositionalAcceleration(zero);                   // ELF +0x78
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setRotationalSpeed(zero);                          // ELF +0x6C
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setRotationalAcceleration(zero);                   // ELF +0x7C
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setRotationalFriction(zero);                       // ELF +0x84
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setPositionalFriction(zero);                       // ELF +0x80
        }
        return;
    }

    // PMLinear
    const float predictTime =
        now - static_cast<float>(io::networkManager->getInterpolationTime());
    m_predictTime = predictTime;                     // +0x54

    UPDATE& rec = m_records[m_next];
    if (m_count > 1)
    {
        const UPDATE& prevRec = m_records[(static_cast<unsigned int>(m_next) + 3) & 3];
        const float span = rec.time - prevRec.time;
        if (span > 0.0f)
        {
            const float perSecond = 1.0f / (span / 1000.0f);
            rec.vec38.x = (rec.position.x - prevRec.position.x) * perSecond;
            rec.vec38.y = (rec.position.y - prevRec.position.y) * perSecond;
            rec.vec38.z = (rec.position.z - prevRec.position.z) * perSecond;
        }
        else
        {
            rec.vec38.x = 0.0f; rec.vec38.y = 0.0f; rec.vec38.z = 0.0f;
        }
    }
    else
    {
        rec.vec38.x = 0.0f; rec.vec38.y = 0.0f; rec.vec38.z = 0.0f;
    }

    if (comp)
    {
        if (nodeIsMobile(comp))
            comp->setPositionalSpeed(rec.vec38);                     // ELF +0x68
        if (nodeIsMobile(comp))                       // tested twice, as compiled
        {
            Vec3 zero;
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setPositionalAcceleration(zero);                   // ELF +0x78
            comp->setRotationalSpeed(rec.vec5C);                     // ELF +0x6C
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setRotationalAcceleration(zero);                   // ELF +0x7C
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setRotationalFriction(zero);                       // ELF +0x84
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            comp->setPositionalFriction(zero);                       // ELF +0x80
        }
    }

    if (predictTime <= rec.time)
    {
        // interpolate
        unsigned int lo = 0, hi = 0;
        if (findEmbracingIndices(predictTime, &lo, &hi))
        {
            const float t0   = m_records[lo].time;
            const float span = m_records[hi].time - t0;
            if (span > 0.0 && span < 500.0)
            {
                const float frac = (span <= 1.0f) ? 1.0f : (predictTime - t0) / span;

                Quat blended;
                blended.x = 0.0f; blended.y = 0.0f; blended.z = 0.0f; blended.w = 1.0f;
                blended.slerp(m_records[lo].rotation, m_records[hi].rotation, frac);

                Vec3 position;
                position.x = m_records[lo].position.x
                    + (m_records[hi].position.x - m_records[lo].position.x) * frac;
                position.y = m_records[lo].position.y
                    + (m_records[hi].position.y - m_records[lo].position.y) * frac;
                position.z = m_records[lo].position.z
                    + (m_records[hi].position.z - m_records[lo].position.z) * frac;

                applyTransform(m_object, blended, position);
                m_dword10 = 2;                       // interpolated
                return;
            }
        }
        // no bracket, or a degenerate span: snap from the newest record.
        applyTransform(m_object, rec.rotation, rec.position);
        m_dword10 = 0;                               // snapped
        return;
    }

    // extrapolate
    const float over      = predictTime - rec.time;
    const float maxExtrap = static_cast<float>(io::networkManager->getExtrapolationTime());
    if (maxExtrap >= over)
    {
        float t = over;
        if (t > maxExtrap)
            t = maxExtrap;
        t /= 1000.0f;                                // milliseconds -> seconds

        Vec3 position;
        position.x = rec.vec38.x * t + rec.position.x;
        position.y = rec.vec38.y * t + rec.position.y;
        position.z = rec.vec38.z * t + rec.position.z;

        applyTransform(m_object, rec.rotation, position);
        m_dword10 = 1;                               // extrapolated
    }
    // too far past the newest record: nothing at all - no snap, no stamp.
}

// findEmbracingIndices - 0x8245D50, 151 B - the identical ring search RotationalBundleNetworkable
// carries, over this class's 188-byte stride.
bool SimpleObjectNetworkable::findEmbracingIndices(float t, unsigned int* lo,
                                                   unsigned int* hi) const
{
    if (m_count <= 1)                                // +0x24
        return false;

    unsigned int idx = static_cast<unsigned int>(m_tail);   // +0x2C
    if (m_records[idx].time > t)
        return false;

    *lo = idx;
    *hi = idx;
    for (unsigned int i = 1; i < static_cast<unsigned int>(m_count); ++i)
    {
        idx = (static_cast<unsigned int>(m_tail) + i) & 3;
        *hi = idx;
        if (m_records[idx].time >= t)
            return true;
        *lo = idx;
    }
    return false;
}

// getBaseLine - 0x8241690, 209 B. Append a thirty-SIX-byte snapshot - position, vec38, vec5C from
// record[m_head] - into the BaseLineData buffer, re-syncing m_next to m_head on the way. The same
// two-byte-unit cursor at +0x400 as its siblings, bound cursor + 0x24 <= 0x200, and the same
// doubled advance. This is the record the sender's baseline-delta arm compresses against.
bool SimpleObjectNetworkable::getBaseLine(io::BaseLineData& baseline)
{
    unsigned char* const base = reinterpret_cast<unsigned char*>(&baseline);
    unsigned short& cursor = *reinterpret_cast<unsigned short*>(base + 0x400);

    if (cursor + 0x24u > 0x200u)
        return false;

    Vec3* const out = reinterpret_cast<Vec3*>(base + cursor * 2);
    m_next = m_head;                                                             // the re-sync
    const UPDATE& rec = m_records[m_next];
    out[0] = rec.position;
    out[1] = rec.vec38;
    out[2] = rec.vec5C;

    cursor = static_cast<unsigned short>(cursor + 0x24);
    return true;
}

// a dead 0x148-byte frame around ONE live store - the m_next re-sync. The debug
// printing this framed is compiled out; the sync is not.
void SimpleObjectNetworkable::printInfo()
{
    m_next = m_head;
}

// the shared shape over the 188-byte stride.
float SimpleObjectNetworkable::getAverageUpdateDeltaTime() const
{
    if (!m_count)
        return 0.0f;
    return (m_records[m_head].time - m_records[m_tail].time) / static_cast<float>(m_count);
}

// 0x50 - eighty bits of descriptor headroom, the widest in the directory.
unsigned int SimpleObjectNetworkable::getMinimumBitSize() const
{
    return 80;
}

// 0x5849B - bits 0, 1, 3, 4, 7, 10, 15, 16, 18: the transform, the secondary
// physics vector, hit points, the wake byte, the replication byte and the supply/PCO scalars; the
// primary vectors and the weapon indices are NOT in the ghost mask.
unsigned int SimpleObjectNetworkable::getGhostStateMask() const
{
    return 0x5849Bu;
}

} // namespace hfe
} // namespace dice
