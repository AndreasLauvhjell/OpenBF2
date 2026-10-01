#include "StringBlockEvent.h"

#include <cstring>                               // memcpy / memset

#include "../IGame.h"                            // queryInterface
#include "../IGameClient.h"
#include "../IGameServer.h"                      // getConnection
#include "../../Core/Debug.h"                    // the constructor's abort
#include "../../Core/Ids.h"                      // IID_IGameClient / IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits / skipBits
#include "../Common/IClientConnection.h"
#include "../Common/StringManager.h"
namespace dice {
namespace hfe {

namespace {
// .rodata dword GEStringBlock (0x888A844) = 15.
const int GEStringBlock = 15;

// Dispatches through the connection's required StringManager; trustSenderId controls sender attribution.
void dispatchToStringManager(IClientConnection* connection, bool newString,
                             unsigned char stringSize,
                             void* data, unsigned char dataSize, bool trustSenderId)
{
    StringManager* manager = connection->getStringManager();
    if (newString)
        manager->newStringDataBlock(stringSize);
    else
        manager->addStringDataChunk(data, dataSize, trustSenderId);
}
} // namespace

//  (C2) / 0x807D3E0 (C1), 335 B. The chunk form. The bound check aborts and does not
// clamp - see the header.
StringBlockEvent::StringBlockEvent(void* data, unsigned char size)
    : GameEvent()
    , m_stringSize(0)
{
    if (size > MAX_CHUNK_SIZE)
    {
        Debug(DEBUGTYPE_6, "Game/GameEvents/StringBlockEvent.cpp", 26, "")
            << " Data out of range";             // type 6 aborts - and does NOT clamp
    }

    m_newString = false;                                     // +0x0C
    m_dataSize  = size;                                      // +0x0E
    memcpy(m_dataBuffer, data, size);                        // +0x0F
}

// The announce form - inlined in StringManager::update, no symbol of its own. See the header.
StringBlockEvent::StringBlockEvent(unsigned char stringSize)
    : GameEvent()
    , m_newString(true)                                      // +0x0C
    , m_stringSize(stringSize)                               // +0x0D
    , m_dataSize(0)                                          // +0x0E - no payload on an announcement
{
}

// The maker's form. The ELF has no such constructor.
StringBlockEvent::StringBlockEvent()
    : GameEvent()
    , m_newString(false)
    , m_stringSize(0)
    , m_dataSize(0)
{
    memset(m_dataBuffer, 0, MAX_CHUNK_SIZE);
}

int StringBlockEvent::getType() const
{
    return GEStringBlock;
}

// Delivers string data when connected, preserving the sender id supplied by the server.
void StringBlockEvent::executeClient()
{
    IGameClient* gameClient = static_cast<IGameClient*>(game->queryInterface(IID_IGameClient));
    if (!gameClient)
        return;

    IClientConnection* connection = gameClient->getConnection();
    if (!connection)
        return;

    dispatchToStringManager(connection, m_newString, m_stringSize,
                            m_dataBuffer, m_dataSize, true);           // true here - trusts the id
}

// executeServer - 0x807D7A0, 151 B. The same body reached through
// IGameServer::getConnection(connectionId), and passing false as the third argument.
bool StringBlockEvent::executeServer(int connectionId)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    IClientConnection* connection = server->getConnection(connectionId);   // +0x3C
    if (!connection)
        return true;                             // No connection is not an error.

    dispatchToStringManager(connection, m_newString, m_stringSize,
                            m_dataBuffer, m_dataSize, false);          // false here - re-attributes
    return true;
}

// [CHOSEN, not recovered] Bounds chunk storage while consuming the complete wire payload.
// Announcement size and chunk size are distinct fields.
bool StringBlockEvent::deSerialize(io::BitStream* stream)
{
    int newString = 0;
    stream->readBits(&newString, 1);
    m_newString = (newString == 1);

    if (!m_newString)
    {
        int size = 0;
        stream->readBits(&size, 8);

        unsigned int onTheWire = static_cast<unsigned int>(size) & 0xFFu;
        unsigned int stored    = onTheWire;
        if (stored > static_cast<unsigned int>(MAX_CHUNK_SIZE))
            stored = MAX_CHUNK_SIZE;             // the divergence - see above

        m_dataSize = static_cast<unsigned char>(stored);
        stream->readBits(m_dataBuffer, stored * 8);
        if (onTheWire > stored)
            stream->skipBits((onTheWire - stored) * 8);      // keep the cursor where the ELF puts it
    }
    else
    {
        int stringSize = 0;
        stream->readBits(&stringSize, 8);
        m_stringSize = static_cast<unsigned char>(stringSize);
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x807D600, 260 B. The flag is written in its own if/else and then tested a second
// time to pick the payload, exactly as in DataBlockEvent.
bool StringBlockEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int newString = m_newString ? 1 : 0;
    stream->writeBits(&newString, 1);

    if (m_newString)
    {
        int stringSize = m_stringSize;
        stream->writeBits(&stringSize, 8);
    }
    else
    {
        int dataSize = m_dataSize;
        stream->writeBits(&dataSize, 8);
        stream->writeBits(m_dataBuffer, static_cast<unsigned int>(m_dataSize) * 8);
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
