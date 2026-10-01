// Client identity and its serialized connection payload.

#include "ClientInfo.h"

#include "../../IO/BitStream.h"                  // io::BitStream - setFromDataBlock reads through it


namespace dice {
namespace hfe {

// Creates a dirty default identity named Player, with no profile, authentication token, or VoIP.
ClientInfo::ClientInfo()
    : m_playerName("Player")
    , m_clanTag()
    , m_playerNameHash(0)
    , m_profileId(-1)
    , m_gameSpyAuthString()
    , m_usesVoip(false)
    , m_shouldUpdateNetBuffer(true)
    // m_netBuffer default-constructs empty: begin == end == capacity == 0
{
}

// The ELF inlines the destructor at every call site (there is no out-of-line dtor symbol): the vector
// frees its storage, then the three std::strings are destroyed in reverse order. Every member has its
// own destructor, so this is exactly what the compiler generates - an empty body.
ClientInfo::~ClientInfo()
{
}

namespace {

// [INFERRED helper] Reads a 16-bit length followed by that many bytes; zero length clears the string.
void readNetString(io::BitStream& stream, std::string& out)
{
    unsigned int len = 0;
    stream.readBits(&len, 16);                   // u16 length (upper bits stay zero)
    if (len)
    {
        std::vector<unsigned char> buf(len);
        stream.readBits(&buf[0], 8 * len);       // len bytes
        out.assign(reinterpret_cast<const char*>(&buf[0]), len);
    }
    else
    {
        out.resize(0);                           // ELF: std::string::resize(&s, 0, 0) - clear
    }
}


// The write half's string helper, the mirror of readNetString: a u16 length, then the
// bytes. The ELF writes the terminator into the copy's own buffer first (`*(data + len) = 0`) and
// then writes exactly `len` bytes - a no-op on a std::string, reproduced by ServerInfo.cpp's
// writeString the same way. The caller hands in the truncated copies (64 / 32 / 92 characters).
inline void writeNetString(io::BitStream& stream, const std::string& text)
{
    int length = static_cast<unsigned short>(text.size());
    stream.writeBits(&length, 16);
    stream.writeBits(text.c_str(), 8 * static_cast<unsigned int>(text.size()));
}
} // namespace

// ClientInfo::setFromDataBlock - ELF 0x8053C80, 1,615 B.  The server read path: parse a
// data-block-id-1 payload into the identity fields. A BitStream in read mode over (data, size), then
// the wire order documented in ClientInfo.h. It does NOT touch m_netBuffer or the dirty bit - this is
// the receive side; the send side (updateNetBuffer) is the mirror.
void ClientInfo::setFromDataBlock(const void* data, unsigned int size)
{
    // The buffer is read, not written; the ctor takes a non-const pointer, so cast through. Matches
    // the ELF, which hands the block straight to the BitStream ctor.
    io::BitStream stream(const_cast<unsigned char*>(static_cast<const unsigned char*>(data)), size);
    stream.setReadMode();                        // m_readWriteSwitch = 1 (position is the read cursor)
    stream.setBitSize(8 * size);

    readNetString(stream, m_playerName);         // u16 len + bytes

    unsigned int hash = 0;
    stream.readBits(&hash, 32);
    m_playerNameHash = hash;

    // profileId: 1 sign bit + 31 magnitude bits, sign == 1 negates. The recurring signed-int pattern
    // (see ServerInfo.h's "s32 = 1 sign bit + 31 magnitude bits").
    unsigned int sign = 0;
    stream.readBits(&sign, 1);
    unsigned int magnitude = 0;
    stream.readBits(&magnitude, 31);
    m_profileId = (sign == 1) ? -static_cast<int>(magnitude) : static_cast<int>(magnitude);

    readNetString(stream, m_clanTag);            // u16 len + bytes
    readNetString(stream, m_gameSpyAuthString);  // u16 len + bytes

    unsigned int voip = 0;
    stream.readBits(&voip, 1);
    m_usesVoip = (voip == 1);
}

// Regenerates a dirty payload before exposing its buffer or size.
const unsigned char* ClientInfo::getNetBuffer()
{
    if (m_shouldUpdateNetBuffer)
        updateNetBuffer();
    return m_netBuffer.empty() ? 0 : &m_netBuffer[0];
}

unsigned int ClientInfo::getNetBufferSize()
{
    if (m_shouldUpdateNetBuffer)
        updateNetBuffer();
    return static_cast<unsigned int>(m_netBuffer.size());
}

// Serializes client information and trims the buffer to the bytes written.
// Player name, clan tag, and auth token are truncated to 64, 32, and 92 bytes.
// profileId uses one sign bit and 31 magnitude bits.
void ClientInfo::updateNetBuffer()
{
    m_netBuffer.resize(198);                                          // 0xC6
    io::BitStream stream(&m_netBuffer[0], static_cast<unsigned int>(m_netBuffer.size()));

    writeNetString(stream, std::string(m_playerName, 0, 64));         // 64 characters at most

    unsigned int hash = m_playerNameHash;
    stream.writeBits(&hash, 32);

    if (m_profileId < 0)
    {
        int sign = 1;
        stream.writeBits(&sign, 1);
        int magnitude = -m_profileId;
        stream.writeBits(&magnitude, 31);
    }
    else
    {
        int sign = 0;
        stream.writeBits(&sign, 1);
        int magnitude = m_profileId;
        stream.writeBits(&magnitude, 31);
    }

    writeNetString(stream, std::string(m_clanTag, 0, 32));            // 32
    writeNetString(stream, std::string(m_gameSpyAuthString, 0, 92));  // 92 (0x5C)

    int voip = m_usesVoip ? 1 : 0;
    stream.writeBits(&voip, 1);

    const unsigned int bytes = (stream.getBitPosition() + 7) >> 3;   // the cursor the switch selects
    m_netBuffer.resize(bytes);                                        // grow with 0 / erase past it
    m_shouldUpdateNetBuffer = false;
}

// Stores a wire-safe token by replacing +, /, and = with [, ], and _, then marks the payload dirty.
void ClientInfo::setGameSpyAuth(const std::string& auth)
{
    m_gameSpyAuthString = auth;

    for (std::string::size_type i = m_gameSpyAuthString.find('+', 0); i != std::string::npos;
         i = m_gameSpyAuthString.find('+', i))
        m_gameSpyAuthString.replace(i, 1, "[");
    for (std::string::size_type i = m_gameSpyAuthString.find('/', 0); i != std::string::npos;
         i = m_gameSpyAuthString.find('/', i))
        m_gameSpyAuthString.replace(i, 1, "]");
    for (std::string::size_type i = m_gameSpyAuthString.find('=', 0); i != std::string::npos;
         i = m_gameSpyAuthString.find('=', i))
        m_gameSpyAuthString.replace(i, 1, "_");

    m_shouldUpdateNetBuffer = true;
}

} // namespace hfe
} // namespace dice
