// Path:     attested - the binary's __FILE__ strings name "OverheadCameraNetworkable.cpp".
// Contents: recovered from the symbolised BF2 1.50 ELF and cross-checked against the Win32 PE.
// The PE check is important here: OverheadCamera's STL-bearing layout and its virtual
// slot numbers differ between the ports, so all camera access below remains typed.

#include "Networkables.h"

#include <cmath>
#include <cstring>

#include "../Game/OverheadCamera.h"
#include "../IObject.h"
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"
#include "../../../IO/BitStream.h"
#include "../../../IO/Network/NetworkManager.h"
#include "../../../IO/Network/NetworkableInfo.h"
#include "../../../Math/Mat4.h"
#include "../../Utility/Vars.h"

namespace dice {
namespace hfe {

namespace {
Vec3 statsColour()
{
    Vec3 colour;
    colour.x = 180.0f;
    colour.y = 180.0f;
    colour.z = 180.0f;
    return colour;
}
} // namespace

// ELF static initialiser 0x823D360: the stats registration occurs before the con-var lookup.
io::RegistredNetworkStats OverheadCameraNetworkable::m_stats(
    "OverheadCameraNetworkable", statsColour());

namespace {
float* const g_overheadCameraMaxPositionDelta =
    Vars::getFloat("overheadCamera-maxPositionDelta", 7.0f);
} // namespace

// create - ELF 0x8779B00, 145 B. Aggregation is rejected; an unsupported requested interface
// releases the newly constructed instance immediately.
IBase* OverheadCameraNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    OverheadCameraNetworkable* self = new OverheadCameraNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

OverheadCameraNetworkable::OverheadCameraNetworkable()
    : m_refCount(1)
    , m_records(0)
{
}

// D2/D1/D0 - ELF 0x823C160/0x823C1B0/0x823C200. The nulling store after delete[] is shipped.
OverheadCameraNetworkable::~OverheadCameraNetworkable()
{
    if (m_records)
    {
        delete[] m_records;
        m_records = 0;
    }
}

void OverheadCameraNetworkable::addRef()
{
    ++m_refCount;
}

int OverheadCameraNetworkable::getRef()
{
    return m_refCount;
}

int OverheadCameraNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int left = --m_refCount;
    if (last && this != 0)
        delete this;
    return left;
}

// IBase/INetworkable use the primary face; INetworkableObject uses +0x3C.
void* OverheadCameraNetworkable::queryInterface(unsigned int iid) const
{
    OverheadCameraNetworkable* self = const_cast<OverheadCameraNetworkable*>(this);
    if (iid == IID_IBase || iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    return 0;
}

// init - ELF 0x823C250, 213 B. Only time and position are cleared in all four records; +0x04,
// +0x08 and the four replicated tail fields in records 1..3 deliberately remain untouched.
bool OverheadCameraNetworkable::init(void* object)
{
    world::IObject* iobject = static_cast<world::IObject*>(object);
    world::OverheadCamera* camera = static_cast<world::OverheadCamera*>(iobject);

    m_object = iobject;
    m_networkableObject = static_cast<io::INetworkableObject*>(
        queryInterface(io::IID_INetworkableObject));

    Record* ring = new Record[4];
    for (int i = 0; i < 4; ++i)
    {
        ring[i].time = 0.0f;
        ring[i].position.x = 0.0f;
        ring[i].position.y = 0.0f;
        ring[i].position.z = 0.0f;
    }
    m_records = ring;
    if (!ring)
        return false;

    ring[0].position = camera->getAbsolutePosition();
    ring[0].defaultTeam = camera->m_defaultTeam;
    ring[0].enabled = world::OverheadCamera::m_enabled;
    ring[0].panX = 0.0f;
    ring[0].panZ = 0.0f;
    return true;
}

// updateStateMask - ELF 0x823C330, 519 B. The two pan integrators share bit 0. Position uses the
// relative transform for a child and the absolute transform for a root. The object flag gate is
// before the ring moves.
void OverheadCameraNetworkable::updateStateMask(unsigned int* stateMask)
{
    if (m_object->getFlags() & 1)
        return;

    world::OverheadCamera* camera = static_cast<world::OverheadCamera*>(m_object);
    m_prev = m_head;
    m_head = (m_head + 1) & 3;
    m_next = m_head;

    Record& next = m_records[m_next];
    const Record& prev = m_records[m_prev];

    next.panX = camera->m_panState[0];
    if (next.panX != prev.panX)
        *stateMask |= 0x01;

    next.panZ = camera->m_panState[2];
    if (next.panZ != prev.panZ)
        *stateMask |= 0x01;

    const Mat4& transform = camera->getParent() ? camera->getRelativeTransformation()
                                                 : camera->getAbsoluteTransformation();
    next.position.x = transform.m[12];
    next.position.y = transform.m[13];
    next.position.z = transform.m[14];

    const float epsilon = 1.1920929e-7f;
    const bool samePosition =
        std::fabs(next.position.x - prev.position.x) <= epsilon
        && std::fabs(next.position.y - prev.position.y) <= epsilon
        && std::fabs(next.position.z - prev.position.z) <= epsilon;
    if (!samePosition)
        *stateMask |= 0x02;

    next.defaultTeam = camera->m_defaultTeam;
    if (next.defaultTeam != prev.defaultTeam)
        *stateMask |= 0x04;

    next.enabled = world::OverheadCamera::m_enabled;
    if (next.enabled != prev.enabled)
        *stateMask |= 0x08;
}

// getBaseLine - ELF 0x823C0F0, 106 B. BaseLineData's cursor is in two-byte units: twelve bytes
// are copied while twelve units (twenty-four bytes) are reserved, matching the shipped reader.
bool OverheadCameraNetworkable::getBaseLine(io::BaseLineData& baseline)
{
    const unsigned int cursor = baseline.m_cursor;
    if (cursor + 12 > 0x200)
        return false;

    const Record& record = m_records[m_head];
    float* out = reinterpret_cast<float*>(baseline.m_data + 2 * cursor);
    out[0] = record.position.x;
    out[1] = record.position.y;
    out[2] = record.position.z;
    baseline.m_cursor = static_cast<unsigned short>(cursor + 12);
    return true;
}

// getNetUpdate - ELF 0x823C540, 813 B. The four-bit frame is panX+panZ, position, default team,
// enabled. The two pan values each call getMaxSpeed independently and each use an eight-bit unit
// mapping. Baseline and precision are unused.
bool OverheadCameraNetworkable::getNetUpdate(io::BitStream& stream,
                                             io::BaseLineData* /*baseline*/,
                                             unsigned int stateMask,
                                             int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();
    m_next = m_head;
    stream.writeBits(&stateMask, 4);

    const Record& record = m_records[m_next];
    world::OverheadCamera* camera = static_cast<world::OverheadCamera*>(m_object);

    if (stateMask & 0x01)
    {
        double unit = (static_cast<double>(record.panX) / camera->getMaxSpeed() + 1.0) * 0.5;
        if (unit < 0.0)
            unit = 0.0;
        else if (unit > 1.0)
            unit = 1.0;
        unsigned int value = static_cast<unsigned int>(unit * 255.0);
        stream.writeBits(&value, 8);
    }
    if (stateMask & 0x01)
    {
        double unit = (static_cast<double>(record.panZ) / camera->getMaxSpeed() + 1.0) * 0.5;
        if (unit < 0.0)
            unit = 0.0;
        else if (unit > 1.0)
            unit = 1.0;
        unsigned int value = static_cast<unsigned int>(unit * 255.0);
        stream.writeBits(&value, 8);
    }

    if (stateMask & 0x02)
    {
        if (stream.hasCompressionVector())
        {
            stream.writeCompressedVector2(record.position, stream.getCompressionVector(), 0.001f);
        }
        else
        {
            unsigned int selector = 0;
            stream.writeBits(&selector, 3);
            stream.writeBits(&record.position.x, 32);
            stream.writeBits(&record.position.y, 32);
            stream.writeBits(&record.position.z, 32);
        }
    }

    if (stateMask & 0x04)
    {
        const int team = record.defaultTeam;
        if (team < 0)
        {
            unsigned int sign = 1;
            unsigned int magnitude = static_cast<unsigned int>(-team);
            stream.writeBits(&sign, 1);
            stream.writeBits(&magnitude, 7);
        }
        else
        {
            unsigned int sign = 0;
            unsigned int magnitude = static_cast<unsigned int>(team);
            stream.writeBits(&sign, 1);
            stream.writeBits(&magnitude, 7);
        }
    }

    if (stateMask & 0x08)
    {
        unsigned int enabled = record.enabled ? 1u : 0u;
        stream.writeBits(&enabled, 1);
    }

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    return true;
}

// setNetUpdate - ELF 0x823C870, 1,680 B; PE 0x5E5A80, 728 B. The PE has the camera team field at
// +0x26C rather than ELF +0x20C and different virtual slots, which is why both applications below
// use the typed OverheadCamera surface.
void OverheadCameraNetworkable::setNetUpdate(io::BitStream& stream, float updateTime,
                                             bool ghostActive,
                                             io::BaseLineData* /*baseline*/,
                                             int /*precision*/)
{
    m_updateStartBitPos = stream.getBitPosition();
    const bool roomForNextUpdate = makeRoomForNextUpdate(ghostActive);

    if (m_prev != m_next)
        std::memcpy(&m_records[m_next], &m_records[m_prev], sizeof(Record));
    Record& record = m_records[m_next];
    record.time = updateTime;

    unsigned int stateMask = 0;
    stream.readBits(&stateMask, 4);
    world::OverheadCamera* camera = static_cast<world::OverheadCamera*>(m_object);

    if (stateMask & 0x01)
    {
        const float maxSpeed = camera->getMaxSpeed();
        unsigned int value = 0;
        stream.readBits(&value, 8);
        const float unit = static_cast<float>(value / 255.0);
        float decoded = unit + unit - 1.0f;
        if (std::fabs(decoded) <= 0.0078431377f)
            decoded = 0.0f;
        record.panX = decoded * maxSpeed;
    }
    if (stateMask & 0x01)
    {
        const float maxSpeed = camera->getMaxSpeed();
        unsigned int value = 0;
        stream.readBits(&value, 8);
        const float unit = static_cast<float>(value / 255.0);
        float decoded = unit + unit - 1.0f;
        if (std::fabs(decoded) <= 0.0078431377f)
            decoded = 0.0f;
        record.panZ = decoded * maxSpeed;
    }

    if (stateMask & 0x02)
        record.position = stream.readCompressedVector2(stream.getCompressionVector(), 0.001f);

    if (stateMask & 0x04)
    {
        unsigned int sign = 0;
        stream.readBits(&sign, 1);
        unsigned int magnitude = 0;
        stream.readBits(&magnitude, 7);
        record.defaultTeam = (sign == 1) ? -static_cast<int>(magnitude)
                                         : static_cast<int>(magnitude);
    }

    if (stateMask & 0x08)
    {
        unsigned int enabled = 0;
        stream.readBits(&enabled, 1);
        record.enabled = (enabled == 1);
    }

    if (stream.hasError())
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/OverheadCameraNetworkable.cpp", 205,
              __FUNCTION__)
            << formatString("error reading data");
        stream.clearError();
        return;
    }

    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    if (roomForNextUpdate)
        camera->setAbsolutePosition(record.position);
    if (stateMask & 0x04)
        camera->m_defaultTeam = record.defaultTeam;
    if (stateMask & 0x08)
        world::OverheadCamera::m_enabled = record.enabled;
}

unsigned int OverheadCameraNetworkable::getMinimumBitSize() const
{
    return 50;                                         // `mov eax, 0x32`
}

// The ring search is intentionally written with the redundant second count check present in the
// ELF. It writes the high index before each time test and the low index only after a failed test.
bool OverheadCameraNetworkable::findEmbracingIndices(float time, unsigned int* lo,
                                                      unsigned int* hi) const
{
    if (static_cast<unsigned int>(m_count) <= 1)
        return false;

    unsigned int index = static_cast<unsigned int>(m_tail);
    if (m_records[index].time > time)
        return false;

    *lo = index;
    *hi = index;
    if (static_cast<unsigned int>(m_count) <= 1)
        return false;

    unsigned int i = 1;
    for (;;)
    {
        index = (static_cast<unsigned int>(m_tail) + i) & 3;
        *hi = index;
        if (m_records[index].time >= time)
            return true;
        ++i;
        *lo = index;
        if (i >= static_cast<unsigned int>(m_count))
            return false;
    }
}

// predict - ELF 0x823CF00, 1,105 B; PE 0x5E5520, 658 B. This class calls getPredictionMode but
// does not branch on its result. Every active pass first fixes the camera to the shipped overhead
// orientation, then interpolates or extrapolates only its position.
void OverheadCameraNetworkable::predict(float now)
{
    m_next = m_head;

    Vec3 predicted;
    predicted.x = 0.0f;
    predicted.y = 0.0f;
    predicted.z = 0.0f;

    if (m_forceFullUpdate)
        return;

    (void)m_networkableInfo->getPredictionMode();

    world::OverheadCamera* camera = static_cast<world::OverheadCamera*>(m_object);
    const Vec3 currentPosition = camera->getAbsolutePosition();
    Mat4 orientation = {};
    orientation.m[0] = 1.0f;
    orientation.m[1] = 0.0f;
    orientation.m[2] = 0.0f;
    orientation.m[4] = 0.0f;
    orientation.m[5] = 0.0f;
    orientation.m[6] = 1.0f;
    orientation.m[8] = 0.0f;
    orientation.m[9] = -1.0f;
    orientation.m[10] = 0.0f;
    orientation.m[12] = currentPosition.x;
    orientation.m[13] = currentPosition.y;
    orientation.m[14] = currentPosition.z;
    orientation.m[15] = 1.0f;
    camera->setAbsoluteTransformation(orientation);

    const float predictionTime =
        now - static_cast<float>(io::networkManager->getInterpolationTime());
    const Record& newest = m_records[m_next];

    if (predictionTime <= newest.time)
    {
        unsigned int lo = 0;
        unsigned int hi = 0;
        if (findEmbracingIndices(predictionTime, &lo, &hi))
        {
            const Record& lower = m_records[lo];
            const Record& upper = m_records[hi];
            const float span = upper.time - lower.time;
            const float dx = upper.position.x - lower.position.x;
            const float dy = upper.position.y - lower.position.y;
            const float dz = upper.position.z - lower.position.z;
            const float maxDelta = *g_overheadCameraMaxPositionDelta;

            if (span > 0.0f && span < 500.0f
                && maxDelta * maxDelta > dx * dx + dy * dy + dz * dz)
            {
                const float fraction = (1.0f >= span)
                    ? 1.0f
                    : (predictionTime - lower.time) / span;
                predicted.x = lower.position.x + dx * fraction;
                predicted.y = lower.position.y + dy * fraction;
                predicted.z = lower.position.z + dz * fraction;
                camera->setAbsolutePosition(predicted);
                return;
            }
        }

        camera->setAbsolutePosition(newest.position);
        return;
    }

    const float beyondNewest = predictionTime - newest.time;
    const float maxExtrapolation =
        static_cast<float>(io::networkManager->getExtrapolationTime());
    if (maxExtrapolation >= beyondNewest)
    {
        const float elapsed = (beyondNewest <= maxExtrapolation)
            ? beyondNewest
            : maxExtrapolation;
        const float seconds = elapsed / 1000.0f;
        predicted.x = newest.position.x + newest.panX * seconds;
        predicted.y = newest.position.y;
        predicted.z = newest.position.z + newest.panZ * seconds;
        camera->setAbsolutePosition(predicted);
    }
}

} // namespace hfe
} // namespace dice
