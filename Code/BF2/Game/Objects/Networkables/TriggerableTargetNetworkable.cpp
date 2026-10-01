// Replicates triggerable transform, motion and remaining cycles.
// Baseline validity forces the first update; unchanged fields retain their previous stored bits.

#include "Networkables.h"

#include <cmath>

#include "../IObject.h"
#include "../Game/Triggerable.h"
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"
#include "../../../IO/BitStream.h"
#include "../../../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace {
Vec3 triggerableStatsColour() { const Vec3 colour={255,0,0}; return colour; }
void writeTriggerVector(io::BitStream& stream,const float* values)
{
    const Vec3 vector={values[0],values[1],values[2]};
    if (stream.hasCompressionVector())
        stream.writeCompressedVector(vector,stream.getCompressionVector(),0.001f);
    else
    {
        const unsigned int zero=0;
        stream.writeBits(&zero,2);
        stream.writeBits(&values[0],32);
        stream.writeBits(&values[1],32);
        stream.writeBits(&values[2],32);
    }
}
void writeTriggerInteger(io::BitStream& stream,int value)
{
    const unsigned int encoded=static_cast<unsigned int>(value)+1u;
    if (encoded>65u)
    {
        stream.setError(4);
        Debug(DEBUGTYPE_7,"IO/BitStream.inl",59,"")
            << "(fatal error) Value out of bounds val: " << encoded << " lower " << 0 << " upper: " << 65;
    }
    else stream.writeBits(&encoded,7);
}
void triggerReadError(io::BitStream& stream,int line)
{
    Debug(DEBUGTYPE_6,"Game/Objects/Networkables/TriggerableTargetNetworkable.cpp",line,"")
        << formatString("error reading data");
    stream.clearError();
}
}
io::RegistredNetworkStats TriggerableTargetNetworkable::m_stats(
    "TriggerableTargetNetworkable",triggerableStatsColour());
TriggerableTargetNetworkable::Record::Record()
    : time(0),used(0),position{0,0,0},rotation{0,0,0},field14C(0),field154(0) {}
TriggerableTargetNetworkable::TriggerableTargetNetworkable() : m_refCount(1),m_records(0) {}
TriggerableTargetNetworkable::~TriggerableTargetNetworkable()
{
    if (m_records) delete[] m_records;
}
IBase* TriggerableTargetNetworkable::create(unsigned int iid,IBase* outer)
{
    if (outer) return 0;
    TriggerableTargetNetworkable* self=new TriggerableTargetNetworkable();
    IBase* result=static_cast<IBase*>(self->queryInterface(iid));
    if (!result) self->release();
    return result;
}
void TriggerableTargetNetworkable::addRef() { ++m_refCount; }
int TriggerableTargetNetworkable::getRef() { return m_refCount; }
int TriggerableTargetNetworkable::release()
{
    const int result=--m_refCount;
    if (!result) delete this;
    return result;
}
void* TriggerableTargetNetworkable::queryInterface(unsigned int iid) const
{
    TriggerableTargetNetworkable* self=const_cast<TriggerableTargetNetworkable*>(this);
    if (iid==IID_IBase || iid==io::IID_INetworkable) return static_cast<io::INetworkable*>(self);
    if (iid==io::IID_INetworkableObject) return static_cast<io::INetworkableObject*>(self);
    return 0;
}
bool TriggerableTargetNetworkable::init(void* object)
{
    m_object=static_cast<world::IObject*>(object);
    m_networkableObject=static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));
    m_records=new Record[4];
    return true; // Both retail ports return1 unconditionally after allocation.
}
bool TriggerableTargetNetworkable::getIsUpdateValid(unsigned int slot) { return m_records[slot].used!=0; }
unsigned int TriggerableTargetNetworkable::getGhostStateMask() const { return 15; }
unsigned int TriggerableTargetNetworkable::getMinimumBitSize() const { return 100; }
void TriggerableTargetNetworkable::predict(float) {} // Both shipped bodies are empty.

// Returns null for absent rings or indices above 3; the marking variant also sets used.
TriggerableTargetNetworkable::Record* TriggerableTargetNetworkable::getUpdate(unsigned int slot)
{
    if (m_records && slot <= 3)
        return m_records + slot;
    return 0;
}

TriggerableTargetNetworkable::Record*
TriggerableTargetNetworkable::getUpdateAndMarkUsed(unsigned int slot)
{
    if (!m_records || slot > 3)
        return 0;

    Record* const record = m_records + slot;
    record->used = 1;                                // the flag every comparison below reads
    return record;
}

// Snapshots triggerable position, rotation, motion and remaining cycles.
// Rotation is stored as negated Y-X-Z components; vector changes use epsilon comparisons.
void TriggerableTargetNetworkable::updateStateMask(unsigned int* stateMask)
{
    const world::TriggerableTarget* const target = static_cast<const world::TriggerableTarget*>(m_object);

    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;

    Record* const next = getUpdateAndMarkUsed(static_cast<unsigned int>(m_next));
    Record* const prev = getUpdate(static_cast<unsigned int>(m_prev));

    const float eps = 1.1920929e-7f;

    // the position
    const Mat4& forPosition = m_object->getParent() ? m_object->getRelativeTransformation()
                                                    : m_object->getAbsoluteTransformation();
    const float live[3] = { forPosition.m[12], forPosition.m[13], forPosition.m[14] };

    const float* source = live;
    if (prev->used
        && std::fabs(prev->position[0] - live[0]) <= eps
        && std::fabs(prev->position[1] - live[1]) <= eps
        && std::fabs(prev->position[2] - live[2]) <= eps)
    {
        source = prev->position;                     // unchanged - keep the previous bits exactly
    }
    else
    {
        *stateMask |= 0x01;
    }
    next->position[0] = source[0];
    next->position[1] = source[1];
    next->position[2] = source[2];

    // the rotation
    const Mat4& forRotation = m_object->getParent() ? m_object->getRelativeTransformation()
                                                    : m_object->getAbsoluteTransformation();
    float zDegrees = 0.0f, xDegrees = 0.0f, yDegrees = 0.0f;
    getRotationZXY(forRotation, zDegrees, xDegrees, yDegrees);

    // negated AND reversed - see the note above.
    const float liveRotation[3] = { -yDegrees, -xDegrees, -zDegrees };

    source = liveRotation;
    if (prev->used
        && std::fabs(prev->rotation[0] - liveRotation[0]) <= eps
        && std::fabs(prev->rotation[1] - liveRotation[1]) <= eps
        && std::fabs(prev->rotation[2] - liveRotation[2]) <= eps)
    {
        source = prev->rotation;
    }
    else
    {
        *stateMask |= 0x02;
    }
    next->rotation[0] = source[0];
    next->rotation[1] = source[1];
    next->rotation[2] = source[2];

    // Native field access: ELF +14C/+154 correspond to PE +17C/+184, after its string.
    const int live14C = target->m_state;
    if (prev->used && prev->field14C == live14C)
        next->field14C = prev->field14C;
    else
    {
        *stateMask |= 0x04;
        next->field14C = live14C;
    }

    const int live154 = target->m_cyclesRemaining;
    if (prev->used && prev->field154 == live154)
        next->field154 = prev->field154;
    else
    {
        *stateMask |= 0x08;
        next->field154 = live154;
    }
}

bool TriggerableTargetNetworkable::getNetUpdate(io::BitStream& stream,io::BaseLineData*,
                                               unsigned int stateMask,int)
{
    m_updateStartBitPos=stream.getBitPosition();
    m_next=m_head;
    Record* record=getUpdate(static_cast<unsigned int>(m_head));
    stream.writeBits(&stateMask,4);
    if (stateMask&1) writeTriggerVector(stream,record->position);
    if (stateMask&2) writeTriggerVector(stream,record->rotation);
    if (stateMask&4) writeTriggerInteger(stream,m_records[m_next].field14C);
    if (stateMask&8) writeTriggerInteger(stream,m_records[m_next].field154);
    if (stream.hasError()) { stream.clearError(); return false; }
    m_stats.m_writeBits+=stream.getBitPosition()-m_updateStartBitPos;
    return true;
}
void TriggerableTargetNetworkable::setNetUpdate(io::BitStream& stream,float updateTime,
                                               bool ghostActive,io::BaseLineData*,int)
{
    world::TriggerableTarget* target=static_cast<world::TriggerableTarget*>(m_object);
    unsigned int stateMask=0;
    makeRoomForNextUpdate(ghostActive);
    Record* record=getUpdateAndMarkUsed(static_cast<unsigned int>(m_next));
    // This copy follows marking, so it can overwrite used with the previous slot's flag.
    if (m_prev!=m_next) *record=*getUpdate(static_cast<unsigned int>(m_prev));
    record->time=updateTime;
    m_updateStartBitPos=stream.getBitPosition();
    stream.readBits(&stateMask,4);
    if (stream.hasError()) { triggerReadError(stream,183); return; }
    if (stateMask&1)
    {
        const Vec3 value=stream.readCompressedVector(stream.getCompressionVector(),0.001f);
        record->position[0]=value.x; record->position[1]=value.y; record->position[2]=value.z;
    }
    if (stateMask&2)
    {
        const Vec3 value=stream.readCompressedVector(stream.getCompressionVector(),0.001f);
        record->rotation[0]=value.x; record->rotation[1]=value.y; record->rotation[2]=value.z;
    }
    if (stateMask&4)
    {
        unsigned int value=0; stream.readBits(&value,7);
        m_records[m_next].field14C=static_cast<int>(value)-1;
    }
    if (stateMask&8)
    {
        unsigned int value=0; stream.readBits(&value,7);
        m_records[m_next].field154=static_cast<int>(value)-1;
    }
    // Retail accounts the payload bits before checking its error latch.
    m_stats.m_readBits+=stream.getBitPosition()-m_updateStartBitPos;
    if (stream.hasError()) { triggerReadError(stream,196); return; }
    if (target)
    {
        if (stateMask&1) { const Vec3 value={record->position[0],record->position[1],record->position[2]}; target->setPos(value); }
        if (stateMask&2) { const Vec3 value={record->rotation[0],record->rotation[1],record->rotation[2]}; target->setRot(value); }
    }
    if ((stateMask&4) && target && m_records[m_next].field14C!=-1)
        target->m_state=m_records[m_next].field14C;
    if ((stateMask&8) && target && m_records[m_next].field154!=-1)
        target->m_cyclesRemaining=m_records[m_next].field154;
}

} // namespace hfe
} // namespace dice
