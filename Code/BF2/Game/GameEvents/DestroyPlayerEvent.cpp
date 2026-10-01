#include "DestroyPlayerEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameClient)
#include "../IGameClient.h"
#include "../../Core/Ids.h"                      // IID_IGameClient
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEDestroyPlayer (0x88891C0) = 8.
const int GEDestroyPlayer = 8;

} // namespace

// [INFERRED] - no ELF constructor exists.
DestroyPlayerEvent::DestroyPlayerEvent(int playerId)
    : GameEvent()
    , m_playerId(static_cast<unsigned char>(playerId))
{
}

int DestroyPlayerEvent::getType() const
{
    return GEDestroyPlayer;
}

// Removes the identified player from the client, if a client is present.
void DestroyPlayerEvent::executeClient()
{
    IGameClient* gameClient = static_cast<IGameClient*>(game->queryInterface(IID_IGameClient));
    if (gameClient)
        gameClient->destroyPlayer(m_playerId);
}

bool DestroyPlayerEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool DestroyPlayerEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
