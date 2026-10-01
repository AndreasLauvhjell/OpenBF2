// End-of-round metadata serialization for server delivery and client display.
#include "EndOfRoundInfo.h"

#include "../Core/Debug.h"
#include "../Core/StringUtil.h"
#include "../IO/BitStream.h"
#include "../Scene/Object/PlayerManager.h"
#include "../Scene/Object/PlayerScoreData.h"
#include "GameLogic/TicketHistory.h"
#include "IGameLogic.h"
#include "Objects/IPlayer.h"

#include "zlib.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <list>

namespace dice {
namespace hfe {

namespace {

// The retail body emits this sign/magnitude pair longhand for every signed field. Keeping the
// helper local preserves the exact write order while making the repeated source idiom explicit.
void writeSignedMagnitude(io::BitStream& stream, int value, unsigned int magnitudeBits)
{
    unsigned int sign = value < 0 ? 1u : 0u;
    unsigned int magnitude = value < 0
        ? 0u - static_cast<unsigned int>(value)
        : static_cast<unsigned int>(value);
    stream.writeBits(&sign, 1);
    stream.writeBits(&magnitude, magnitudeBits);
}

void writeBoundedString(io::BitStream& stream, const std::string& text, unsigned int limit)
{
    const unsigned int length = std::min<unsigned int>(static_cast<unsigned int>(text.size()), limit);
    stream.writeBits(&length, 8);
    for (unsigned int i = 0; i < length; ++i)
    {
        const unsigned int character = static_cast<unsigned char>(text[i]);
        stream.writeBits(&character, 8);
    }
}

int readSignedMagnitude(io::BitStream& stream, unsigned int magnitudeBits)
{
    unsigned int sign = 0;
    stream.readBits(&sign, 1);
    unsigned int magnitude = 0;
    stream.readBits(&magnitude, magnitudeBits);
    return sign == 1 ? -static_cast<int>(magnitude) : static_cast<int>(magnitude);
}

// Ticket strings are an eight-bit length followed by individual bytes. The retail reader zeroes a
// 256-byte scratch buffer and then assigns strlen(buffer) bytes, so an embedded NUL truncates the
// received string. The one-byte length makes the final byte remain zero even at the maximum 255.
void readTicketString(io::BitStream& stream, std::string& out)
{
    unsigned int length = 0;
    stream.readBits(&length, 8);

    char buffer[256] = {0};
    for (unsigned int index = 0; index < length; ++index)
    {
        unsigned int character = 0;
        stream.readBits(&character, 8);
        buffer[index] = static_cast<char>(character);
    }
    out.assign(buffer, std::strlen(buffer));
}

} // anonymous namespace

EndOfRoundInfo::EndOfRoundInfo()
    : m_data()
    // m_delay is deliberately not initialised: both shipped constructors leave +0x04 untouched.
    , m_dirty(true)
    , m_netBuffer()
{
}

void EndOfRoundInfo::clear()
{
    // Shipped quirk: clear does not dirty the already-generated buffer.
    m_data = getEmptyString();
    m_delay = 0;
}

const std::string& EndOfRoundInfo::getData() const
{
    return m_data;
}

void EndOfRoundInfo::setData(const std::string& data)
{
    m_data = data;
    m_dirty = true;
}

int EndOfRoundInfo::getDelay()
{
    return m_delay;
}

void EndOfRoundInfo::setDelay(int delay)
{
    m_dirty = true;
    m_delay = delay;
}

const void* EndOfRoundInfo::getNetBuffer()
{
    if (m_dirty)
        updateNetBuffer();
    return m_netBuffer.data();
}

unsigned int EndOfRoundInfo::getNetBufferSize()
{
    if (m_dirty)
        updateNetBuffer();
    return static_cast<unsigned int>(m_netBuffer.size());
}

// Restores script payload, delay, known-player scores, and ticket history. Stream
// overread is reported after consumption; advertised zlib lengths are allocated
// without a size preflight, and decompression failure aborts.
void EndOfRoundInfo::setFromDataBlock(const void* data, unsigned int size)
{
    unsigned char* const input = new unsigned char[size + 5];
    std::memcpy(input, data, size);

    io::BitStream stream(input, size);
    stream.setReadMode();
    stream.setBitSize(8 * size);

    uLongf uncompressedLength = 0;
    stream.readBits(&uncompressedLength, 32);

    unsigned int compressedLength = 0;
    stream.readBits(&compressedLength, 32);
    unsigned char* const compressed = new unsigned char[compressedLength];
    stream.readBits(compressed, 8 * compressedLength);

    m_delay = readSignedMagnitude(stream, 31);

    unsigned char* const uncompressed = new unsigned char[uncompressedLength];
    if (uncompress(uncompressed, &uncompressedLength, compressed,
                   static_cast<uLong>(compressedLength)) != Z_OK)
    {
        std::abort();
    }

    const std::string decoded(reinterpret_cast<const char*>(uncompressed),
                              static_cast<std::size_t>(uncompressedLength));
    m_data = decoded;
    delete[] compressed;
    delete[] uncompressed;

    int playerCount = 0;
    stream.readBits(&playerCount, 8);
    while (playerCount > 0)
    {
        int playerId = 0;
        stream.readBits(&playerId, 8);
        world::IPlayer* const player = world::playerManager->getPlayerFromId(playerId);

        const int field14 = readSignedMagnitude(stream, 15);
        const int field08 = readSignedMagnitude(stream, 15);
        const int field10 = readSignedMagnitude(stream, 15);
        const int score   = readSignedMagnitude(stream, 15);
        const int field04 = readSignedMagnitude(stream, 15);
        const int field0C = readSignedMagnitude(stream, 15);

        if (player)
        {
            world::PlayerScoreData* const scoreData = player->getScoreData();
            if (scoreData)
            {
                scoreData->m_field14 = field14;
                scoreData->m_field08 = field08;
                scoreData->m_field10 = field10;
                scoreData->m_score   = score;
                scoreData->m_field04 = field04;
                scoreData->m_field0C = field0C;
            }
        }
        --playerCount;
    }

    TicketHistory& history = gameLogic->getTicketHistory();
    history.tickets.erase(history.tickets.begin(), history.tickets.end());

    unsigned int historyCount = 0;
    stream.readBits(&historyCount, 8);
    for (unsigned int index = 0; index < historyCount; ++index)
    {
        TicketHistory::Ticket ticket;

        unsigned int value = 0;
        stream.readBits(&value, 8);
        ticket.dword8 = static_cast<int>(value);
        ticket.dwordC = readSignedMagnitude(stream, 7);
        value = 0;
        stream.readBits(&value, 8);
        ticket.dword10 = static_cast<int>(value);

        readTicketString(stream, ticket.string0);
        readTicketString(stream, ticket.string4);
        for (unsigned int team = 0; team < 2; ++team)
        {
            readTicketString(stream, ticket.pair[team].text);
            ticket.pair[team].value = readSignedMagnitude(stream, 31);
        }

        // Retail re-dispatches getTicketHistory for every appended record.
        gameLogic->getTicketHistory().tickets.push_back(ticket);
    }

    const bool streamError = stream.hasError();
    stream.clearError();
    if (streamError)
    {
        Debug(DEBUGTYPE_7, "Game/EndOfRoundInfo.cpp", 149)
            << "deserialize of EndOfRoundInfo failed";
    }

    delete[] input;
}

// Writes script payload, signed delay, player scores, and up to twenty recent ticket-history records.
// The packed buffer is delivered as server data block 8.
void EndOfRoundInfo::updateNetBuffer()
{
    static const unsigned int kStagingBytes = 7166;
    m_netBuffer.resize(kStagingBytes, 0);
    io::BitStream stream(&m_netBuffer[0], static_cast<unsigned int>(m_netBuffer.size()));

    const uLong sourceSize = static_cast<uLong>(m_data.size());
    uLongf compressedSize =
        static_cast<uLongf>(static_cast<double>(sourceSize) * 1.001 + 1.0) + 12;
    unsigned char* compressed = new unsigned char[compressedSize];
    if (compress2(compressed, &compressedSize,
                  reinterpret_cast<const Bytef*>(m_data.data()), sourceSize,
                  Z_DEFAULT_COMPRESSION) != Z_OK
        || compressedSize > 0x1000u)
    {
        std::abort();
    }

    const unsigned int uncompressedLength = static_cast<unsigned int>(sourceSize);
    const unsigned int compressedLength = static_cast<unsigned int>(compressedSize);
    stream.writeBits(&uncompressedLength, 32);
    stream.writeBits(&compressedLength, 32);
    stream.writeBits(compressed, compressedLength * 8);
    delete[] compressed;

    writeSignedMagnitude(stream, m_delay, 31);

    const std::list<world::IPlayer*>& players = world::playerManager->getPlayers();
    const unsigned int playerCount = static_cast<unsigned int>(players.size());
    stream.writeBits(&playerCount, 8);
    for (std::list<world::IPlayer*>::const_iterator it = players.begin(); it != players.end(); ++it)
    {
        world::IPlayer* const player = *it;
        const int playerId = player->getId();
        stream.writeBits(&playerId, 8);

        world::PlayerScoreData* const score = player->getScoreData();
        writeSignedMagnitude(stream, score->m_field14, 15);
        writeSignedMagnitude(stream, score->m_field08, 15);
        writeSignedMagnitude(stream, score->m_field10, 15);
        writeSignedMagnitude(stream, score->m_score,   15);
        writeSignedMagnitude(stream, score->m_field04, 15);
        writeSignedMagnitude(stream, score->m_field0C, 15);
    }

    const TicketHistory& sizedHistory = gameLogic->getTicketHistory();
    const unsigned int historySize = static_cast<unsigned int>(sizedHistory.tickets.size());
    const unsigned int historyCount = std::min<unsigned int>(historySize, 20);
    stream.writeBits(&historyCount, 8);

    int firstHistoryIndex = static_cast<int>(historySize - historyCount);
    if (firstHistoryIndex <= 0)
        firstHistoryIndex = 0;
    for (unsigned int index = static_cast<unsigned int>(firstHistoryIndex); index < historySize; ++index)
    {
        // Retail re-dispatches getTicketHistory for every indexed record after the initial size read.
        const TicketHistory::Ticket& ticket = gameLogic->getTicketHistory().tickets[index];
        stream.writeBits(&ticket.dword8, 8);
        writeSignedMagnitude(stream, ticket.dwordC, 7);
        stream.writeBits(&ticket.dword10, 8);
        writeBoundedString(stream, ticket.string0, 32);
        writeBoundedString(stream, ticket.string4, 32);
        for (unsigned int team = 0; team < 2; ++team)
        {
            writeBoundedString(stream, ticket.pair[team].text, 16);
            writeSignedMagnitude(stream, ticket.pair[team].value, 31);
        }
    }

    const bool streamError = stream.hasError();
    stream.clearError();
    if (streamError)
    {
        Debug(DEBUGTYPE_7, "Game/EndOfRoundInfo.cpp", 268)
            << "dep: stream write size mismatch, check data size for members of EndOfRoundInfo";
    }

    m_netBuffer.resize((stream.getBitPosition() + 7) >> 3, 0);
    m_dirty = false;
}

} // namespace hfe
} // namespace dice
