#include "UpdateTriggerEvent.h"

#include "../IGame.h"                            // confirmGhost
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEUpdateTrigger (0x888AD04) = 59.
const int GEUpdateTrigger = 59;
} // namespace

//  (C2) / 0x8080790 (C1), 119 B. Two arguments accepted and discarded, and m_netId not
// set at all - see the header block. Raw store map: [+0x0C] = arg1, [+0x14..+0x1C] = arg2's three
// floats, [+0x20..+0x28] = arg3's, [+0x2C] = 0, [+0x30] = 0.
UpdateTriggerEvent::UpdateTriggerEvent(unsigned int field0C, const Vec3& min, const Vec3& max,
                                       int /*discarded1*/, int /*discarded2*/)
    : GameEvent()
    , m_field0C(field0C)                                     // +0x0C
    , m_min(min)                                             // +0x14
    , m_max(max)                                             // +0x20
    , m_field2C(0)                                           // literal 0, NOT the fourth argument
    , m_field30(0)                                           // literal 0, NOT the fifth argument
{
    // m_netId (+0x10) and m_field34 (+0x34) are deliberately NOT initialised - the ELF's 119
    // bytes touch neither. Faithful, not an omission.
}

// The maker's form. The ELF has no such constructor.
UpdateTriggerEvent::UpdateTriggerEvent()
    : GameEvent()
    , m_field0C(0)
    , m_netId(0)
    , m_field2C(0)
    , m_field30(0)
    , m_field34(0)
{
    m_min.x = 0.0f; m_min.y = 0.0f; m_min.z = 0.0f;
    m_max.x = 0.0f; m_max.y = 0.0f; m_max.z = 0.0f;
}

int  UpdateTriggerEvent::getType() const { return GEUpdateTrigger; }
void UpdateTriggerEvent::logInfo(bool, int) {}

// `mov eax, 1; retn`. Genuinely empty.
void UpdateTriggerEvent::executeClient()
{
}

// Confirms the trigger ghost when delivery is acknowledged. node must begin with
// the connection ID integer. [UNRESOLVED] The full node type remains opaque.
void UpdateTriggerEvent::eventReceivedByRemote(void* node)
{
    game->confirmGhost(*reinterpret_cast<int*>(node), m_netId);   // +0x70
}

// Wire order: field0C, netId, minimum, maximum, field30, field2C.
bool UpdateTriggerEvent::deSerialize(io::BitStream* stream)
{
    int field0C = 0;
    stream->readBits(&field0C, 32);
    m_field0C = static_cast<unsigned int>(field0C);

    int netId = 0;
    stream->readBits(&netId, 16);
    m_netId = static_cast<unsigned short>(netId);

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    stream->readBits(&x, 32);
    stream->readBits(&y, 32);
    stream->readBits(&z, 32);
    m_min.x = x; m_min.y = y; m_min.z = z;

    x = 0.0f; y = 0.0f; z = 0.0f;
    stream->readBits(&x, 32);
    stream->readBits(&y, 32);
    stream->readBits(&z, 32);
    m_max.x = x; m_max.y = y; m_max.z = z;

    int field30 = 0;
    stream->readBits(&field30, 8);
    m_field30 = field30;

    int field2C = 0;
    stream->readBits(&field2C, 8);
    m_field2C = field2C;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x80809E0, 376 B. The same order; the two vectors are written straight out of the
// object (six `lea eax, [esi+NN]` at 0x8080A77..0x8080AEB) rather than through temporaries.
bool UpdateTriggerEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int field0C = static_cast<int>(m_field0C);
    stream->writeBits(&field0C, 32);

    int netId = m_netId;
    stream->writeBits(&netId, 16);

    stream->writeBits(&m_min.x, 32);
    stream->writeBits(&m_min.y, 32);
    stream->writeBits(&m_min.z, 32);

    stream->writeBits(&m_max.x, 32);
    stream->writeBits(&m_max.y, 32);
    stream->writeBits(&m_max.z, 32);

    int field30 = m_field30;
    stream->writeBits(&field30, 8);

    int field2C = m_field2C;
    stream->writeBits(&field2C, 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
