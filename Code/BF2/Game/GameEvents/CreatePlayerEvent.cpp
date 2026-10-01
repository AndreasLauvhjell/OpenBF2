#include "CreatePlayerEvent.h"

#include <cstring>                           // memset - the maker form
#include "../Objects/IPlayer.h"
#include "../Common/GameEvent.h"             // gameEventTypeIdBitCount
#include "../IGame.h"                        // game->queryInterface
#include "../../Core/Ids.h"                  // IID_IGameClient
#include "../../IO/BitStream.h"              // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h" // world::playerManager - createPlayer / setLocalHumanPlayer
#include "../../Core/Debug.h"                 // "Failed to create requested player" (line 157)
#include "../IGameClient.h"                  // getConnection (+0x20)
#include "../Common/IClientConnection.h"     // setPlayer (+0x1C)
#include "../../IO/Network/INetworkable.h"
#include "../../IO/Network/NetworkManager.h"  // io::networkManager->addNetworkable(networkable, netId)

#include <string>

namespace dice {
namespace hfe {

// CreatePlayer uses wire event id 5.
const int GECreatePlayer = 5;

// Maker-only default form; the original event maker did not use this constructor.
CreatePlayerEvent::CreatePlayerEvent()
    : m_teamId(0)
    , m_spawnGroupId(0)
    , m_isRemote(false)
    , m_isAIPlayer(false)
    , m_playerId(0)
    , m_pad31(0)
    , m_playerNetId(0)
    , m_word34(0)
    , m_pad36(0)
{
    std::memset(m_playerName, 0, sizeof(m_playerName));
}

CreatePlayerEvent::CreatePlayerEvent(world::IPlayer* player, bool isRemote)
    : GameEvent()
{
    // The name copy: at most 31 characters, then an explicit NUL. `i` survives the loop and indexes
    // the terminator, so a 31-character name fills the buffer exactly.
    int limit = 31;
    if ((int)player->getName().size() < 31)
        limit = (int)player->getName().size();

    int i = 0;
    for (; i < limit; ++i)
        m_playerName[i] = player->getName()[i];
    m_playerName[i] = 0;

    // Each of these three is an `int` at the source and a byte in the event. See the header.
    m_playerId     = (unsigned char)player->getId();                        // +0x54
    // A field read, NOT AN accessor - `movzx eax, word ptr [eax+4]` at 0x806EA8B. INetworkable
    // has no getter for its id; m_networkableId is public precisely because callers reach it this
    // way (INetworkable.h records that it was made public on the same kind of evidence).
    m_playerNetId  = player->getNetworkable()->m_networkableId;             // +0x190, word at +0x04
    m_teamId       = (unsigned char)player->getTeam();                      // +0xE8
    m_spawnGroupId = (unsigned char)player->getSpawnGroup();                // +0x158
    m_word34       = 0;
    m_isRemote     = isRemote;
    m_isAIPlayer   = player->getIsAIPlayer();                               // +0x64
}

// Retains a player lookup whose result is discarded; no log output remains.
// Both arguments are unused.
void CreatePlayerEvent::logInfo(bool, int) const
{
    if (world::playerManager)
        world::playerManager->getPlayerFromId(m_playerId);   // result discarded - see above
}

// Serializes 305 fixed-width bits after the type id, including a 32-byte player name.
// The name has no guaranteed terminator, although executeClient consumes it as a C string.
// m_word34 remains [UNRESOLVED] but occupies 16 bits on the wire.
bool CreatePlayerEvent::deSerialize(io::BitStream* stream)
{
    int teamId = 0;
    stream->readBits(&teamId, 3);
    m_teamId = static_cast<unsigned char>(teamId);

    int spawnGroupId = 0;
    stream->readBits(&spawnGroupId, 4);
    m_spawnGroupId = static_cast<unsigned char>(spawnGroupId);

    int isRemote = 0;
    stream->readBits(&isRemote, 1);
    m_isRemote = (isRemote == 1);

    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = static_cast<unsigned char>(playerId);

    int playerNetId = 0;
    stream->readBits(&playerNetId, 16);
    m_playerNetId = static_cast<unsigned short>(playerNetId);

    int word34 = 0;
    stream->readBits(&word34, 16);
    m_word34 = static_cast<unsigned short>(word34);

    int isAIPlayer = 0;
    stream->readBits(&isAIPlayer, 1);
    m_isAIPlayer = (isAIPlayer == 1);

    stream->readBits(m_playerName, 256);        // 32 bytes, no terminator - see above

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool CreatePlayerEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int teamId = m_teamId;
    stream->writeBits(&teamId, 3);

    int spawnGroupId = m_spawnGroupId;
    stream->writeBits(&spawnGroupId, 4);

    if (m_isRemote) { int one = 1; stream->writeBits(&one,  1); }
    else            { int zero = 0; stream->writeBits(&zero, 1); }

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int playerNetId = m_playerNetId;
    stream->writeBits(&playerNetId, 16);

    int word34 = m_word34;
    stream->writeBits(&word34, 16);

    int aiTrue = 1, aiFalse = 0;                // the pointer-select - see above
    int* aiSlot = m_isAIPlayer ? &aiTrue : &aiFalse;
    stream->writeBits(aiSlot, 1);

    stream->writeBits(m_playerName, 256);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Creates and registers a client-side player, ignoring calls before the client connection exists.
// Local setup repeats the local-player registration and spawn-group reset. Player creation failure is fatal.
// Requires player/network managers and a valid networkable interface. The name buffer must be terminated.
// [CHOSEN] The interface's void return discards the original handled result.
void CreatePlayerEvent::executeClient()
{
    IGameClient* const client = static_cast<IGameClient*>(game->queryInterface(IID_IGameClient));
    if (!client)
        return;                                          // the dedicated server stops here, always

    IClientConnection* const connection = client->getConnection();          // IGameClient +0x20
    if (!connection)
        return;                                          // the ELF's `return 1`

    std::string name(m_playerName);                      // not NUL-terminated by the wire
    world::IPlayer* const player =
        world::playerManager->createPlayer(world::CID_Player, name, m_playerId);   // +0x74
    if (!player)
    {
        Debug(DEBUGTYPE_6, "Game/GameEvents/CreatePlayerEvent.cpp", 157, "")
            << "Failed to create requested player";
        return;                                          // the ELF's `return 0`
    }

    if (m_isRemote)
    {
        player->setIsRemote(true);                       // +0x58
        player->setIsAIPlayer(m_isAIPlayer);             // +0x60
    }
    else
    {
        world::playerManager->setLocalHumanPlayer(player);   // +0x34
        player->setSpawnGroup(0);                        // +0x154
        player->setIsRemote(false);
        connection->setPlayer(player);                   // IClientConnection +0x1C
        world::playerManager->setLocalHumanPlayer(player);   // again, as shipped
    }

    player->setTeam(m_teamId, false);                    // +0xE4
    player->setSpawnGroup(0);                            // +0x154

    io::INetworkable* const networkable =
        static_cast<io::INetworkable*>(player->queryInterface(io::IID_INetworkable));   // Core/Ids.h, namespace io
    io::networkManager->addNetworkable(networkable, m_playerNetId);
}

int CreatePlayerEvent::getType() const
{
    return GECreatePlayer;
}

} // namespace hfe
} // namespace dice
