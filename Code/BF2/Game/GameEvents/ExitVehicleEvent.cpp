#include "ExitVehicleEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameClient)
#include "../IGameClient.h"
#include "../../Core/Ids.h"                      // IID_IGameClient
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEExitVehicle (0x8889480) = 10.
const int GEExitVehicle = 10;

} // namespace

// [INFERRED] argument order - no ELF constructor exists.
ExitVehicleEvent::ExitVehicleEvent(int playerId, bool field0D)
    : GameEvent()
    , m_playerId(static_cast<unsigned char>(playerId))
    , m_field0D(field0D)
{
}

int  ExitVehicleEvent::getType() const { return GEExitVehicle; }
void ExitVehicleEvent::logInfo(bool, int) {}

void ExitVehicleEvent::executeClient()
{
    IGameClient* gameClient = static_cast<IGameClient*>(game->queryInterface(IID_IGameClient));
    if (gameClient)
        gameClient->exitVehicle(m_playerId, m_field0D);
}

bool ExitVehicleEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    int field0D = 0;
    stream->readBits(&field0D, 1);
    m_field0D = (field0D == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool ExitVehicleEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int field0D = m_field0D ? 1 : 0;
    stream->writeBits(&field0D, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
