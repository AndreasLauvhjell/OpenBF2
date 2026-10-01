#include "ChangePlayerNameEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../IGameServer.h"                      // changePlayerName
#include "../Objects/IPlayer.h"                  // getName / getId / setName
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEChangePlayerName (0x8888740) = 12.
const int GEChangePlayerName = 12;
} // namespace

//  (C2) / 0x806AF30 (C1), 125 B. Clamp, copy byte by byte, terminate, then the id.
// getName() IS called once PER character in the ELF - the loop body reloads it every iteration
// (0x806AEB0's `*(i + *(*(*a2 + 36))(a2))`). Reproduced: the reference is taken inside the loop.
// It is a virtual call per byte, and it is what the binary does.
ChangePlayerNameEvent::ChangePlayerNameEvent(world::IPlayer* player)
    : GameEvent()
{
    int length = static_cast<int>(player->getName().size());                  // +0x24
    if (length > MAX_NAME_LENGTH)
        length = MAX_NAME_LENGTH;

    int i = 0;
    for (; i < length; ++i)
        m_name[i] = player->getName()[i];                                     // +0x24, per byte
    m_name[i] = '\0';

    m_playerId = static_cast<unsigned char>(player->getId());                 // +0x54 -> +0x2C
}

//  (C2) / 0x806B020 (C1), 100 B. The same clamp-copy-terminate, name and id supplied.
ChangePlayerNameEvent::ChangePlayerNameEvent(int playerId, const std::string& name)
    : GameEvent()
{
    int length = static_cast<int>(name.size());
    if (length > MAX_NAME_LENGTH)
        length = MAX_NAME_LENGTH;

    int i = 0;
    for (; i < length; ++i)
        m_name[i] = name[i];
    m_name[i] = '\0';

    m_playerId = static_cast<unsigned char>(playerId);                        // +0x2C
}

int  ChangePlayerNameEvent::getType() const { return GEChangePlayerName; }
void ChangePlayerNameEvent::logInfo(bool, int) {}

// executeClient - 0x806B1D0, 133 B. Look the player up by the packet's id and rename them.
// NO IID_IGameClient gate. Unlike DestroyPlayerEvent and the vehicle pair, this arm does not ask
// whether it is a client at all - it just does the rename. On a listen server it therefore runs on
// both sides.
void ChangePlayerNameEvent::executeClient()
{
    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);   // +0x20
    if (!player)
        return;

    player->setName(std::string(m_name));                                         // +0x20
}

// Rejects renaming another player. Routes accepted changes through GameServer
// so the server can broadcast them.
bool ChangePlayerNameEvent::executeServer(int connectionId)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    if (connectionId != m_playerId)
        return false;

    server->changePlayerName(m_playerId, std::string(m_name));                     // +0x38
    return true;
}

// Reads a player id and fixed 32-byte name. [CHOSEN, not recovered] Forces a final
// terminator so malformed names cannot make string consumers read beyond the buffer;
// well-formed names and the stream cursor are unchanged.
bool ChangePlayerNameEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    stream->readBits(m_name, NAME_BUFFER_SIZE * 8);
    m_name[NAME_BUFFER_SIZE - 1] = '\0';         // the divergence - see above

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x806B120, 166 B. The id, then the whole 32-byte block including whatever follows
// the terminator - the buffer is sent in full, not up to the NUL.
bool ChangePlayerNameEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    stream->writeBits(m_name, NAME_BUFFER_SIZE * 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
