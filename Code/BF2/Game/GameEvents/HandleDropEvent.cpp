#include "HandleDropEvent.h"

#include "../IGameLogic.h"                       // gameLogic->handleDrop
#include "../Objects/IObject.h"                  // setAbsolutePosition
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../IO/Network/NetworkManager.h"     // getObject
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {
namespace io { extern NetworkManager* networkManager; }   // ds:0x8AC3D1C - Core/Globals.cpp

namespace {
// .rodata dword GEHandleDrop (0x88898C8) = 13.
const int GEHandleDrop = 13;
} // namespace

//  (C2) / 0x80735F0 (C1), 103 B. Store order in the ELF is +0x10, +0x0E, +0x0C, then the
// vector; written in field order here.
HandleDropEvent::HandleDropEvent(unsigned char playerId, unsigned short kitNetId,
                                 unsigned short vehicleNetId, const Vec3& position)
    : GameEvent()
    , m_playerId(playerId)                                   // +0x0C
    , m_kitNetId(kitNetId)                                   // +0x0E
    , m_vehicleNetId(vehicleNetId)                           // +0x10
    , m_position(position)                                   // +0x14
{
}

int  HandleDropEvent::getType() const { return GEHandleDrop; }
void HandleDropEvent::logInfo(bool, int) {}

// Processes the drop before moving the kit to its reported position. Requires
// networkManager; no current-vehicle precondition is applied. The final flag is true.
void HandleDropEvent::executeClient()
{
    world::IObject* kit     = io::networkManager->getObject(m_kitNetId);
    world::IObject* vehicle = io::networkManager->getObject(m_vehicleNetId);
    world::IPlayer* player  = world::playerManager->getPlayerFromId(m_playerId);   // +0x20

    if (!player || !kit || !vehicle)
        return;

    gameLogic->handleDrop(kit, player, vehicle, true);                             // +0xF0
    kit->setAbsolutePosition(m_position);                                          // +0x70
}

// deSerialize - 0x8073670, 269 B. A byte, two shorts, then three raw floats through temporaries.
bool HandleDropEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    int kitNetId = 0;
    stream->readBits(&kitNetId, 16);
    m_kitNetId = static_cast<unsigned short>(kitNetId);

    int vehicleNetId = 0;
    stream->readBits(&vehicleNetId, 16);
    m_vehicleNetId = static_cast<unsigned short>(vehicleNetId);

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    stream->readBits(&x, 32);
    stream->readBits(&y, 32);
    stream->readBits(&z, 32);

    m_position.x = x;
    m_position.y = y;
    m_position.z = z;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool HandleDropEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int kitNetId = m_kitNetId;
    stream->writeBits(&kitNetId, 16);

    int vehicleNetId = m_vehicleNetId;
    stream->writeBits(&vehicleNetId, 16);

    stream->writeBits(&m_position.x, 32);
    stream->writeBits(&m_position.y, 32);
    stream->writeBits(&m_position.z, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
