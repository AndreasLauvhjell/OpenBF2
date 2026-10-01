#include "EnterVehicleEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameClient)
#include "../IGameClient.h"
#include "../../Core/Ids.h"                      // IID_IGameClient
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEEnterVehicle (0x88893E8) = 9.
const int GEEnterVehicle = 9;

} // namespace

// [INFERRED] argument order - no ELF constructor exists.
EnterVehicleEvent::EnterVehicleEvent(int playerId, unsigned short netId, bool setAsDefaultVehicle)
    : GameEvent()
    , m_playerId(static_cast<unsigned char>(playerId))
    , m_netId(netId)
    , m_setAsDefaultVehicle(setAsDefaultVehicle)
{
}

int  EnterVehicleEvent::getType() const { return GEEnterVehicle; }
void EnterVehicleEvent::logInfo(bool, int) {}

// Applies vehicle entry on the client, including the default-vehicle flag.
void EnterVehicleEvent::executeClient()
{
    IGameClient* gameClient = static_cast<IGameClient*>(game->queryInterface(IID_IGameClient));
    if (gameClient)
        gameClient->enterVehicle(m_playerId, m_netId, m_setAsDefaultVehicle);
}

// deSerialize - 0x8070DE0, 167 B. Byte, short, bit.
bool EnterVehicleEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    int netId = 0;
    stream->readBits(&netId, 16);
    m_netId = static_cast<unsigned short>(netId);

    int setAsDefaultVehicle = 0;
    stream->readBits(&setAsDefaultVehicle, 1);
    m_setAsDefaultVehicle = (setAsDefaultVehicle == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool EnterVehicleEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int netId = m_netId;
    stream->writeBits(&netId, 16);

    int setAsDefaultVehicle = m_setAsDefaultVehicle ? 1 : 0;
    stream->writeBits(&setAsDefaultVehicle, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
