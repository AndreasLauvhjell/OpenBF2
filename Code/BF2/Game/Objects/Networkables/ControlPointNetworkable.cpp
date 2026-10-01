// Control-point replication and baseline state.

#include "ControlPointNetworkable.h"

#include <cmath>

#include "../Game/ControlPoint.h"
#include "../IObject.h"
#include "../../../IO/BitStream.h"
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"

namespace dice {
namespace hfe {

// The TU's static: built by the static initialiser (0x82321B0) as
// RegistredNetworkStats("ControlPointNetworkable", Vec3(255, 200, 240)) - the three floats are
// / 0x43480000 / 0x43700000, a graph colour.
namespace {
Vec3 statsColour()
{
    Vec3 c; c.x = 255.0f; c.y = 200.0f; c.z = 240.0f;
    return c;
}
} // namespace
io::RegistredNetworkStats ControlPointNetworkable::m_stats("ControlPointNetworkable", statsColour());

// create - 0x877A060, 145 B: outer -> 0; new(0x5C); NetworkableBase::NetworkableBase; the
// NetworkableObjectBase stores (+0x40 -1, +0x44 0, +0x48 0, +0x4C -1); the two vptrs; +0x50 = 1;
// +0x54 = 0; then queryInterface(iid) and release() (vptr +0x08) if it answers null.
IBase* ControlPointNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    ControlPointNetworkable* self = new ControlPointNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

ControlPointNetworkable::ControlPointNetworkable()
{
    m_refCount     = 1;                              // +0x50
    m_baselines    = 0;                              // +0x54
    m_controlPoint = 0;                              // +0x58 [CHOSEN], see the header
}

// / 0x8231110: delete[] the ring (and null it), then NetworkableBase's chain.
ControlPointNetworkable::~ControlPointNetworkable()
{
    if (m_baselines)
    {
        delete[] m_baselines;
        m_baselines = 0;
    }
}

// The quartet - refcount +0x50. release is the "count WAS 1, then delete" shape (the decrement
// happens first; the delete keys on the pre-decrement value), returning the new count.
void ControlPointNetworkable::addRef() { ++m_refCount; }
int  ControlPointNetworkable::getRef() { return m_refCount; }
int  ControlPointNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;                                 // vptr +0x14 - the deleting destructor
    return r;
}

// IBase / INetworkable -> the primary; INetworkableObject -> the +0x3C base; else 0.
void* ControlPointNetworkable::queryInterface(unsigned int iid) const
{
    ControlPointNetworkable* self = const_cast<ControlPointNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// Binds a control point and creates four baseline records, seeding record zero from its current state.
// Returns false if baseline allocation fails.
bool ControlPointNetworkable::init(void* object)
{
    world::IObject* iobject = static_cast<world::IObject*>(object);
    m_object = iobject;                                                          // +0x44
    io::INetworkableObject* self =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // vptr +0x0C
    m_controlPoint = static_cast<world::ControlPoint*>(iobject);                 // +0x58
    m_networkableObject = self;                                                  // +0x18

    BaseLine* ring = new BaseLine[4];                                            // 0x90 bytes
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time   = 0.0f;
        ring[i].byte04 = false;
    }
    m_baselines = ring;                                                          // +0x54
    if (!ring)
        return false;

    world::ControlPoint* cp = m_controlPoint;
    ring[0].team                    = cp->getTeam();                             // vcall +0x1A0
    ring[0].bool158                 = cp->m_isNeutral;                              // +0x158
    ring[0].takeOverCounter         = cp->m_takeOverCounter;                     // +0x154
    ring[0].takeOverChangePerSecond = cp->m_takeOverChangePerSecond;             // +0x14C
    ring[0].allowCaptureByTeam1     = cp->m_allowCaptureByTeam[0];               // +0x170
    ring[0].allowCaptureByTeam2     = cp->m_allowCaptureByTeam[1];               // +0x171
    ring[0].flagTeam                = cp->getFlag();                             // vcall +0x1C0
    ring[0].capturingState          = cp->getCapturingState();                   // vcall +0x1C8
    return true;
}

// Advances head/next directly without changing tail/count, then marks changed fields.
// NaNs count as changed. The rate accompanies counter bit 2 but is never compared independently.
void ControlPointNetworkable::updateStateMask(unsigned int* stateMask)
{
    world::ControlPoint* cp = m_controlPoint;
    BaseLine* ring = m_baselines;

    m_prev = m_head;                                                             // +0x34
    m_head = (m_head + 1) & 3;                                                   // +0x28
    m_next = m_head;                                                             // +0x38
    BaseLine& next = ring[m_next];
    BaseLine& prev = ring[m_prev];

    next.team = cp->getTeam();                                                   // vcall +0x1A0
    if (next.team != prev.team)                          *stateMask |= 0x01;
    next.bool158 = cp->m_isNeutral;
    if (prev.bool158 != next.bool158)                    *stateMask |= 0x02;
    next.takeOverCounter = cp->m_takeOverCounter;
    if (next.takeOverCounter != prev.takeOverCounter)    *stateMask |= 0x04;
    next.allowCaptureByTeam1 = cp->m_allowCaptureByTeam[0];
    if (prev.allowCaptureByTeam1 != next.allowCaptureByTeam1) *stateMask |= 0x08;
    next.allowCaptureByTeam2 = cp->m_allowCaptureByTeam[1];
    if (prev.allowCaptureByTeam2 != next.allowCaptureByTeam2) *stateMask |= 0x10;
    next.flagTeam = cp->m_currentFlagTeam;                                       // +0x168 raw
    if (next.flagTeam != prev.flagTeam)                  *stateMask |= 0x20;
    next.capturingState = cp->m_capturingState;                                  // +0x16C raw
    if (next.capturingState != prev.capturingState)      *stateMask |= 0x40;

    next.takeOverCounter         = cp->m_takeOverCounter;                        // re-snapped
    next.takeOverChangePerSecond = cp->m_takeOverChangePerSecond;                // never compared
    if (next.takeOverCounter != prev.takeOverCounter)    *stateMask |= 0x04;     // re-tested
}

// Control-point encoding uses a 7-bit mask and absolute values.
// Team/state fields use sign plus 3 magnitude bits; takeover values use 16-bit quantization over -1.25..1.25.
namespace {
// The three write shapes, each spelled once. Every writeBits source is a dword; the ELF's are too.
void writeSigned3(io::BitStream& stream, int value)
{
    unsigned int sign; unsigned int magnitude;
    if (value < 0) { sign = 1; magnitude = static_cast<unsigned int>(-value); }
    else           { sign = 0; magnitude = static_cast<unsigned int>(value); }
    stream.writeBits(&sign, 1);
    stream.writeBits(&magnitude, 3);
}
void writeFlag3(io::BitStream& stream, bool value)   // bits 3 / 4: the sign slot is a constant 0
{
    unsigned int zero = 0; unsigned int v = value ? 1u : 0u;
    stream.writeBits(&zero, 1);
    stream.writeBits(&v, 3);
}
void writeUnit16(io::BitStream& stream, float value)   // bit 2's two floats
{
    double t = (static_cast<double>(value) / 1.25 + 1.0) * 0.5;
    if (t < 0.0)      t = 0.0;
    else if (t > 1.0) t = 1.0;
    unsigned int q = static_cast<unsigned int>(static_cast<int>(t * 65535.0));   // truncation
    stream.writeBits(&q, 16);
}
// The mirror: q / 65535.0 (double) rounded to a float, doubled minus one in x87, a dead zone
// (|f| <= flt_88B28D4 = 0x38000080 = 3.0518044e-05 -> 0), times 1.25. The ELF's two decodes are
// not quite twins: the first tests |f| on the extended value, the second (SSE andnps) on the
// float-rounded copy. Both spelled as the first here - a value that lands within one float ulp of
// 2^-15 is the only case that could tell them apart, and it decodes to ~0 either way.
float readUnit16(io::BitStream& stream)
{
    unsigned int q = 0;
    stream.readBits(&q, 16);
    const float t = static_cast<float>(q / 65535.0);
    double f = static_cast<double>(t) + static_cast<double>(t) - 1.0;   // `fadd st, st` = t + t
    if (!(std::fabs(f) > 3.0518044e-05))
        f = 0.0;
    return static_cast<float>(f * 1.25);
}
int readSigned3(io::BitStream& stream)
{
    unsigned int sign = 0; unsigned int magnitude = 0;
    stream.readBits(&sign, 1);
    stream.readBits(&magnitude, 3);
    return sign == 1 ? -static_cast<int>(magnitude) : static_cast<int>(magnitude);
}
} // namespace

bool ControlPointNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                           unsigned int stateMask, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    unsigned int mask = stateMask;
    stream.writeBits(&mask, 7);
    const BaseLine& rec = m_baselines[m_next];
    if (mask & 0x01) writeSigned3(stream, rec.team);
    if (mask & 0x02) { unsigned int b = rec.bool158 ? 1u : 0u; stream.writeBits(&b, 1); }
    if (mask & 0x04) { writeUnit16(stream, rec.takeOverCounter); writeUnit16(stream, rec.takeOverChangePerSecond); }
    if (mask & 0x08) writeFlag3(stream, rec.allowCaptureByTeam1);
    if (mask & 0x10) writeFlag3(stream, rec.allowCaptureByTeam2);
    if (mask & 0x20) writeSigned3(stream, rec.flagTeam);
    if (mask & 0x40) writeSigned3(stream, rec.capturingState);

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// Decodes the control-point update, carries unsent fields forward and applies received fields.
// Updates flag position after a nonzero mask. updateTime and baseline are unused.
void ControlPointNetworkable::setNetUpdate(io::BitStream& stream, float /*updateTime*/, bool ghostActive,
                                           io::BaseLineData* /*baseline*/, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();                               // +0x30
    unsigned int mask = 0;
    makeRoomForNextUpdate(ghostActive);
    if (m_prev != m_next)
        m_baselines[m_next] = m_baselines[m_prev];                               // the nine-field copy
    stream.readBits(&mask, 7);
    BaseLine& rec = m_baselines[m_next];
    if (mask & 0x01) rec.team = readSigned3(stream);
    if (mask & 0x02) { unsigned int b = 0; stream.readBits(&b, 1); rec.bool158 = (b == 1); }
    if (mask & 0x04) { rec.takeOverCounter = readUnit16(stream); rec.takeOverChangePerSecond = readUnit16(stream); }
    if (mask & 0x08) rec.allowCaptureByTeam1 = (readSigned3(stream) != 0);
    if (mask & 0x10) rec.allowCaptureByTeam2 = (readSigned3(stream) != 0);
    if (mask & 0x20) rec.flagTeam = readSigned3(stream);
    if (mask & 0x40) rec.capturingState = readSigned3(stream);

    if (stream.hasError())
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/ControlPointNetworkable.cpp", 190, __FUNCTION__)
            << "read error";
        stream.clearError();
    }
    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    if (mask)
    {
        world::ControlPoint* cp = m_controlPoint;
        if (mask & 0x01) cp->setTeam(static_cast<unsigned int>(rec.team));       // vcall +0x19C
        if (mask & 0x02) cp->m_isNeutral = rec.bool158;                             // +0x158 raw
        if (mask & 0x04)
        {
            cp->m_takeOverCounter = rec.takeOverCounter;                         // +0x154 raw
            cp->setTakeOverChangePerSecond(rec.takeOverChangePerSecond);         // vcall +0x1B0
        }
        if (mask & 0x08) cp->setAllowCaptureByTeam(0, rec.allowCaptureByTeam1);  // vcall +0x194 - team 0 [sic]
        if (mask & 0x10) cp->setAllowCaptureByTeam(1, rec.allowCaptureByTeam2);
        if (mask & 0x20) cp->setFlag(rec.flagTeam, false);                       // vcall +0x1BC
        if (mask & 0x40) cp->setCapturingState(rec.capturingState);              // vcall +0x1C4
        cp->updateFlagPosition();                                                // direct call
    }
}

// / 59 B / 5 B.
unsigned int ControlPointNetworkable::getMinimumBitSize() const { return 6; }

float ControlPointNetworkable::getAverageUpdateDeltaTime() const
{
    if (m_count == 0)
        return 0.0f;
    return (m_baselines[m_head].time - m_baselines[m_tail].time) / static_cast<float>(m_count);
}

void ControlPointNetworkable::predict(float) {}

} // namespace hfe
} // namespace dice
