// Serializes map metadata using length-prefixed mode/path/name strings, followed by settings.
// String limits are 32, 256 and 64 characters. Signed fields use sign-and-magnitude encoding.

#include "MapInfo.h"

#include "../Core/StringUtil.h"     // getEmptyString - 0x805F680 uses it
#include "../IO/BitStream.h"

namespace dice {
namespace hfe {

// ctor - ELF 0x805F610, 105 bytes. Three empty strings; the scalar defaults; +0x18/+0x19 left
// uninitialised exactly as shipped (our members are equally uninitialised here).
MapInfo::MapInfo()
{
    m_maxNumberOfPlayers = 16;          // : [this+0x0C] = 16
    m_commanderEnabled   = true;        // [this+0x10] = 1  commander ON by default
    m_challengeOrdinal   = 0;           // [this+0x14] = 0
    m_netBufferDirty     = true;        // [this+0x1A] = 1
    // m_field18 / m_field19: NOT initialised by the ELF ctor - preserved.
    // m_netBuffer: the three dwords +0x1C/+0x20/+0x24 are zeroed = an empty vector, which the
    // std::vector default ctor already is.
}

// Clears strings and resets scalar metadata, retaining the challenge ordinal and m_field19.
void MapInfo::clear()
{
    m_gameMode  = getEmptyString();     // : three assigns from the shared empty string
    m_levelPath = getEmptyString();
    m_mapName   = getEmptyString();
    m_maxNumberOfPlayers = 16;          // [+0x0C] = 0x10
    m_commanderEnabled   = true;        // [+0x10] = 1  commander ON by default
    m_field18            = false;       // [+0x18] = 0
    m_netBufferDirty     = true;        // [+0x1A] = 1
}

// accessors - offsets proven in MapInfo.h; the four dirty-marking setters and the one that
// does not (setChallengeOrdinal - the shipped BUG, see MapInfo.h)
const std::string& MapInfo::getGameMode() const  { return m_gameMode;  }
const std::string& MapInfo::getLevelPath() const { return m_levelPath; }
const std::string& MapInfo::getMapName() const   { return m_mapName;   }
int  MapInfo::getMaxNumberOfPlayers() const      { return m_maxNumberOfPlayers; }
int  MapInfo::getChallengeOrdinal() const        { return m_challengeOrdinal;   }

void MapInfo::setGameMode(const std::string& v)  { m_gameMode  = v; m_netBufferDirty = true; }
void MapInfo::setLevelPath(const std::string& v) { m_levelPath = v; m_netBufferDirty = true; }
void MapInfo::setMapName(const std::string& v)   { m_mapName   = v; m_netBufferDirty = true; }
void MapInfo::setMaxNumberOfPlayers(int v)       { m_maxNumberOfPlayers = v; m_netBufferDirty = true; }

// Does NOT mark dirty - a client that receives a MapInfo after only the ordinal changed gets the
// OLD ordinal. Defect in shipped BF2; reproduced as found (MapInfo.h has the full account).
void MapInfo::setChallengeOrdinal(int v)         { m_challengeOrdinal = v; }

// The net-buffer readers - both rebuild first if dirty (0x805FF30 / 0x805FF60).
const void* MapInfo::getNetBuffer()
{
    if (m_netBufferDirty)
        updateNetBuffer();
    return m_netBuffer.empty() ? 0 : &m_netBuffer[0];
}

int MapInfo::getNetBufferSize()
{
    if (m_netBufferDirty)
        updateNetBuffer();
    return (int)m_netBuffer.size();     // ELF: [this+0x20] - [this+0x1C]
}

// Serializes into a 369-byte scratch buffer, then shrinks it to the active bitstream cursor.
void MapInfo::updateNetBuffer()
{
    m_netBuffer.resize(369);                                     // _M_fill_insert / erase to 0x171

    io::BitStream stream(m_netBuffer.empty() ? 0 : &m_netBuffer[0],
                         (unsigned int)m_netBuffer.size());

    // Each string: truncate (string(src, 0, max)), write length as 16 bits, then the bytes.
    {
        std::string s(m_gameMode, 0, 0x20);                      // 32 chars max
        unsigned int len = (unsigned int)s.length();
        stream.writeBits(&len, 16);
        stream.writeBits(s.c_str(), 8 * len);
    }
    {
        std::string s(m_levelPath, 0, 0x100);                    // 256 chars max
        unsigned int len = (unsigned int)s.length();
        stream.writeBits(&len, 16);
        stream.writeBits(s.c_str(), 8 * len);
    }
    {
        std::string s(m_mapName, 0, 0x40);                       // 64 chars max
        unsigned int len = (unsigned int)s.length();
        stream.writeBits(&len, 16);
        stream.writeBits(s.c_str(), 8 * len);
    }

    // maxNumberOfPlayers, sign-magnitude: 1 sign bit + 31 magnitude bits.
    {
        unsigned int sign = (m_maxNumberOfPlayers < 0) ? 1u : 0u;
        unsigned int mag  = (m_maxNumberOfPlayers < 0) ? (unsigned int)(-m_maxNumberOfPlayers)
                                                       : (unsigned int)m_maxNumberOfPlayers;
        stream.writeBits(&sign, 1);
        stream.writeBits(&mag, 31);
    }
    {
        unsigned int b = m_commanderEnabled ? 1u : 0u;   // +0x10 goes on the wire
        stream.writeBits(&b, 1);
    }
    // challengeOrdinal, sign-magnitude like maxPlayers.
    {
        unsigned int sign = (m_challengeOrdinal < 0) ? 1u : 0u;
        unsigned int mag  = (m_challengeOrdinal < 0) ? (unsigned int)(-m_challengeOrdinal)
                                                     : (unsigned int)m_challengeOrdinal;
        stream.writeBits(&sign, 1);
        stream.writeBits(&mag, 31);
    }
    {
        unsigned int b = m_field18 ? 1u : 0u;
        stream.writeBits(&b, 1);
    }
    {
        unsigned int b = m_field19 ? 1u : 0u;
        stream.writeBits(&b, 1);
    }

    // Shrink to the written byte count. The ELF reads the +0x14 write cursor - or the +0x10 read
    // cursor when the +0x0C mode switch is 1, which a freshly-constructed write-only stream never
    // is (see BitStream.h's accessor note).
    unsigned int bits = stream.getWriteBitPosition();
    m_netBuffer.resize((bits + 7) >> 3);

    m_netBufferDirty = false;
}

// Marks the network buffer dirty before changing commander availability.
void MapInfo::setCommanderEnabled(bool value)
{
    m_netBufferDirty   = true;                                   // +0x1A  first
    m_commanderEnabled = value;                                  // +0x10
}

// see the header. No ELF body.
bool MapInfo::getCommanderEnabled() const
{
    return m_commanderEnabled;
}

namespace {
// The three read helpers setFromDataBlock needs, spelled the way the ELF unrolls them (zeroed int,
// readBits, narrow). ServerInfo.cpp and ClientInfo.cpp carry their own copies of the same three;
// each TU had them inlined in the binary, so each TU keeps them.
inline bool readBool(io::BitStream& stream)
{
    int bit = 0;
    stream.readBits(&bit, 1);
    return bit == 1;
}
inline int readSigned(io::BitStream& stream)                    // 1 sign bit + 31 magnitude bits
{
    int sign = 0;
    stream.readBits(&sign, 1);
    int magnitude = 0;
    stream.readBits(&magnitude, 31);
    return sign == 1 ? -magnitude : magnitude;
}
inline void readString(io::BitStream& stream, std::string& out)  // u16 length + bytes; 0 -> resize(0)
{
    int length = 0;
    stream.readBits(&length, 16);
    const unsigned short n = static_cast<unsigned short>(length);
    if (n)
    {
        std::vector<unsigned char> buffer(n, 0);
        stream.readBits(&buffer[0], 8u * n);
        out.assign(reinterpret_cast<const char*>(&buffer[0]), n);
    }
    else
        out.resize(0);
}
} // namespace

// Reads server map information in serialization order. Does not inspect stream
// errors or change m_netBufferDirty. [UNRESOLVED] The two final Boolean meanings.
void MapInfo::setFromDataBlock(const void* data, unsigned int size)
{
    io::BitStream stream(const_cast<unsigned char*>(static_cast<const unsigned char*>(data)), size);
    stream.setReadMode();
    stream.setBitSize(8 * size);

    readString(stream, m_gameMode);                  // +0x00
    readString(stream, m_levelPath);                 // +0x04
    readString(stream, m_mapName);                   // +0x08
    m_maxNumberOfPlayers = readSigned(stream);       // +0x0C
    m_commanderEnabled   = readBool(stream);         // +0x10
    m_challengeOrdinal   = readSigned(stream);       // +0x14
    m_field18            = readBool(stream);         // +0x18
    m_field19            = readBool(stream);         // +0x19
}

} // namespace hfe
} // namespace dice
