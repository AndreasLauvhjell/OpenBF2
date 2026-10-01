// Current-level metadata and the serialized block delivered to clients.
// [INFERRED] Path beside MapList; members recovered from ELF accessors.
// This non-polymorphic type contains STL members; offsets are toolchain-specific.

#ifndef GAME_MAPINFO_H     // [INFERRED] guard style
#define GAME_MAPINFO_H

#include <string>
#include <vector>

namespace dice {
namespace hfe {

class MapInfo
{
public:
    // Creates dirty metadata for 16 players. m_field18 and m_field19 remain uninitialized.
    MapInfo();
    void clear();

    const std::string& getGameMode() const;                //   +0x00
    void               setGameMode(const std::string&);    //   marks dirty
    const std::string& getLevelPath() const;               //   +0x04
    void               setLevelPath(const std::string&);   //   marks dirty
    const std::string& getMapName() const;                 //   +0x08
    void               setMapName(const std::string&);     //   marks dirty

    int  getMaxNumberOfPlayers() const;                    //   +0x0C
    void setMaxNumberOfPlayers(int);                       //   marks dirty
    int  getChallengeOrdinal() const;                      //   +0x14
    void setChallengeOrdinal(int);                         //   does NOT mark dirty

    // this is the sixth setter, and it is what identifies +0x10.
    // GameServer::handleHudEvent calls it for HUD id 63. 18 bytes: mark dirty, store the flag.
    void setCommanderEnabled(bool);                        //   marks dirty
    // [INFERRED name] Client-facing commander-enabled flag.
    bool getCommanderEnabled() const;

    // Serialisation. Both readers rebuild first if +0x1A is set.
    const void* getNetBuffer();                            //   [UNRESOLVED] pointer type
    int         getNetBufferSize();
    void        updateNetBuffer();                         // not yet read
    void        setFromDataBlock(const void*, unsigned int);
                                                              // the client's receive half; the
                                                              // exact mirror of updateNetBuffer.

private:
    /* 0x00 */ std::string m_gameMode;
    /* 0x04 */ std::string m_levelPath;
    /* 0x08 */ std::string m_mapName;
    /* 0x0C */ int         m_maxNumberOfPlayers;   // clear() -> 16
    /* 0x10 */ bool        m_commanderEnabled;     // by setCommanderEnabled
                                                   //   (0x805F7F0). clear() -> 1, so commander IS
                                                   //   enabled BY default and HUD id 63 turns it off.
    /* 0x14 */ int         m_challengeOrdinal;
    /* 0x18 */ bool        m_field18;              // [UNRESOLVED]; clear() -> 0; serialised
    /* 0x19 */ bool        m_field19;              // [UNRESOLVED]; serialised - NOT padding
    /* 0x1A */ bool        m_netBufferDirty;       // clear() -> 1
    // +0x1C/+0x20/+0x24 are a std::vector<unsigned char> (begin/end/cap) -
    // updateNetBuffer resizes it via _M_fill_insert/erase and builds a BitStream over
    // (begin, end-begin). The old "+0x1C/+0x20 pointer pair" reading was the same object seen
    // through its first two members.
    /* 0x1C */ std::vector<unsigned char> m_netBuffer;
};

} // namespace hfe
} // namespace dice

#endif
