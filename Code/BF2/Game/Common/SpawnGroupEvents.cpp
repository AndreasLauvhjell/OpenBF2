// Spawn-group creation and removal events.
#include "SpawnGroupEvents.h"

#include "../../IO/BitStream.h"              // readBits / writeBits

#include "../IGame.h"                              // game->queryInterface(IID_IGameClient)
#include "../GameLogic/SpawnSystem/SpawnManager.h" // the client arms' createClientSpawnGroup / removeClientSpawnGroup
#include "../../Core/Ids.h"

namespace dice {
namespace hfe {

extern IBase* spawnManager;                        // ds:0x8AD3A0C-ish - Core/Globals.cpp (IBase*, the tree's form)

// The GE type ids - .rodata dwords GECreateSpawnGroupEvent (0x8888FC0) = 57 and
// GERemoveSpawnGroupEvent (0x888A380) = 58.
namespace {
const int GECreateSpawnGroupEvent = 57;
const int GERemoveSpawnGroupEvent = 58;
} // namespace

// 122 B: GameEvent, the vptr, then the eight fields in the ctor's store order.
CreateSpawnGroupEvent::CreateSpawnGroupEvent(unsigned char spawnGroupId, unsigned short netId, int team,
                                             bool active, bool aiOnly, bool hasSpawnPoints,
                                             unsigned char x, unsigned char z)
{
    m_spawnGroupId = spawnGroupId;                 // +0x0C
    m_team = team;                                 // +0x10
    m_active = active;                             // +0x14
    m_aiOnly = aiOnly;                             // +0x15
    m_hasSpawnPoints = hasSpawnPoints;             // +0x16
    m_x = x;                                       // +0x17
    m_z = z;                                       // +0x18
    m_netId = netId;                               // +0x1A
}
// The maker's forms - see the header. The ELF has no such constructors.
CreateSpawnGroupEvent::CreateSpawnGroupEvent()
    : GameEvent()
    , m_spawnGroupId(0)
    , m_team(0)
    , m_active(false)
    , m_aiOnly(false)
    , m_hasSpawnPoints(false)
    , m_x(0)
    , m_z(0)
    , m_netId(0)
{
}

RemoveSpawnGroupEvent::RemoveSpawnGroupEvent()
    : GameEvent()
    , m_spawnGroupId(0)
    , m_netId(0)
{
}

int  CreateSpawnGroupEvent::getType() const           { return GECreateSpawnGroupEvent; }
void CreateSpawnGroupEvent::logInfo(bool, int)        {}
void CreateSpawnGroupEvent::eventReceivedByRemote(void*) {}

// 142 B: only a client (IID_IGameClient answered) with a spawn manager acts - it mirrors the
// group locally through createClientSpawnGroup. A dedicated server never answers IID_IGameClient.
void CreateSpawnGroupEvent::executeClient()
{
    if (game->queryInterface(IID_IGameClient) && spawnManager)
    {
        static_cast<SpawnManager*>(spawnManager)->createClientSpawnGroup(
            m_spawnGroupId, m_netId, m_team, m_active, m_aiOnly, m_hasSpawnPoints, m_x, m_z);
        // The group id it returns is discarded here, exactly as the ELF discards it.
    }
}

// 61 B.
RemoveSpawnGroupEvent::RemoveSpawnGroupEvent(unsigned char spawnGroupId, unsigned short netId)
{
    m_spawnGroupId = spawnGroupId;                 // +0x0C (a dword store of the byte)
    m_netId = netId;                               // +0x0E
}
int  RemoveSpawnGroupEvent::getType() const           { return GERemoveSpawnGroupEvent; }
void RemoveSpawnGroupEvent::logInfo(bool, int)        {}
void RemoveSpawnGroupEvent::eventReceivedByRemote(void*) {}

// 77 B: the client arm - removeClientSpawnGroup(id).
void RemoveSpawnGroupEvent::executeClient()
{
    if (game->queryInterface(IID_IGameClient) && spawnManager)
        static_cast<SpawnManager*>(spawnManager)->removeClientSpawnGroup(m_spawnGroupId);
}

// Reads fixed-width spawn-group state and quantized X/Z coordinates without rescaling them.
// The payload uses 47 bits after its event-type prefix.
bool CreateSpawnGroupEvent::deSerialize(io::BitStream* stream)
{
    int spawnGroupId = 0;
    stream->readBits(&spawnGroupId, 8);
    m_spawnGroupId = static_cast<unsigned char>(spawnGroupId);

    int team = 0;
    stream->readBits(&team, 4);
    m_team = team;

    int active = 0;
    stream->readBits(&active, 1);
    m_active = (active == 1);

    int aiOnly = 0;
    stream->readBits(&aiOnly, 1);
    m_aiOnly = (aiOnly == 1);

    int hasSpawnPoints = 0;
    stream->readBits(&hasSpawnPoints, 1);
    m_hasSpawnPoints = (hasSpawnPoints == 1);

    int x = 0;
    stream->readBits(&x, 8);
    m_x = static_cast<unsigned char>(x);

    int z = 0;
    stream->readBits(&z, 8);
    m_z = static_cast<unsigned char>(z);

    int netId = 0;
    stream->readBits(&netId, 16);
    m_netId = static_cast<unsigned short>(netId);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool CreateSpawnGroupEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int spawnGroupId = m_spawnGroupId;
    stream->writeBits(&spawnGroupId, 8);

    int team = m_team;
    stream->writeBits(&team, 4);

    if (m_active)   { int one = 1; stream->writeBits(&one,  1); }   // the if/else pair - see above
    else            { int zero = 0; stream->writeBits(&zero, 1); }

    if (m_aiOnly)   { int one = 1; stream->writeBits(&one,  1); }
    else            { int zero = 0; stream->writeBits(&zero, 1); }

    if (m_hasSpawnPoints) { int one = 1; stream->writeBits(&one,  1); }
    else                  { int zero = 0; stream->writeBits(&zero, 1); }

    int x = m_x;
    stream->writeBits(&x, 8);

    int z = m_z;
    stream->writeBits(&z, 8);

    int netId = m_netId;
    stream->writeBits(&netId, 16);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Serializes an eight-bit group ID and sixteen-bit network ID. Only those widths
// are accessed, independently of wider host declarations.
bool RemoveSpawnGroupEvent::deSerialize(io::BitStream* stream)
{
    int spawnGroupId = 0;
    stream->readBits(&spawnGroupId, 8);
    m_spawnGroupId = spawnGroupId;

    int netId = 0;
    stream->readBits(&netId, 16);
    m_netId = static_cast<unsigned short>(netId);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool RemoveSpawnGroupEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int spawnGroupId = m_spawnGroupId;
    stream->writeBits(&spawnGroupId, 8);

    int netId = m_netId;
    stream->writeBits(&netId, 16);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
