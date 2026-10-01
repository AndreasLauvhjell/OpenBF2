// Replicates target position, activation and player id through a four-entry baseline ring.

#include "Networkables.h"

#include <cmath>

#include "../Game/TargetObject.h"
#include "../IObject.h"
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"
#include "../../../IO/BitStream.h"
#include "../../../Math/Mat4.h"

namespace dice {
namespace hfe {

namespace {
Vec3 targetObjectStatsColour()
{
    Vec3 colour;
    colour.x = 180.0f;
    colour.y = 240.0f;
    colour.z = 240.0f;
    return colour;
}
} // namespace

// ELF TU initialiser 0x824D5C0 and PE 0x76FFD0.
io::RegistredNetworkStats TargetObjectNetworkable::m_stats(
    "TargetObjectNetworkable", targetObjectStatsColour());

// Factory - ELF 0x877A1A0 / PE 0x4DA570. Aggregation is rejected, and an unsupported interface
// releases the fresh instance through the ordinary quartet.
IBase* TargetObjectNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    TargetObjectNetworkable* self = new TargetObjectNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// Inlined into both shipped factories.
TargetObjectNetworkable::TargetObjectNetworkable()
    : m_refCount(1)
    , m_baselines(0)
{
    // m_targetObject is deliberately not initialised until init(), matching both factories.
}

// D2/D1/D0 - ELF 0x824CDD0/0x824CE20/0x824CE70; PE 0x5EB6A0 + scalar deleting wrapper.
TargetObjectNetworkable::~TargetObjectNetworkable()
{
    if (m_baselines)
    {
        delete[] m_baselines;
        m_baselines = 0;
    }
}

void TargetObjectNetworkable::addRef()
{
    ++m_refCount;
}

int TargetObjectNetworkable::getRef()
{
    return m_refCount;
}

int TargetObjectNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int left = --m_refCount;
    if (last && this != 0)
        delete this;
    return left;
}

// ; PE 0x4D9DC0. The secondary answer is compiler-adjusted, not a frozen +0x3C.
void* TargetObjectNetworkable::queryInterface(unsigned int iid) const
{
    TargetObjectNetworkable* self = const_cast<TargetObjectNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// init - ELF 0x824CEC0, 157 B / PE 0x5EB8B0. Each ring header and position is cleared. The two
// payload bytes are left untouched in records 1..3; record 0 primes only targetActive, leaving its
// targetPlayerId untouched too. That partial initialisation is shipped behavior.
bool TargetObjectNetworkable::init(void* object)
{
    world::TargetObject* target = static_cast<world::TargetObject*>(object);
    m_object = target;
    m_networkableObject = static_cast<io::INetworkableObject*>(
        queryInterface(io::IID_INetworkableObject));
    m_targetObject = target;

    BaseLine* ring = new BaseLine[4];
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time = 0.0f;
        ring[i].byte04 = false;
        ring[i].position.x = 0.0f;
        ring[i].position.y = 0.0f;
        ring[i].position.z = 0.0f;
    }
    m_baselines = ring;
    if (!ring)
        return false;

    ring[0].position.x = 0.0f;
    ring[0].position.y = 0.0f;
    ring[0].position.z = 0.0f;
    ring[0].targetActive = 0;
    return true;
}

// Marks position, activation and player-id changes.
// Position uses component epsilon tests; player id is compared and stored as its low byte.
void TargetObjectNetworkable::updateStateMask(unsigned int* stateMask)
{
    BaseLine* const ring = m_baselines;

    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;

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
        *stateMask |= 0x01;

    ring[m_next].targetActive = m_targetObject->m_targetActive ? 1 : 0;
    if (ring[m_prev].targetActive != ring[m_next].targetActive)
        *stateMask |= 0x02;

    ring[m_next].targetPlayerId = static_cast<unsigned char>(m_targetObject->m_targetPlayerId);
    if (ring[m_next].targetPlayerId != ring[m_prev].targetPlayerId)
        *stateMask |= 0x04;
}

// getNetUpdate - ELF 0x824D0E0, 444 B / PE 0x5EB7C0. Five mask bits are always sent even though
// only the low three are used. Position uses the 0.0005 compressed-vector format (selector-zero
// plus three raw floats when no compression vector is installed); active is one bit;
// m_targetPlayerId's
// low byte is eight bits. Baseline and precision are unused, and unlike several siblings this body
// deliberately does not resynchronise m_next to m_head.
bool TargetObjectNetworkable::getNetUpdate(io::BitStream& stream,
                                           io::BaseLineData* /*baseline*/,
                                           unsigned int stateMask,
                                           int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();
    stream.writeBits(&stateMask, 5);

    const BaseLine& record = m_baselines[m_next];
    if (stateMask & 0x01)
    {
        if (stream.hasCompressionVector())
        {
            stream.writeCompressedVector(record.position, stream.getCompressionVector(),
                                         0.00050000002f);
        }
        else
        {
            unsigned int selector = 0;
            stream.writeBits(&selector, 2);
            stream.writeBits(&record.position.x, 32);
            stream.writeBits(&record.position.y, 32);
            stream.writeBits(&record.position.z, 32);
        }
    }

    if (stateMask & 0x02)
    {
        unsigned int active = record.targetActive ? 1u : 0u;
        stream.writeBits(&active, 1);
    }

    if (stateMask & 0x04)
    {
        unsigned int value = record.targetPlayerId;
        stream.writeBits(&value, 8);
    }

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }

    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// setNetUpdate - ELF 0x824D2A0, 792 B / PE 0x5EB950. The receiver advances/copies the ring but
// does not stamp updateTime or copy decoded payload into it: it applies all three fields directly
// to the TargetObject and predict() is shipped empty. The error arm clears and falls through to
// the stats update after its fatal Debug, exactly as emitted.
void TargetObjectNetworkable::setNetUpdate(io::BitStream& stream, float /*updateTime*/,
                                           bool ghostActive,
                                           io::BaseLineData* /*baseline*/,
                                           int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();
    makeRoomForNextUpdate(ghostActive);
    if (m_prev != m_next)
        m_baselines[m_next] = m_baselines[m_prev];

    unsigned int stateMask = 0;
    stream.readBits(&stateMask, 5);

    if (stateMask & 0x01)
    {
        const Vec3 position =
            stream.readCompressedVector(stream.getCompressionVector(), 0.00050000002f);
        m_targetObject->setAbsolutePosition(position);
    }

    if (stateMask & 0x02)
    {
        unsigned int active = 0;
        stream.readBits(&active, 1);
        m_targetObject->setTargetActive(active == 1, true);
    }

    if (stateMask & 0x04)
    {
        unsigned int value = 0;
        stream.readBits(&value, 8);
        m_targetObject->m_targetPlayerId = static_cast<int>(value);
    }

    if (stream.hasError())
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/TargetObjectNetworkable.cpp", 197,
              __FUNCTION__)
            << formatString("read error");
        stream.clearError();
    }

    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
}


unsigned int TargetObjectNetworkable::getMinimumBitSize() const
{
    return 40;
}

// intentionally empty shipped prediction hook.
void TargetObjectNetworkable::predict(float)
{
}

} // namespace hfe
} // namespace dice
