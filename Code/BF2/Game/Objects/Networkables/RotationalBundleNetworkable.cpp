// Replicates bundle rotation, angular speed and excess input with client interpolation.

#include "Networkables.h"

#include <cmath>

#include "../IObject.h"
#include "../RotationalBundleTemplate.h"      // the input maps, automaticReset, rememberExcessInput
#include "../Game/RotationalBundle.h"         // setState - the receiver's final act
#include "../../../IO/BitStream.h"
#include "../../../IO/Network/NetworkManager.h"   // StatePrecisionFull + the interp/extrap times
#include "../../../IO/Network/NetworkableInfo.h"  // getPredictionMode - predict's mode fork
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"         // formatString - the Debug payload, as shipped

namespace dice {
namespace hfe {

// The TU's static - RegistredNetworkStats("RotationalBundleNetworkable", Vec3(255, 255, 255)) at
// ds:0x8AAE8C0; all three colour components are 0x437F0000 = 255.0f. No con-var fetch in this
// TU's initialiser - unlike Spring's and Engine's, there is no "network-rot-*" handle.
namespace {
Vec3 rotStatsColour()
{
    Vec3 c; c.x = 255.0f; c.y = 255.0f; c.z = 255.0f;
    return c;
}
} // namespace
io::RegistredNetworkStats RotationalBundleNetworkable::m_stats("RotationalBundleNetworkable",
                                                               rotStatsColour());

namespace {

// Folds rotations outside [-180, 180] modulo 180, using truncation toward zero.
inline float foldRotation(float v)
{
    if (v > 180.0f)
        return v - 180.0f * static_cast<float>(static_cast<int>(v / 180.0f));
    if (v < -180.0f)
        return v + 180.0f * static_cast<float>(static_cast<int>(-v / 180.0f));
    return v;
}

} // namespace

// Marks exact changes in three rotation axes, three speed axes and excess input.
// Changed angles are folded in the baseline, so an out-of-range source can remain dirty across frames.
void RotationalBundleNetworkable::updateStateMask(unsigned int* stateMask)
{
    if (m_object->getFlags() & 1)
        return;                                      // before the ring moves

    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;

    Record& next = m_records[m_next];
    Record& prev = m_records[m_prev];

    const world::RotationalBundle* const bundle =
        static_cast<const world::RotationalBundle*>(m_object);
    const float* const rotationSrc = &bundle->m_currentRotation.x;
    const float* const speedSrc    = &bundle->m_currentRotationalSpeed.x;

    next.rotation[0] = rotationSrc[0];
    next.rotation[1] = rotationSrc[1];
    next.rotation[2] = rotationSrc[2];

    for (int axis = 0; axis < 3; ++axis)
    {
        if (next.rotation[axis] != prev.rotation[axis])
        {
            *stateMask |= (1u << axis);
            next.rotation[axis] = foldRotation(next.rotation[axis]);   // after the test
        }
    }

    next.speed[0] = speedSrc[0];
    next.speed[1] = speedSrc[1];
    next.speed[2] = speedSrc[2];

    for (int axis = 0; axis < 3; ++axis)
    {
        if (next.speed[axis] != prev.speed[axis])
            *stateMask |= (1u << (axis + 3));
    }

    next.excessInput = bundle->m_excessInput.z;
    if (next.excessInput != prev.excessInput)
        *stateMask |= 0x40;
}

// The shared bits of the wire pair. The channel codec is the generic width-parameterised quantiser
// the ELF inlines seven times per direction, including its "width == 32 -> raw float" arm - dead at
// the widths this class ever picks (12 or 16), but compiled in at every site and kept here so the
// helper is the codec that was shipped, not a trimmed one.
namespace {

inline const world::RotationalBundleTemplate* bundleTemplateOf(const world::IObject* object)
{
    return static_cast<const world::RotationalBundleTemplate*>(object->getTemplate());
}

void writeUnitChannel(io::BitStream& stream, float v, unsigned int width)
{
    if (width == 32)                                     // the inlined codec's raw arm - dead here
    {
        stream.writeBits(&v, 32);
        return;
    }
    double t = (static_cast<double>(v) + 1.0) * 0.5;
    if (t < 0.0)      t = 0.0;
    else if (t > 1.0) t = 1.0;
    unsigned int q = static_cast<unsigned int>(t * ((1u << width) - 1u));   // truncation
    stream.writeBits(&q, width);
}

float readUnitChannel(io::BitStream& stream, unsigned int width)
{
    if (width == 32)                                     // the raw arm - dead here
    {
        float f = 0.0f;
        stream.readBits(&f, 32);
        return f;
    }
    const unsigned int max = (1u << width) - 1u;
    unsigned int q = 0;
    stream.readBits(&q, width);
    const float t = static_cast<float>(q) / static_cast<float>(max);
    double f = static_cast<double>(t) + static_cast<double>(t) - 1.0;
    // the ONE-quantum dead zone, dynamic-width form: 2/max, exactly what the fixed-width
    // decoders spell as 3.0518044e-05 at 16 bits.
    if (!(2.0 / static_cast<double>(max) < std::fabs(f)))
        f = 0.0;
    return static_cast<float>(f);
}

} // namespace

// create - 0x87799C0, 145 B: outer -> 0; new(0x5C); the usual factory shape; +0x50 = 1, +0x58 = 0.
// +0x54 (m_localStateMask) is left unwritten by create - init() is its real writer. Zeroed in
// the ctor here [CHOSEN], the house pattern.
IBase* RotationalBundleNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    RotationalBundleNetworkable* self = new RotationalBundleNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

RotationalBundleNetworkable::RotationalBundleNetworkable()
{
    m_refCount       = 1;                            // +0x50
    m_localStateMask = 0;                            // +0x54 [CHOSEN] - see create's note
    m_records        = 0;                            // +0x58
}

// D2/D1 0x823EE60/0x823EEB0, 65 B each: revector, delete[] the ring and NULL it, chain.
RotationalBundleNetworkable::~RotationalBundleNetworkable()
{
    if (m_records)
    {
        delete[] m_records;
        m_records = 0;
    }
}

void  RotationalBundleNetworkable::addRef() { ++m_refCount; }
int   RotationalBundleNetworkable::getRef() { return m_refCount; }
int   RotationalBundleNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;
    return r;
}

void* RotationalBundleNetworkable::queryInterface(unsigned int iid) const
{
    RotationalBundleNetworkable* self = const_cast<RotationalBundleNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// Binds the bundle and configures replicated axes from template input mappings.
// Automatic-reset axes send rotation only; other mapped axes also send speed. Returns true after reset.
bool RotationalBundleNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);                             // +0x44
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // +0x18

    Record* ring = new Record[4];                                                // 0xC0 bytes
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time   = 0.0f;
        ring[i].byte04 = false;
        ring[i].speed[0] = 0.0f; ring[i].speed[1] = 0.0f; ring[i].speed[2] = 0.0f;
        ring[i].rotation[0] = 0.0f; ring[i].rotation[1] = 0.0f; ring[i].rotation[2] = 0.0f;
        ring[i].tail24[0] = 0.0f; ring[i].tail24[1] = 0.0f; ring[i].tail24[2] = 0.0f;
        // excessInput (+0x20) deliberately NOT zeroed - the ELF's loop skips exactly it.
    }
    m_records = ring;                                                            // +0x58
    if (!ring)
        return false;

    m_localStateMask = 0;                                                        // +0x54
    const world::RotationalBundleTemplate* templ = bundleTemplateOf(m_object);
    if (templ->getInputToYaw() != 64)
    {
        const unsigned int old = m_localStateMask;
        m_localStateMask = old | 0x01;
        if (!templ->m_automaticReset)
            m_localStateMask = old | 0x09;
    }
    if (templ->getInputToPitch() != 64)
    {
        const unsigned int old = m_localStateMask;
        m_localStateMask = old | 0x02;
        if (!templ->m_automaticReset)
            m_localStateMask = old | 0x12;
    }
    if (templ->getInputToRoll() != 64)
    {
        const unsigned int old = m_localStateMask;
        m_localStateMask = old | 0x04;
        if (!templ->m_automaticReset)
            m_localStateMask = old | 0x24;
    }
    if (templ->getRememberExcessInput())
        m_localStateMask |= 0x40u;

    reset();                                         // vptr +0x1C
    return true;
}

// Resets the base and seeds only record zero from the live bundle.
void RotationalBundleNetworkable::reset()
{
    io::NetworkableBase::reset();

    const world::RotationalBundle* const bundle =
        static_cast<const world::RotationalBundle*>(m_object);
    Record& rec0 = m_records[0];
    const float* const rotationSrc = &bundle->m_currentRotation.x;
    const float* const speedSrc    = &bundle->m_currentRotationalSpeed.x;

    rec0.rotation[0] = rotationSrc[0];
    rec0.rotation[1] = rotationSrc[1];
    rec0.rotation[2] = rotationSrc[2];
    rec0.speed[0] = speedSrc[0];
    rec0.speed[1] = speedSrc[1];
    rec0.speed[2] = speedSrc[2];
    rec0.excessInput = bundle->m_excessInput.z;
}

// Writes the seven-bit mask after filtering unmapped axes. Rotation/speed use 12 bits,
// or 16 at full precision; excess input always uses 16 bits.
bool RotationalBundleNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                               unsigned int stateMask, int precision)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    m_next = m_head;                                                             // the re-sync

    unsigned int mask = stateMask & m_localStateMask;                            // the local AND
    stream.writeBits(&mask, 7);

    const world::RotationalBundleTemplate* templ = bundleTemplateOf(m_object);
    const unsigned int width = (io::StatePrecisionFull == precision) ? 16u : 12u;
    const Record& rec = m_records[m_next];

    if (mask & 0x01)
        writeUnitChannel(stream, rec.rotation[0] / 180.0f, width);
    if (mask & 0x02)
        writeUnitChannel(stream, rec.rotation[1] / 180.0f, width);
    if (mask & 0x04)
        writeUnitChannel(stream, rec.rotation[2] / 180.0f, width);

    if (mask & 0x08)
        writeUnitChannel(stream, rec.speed[0] / (std::fabs(templ->m_maxSpeed.x) * 16.0f), width);
    if (mask & 0x10)
        writeUnitChannel(stream, rec.speed[1] / (std::fabs(templ->m_maxSpeed.y) * 16.0f), width);
    if (mask & 0x20)
        writeUnitChannel(stream, rec.speed[2] / (std::fabs(templ->m_maxSpeed.z) * 16.0f), width);

    if (mask & 0x40)
    {
        double t = (static_cast<double>(rec.excessInput) / 41.0 + 1.0) * 0.5;
        if (t < 0.0)      t = 0.0;
        else if (t > 1.0) t = 1.0;
        unsigned int q = static_cast<unsigned int>(t * 65535.0);
        stream.writeBits(&q, 16);
    }

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// Decodes rotational state and snaps all seven stored values within 0.001 of zero to zero.
// Applies state only when the ring accepts the update; read failure reports a fatal error.
void RotationalBundleNetworkable::setNetUpdate(io::BitStream& stream, float updateTime,
                                               bool ghostActive, io::BaseLineData* /*baseline*/,
                                               int precision)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    const world::RotationalBundleTemplate* templ = bundleTemplateOf(m_object);

    const bool roomForNextUpdate = makeRoomForNextUpdate(ghostActive);
    if (m_prev != m_next)
        m_records[m_next] = m_records[m_prev];                                   // twelve dwords
    m_records[m_next].time = updateTime;

    unsigned int mask = 0;
    stream.readBits(&mask, 7);

    const unsigned int width = (io::StatePrecisionFull == precision) ? 16u : 12u;
    Record& rec = m_records[m_next];

    if (mask & 0x01)
        rec.rotation[0] = readUnitChannel(stream, width) * 180.0f;
    if (mask & 0x02)
        rec.rotation[1] = readUnitChannel(stream, width) * 180.0f;
    if (mask & 0x04)
        rec.rotation[2] = readUnitChannel(stream, width) * 180.0f;

    if (mask & 0x08)
        rec.speed[0] = readUnitChannel(stream, width) * (std::fabs(templ->m_maxSpeed.x) * 16.0f);
    if (mask & 0x10)
        rec.speed[1] = readUnitChannel(stream, width) * (std::fabs(templ->m_maxSpeed.y) * 16.0f);
    if (mask & 0x20)
        rec.speed[2] = readUnitChannel(stream, width) * (std::fabs(templ->m_maxSpeed.z) * 16.0f);

    if (mask & 0x40)
    {
        unsigned int q = 0;
        stream.readBits(&q, 16);
        const float t = static_cast<float>(q / 65535.0);
        double f = static_cast<double>(t) + static_cast<double>(t) - 1.0;
        if (std::fabs(f) <= 3.0518044e-05)               // the fixed 16-bit dead zone
            f = 0.0;
        rec.excessInput = static_cast<float>(f * 41.0);
    }

    // the unconditional 0.001 snap pass
    for (int axis = 0; axis < 3; ++axis)
    {
        if (std::fabs(rec.rotation[axis]) <= 0.001)
            rec.rotation[axis] = 0.0f;
    }
    for (int axis = 0; axis < 3; ++axis)
    {
        if (std::fabs(rec.speed[axis]) <= 0.001)
            rec.speed[axis] = 0.0f;
    }
    if (std::fabs(rec.excessInput) <= 0.001)
        rec.excessInput = 0.0f;

    if (stream.hasError())
    {
        // attested AND fatal - type 6 aborts; the clear is the ELF's dead tail.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/RotationalBundleNetworkable.cpp", 313,
              __FUNCTION__)
            << formatString("error reading data");
        stream.clearError();
        return;
    }
    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    if (roomForNextUpdate)
    {
        world::RotationalBundle* const bundle =
            static_cast<world::RotationalBundle*>(m_object);
        float* const speedDst    = &bundle->m_currentRotationalSpeed.x;
        float* const rotationDst = &bundle->m_currentRotation.x;
        float* const tailDst     = &bundle->m_forwardAxis.x;

        speedDst[0] = rec.speed[0];  speedDst[1] = rec.speed[1];  speedDst[2] = rec.speed[2];
        rotationDst[0] = rec.rotation[0]; rotationDst[1] = rec.rotation[1]; rotationDst[2] = rec.rotation[2];
        tailDst[0] = rec.tail24[0];  tailDst[1] = rec.tail24[1];  tailDst[2] = rec.tail24[2];
        if (mask & 0x40)
            bundle->m_excessInput.z = rec.excessInput;

        // pointer identity is the ELF's own: setState is called on the object pointer itself.
        bundle->setState();
    }
}

// findEmbracingIndices - 0x82411B0, 151 B. predict's ring search: walk tail -> head and return the
// two record indices whose times bracket t. False when fewer than two records exist, when t is
// older than the oldest record, or when t is newer than the newest (the walk runs out).
// *hi is stored before the time test each step and *lo after it fails, so on success *lo/*hi
// really are the bracketing pair; on the run-out failure both hold the last records touched.
bool RotationalBundleNetworkable::findEmbracingIndices(float t, unsigned int* lo,
                                                       unsigned int* hi) const
{
    if (m_count <= 1)                                    // +0x24
        return false;

    unsigned int idx = static_cast<unsigned int>(m_tail);   // +0x2C
    if (m_records[idx].time > t)
        return false;                                    // t predates the oldest record

    *lo = idx;
    *hi = idx;
    for (unsigned int i = 1; i < static_cast<unsigned int>(m_count); ++i)
    {
        idx = (static_cast<unsigned int>(m_tail) + i) & 3;
        *hi = idx;                                       // stored before the test
        if (m_records[idx].time >= t)
            return true;
        *lo = idx;                                       // still below t - keep walking
    }
    return false;                                        // t is beyond the newest record
}

// Predicts rotation at now by snapping, interpolating or extrapolating according to the configured mode.
// Interpolation handles the 360-degree seam; invalid brackets and expired extrapolation fall back to a snapshot.
void RotationalBundleNetworkable::predict(float now)
{
    m_next = m_head;                                     // before every gate

    if (m_forceFullUpdate)                               // +0x1C
        return;

    const int mode = static_cast<int>(m_networkableInfo->getPredictionMode());   // vptr +0x24
    world::RotationalBundle* const bundle =
        static_cast<world::RotationalBundle*>(m_object);
    float* const rotationDst = &bundle->m_currentRotation.x;
    float* const speedDst    = &bundle->m_currentRotationalSpeed.x;
    const Record* const ring = m_records;

    if (mode != 1)
    {
        if (mode)
            return;                                      // neither PMNone nor PMLinear - nothing

        // PMNone: snap and go
        const Record& rec = ring[m_next];
        speedDst[0] = rec.speed[0];  speedDst[1] = rec.speed[1];  speedDst[2] = rec.speed[2];
        rotationDst[0] = rec.rotation[0]; rotationDst[1] = rec.rotation[1]; rotationDst[2] = rec.rotation[2];
        bundle->setState();
        return;
    }

    // PMLinear
    const unsigned int count = static_cast<unsigned int>(m_count);
    const float predictTime =
        now - static_cast<float>(io::networkManager->getInterpolationTime());    // fild [+0x50]

    float lastUpdateSpan = 0.0f;
    if (count > 1)
        lastUpdateSpan = ring[m_next].time
                       - ring[(static_cast<unsigned int>(m_tail) + count - 1) & 3].time;

    const float newestTime = ring[m_next].time;
    if (predictTime <= newestTime)
    {
        // interpolate
        unsigned int lo = 0, hi = 0;
        if (findEmbracingIndices(predictTime, &lo, &hi))
        {
            const float t0   = ring[lo].time;
            const float span = ring[hi].time - t0;
            if (span > 0.0 && span < 500.0)
            {
                const float frac = (1.0f >= span) ? 1.0f : (predictTime - t0) / span;

                // speed, with the 360° seam fix - endpoints adjusted, deltas recomputed after.
                float s0[3] = { ring[lo].speed[0], ring[lo].speed[1], ring[lo].speed[2] };
                float s1[3] = { ring[hi].speed[0], ring[hi].speed[1], ring[hi].speed[2] };
                for (int axis = 0; axis < 3; ++axis)
                {
                    if (std::fabs(s1[axis] - s0[axis]) > 180.0)
                    {
                        if (s0[axis] < 0.0f) s0[axis] = s0[axis] + 360.0f;
                        if (s1[axis] < 0.0f) s1[axis] = s1[axis] + 360.0f;
                    }
                }
                speedDst[0] = s0[0] + (s1[0] - s0[0]) * frac;
                speedDst[1] = s0[1] + (s1[1] - s0[1]) * frac;
                speedDst[2] = s0[2] + (s1[2] - s0[2]) * frac;

                // rotation, the same treatment.
                float r0[3] = { ring[lo].rotation[0], ring[lo].rotation[1], ring[lo].rotation[2] };
                float r1[3] = { ring[hi].rotation[0], ring[hi].rotation[1], ring[hi].rotation[2] };
                for (int axis = 0; axis < 3; ++axis)
                {
                    if (std::fabs(r1[axis] - r0[axis]) > 180.0)
                    {
                        if (r0[axis] < 0.0f) r0[axis] = r0[axis] + 360.0f;
                        if (r1[axis] < 0.0f) r1[axis] = r1[axis] + 360.0f;
                    }
                }
                rotationDst[0] = r0[0] + (r1[0] - r0[0]) * frac;
                rotationDst[1] = r0[1] + (r1[1] - r0[1]) * frac;
                rotationDst[2] = r0[2] + (r1[2] - r0[2]) * frac;

                bundle->setState();
                return;
            }
        }
        // no bracket, or a degenerate span - fall through to the snap tail.
    }
    else
    {
        // extrapolate
        const float over      = predictTime - newestTime;
        const float maxExtrap =
            static_cast<float>(io::networkManager->getExtrapolationTime());      // fild [+0x54]
        if (maxExtrap >= over)
        {
            float span = lastUpdateSpan;
            if (span == 0.0f)
                span = maxExtrap;
            const float frac = over / span;
            const Record& rec = ring[m_next];

            if (count <= 1)
            {
                // one record: rotation slides along the received speed; speed itself untouched.
                rotationDst[0] = rec.speed[0] * frac + rec.rotation[0];
                rotationDst[1] = rec.speed[1] * frac + rec.rotation[1];
                rotationDst[2] = rec.speed[2] * frac + rec.rotation[2];
                bundle->setState();
                return;
            }

            const Record& prev = ring[(static_cast<unsigned int>(m_tail) + count - 1) & 3];
            speedDst[0] = (rec.speed[0] - prev.speed[0]) * frac + rec.speed[0];
            speedDst[1] = (rec.speed[1] - prev.speed[1]) * frac + rec.speed[1];
            speedDst[2] = (rec.speed[2] - prev.speed[2]) * frac + rec.speed[2];

            rotationDst[0] = (rec.rotation[0] - prev.rotation[0]) * frac + rec.rotation[0];
            rotationDst[1] = (rec.rotation[1] - prev.rotation[1]) * frac + rec.rotation[1];
            rotationDst[2] = (rec.rotation[2] - prev.rotation[2]) * frac + rec.rotation[2];

            bundle->setState();
            return;
        }
        // too far past the newest record - fall through to the snap tail.
    }

    // the snap tail
    {
        const Record& rec = ring[m_next];
        speedDst[0] = rec.speed[0];  speedDst[1] = rec.speed[1];  speedDst[2] = rec.speed[2];
        rotationDst[0] = rec.rotation[0]; rotationDst[1] = rec.rotation[1]; rotationDst[2] = rec.rotation[2];
        bundle->setState();
    }
}

// getBaseLine - 0x823EF60, 170 B. Append a twenty-four-byte snapshot - speed[3] then rotation[3]
// from record[m_head] - into the BaseLineData buffer, re-syncing m_next to m_head on the way. The
// same two-byte-unit cursor at +0x400 as EngineNetworkable's, bound cursor + 0x18 <= 0x200, and the
// same doubled advance: 24 bytes written, 48 bytes stepped. Reproduced without tidying.
bool RotationalBundleNetworkable::getBaseLine(io::BaseLineData& baseline)
{
    unsigned char* const base = reinterpret_cast<unsigned char*>(&baseline);
    unsigned short& cursor = *reinterpret_cast<unsigned short*>(base + 0x400);

    if (cursor + 0x18u > 0x200u)
        return false;

    float* const out = reinterpret_cast<float*>(base + cursor * 2);
    m_next = m_head;                                                             // the re-sync
    const Record& rec = m_records[m_next];
    out[0] = rec.speed[0];
    out[1] = rec.speed[1];
    out[2] = rec.speed[2];
    out[3] = rec.rotation[0];
    out[4] = rec.rotation[1];
    out[5] = rec.rotation[2];

    cursor = static_cast<unsigned short>(cursor + 0x18);
    return true;
}

// 0x18 - twenty-four bits of descriptor headroom.
unsigned int RotationalBundleNetworkable::getMinimumBitSize() const
{
    return 24;
}

// 0x3F - bits 0..5, both triplets; the excess input is NOT in the ghost mask.
unsigned int RotationalBundleNetworkable::getGhostStateMask() const
{
    return 0x3Fu;
}

} // namespace hfe
} // namespace dice
