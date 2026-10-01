#include "HandlePickupEvent.h"

#include "../IGameLogic.h"                       // gameLogic->handlePickup
#include "../Objects/IPlayer.h"                  // getVehicle
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../IO/Network/NetworkManager.h"     // getObject
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {
namespace io { extern NetworkManager* networkManager; }   // ds:0x8AC3D1C - Core/Globals.cpp

namespace {
// .rodata dword GEHandlePickup (0x8889948) = 14.
const int GEHandlePickup = 14;
} // namespace

// [INFERRED] argument order - no ELF constructor exists.
HandlePickupEvent::HandlePickupEvent(unsigned char playerId, unsigned short kitNetId,
                                     unsigned short vehicleNetId)
    : GameEvent()
    , m_playerId(playerId)
    , m_kitNetId(kitNetId)
    , m_vehicleNetId(vehicleNetId)
{
}

int  HandlePickupEvent::getType() const { return GEHandlePickup; }
void HandlePickupEvent::logInfo(bool, int) {}

// Resolves kit, vehicle, and player before testing availability. Requires initialized
// network/player managers and an embodied player. Pickup uses the event's vehicle,
// not the vehicle returned by the embodiment check.
void HandlePickupEvent::executeClient()
{
    world::IObject* kit     = io::networkManager->getObject(m_kitNetId);
    world::IObject* vehicle = io::networkManager->getObject(m_vehicleNetId);
    world::IPlayer* player  = world::playerManager->getPlayerFromId(m_playerId);   // +0x20

    if (!player || !kit || !vehicle)
        return;

    if (!player->getVehicle())                                                     // +0x48
        return;

    gameLogic->handlePickup(kit, player, vehicle, true);                           // +0xEC
}

// deSerialize - 0x8073A40, 167 B. A byte then two shorts.
bool HandlePickupEvent::deSerialize(io::BitStream* stream)
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

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool HandlePickupEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int kitNetId = m_kitNetId;
    stream->writeBits(&kitNetId, 16);

    int vehicleNetId = m_vehicleNetId;
    stream->writeBits(&vehicleNetId, 16);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
