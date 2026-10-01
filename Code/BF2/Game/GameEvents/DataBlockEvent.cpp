#include "DataBlockEvent.h"

#include <cstring>                               // memcpy / memset

#include "../IGame.h"                            // queryInterface
#include "../IGameClient.h"
#include "../IGameServer.h"                      // getConnection
#include "../../Core/Debug.h"                    // the constructor's abort
#include "../../Core/Ids.h"                      // IID_IGameClient / IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits / skipBits
#include "../Common/IClientConnection.h"
#include "../Common/DataBlockManager.h"
namespace dice {
namespace hfe {

namespace {
// .rodata dword GEDataBlock (0x8889064) = 4.
const int GEDataBlock = 4;

// Dispatches block announcements/chunks through typed interfaces. Each branch
// performs its own manager lookup; do not hoist it across the branch.
void dispatchToDataBlockManager(IClientConnection* connection, bool isNewBlock,
                                unsigned int blockEventId, unsigned int blockLength,
                                void* data, unsigned char size)
{
    if (isNewBlock)
    {
        DataBlockManager* manager = connection->getDataBlockManager();
        manager->newDataBlock(blockEventId, blockLength);
    }
    else
    {
        DataBlockManager* manager = connection->getDataBlockManager();
        manager->addDataChunk(data, size);
    }
}
} // namespace

// Copies a data chunk; oversized input aborts instead of being clamped.
DataBlockEvent::DataBlockEvent(void* data, unsigned int size)
    : GameEvent()
    , m_blockEventId(0)
    , m_blockLength(0)
{
    if (size > MAX_CHUNK_SIZE)
    {
        Debug(DEBUGTYPE_6, "Game/GameEvents/DataBlockEvent.cpp", 26, "")
            << " Data out of range";             // type 6 aborts - and does NOT clamp
    }

    m_isNewBlock = false;                                    // +0x0C
    m_size = static_cast<unsigned char>(size);               // +0x18
    memcpy(m_data, data, size);                              // +0x19
}

// The maker's form. The ELF has no such constructor.
// The announce form - inlined in DataBlockManager::update, no symbol of its own. See the header.
DataBlockEvent::DataBlockEvent(unsigned int blockEventId, unsigned int blockLength)
    : GameEvent()
    , m_isNewBlock(true)                 // +0x0C
    , m_blockEventId(blockEventId)       // +0x10
    , m_blockLength(blockLength)         // +0x14
    , m_size(0)                          // +0x18 - no payload on an announcement
{
}

DataBlockEvent::DataBlockEvent()
    : GameEvent()
    , m_isNewBlock(false)
    , m_blockEventId(0)
    , m_blockLength(0)
    , m_size(0)
{
    memset(m_data, 0, MAX_CHUNK_SIZE);
}

int DataBlockEvent::getType() const
{
    return GEDataBlock;
}

// Delivers the announcement or chunk when the client has a connection.
void DataBlockEvent::executeClient()
{
    IGameClient* gameClient = static_cast<IGameClient*>(game->queryInterface(IID_IGameClient));
    if (!gameClient)
        return;

    IClientConnection* connection = gameClient->getConnection();
    if (!connection)
        return;

    dispatchToDataBlockManager(connection, m_isNewBlock,
                               m_blockEventId, m_blockLength, m_data, m_size);
}

// executeServer - 0x806FCB0, 145 B. The same body with the connection reached through
// IGameServer::getConnection(connectionId) (+0x3C) instead. Byte-for-byte the same length as the
// client arm, which is what makes the shared tail obvious.
bool DataBlockEvent::executeServer(int connectionId)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    IClientConnection* connection = server->getConnection(connectionId);   // +0x3C
    if (!connection)
        return true;                             // No connection is not an error.

    dispatchToDataBlockManager(connection, m_isNewBlock,
                               m_blockEventId, m_blockLength, m_data, m_size);
    return true;
}

// Decodes an announcement or chunk. [CHOSEN, not recovered] Limits chunk storage
// to the local buffer and consumes excess bytes to preserve following event alignment.
bool DataBlockEvent::deSerialize(io::BitStream* stream)
{
    int isNewBlock = 0;
    stream->readBits(&isNewBlock, 1);
    m_isNewBlock = (isNewBlock == 1);

    if (!m_isNewBlock)
    {
        int size = 0;
        stream->readBits(&size, 8);

        unsigned int onTheWire = static_cast<unsigned int>(size) & 0xFFu;
        unsigned int stored    = onTheWire;
        if (stored > static_cast<unsigned int>(MAX_CHUNK_SIZE))
            stored = MAX_CHUNK_SIZE;             // the divergence - see above

        m_size = static_cast<unsigned char>(stored);
        stream->readBits(m_data, stored * 8);
        if (onTheWire > stored)
            stream->skipBits((onTheWire - stored) * 8);      // keep the cursor where the ELF puts it
    }
    else
    {
        int blockEventId = 0;
        stream->readBits(&blockEventId, 32);
        m_blockEventId = static_cast<unsigned int>(blockEventId);

        int blockLength = 0;
        stream->readBits(&blockLength, 32);
        m_blockLength = static_cast<unsigned int>(blockLength);
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// THE flag IS  IN ITS OWN if/else AND then tested again. The ELF emits the bit through two
// separate branches and then branches a second time on the same value to pick the payload - it does
// not fold them. Reproduced as two statements for the same reason: the shape is what the binary
// has, and the second test reads a variable the compiler had already cached in bl.
bool DataBlockEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int isNewBlock = m_isNewBlock ? 1 : 0;
    stream->writeBits(&isNewBlock, 1);

    if (m_isNewBlock)
    {
        int blockEventId = static_cast<int>(m_blockEventId);
        stream->writeBits(&blockEventId, 32);

        int blockLength = static_cast<int>(m_blockLength);
        stream->writeBits(&blockLength, 32);
    }
    else
    {
        int size = m_size;
        stream->writeBits(&size, 8);
        stream->writeBits(m_data, static_cast<unsigned int>(m_size) * 8);
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
