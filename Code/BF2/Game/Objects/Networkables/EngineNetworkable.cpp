// Replicates engine state and seeds client prediction using a four-entry baseline ring.

#include "Networkables.h"

#include <cmath>

#include "../IObject.h"
#include "../ObjectLayoutAccess.h"           // the +0x14 template slot, typed
#include "../Game/EngineBase.h"              // the typed engine virtuals AND fields - see the +0x184.. note
#include "../EngineTemplate.h"               // getEngineType / getMaxAngleOfAttack - typed
#include "../../../IO/BitStream.h"           // the wire pair
#include "../../../IO/Network/NetworkManager.h"   // io::StatePrecisionFull - stored on the engine
#include "../../../Core/Debug.h"             // setNetUpdate's attested "error reading data" site
#include "../../../Core/Ids.h"               // the quartet's IIDs
#include "../../../Core/StringUtil.h"        // formatString - the Debug payload, as shipped
#include "../../Utility/Vars.h"              // the TU's dead con-var handle - see below
#include <cstdio>                            // [TEMP] the engine-wire gate trace
#include <cstdlib>                           // [TEMP] its getenv arm

namespace dice {
namespace hfe {

// Registers engine precision tuning and replication statistics; the tuning handle is unused.
namespace {
int* g_networkEngineFullPrecision = Vars::getInt("network-engine-fullPrecision", 0);   // ds:0x8AAE774

Vec3 engineStatsColour()
{
    Vec3 c; c.x = 220.0f; c.y = 220.0f; c.z = 220.0f;
    return c;
}
} // namespace
io::RegistredNetworkStats EngineNetworkable::m_stats("EngineNetworkable", engineStatsColour());

namespace {

// Returns the engine interface for typed dispatch across differing PE and ELF virtual layouts.
inline world::EngineBase* engineOf(const world::IObject* e)
{
    return static_cast<world::EngineBase*>(
        const_cast<world::IObject*>(e));                 // the engine object IS an EngineBase
}

inline const float* engineLinearSpeed (const world::IObject* e)
    { return &engineOf(e)->getCurrentRotation().x; }        // ELF +0x184
inline const float* engineAngularRate (const world::IObject* e)
    { return &engineOf(e)->getCurrentRotationalSpeed().x; } // ELF +0x18C
inline int          engineState       (const world::IObject* e)
    { return engineOf(e)->getEngineType(); }                // ELF +0x1AC

inline bool signedLowByteIsNegative(int value)
{
    // All three newCar2 wire gates compile as `test al, al`; c_ETNewCar2 is stored as +0xC2.
    return (static_cast<unsigned int>(value) & 0x80u) != 0;
}

// Returns the engine template through native members; original STL-containing offsets are not portable.
inline const world::EngineTemplate* engineTemplateOf(const world::IObject* engineObject)
{
    return static_cast<const world::EngineTemplate*>(
        world::object_detail::LayoutAccess::objectTemplate(engineObject));   // the +0x14 slot, typed
}

} // namespace

// Snapshots engine state and marks changed channels, clamping throttle to [-1, 1].
// May clear bit 10 when the engine type lacks differential state; callers must not assume an accumulating mask.
void EngineNetworkable::updateStateMask(unsigned int* stateMask)
{
    if (m_object->getFlags() & 1)
        return;                                      // before the ring moves

    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;

    Record& next = m_records[m_next];
    Record& prev = m_records[m_prev];

    world::IObject* const engine = m_engine;
    world::EngineBase* const eng = engineOf(engine);     // the ELF offsets in
                                                         // the comments below are evidence, not
                                                         // arithmetic (see the helper's note)

    next.currentGear = eng->m_gear;                                 // ELF +0x1E8
    if (next.currentGear != prev.currentGear)
        *stateMask |= 0x002;

    next.forwardSpeed = engineLinearSpeed(engine)[2];          // ->z
    if (next.forwardSpeed != prev.forwardSpeed)
        *stateMask |= 0x004;

    next.lateralSpeed = engineLinearSpeed(engine)[0];          // ->x
    if (next.lateralSpeed != prev.lateralSpeed)
        *stateMask |= 0x008;

    next.angularRate = engineAngularRate(engine)[2];           // ->z
    if (next.angularRate != prev.angularRate)
        *stateMask |= 0x010;

    next.field1C4 = eng->m_input;                                   // ELF +0x1C4
    if (next.field1C4 != prev.field1C4)
        *stateMask |= 0x020;

    next.field1D4 = eng->m_float1D4;                                // ELF +0x1D4 - the differential's turn factor
    if (next.field1D4 != prev.field1D4)
        *stateMask |= 0x040;

    // the clamp - written as the ELF's two-sided branch, not min/max.
    const float throttle = eng->m_float1C8;                         // ELF +0x1C8
    float clamped;
    if (throttle < -1.0f)
        clamped = -1.0f;
    else
    {
        clamped = throttle;
        if (clamped > 1.0f)
            clamped = 1.0f;
    }
    next.throttle = clamped;
    if (next.throttle != prev.throttle)
        *stateMask |= 0x080;

    // the excess-RPM branch - the only place a bit is cleared
    if (!signedLowByteIsNegative(engineState(engine)))
    {
        *stateMask &= ~0x400u;                       // taken back out - see above
    }
    else
    {
        const world::EngineTemplate* const engineTemplate = engineTemplateOf(engine);   // ELF +0x14
        const float lowerBound = engineTemplate->m_newCar2MinRpm;   // ELF +0x620
        const float upperBound = engineTemplate->m_newCar2MaxRpm;   // ELF +0x624

        const int divisor = eng->m_dword1F0;                        // ELF +0x1F0 - wheels summed
        float ratio = 0.0f;
        if (divisor > 0)                             // zero or less yields 0.0, no division
            ratio = eng->m_dword1EC / static_cast<float>(divisor);  // ELF +0x1EC - the rpm sum

        float excess = std::fabs(ratio);
        if (lowerBound > excess)
            excess = lowerBound;
        else if (excess > upperBound)
            excess = upperBound;

        next.excessRpm = excess;
        if (next.excessRpm != prev.excessRpm)
            *stateMask |= 0x400;

        next.differentialNegative = (0.0f > ratio) ? 1 : 0;
        next.field1E4 = eng->m_soundInput; // Sound-input baseline.

        *stateMask |= 0x400;                         // unconditional, on top of the test above
    }

    // bit 9's two booleans
    const float measure = eng->m_rotationInput.z;                   // ELF +0x160 - the throttle axis
                                                                    // (RotationalBundle base)
    next.aboveThreshold = (std::fabs(measure) > 0.0099999998f) ? 1 : 0;
    if (prev.aboveThreshold != next.aboveThreshold)
        *stateMask |= 0x200;

    next.isNegative = (0.0f > measure) ? 1 : 0;      // never compared - it rides on bit 9

    // bit 8, gated on the state's 0x20
    if ((engineState(engine) & 0x20) != 0)
    {
        next.field1D8 = eng->m_currentAngle;                        // ELF +0x1D8 - the angle of attack
        if (next.field1D8 != prev.field1D8)
            *stateMask |= 0x100;
    }
}

// Template-dependent bounds and quantization helpers for engine replication.
namespace {

// Typed engine accessors used by replication.
inline int  engineGearChange(const world::IObject* e)
    { return engineOf(e)->getGearChange(); }             // ELF +0x1B0
inline void engineSetGearChange(const world::IObject* e, int v)
    { engineOf(e)->setGearChange(v); }                   // ELF +0x1B4
inline void engineSetCurrentRotation(const world::IObject* e, const Vec3& v)
    { engineOf(e)->setCurrentRotation(v); }              // ELF +0x180
inline void engineSetCurrentRotationalSpeed(const world::IObject* e, const Vec3& v)
    { engineOf(e)->setCurrentRotationalSpeed(v); }       // ELF +0x188

// The 16-bit scaled quantiser both wire directions use: value / (scale * 1.25) mapped to 0..65535,
// truncated. The 12-bit form fixes the scale at 1.0. Shared spelling; the ELF writes each out
// longhand.
inline unsigned int quantiseUnit(double t, double steps)
{
    if (t < 0.0)      t = 0.0;
    else if (t > 1.0) t = 1.0;
    return static_cast<unsigned int>(t * steps);     // truncation - the house cast
}

} // namespace

// create - 0x877A100, 145 B: outer -> 0; new(0x60); the usual factory shape. +0x54 AND +0x58 are
// left unwritten by create - init() stores the engine; nothing recovered stores +0x54. Both nulled
// in the ctor here [CHOSEN]: deterministic beats residue at no observable cost.
IBase* EngineNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    EngineNetworkable* self = new EngineNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

EngineNetworkable::EngineNetworkable()
{
    m_refCount = 1;                                  // +0x50
    m_dword54  = 0;                                  // +0x54 [CHOSEN] - see create's note
    m_engine   = 0;                                  // +0x58 [CHOSEN] - init() is the real writer
    m_records  = 0;                                  // +0x5C
}

// D2/D1 0x8233B30/0x8233B80, 65 B each: revector, delete[] the ring and NULL it, chain.
EngineNetworkable::~EngineNetworkable()
{
    if (m_records)
    {
        delete[] m_records;
        m_records = 0;
    }
}

void  EngineNetworkable::addRef() { ++m_refCount; }
int   EngineNetworkable::getRef() { return m_refCount; }
int   EngineNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;
    return r;
}

void* EngineNetworkable::queryInterface(unsigned int iid) const
{
    EngineNetworkable* self = const_cast<EngineNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// init - 0x8233CF0, 113 B. Bind three pointers this time - +0x44, +0x58 (the engine, the same
// object) and the cached +0x18 interface - then the ring: 4 x 60, time/byte04 zeroed per record,
// and reset() through the vptr. No priming updateStateMask (compare SpringNetworkable::init).
bool EngineNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);                             // +0x44
    m_engine = static_cast<world::IObject*>(object);                             // +0x58 the same
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // +0x18

    Record* ring = new Record[4];                                                // 0xF0 bytes
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time   = 0.0f;
        ring[i].byte04 = false;
        // everything else starts as heap residue - the ELF's loop is two stores per record.
    }
    m_records = ring;                                                            // +0x5C
    if (!ring)
        return false;
    reset();                                         // vptr +0x1C
    return true;
}

// reset - 0x8233D70, 83 B. NetworkableBase::reset() first (the base call - cursors, count and the
// full-update latch), then the whole ring is zeroed - 4 x 15 dwords, a rep stosd per record. The
// one sibling reset that wipes every record instead of reseeding from the live object.
void EngineNetworkable::reset()
{
    io::NetworkableBase::reset();
    if (m_records)                                   // the ELF has no guard - init() always
    {                                                // allocated first; kept for the ctor path
        char* raw = reinterpret_cast<char*>(m_records);
        for (int i = 0; i < 4; ++i)
        {
            unsigned int* p = reinterpret_cast<unsigned int*>(raw + 60 * i);
            for (int d = 0; d < 15; ++d)
                p[d] = 0;
        }
    }
}

// Writes an 11-bit mask and selected engine channels using template-dependent quantization ranges.
// Angle-of-attack and differential channels also require the matching engine-type flags.
// Returns false on stream failure; gear-change state is read live at send time.
bool EngineNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                     unsigned int stateMask, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    m_next = m_head;                                                             // the re-sync
    const world::EngineTemplate* const templ = engineTemplateOf(m_object);

    // [TEMP] the engine-wire gate trace, armed by the same BF2_TRACE_GHOST env as the walk tracer
    // (any value; child networkables carry id 0, so no per-ghost filter is possible here). One
    // line per engine send with the three template-derived wire inputs - maxGear (the gear arm's
    // width), the template engineType (the 0x100 channel gate), and the engine object's
    // engineState (the 0x400 block gate) - the values the jeep_faav 71-vs-44 desync turned on.
    // [RETIRE-WHEN: ghost-wire-verified]
    {
        static int traceArmed = -1;
        if (traceArmed < 0)
            traceArmed = std::getenv("BF2_TRACE_GHOST") ? 1 : 0;
        if (traceArmed)
        {
            std::fprintf(stderr,
                         "[ENG %s] mask=%03X maxGear=%u templType=%d engState=%d start=%u\n",
                         templ->getName().c_str(), stateMask,
                         static_cast<unsigned int>(templ->m_numberOfGears),
                         templ->getEngineType(),
                         m_engine ? engineState(m_engine) : -999,
                         m_updateStartBitPos);
            std::fflush(stderr);
        }
    }

    unsigned int mask = stateMask;
    stream.writeBits(&mask, 11);

    const Record& rec = m_records[m_next];

    if (stateMask & 0x002)
    {
        const unsigned int gear    = static_cast<unsigned int>(rec.currentGear);
        const unsigned int maxGear = static_cast<unsigned int>(templ->m_numberOfGears);   // ELF +0x3C8
        if (gear > maxGear)
        {
            stream.setError(4);
            Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
                << "(fatal error) Value out of bounds val: " << gear
                << " lower " << 0
                << " upper: " << maxGear;
        }
        else
        {
            unsigned int bits = 0;
            if (maxGear != 0xFFFFFFFFu)                  // the -1 guard - zero bits otherwise
            {
                do
                    ++bits;
                while (maxGear + 1 > (1u << bits) - 1u);
            }
            unsigned int value = gear;
            stream.writeBits(&value, bits);
        }
    }

    // The speed scales are the RB base's rotation-limit fields - engines reuse min/maxRotation as
    // their speed range (ELF +0x184/+0x190 = minRotation.z/maxRotation.z, +0x17C/+0x188 = the .x
    // pair, +0x1A8 = maxSpeed.z). Typed via the RotationalBundleTemplate public island.
    const float fwdScale = (std::fabs(templ->m_minRotation.z) <=
                            std::fabs(templ->m_maxRotation.z))
                               ? std::fabs(templ->m_maxRotation.z)
                               : std::fabs(templ->m_minRotation.z);
    if (stateMask & 0x004)
    {
        unsigned int q = quantiseUnit((rec.forwardSpeed / (fwdScale * 1.25) + 1.0) * 0.5, 65535.0);
        stream.writeBits(&q, 16);
    }

    const float latScale = (std::fabs(templ->m_minRotation.x) <=
                            std::fabs(templ->m_maxRotation.x))
                               ? std::fabs(templ->m_maxRotation.x)
                               : std::fabs(templ->m_minRotation.x);
    if (stateMask & 0x008)
    {
        unsigned int q = quantiseUnit((rec.lateralSpeed / (latScale * 1.25) + 1.0) * 0.5, 65535.0);
        stream.writeBits(&q, 16);
    }

    if (stream.hasError())                               // mid-stream check ONE
    {
        stream.clearError();
        return false;
    }

    if (stateMask & 0x010)
    {
        const float angScale = std::fabs(templ->m_maxSpeed.z);      // ELF +0x1A8
        unsigned int q = quantiseUnit((rec.angularRate / (angScale * 1.25) + 1.0) * 0.5, 65535.0);
        stream.writeBits(&q, 16);
    }

    if (stream.hasError())                               // mid-stream check TWO
    {
        stream.clearError();
        return false;
    }

    if (stateMask & 0x020)
    {
        unsigned int q = quantiseUnit((rec.field1C4 / 1.25 + 1.0) * 0.5, 4095.0);
        stream.writeBits(&q, 12);
    }
    if (stateMask & 0x040)
    {
        unsigned int q = quantiseUnit((rec.field1D4 / 1.25 + 1.0) * 0.5, 4095.0);
        stream.writeBits(&q, 12);
    }
    if (stateMask & 0x080)
    {
        unsigned int q = quantiseUnit((rec.throttle / 1.25 + 1.0) * 0.5, 4095.0);
        stream.writeBits(&q, 12);
    }

    if ((static_cast<unsigned int>(engineTemplateOf(m_object)->getEngineType()) & 0x20u) != 0)   // ELF vt +0x144
    {
        if (stateMask & 0x100)
        {
            const float scale = std::fabs(engineTemplateOf(m_object)->getMaxAngleOfAttack());   // ELF vt +0x154
            unsigned int q = quantiseUnit((rec.field1D8 / (scale * 1.25) + 1.0) * 0.5, 65535.0);
            stream.writeBits(&q, 16);
        }
    }

    if (stateMask & 0x200)
    {
        unsigned int above = rec.aboveThreshold ? 1u : 0u;
        stream.writeBits(&above, 1);
    }
    if (stateMask & 0x200)                               // TWO separate tests, as compiled
    {
        unsigned int negative = rec.isNegative ? 1u : 0u;
        stream.writeBits(&negative, 1);
    }

    if (signedLowByteIsNegative(engineState(m_engine)) && (stateMask & 0x400))
    {
        const world::EngineTemplate* const tpl = engineTemplateOf(m_engine);
        const float lower = tpl->m_newCar2MinRpm;            // ELF +0x620
        const float upper = tpl->m_newCar2MaxRpm;            // ELF +0x624

        unsigned int negativeBit = rec.differentialNegative ? 1u : 0u;
        stream.writeBits(&negativeBit, 1);

        unsigned int q16 = quantiseUnit((rec.excessRpm - lower) / (upper - lower), 65535.0);
        stream.writeBits(&q16, 16);

        double t = rec.field1E4;
        unsigned int q8 = quantiseUnit(t, 255.0);
        stream.writeBits(&q8, 8);

        const int gearChange = engineGearChange(m_engine);
        if (gearChange < 0)
        {
            unsigned int sign = 1, magnitude = static_cast<unsigned int>(-gearChange);
            stream.writeBits(&sign, 1);
            stream.writeBits(&magnitude, 2);
        }
        else
        {
            unsigned int sign = 0, magnitude = static_cast<unsigned int>(gearChange);
            stream.writeBits(&sign, 1);
            stream.writeBits(&magnitude, 2);
        }
    }

    if (stream.hasError())                               // the final check
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// Decodes unit values to [-1, 1] with a one-quantum deadzone at the selected precision.
namespace {
float dequantiseUnit16(io::BitStream& stream)
{
    unsigned int q = 0;
    stream.readBits(&q, 16);
    const float t = static_cast<float>(q / 65535.0);
    double f = static_cast<double>(t) + static_cast<double>(t) - 1.0;
    if (!(std::fabs(f) > 3.0518044e-05))
        f = 0.0;
    return static_cast<float>(f);
}

float dequantiseUnit12(io::BitStream& stream)
{
    unsigned int q = 0;
    stream.readBits(&q, 12);
    const float t = static_cast<float>(q / 4095.0);
    double f = static_cast<double>(t) + static_cast<double>(t) - 1.0;
    if (!(std::fabs(f) > 0.00048840052))
        f = 0.0;
    return static_cast<float>(f);
}
} // namespace

// Decodes engine channels and applies them to the engine, carrying unsent values forward.
// Differential state reseeds the integrator; threshold/sign flags preserve negative zero.
// Read failure reports a fatal error.
void EngineNetworkable::setNetUpdate(io::BitStream& stream, float updateTime, bool ghostActive,
                                     io::BaseLineData* /*baseline*/, int precision)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    const world::EngineTemplate* const templ = engineTemplateOf(m_object);

    makeRoomForNextUpdate(ghostActive);                                          // result unused
    if (m_prev != m_next)
        m_records[m_next] = m_records[m_prev];                                   // fifteen dwords
    m_records[m_next].time = updateTime;

    unsigned int mask = 0;
    stream.readBits(&mask, 11);

    Record& rec = m_records[m_next];
    world::EngineBase* const eng = engineOf(m_engine);   // see the helper's note

    if (mask & 0x002)
    {
        const unsigned int maxGear = static_cast<unsigned int>(templ->m_numberOfGears);   // ELF +0x3C8
        unsigned int bits = 0;
        if (maxGear != 0xFFFFFFFFu)                      // the same -1 guard as the sender
        {
            do
                ++bits;
            while (maxGear + 1 > (1u << bits) - 1u);
        }
        unsigned int value = 0;
        stream.readBits(&value, bits);
        rec.currentGear = static_cast<int>(value);
    }

    const float fwdScale = (std::fabs(templ->m_minRotation.z) <=      // ELF +0x184 / +0x190
                            std::fabs(templ->m_maxRotation.z))
                               ? std::fabs(templ->m_maxRotation.z)
                               : std::fabs(templ->m_minRotation.z);
    if (mask & 0x004)
        rec.forwardSpeed = dequantiseUnit16(stream) * (fwdScale * 1.25f);

    const float latScale = (std::fabs(templ->m_minRotation.x) <=      // ELF +0x17C / +0x188
                            std::fabs(templ->m_maxRotation.x))
                               ? std::fabs(templ->m_maxRotation.x)
                               : std::fabs(templ->m_minRotation.x);
    if (mask & 0x008)
        rec.lateralSpeed = dequantiseUnit16(stream) * (latScale * 1.25f);

    if (mask & 0x010)
    {
        const float angScale = std::fabs(templ->m_maxSpeed.z);        // ELF +0x1A8
        rec.angularRate = dequantiseUnit16(stream) * (angScale * 1.25f);
    }

    if (mask & 0x020)
        rec.field1C4 = dequantiseUnit12(stream) * 1.25f;
    if (mask & 0x040)
        rec.field1D4 = dequantiseUnit12(stream) * 1.25f;
    if (mask & 0x080)
        rec.throttle = dequantiseUnit12(stream) * 1.25f;

    if ((static_cast<unsigned int>(engineTemplateOf(m_object)->getEngineType()) & 0x20u) != 0)   // ELF vt +0x144
    {
        if (mask & 0x100)
        {
            const float scale = std::fabs(engineTemplateOf(m_object)->getMaxAngleOfAttack());   // ELF vt +0x154
            rec.field1D8 = dequantiseUnit16(stream) * (scale * 1.25f);
        }
    }

    if (mask & 0x200)
    {
        unsigned int above = 0;
        stream.readBits(&above, 1);
        rec.aboveThreshold = (above == 1) ? 1 : 0;
    }
    if (mask & 0x200)                                    // two separate tests, as compiled
    {
        unsigned int negative = 0;
        stream.readBits(&negative, 1);
        rec.isNegative = (negative == 1) ? 1 : 0;
    }

    if (signedLowByteIsNegative(engineState(m_engine)) && (mask & 0x400))
    {
        const world::EngineTemplate* const tpl = engineTemplateOf(m_engine);
        const float lower = tpl->m_newCar2MinRpm;         // ELF +0x620
        const float upper = tpl->m_newCar2MaxRpm;         // ELF +0x624

        unsigned int negativeBit = 0;
        stream.readBits(&negativeBit, 1);
        unsigned int q16 = 0;
        stream.readBits(&q16, 16);
        const float span = upper - lower;
        const float rpm  = static_cast<float>(q16 / 65535.0) * span + lower;
        rec.excessRpm = (negativeBit == 1) ? -rpm : rpm;  // the bit is the decode's sign

        // the integrator reseed - engine ELF +0x1EC / +0x1F0, in place.
        eng->m_dword1EC = 0.0f;
        eng->m_dword1F0 = 0;
        if (rec.excessRpm != 0.0f)
        {
            eng->m_dword1EC += rec.excessRpm;
            ++eng->m_dword1F0;
        }

        unsigned int q8 = 0;
        stream.readBits(&q8, 8);
        rec.field1E4 = static_cast<float>(q8 / 255.0);
        eng->m_soundInput = rec.field1E4;                 // ELF +0x1E4 - applied NOW

        unsigned int sign = 0;
        stream.readBits(&sign, 1);
        int gearChange;
        if (sign == 1)
        {
            unsigned int magnitude = 0;
            stream.readBits(&magnitude, 2);
            gearChange = -static_cast<int>(magnitude);
        }
        else
        {
            unsigned int magnitude = 0;
            stream.readBits(&magnitude, 2);
            gearChange = static_cast<int>(magnitude);
        }
        engineSetGearChange(m_engine, gearChange);       // vslot +0x1B4 [EngineBase::setGearChange]
    }

    if (stream.hasError())
    {
        // attested AND fatal - type 6 aborts; the clear is the ELF's dead tail.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/EngineNetworkable.cpp", 383, __FUNCTION__)
            << formatString("error reading data");
        stream.clearError();
        return;
    }
    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    // the apply tail - ungated
    eng->m_byte208 = (precision != io::StatePrecisionFull);         // ELF +0x208

    if (mask & 0x002)
        eng->m_gear = rec.currentGear;                              // ELF +0x1E8
    if (mask & 0x00C)                                    // bits 2 OR 3 - one call carries both
    {
        Vec3 v; v.x = rec.lateralSpeed; v.y = 0.0f; v.z = rec.forwardSpeed;
        engineSetCurrentRotation(m_engine, v);           // vslot +0x180 [RotationalBundle::]
    }
    if (mask & 0x010)
    {
        Vec3 w; w.x = 0.0f; w.y = 0.0f; w.z = rec.angularRate;
        engineSetCurrentRotationalSpeed(m_engine, w);    // vslot +0x188 [RotationalBundle::]
    }
    if (mask & 0x020)
        eng->m_input = rec.field1C4;                                // ELF +0x1C4
    if (mask & 0x040)
        eng->m_float1D4 = rec.field1D4;                             // ELF +0x1D4
    if (mask & 0x080)
        eng->m_float1C8 = rec.throttle;                             // ELF +0x1C8
    if (mask & 0x100)
        eng->m_currentAngle = rec.field1D8;                         // ELF +0x1D8
    if (mask & 0x200)
    {
        unsigned int bits = 0;                           // ±1.0f / ±0.0f as RAW bits - see above
        if (rec.aboveThreshold)
            bits = 0x3F800000u;                          // 1.0f
        if (rec.isNegative)
            bits ^= 0x80000000u;                         // the sign flip - even onto 0.0f
        eng->m_dword204 = static_cast<int>(bits);                   // ELF +0x204 - raw float bits, as shipped
    }
}

// predict - 0x8235DB0, 301 B - the client-side dead-reckoning seed, and a real body (most siblings
// stub it). Gated twice: nothing while m_forceFullUpdate (+0x1C) is set - a full update is coming -
// and nothing unless the engine's byte +0x1B4 is raised. Then record[m_next] is pushed into the
// engine wholesale: byte06 -> engine +0x1B5, gear -> +0x1E8, {lateral, 0, forward} ->
// setCurrentRotation, {0, 0, angular} -> setCurrentRotationalSpeed, and field1C4/-1D4/-1D8 ->
// +0x1C4/+0x1D4/+0x1D8. The float parameter is never read.
void EngineNetworkable::predict(float)
{
    if (m_forceFullUpdate)                               // +0x1C
        return;
    world::EngineBase* const eng = engineOf(m_engine);
    if (!eng->m_engineRunning)                                      // ELF +0x1B4
        return;

    const Record& rec = m_records[m_next];
    eng->m_wrecked = (rec.byte06 != 0);                             // ELF +0x1B5
    eng->m_gear = rec.currentGear;                                  // ELF +0x1E8

    Vec3 v; v.x = rec.lateralSpeed; v.y = 0.0f; v.z = rec.forwardSpeed;
    engineSetCurrentRotation(m_engine, v);

    Vec3 w; w.x = 0.0f; w.y = 0.0f; w.z = rec.angularRate;
    engineSetCurrentRotationalSpeed(m_engine, w);

    eng->m_input = rec.field1C4;                                    // ELF +0x1C4
    eng->m_float1D4 = rec.field1D4;                                 // ELF +0x1D4
    eng->m_currentAngle = rec.field1D8;                             // ELF +0x1D8
}

// Appends a 16-byte snapshot, advancing the two-byte-unit cursor by 16 (leaving a 16-byte gap).
// Refuses overflow without writing. [UNRESOLVED] remaining BaseLineData layout.
bool EngineNetworkable::getBaseLine(io::BaseLineData& baseline)
{
    unsigned char* const base = reinterpret_cast<unsigned char*>(&baseline);
    unsigned short& cursor = *reinterpret_cast<unsigned short*>(base + 0x400);

    if (cursor + 0x10u > 0x200u)
        return false;

    float* const out = reinterpret_cast<float*>(base + cursor * 2);
    m_next = m_head;                                                             // the re-sync
    const Record& rec = m_records[m_next];
    out[0] = rec.angularRate;
    out[1] = rec.forwardSpeed;
    out[2] = rec.field1C4;
    out[3] = rec.field1D4;

    cursor = static_cast<unsigned short>(cursor + 0x10);
    return true;
}

// empty.
void EngineNetworkable::dumpState(int)
{
}

// 0x20 - thirty-TWO bits of descriptor headroom.
unsigned int EngineNetworkable::getMinimumBitSize() const
{
    return 32;
}

// 0x327 = bits 0, 1, 2, 5, 8, 9 - the subset of channels a ghost update carries.
unsigned int EngineNetworkable::getGhostStateMask() const
{
    return 0x327u;
}

} // namespace hfe
} // namespace dice
