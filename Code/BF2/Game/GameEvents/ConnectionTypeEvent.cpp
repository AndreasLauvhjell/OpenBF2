#include "ConnectionTypeEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../../Core/Debug.h"                    // the inlined range assert
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEConnectionType (0x8888AA4) = 3.
const int GEConnectionType = 3;

// Returns bits needed for highest, with a minimum width of one even for zero.
unsigned int bitsToHold(unsigned int highest)
{
    unsigned int bits = 0;
    do
        ++bits;
    while (((1u << bits) - 1u) < highest);
    return bits;
}

// Writes a bounded value; out-of-range input sets the stream error and reports a
// fatal diagnostic. [UNRESOLVED] Original diagnostic function-name text; an empty
// string is used until identified.
void writeRangedBits(io::BitStream* stream, unsigned int value,
                     unsigned int lower, unsigned int upper)
{
    if (value > upper)
    {
        stream->setError(4);                     // reachedEndOfStream = 1, errorCode = 4
        Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
            << "(fatal error) Value out of bounds val: " << value
            << " lower " << lower
            << " upper: " << upper;
        return;                                  // the value is NOT written
    }

    int scratch = static_cast<int>(value);
    stream->writeBits(&scratch, bitsToHold(upper + 1));
}
} // namespace

// [INFERRED] - no ELF constructor exists.
ConnectionTypeEvent::ConnectionTypeEvent(ConnectionType type)
    : GameEvent()
    , m_connectionType(static_cast<unsigned char>(static_cast<int>(type)))
{
}

int  ConnectionTypeEvent::getType() const { return GEConnectionType; }
void ConnectionTypeEvent::logInfo(bool, int) {}

// Stores the packet's connection type before recalculating bandwidth. Uses the
// transport connection ID; the three-bit type is otherwise unvalidated, including 7.
bool ConnectionTypeEvent::executeServer(int connectionId)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    server->setConnectionType(connectionId,
                              static_cast<ConnectionType>(m_connectionType));   // +0x30
    server->chokeBandwidth();                                                   // +0x34
    return true;
}

// Reads three bits without range validation, accepting value 7 even though serialization rejects it.
bool ConnectionTypeEvent::deSerialize(io::BitStream* stream)
{
    const unsigned int bits = bitsToHold(CONNECTION_TYPE_COUNT);

    int connectionType = 0;
    stream->readBits(&connectionType, bits);
    m_connectionType = static_cast<unsigned char>(connectionType);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x806CFB0, 523 B. It is 523 bytes because ~440 of them are the inlined range assert
// above - two std::string constructions, a Debug object, five stream insertions and two COW
// releases. The actual payload is three bits.
bool ConnectionTypeEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    writeRangedBits(stream, m_connectionType, 0, CONNECTION_TYPE_MAX);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
