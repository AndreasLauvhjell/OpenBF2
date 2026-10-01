// Path:     attested - the binary's __FILE__ strings name "FreeCameraNetworkable.cpp", and
// setNetUpdate's Debug site embeds "Game/Objects/Networkables/FreeCameraNetworkable.cpp".
// Contents: recovered from bf2.i64 (ELF, symbolised) and the BF2 1.50 Win32 images.
// Construction, both wire halves and prediction are recovered from complete paired
// bodies. Prediction does not change the three-bit wire payload or 56-byte snapshot.

#include "FreeCameraNetworkable.h"
#include <cmath>
#include <cstring>

#include "../Base/FreeCamera.h"
#include "../IObject.h"
#include "../ObjectLayoutAccess.h"      // parent() - init's transform choice
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"
#include "../../../IO/BitStream.h"                // the wire - getNetUpdate
#include "../../../IO/Network/NetworkManager.h"   // io::BaseLineData - getBaseLine
#include "../../../IO/Network/NetworkableInfo.h"
#include "../../../Math/Mat4.h"

namespace dice {
namespace hfe {

// The TU's static, built by the static initialiser (0x82374A0) as
// RegistredNetworkStats("FreeCameraNetworkable", Vec3(10, 180, 180)) - 0x41200000 / 0x43340000
// twice, a graph colour. Its sibling's is 255/200/240.
namespace {
Vec3 statsColour()
{
    Vec3 c; c.x = 10.0f; c.y = 180.0f; c.z = 180.0f;
    return c;
}
} // namespace
io::RegistredNetworkStats FreeCameraNetworkable::m_stats("FreeCameraNetworkable", statsColour());

// Rejects aggregation; releases the created object when the requested interface is unsupported.
IBase* FreeCameraNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    FreeCameraNetworkable* self = new FreeCameraNetworkable();       // operator new(0x58u)
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// The stores create makes after NetworkableBase's constructor: the two base sub-objects' scalars
// (m_ownerId -1, m_object 0, +0x0C 0, +0x10 -1 - NetworkableObjectBase's own constructor does those
// four here), then the refcount and the empty ring.
FreeCameraNetworkable::FreeCameraNetworkable()
    : m_refCount(1)                                  // +0x50
    , m_records(0)                                   // +0x54
{
}

// Deletes the snapshot ring and clears its pointer.
FreeCameraNetworkable::~FreeCameraNetworkable()
{
    if (m_records)
    {
        delete[] m_records;
        m_records = 0;
    }
}

// The quartet. addRef returns THE NEW count in the ELF (the interface's return is void, so the
// value is dropped at every call site), and release returns the count it just decremented.
void FreeCameraNetworkable::addRef()
{
    ++m_refCount;
}

int FreeCameraNetworkable::getRef()
{
    return m_refCount;
}

int FreeCameraNetworkable::release()
{
    const int left = m_refCount - 1;
    m_refCount = left;
    if (left == 0 && this != 0)
        delete this;                                 // `call [vptr+0x14]`, the deleting destructor
    return left;
}

// Returns the requested base, networkable or world-object interface, or null for an unsupported id.
void* FreeCameraNetworkable::queryInterface(unsigned int iid) const
{
    FreeCameraNetworkable* self = const_cast<FreeCameraNetworkable*>(this);

    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// Binds the camera and seeds the first of four baseline records from its current transform.
// Parented cameras use relative coordinates; root cameras use absolute coordinates.
bool FreeCameraNetworkable::init(void* object)
{
    typedef world::object_detail::LayoutAccess L;

    world::IObject* iobject = static_cast<world::IObject*>(object);
    m_object = iobject;                                                          // +0x44
    m_networkableObject = static_cast<io::INetworkableObject*>(
        queryInterface(io::IID_INetworkableObject));                             // +0x18

    Record* ring = new Record[4];                                                // new[](0xE0)
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time     = 0.0f;
        ring[i].byte04   = false;
        ring[i].position[0] = 0.0f;
        ring[i].position[1] = 0.0f;
        ring[i].position[2] = 0.0f;
        ring[i].velocity[0] = 0.0f;
        ring[i].velocity[1] = 0.0f;
        ring[i].velocity[2] = 0.0f;
        ring[i].rotation.x = 0.0f;
        ring[i].rotation.y = 0.0f;
        ring[i].rotation.z = 0.0f;
        ring[i].rotation.w = 1.0f;                   // the identity quaternion, the one non-zero
        // +0x08 and +0x34 are NOT touched by the ELF's loop - see the header.
    }

    m_records = ring;                                                            // +0x54
    if (!ring)
        return false;

    const Mat4& rotationSource = L::parent(m_object)
                                     ? m_object->getRelativeTransformation()      // vcall +0xE8
                                     : m_object->getAbsoluteTransformation();     // vcall +0x84
    ring[0].rotation.fromMat(rotationSource);

    const Mat4& positionSource = L::parent(m_object)                              // asked again
                                     ? m_object->getRelativeTransformation()
                                     : m_object->getAbsoluteTransformation();
    ring[0].position[0] = positionSource.m[12];
    ring[0].position[1] = positionSource.m[13];
    ring[0].position[2] = positionSource.m[14];

    ring[0].followedPlayerId =
        static_cast<world::FreeCamera*>(m_object)->m_followedPlayerId;            // object +0x1A8
    return true;
}


// Marks rotation, position and followed-player changes unless object flag 1 disables replication.
// Rotation compares exact quaternion bytes; position compares each component with an epsilon.
void FreeCameraNetworkable::updateStateMask(unsigned int* stateMask)
{
    if (m_object->getFlags() & 1)
        return;                                      // before the ring moves - see above

    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;

    Record& next = m_records[m_next];
    Record& prev = m_records[m_prev];

    const Mat4& forRotation = m_object->getParent() ? m_object->getRelativeTransformation()
                                                    : m_object->getAbsoluteTransformation();
    next.rotation.fromMat(forRotation);
    if (std::memcmp(&next.rotation, &prev.rotation, 16) != 0)   // memcmp, not an epsilon
        *stateMask |= 0x01;

    const Mat4& forPosition = m_object->getParent() ? m_object->getRelativeTransformation()
                                                    : m_object->getAbsoluteTransformation();
    next.position[0] = forPosition.m[12];
    next.position[1] = forPosition.m[13];
    next.position[2] = forPosition.m[14];

    const float eps  = 1.1920929e-7f;
    const bool same  = std::fabs(next.position[0] - prev.position[0]) <= eps
                    && std::fabs(next.position[1] - prev.position[1]) <= eps
                    && std::fabs(next.position[2] - prev.position[2]) <= eps;
    if (!same)
        *stateMask |= 0x02;

    // reads camera +0x1A8; PE 0x5E49F1 reads +0x1F0.
    // Use the modeled member, as init does: neither retail offset describes our native class.
    next.followedPlayerId =
        static_cast<world::FreeCamera*>(m_object)->m_followedPlayerId;
    if (next.followedPlayerId != prev.followedPlayerId)
        *stateMask |= 0x04;
}

// Copies the newest position to baseline and advances its cursor by 12 two-byte units.
// Requires initialized records; the reserved stride is twice the copied position size.
bool FreeCameraNetworkable::getBaseLine(io::BaseLineData& baseline)
{
    const unsigned int cursor = baseline.m_cursor;               // +0x400
    if (cursor + 12 > 0x200)
        return false;

    const Record& record = m_records[m_head];                    // +0x54 ring, +0x28 newest slot
    float* out = reinterpret_cast<float*>(baseline.m_data + 2 * cursor);
    out[0] = record.position[0];
    out[1] = record.position[1];
    out[2] = record.position[2];
    baseline.m_cursor = static_cast<unsigned short>(cursor + 12);
    return true;
}

// Writes a 3-bit mask, compressed position, 16-bit quaternion and sign-plus-8-bit followed-player id.
// Returns false on stream failure. Baseline and precision are unused.
bool FreeCameraNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                         unsigned int stateMask, int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();               // +0x30
    m_next = m_head;                                             // +0x38 <- +0x28, the send slot

    stream.writeBits(&stateMask, 3);

    const Record& record = m_records[m_next];

    if (stateMask & 0x02)
    {
        if (stream.m_hasCompressionVector)
        {
            stream.writeCompressedVector2(
                *reinterpret_cast<const Vec3*>(record.position),
                *reinterpret_cast<const Vec3*>(stream.m_compressionVector), 0.001f);
        }
        else
        {
            unsigned int selector = 0;               // the hand-written selector-0 escape
            stream.writeBits(&selector, 3);
            stream.writeBits(&record.position[0], 32);
            stream.writeBits(&record.position[1], 32);
            stream.writeBits(&record.position[2], 32);
        }
    }

    if (stateMask & 0x01)
        stream.writeUnitQuaternion(record.rotation, 16);

    if (stateMask & 0x04)
    {
        const int followedPlayerId = record.followedPlayerId;    // +0x34
        if (followedPlayerId < 0)
        {
            unsigned int negative = 1;
            stream.writeBits(&negative, 1);
            unsigned int magnitude = static_cast<unsigned int>(-followedPlayerId);
            stream.writeBits(&magnitude, 8);
        }
        else
        {
            unsigned int negative = 0;
            stream.writeBits(&negative, 1);
            unsigned int magnitude = static_cast<unsigned int>(followedPlayerId);
            stream.writeBits(&magnitude, 8);
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

// Decodes camera state, carrying omitted fields forward and recording updateTime.
// Available ring space gates transform application; a received followed-player id is always applied.
// Read failure returns without applying state.
void FreeCameraNetworkable::setNetUpdate(io::BitStream& stream, float updateTime,
                                         bool ghostActive, io::BaseLineData* /*baseline*/,
                                         int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();
    const bool roomForNextUpdate = makeRoomForNextUpdate(ghostActive);

    if (m_prev != m_next)
        std::memcpy(&m_records[m_next], &m_records[m_prev], sizeof(Record));
    Record& record = m_records[m_next];
    record.time = updateTime;

    unsigned int stateMask = 0;
    stream.readBits(&stateMask, 3);

    if (stateMask & 0x02)
    {
        const Vec3 position =
            stream.readCompressedVector2(stream.getCompressionVector(), 0.001f);
        record.position[0] = position.x;
        record.position[1] = position.y;
        record.position[2] = position.z;
    }

    if (stateMask & 0x01)
        record.rotation = stream.readUnitQuaternion(16);

    if (stateMask & 0x04)
    {
        unsigned int sign = 0;
        stream.readBits(&sign, 1);
        unsigned int magnitude = 0;
        stream.readBits(&magnitude, 8);
        record.followedPlayerId = (sign == 1) ? -static_cast<int>(magnitude)
                                               : static_cast<int>(magnitude);
    }

    if (stream.hasError())
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/FreeCameraNetworkable.cpp", 180,
              __FUNCTION__)
            << formatString("error reading data");
        stream.clearError();
        return;
    }

    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    if (roomForNextUpdate)
    {
        Mat4 transform = Mat4::getIdentity();
        record.rotation.toMat(transform);
        transform.m[12] = record.position[0];
        transform.m[13] = record.position[1];
        transform.m[14] = record.position[2];

        if (m_object->getParent())
            m_object->setRelativeTransformation(transform);
        else
            m_object->setAbsoluteTransformation(transform);
    }

    if (stateMask & 0x04)
        static_cast<world::FreeCamera*>(m_object)->m_followedPlayerId = record.followedPlayerId;
}

// . The count and ring arithmetic are unsigned in both ports.
// Early rejection leaves the output pair untouched; exhaustion leaves both at the last slot.
bool FreeCameraNetworkable::findEmbracingIndices(float time, unsigned int* lo,
                                                unsigned int* hi) const
{
    if (static_cast<unsigned int>(m_count) <= 1u)
        return false;
    if (m_records[m_tail].time > time)
        return false;
    *lo = static_cast<unsigned int>(m_tail);
    *hi = static_cast<unsigned int>(m_tail);
    for (unsigned int i = 1; i < static_cast<unsigned int>(m_count); ++i)
    {
        *hi = (i + static_cast<unsigned int>(m_tail)) & 3u;
        if (m_records[*hi].time >= time)
            return true;
        *lo = *hi;
    }
    return false;
}

// / PE secondary-base body 0x5E4EF0, with transform helper 0x5E30C0.
// [CHOSEN, not recovered] typed native fields replace all shipped pointer/byte arithmetic.
// Preserve the discarded policy query, unsigned millisecond settings and zero/500-ms gates.
void FreeCameraNetworkable::predict(float time)
{
    m_next = m_head;
    if (m_forceFullUpdate)
        return;
    if (static_cast<world::FreeCamera*>(m_object)->getViewMode() == 6)
        return;
    m_networkableInfo->getPredictionMode();

    const double predictedTime = static_cast<double>(time)
        - static_cast<unsigned int>(io::networkManager->getInterpolationTime());
    float timeDelta = 0.0f;
    Record& newest = m_records[m_next];
    if (static_cast<unsigned int>(m_count) > 1u)
    {
        const Record& previous = m_records[(static_cast<unsigned int>(m_next) + 3u) & 3u];
        timeDelta = newest.time - previous.time;
        if (timeDelta > 0.0f)
        {
#if defined(_WIN32)
            // [PLATFORM] PE multiplies by float 0.001; ELF divides by float 1000.
            const float seconds = timeDelta * 0.001f;
#else
            const float seconds = timeDelta / 1000.0f;
#endif
            const float reciprocal = 1.0f / seconds;
            for (int i = 0; i < 3; ++i)
                newest.velocity[i] = (newest.position[i] - previous.position[i]) * reciprocal;
        }
        else
            newest.velocity[0] = newest.velocity[1] = newest.velocity[2] = 0.0f;
    }
    else
        newest.velocity[0] = newest.velocity[1] = newest.velocity[2] = 0.0f;

    // The PE factors this exact sequence into 0x5E30C0; ELF emits it in each branch.
    const auto applyTransform = [this](const Quat& rotation, const float* position)
    {
        Mat4 transform = Mat4::getIdentity();
        rotation.toMat(transform);
        transform.m[12] = position[0];
        transform.m[13] = position[1];
        transform.m[14] = position[2];
        if (m_object->getParent())
            m_object->setRelativeTransformation(transform);
        else
            m_object->setAbsoluteTransformation(transform);
    };

    if (predictedTime > newest.time)
    {
        const unsigned int maximum = static_cast<unsigned int>(io::networkManager->getExtrapolationTime());
        if (maximum < predictedTime - newest.time)
        {
            applyTransform(newest.rotation, newest.position);
            return;
        }
        if (timeDelta == 0.0f)
            timeDelta = static_cast<float>(maximum);
        const float factor = static_cast<float>((predictedTime - newest.time) / timeDelta);
#if defined(_WIN32)
        const float seconds = timeDelta * 0.001f;
#else
        const float seconds = timeDelta / 1000.0f;
#endif
        float position[3];
        for (int i = 0; i < 3; ++i)
        {
            const float displacement = newest.velocity[i] * seconds;
            position[i] = newest.position[i] + displacement * factor;
        }
        applyTransform(newest.rotation, position);
        return;
    }

    unsigned int lo, hi;
    const float interpolationTime = static_cast<float>(predictedTime);
    if (findEmbracingIndices(interpolationTime, &lo, &hi))
    {
        const Record& first = m_records[lo];
        const Record& second = m_records[hi];
        const float span = second.time - first.time;
        if (span > 0.0f && span < 500.0f)
        {
            const float factor = span > 1.0f ? (interpolationTime - first.time) / span : 1.0f;
            Quat rotation = {0.0f, 0.0f, 0.0f, 1.0f};
            rotation.slerp(first.rotation, second.rotation, factor);
            float position[3];
            for (int i = 0; i < 3; ++i)
                position[i] = first.position[i] + (second.position[i] - first.position[i]) * factor;
            applyTransform(rotation, position);
            return;
        }
    }
    applyTransform(newest.rotation, newest.position);
}

} // namespace hfe
} // namespace dice
