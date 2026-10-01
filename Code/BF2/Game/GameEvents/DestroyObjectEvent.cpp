#include "DestroyObjectEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameClient)
#include "../IGameClient.h"
#include "../../Core/Ids.h"                      // IID_IGameClient
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEDestroyObject (0x8889124) = 7.
const int GEDestroyObject = 7;

} // namespace

// [INFERRED] - no ELF constructor exists.
DestroyObjectEvent::DestroyObjectEvent(unsigned short netId)
    : GameEvent()
    , m_netId(netId)
{
}

int  DestroyObjectEvent::getType() const { return GEDestroyObject; }
void DestroyObjectEvent::logInfo(bool, int) {}

// Removes the networked object from the client, if a client is present.
void DestroyObjectEvent::executeClient()
{
    IGameClient* gameClient = static_cast<IGameClient*>(game->queryInterface(IID_IGameClient));
    if (gameClient)
        gameClient->destroyObject(m_netId);
}

// deSerialize - 0x806FE40, 87 B. One 16-bit networkable id.
bool DestroyObjectEvent::deSerialize(io::BitStream* stream)
{
    int netId = 0;
    stream->readBits(&netId, 16);
    m_netId = static_cast<unsigned short>(netId);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool DestroyObjectEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int netId = m_netId;
    stream->writeBits(&netId, 16);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
