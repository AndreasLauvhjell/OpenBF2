// Client identity sent as data block 1, with a lazily regenerated wire buffer.
// Wire order is name, name hash, signed profile ID, clan tag, authentication token and VoIP flag.
// Strings have 16-bit lengths; profile IDs use sign-and-magnitude encoding.

#ifndef GAME_GAMESERVER_CLIENTINFO_H     // [INFERRED] guard style, matching ServerInfo.h
#define GAME_GAMESERVER_CLIENTINFO_H

#include <string>
#include <vector>

namespace dice {
namespace hfe {

class ClientInfo
{
public:
    ClientInfo();
    ~ClientInfo();                                       // inlined at every call site - no out-of-line symbol

    // Reads a client identity data block into this record.
    void setFromDataBlock(const void* data, unsigned int size);

    // Regenerates the serialized payload if dirty. The returned buffer belongs to this object.
    const unsigned char* getNetBuffer();
    unsigned int         getNetBufferSize();

    // Serializes the identity and clears its dirty flag.
    void updateNetBuffer();
    void setGameSpyAuth(const std::string& auth);

    // NO ELF symbols - GCC inlined both at their one call site. MSVC emitted them as COMDATs in
    // GameClient.cpp's TU (BF2.exe 4C4D00 and 4D1C50, 25 and 28 B: assign the string, set the dirty
    // byte), so they exist as functions in the client image. [INFERRED] names.
    void setPlayerName(const std::string& name) { m_playerName = name; m_shouldUpdateNetBuffer = true; }
    void setClanTag(const std::string& tag)     { m_clanTag = tag;     m_shouldUpdateNetBuffer = true; }

    /* +0x00 */ std::string   m_playerName;              // "Player"
    /* +0x04 */ std::string   m_clanTag;                 // ""
    /* +0x08 */ unsigned int  m_playerNameHash;          // 0
    /* +0x0C */ int           m_profileId;               // 1
    /* +0x10 */ std::string   m_gameSpyAuthString;       // ""
    /* +0x14 */ bool          m_usesVoip;                // false
    /* +0x15 */ bool          m_shouldUpdateNetBuffer;   // true (dirty)
    /* +0x18 */ std::vector<unsigned char> m_netBuffer;  // begin/end/capacity - the serialised block
};                                                       // sizeof 0x24

} // namespace hfe
} // namespace dice

#endif // GAME_GAMESERVER_CLIENTINFO_H
