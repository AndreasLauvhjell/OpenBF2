// Shared server-info snapshot and serialized data block. Mutating setters mark it
// dirty; generation refreshes settings and voice values before writing the buffer.
// GameServer broadcasts dirty state periodically, then clears the dirty flag.
// String setters enforce their content limits. Signed wire integers use a sign bit
// and magnitude. updateFromDataBlock reads the corresponding receive format.

#ifndef GAME_GAMESERVER_SERVERINFO_H     // [INFERRED] guard style
#define GAME_GAMESERVER_SERVERINFO_H

#include <string>
#include <vector>

namespace dice {
namespace hfe {

struct ServerInfoBlock                               // 0x60 bytes; ctor 0x8065B00 (358 B)
{
    ServerInfoBlock();

    /* +0x00 */ std::string   gameName;              // "bf2"
    /* +0x04 */ std::vector<std::string> welcomeMessages;   // {"Welcome to yet another BF2 server"}
    /* +0x10 */ unsigned char requestResponse;       // 0 - getRequestResponse's byte [CHOSEN name from the getter];
                                                     // VoIP settings are exchanged with GameLogic during server-info serialization. Field names are [CHOSEN].
    /* +0x11 */ unsigned char voipEnabled;           // 0 (was byte11)
    /* +0x12 */ unsigned char allowNoseCam;          // 0 - sv
    /* +0x13 */ unsigned char hitIndicator;          // 0 - sv
    /* +0x14 */ unsigned char tkPunishDefault;       // 1 - sv
    /* +0x15 */ unsigned char allowFreeCam;          // 0 - ServerSettings::setAllowFreeCam's mirror (ServerInfo +0x19); sv
    /* +0x16 */ unsigned char allowExternalViews;    // 0 - setAllowExternalViews' mirror (+0x1A); sv
    /* +0x18 */ unsigned int  voipServerAddress;     // 0 (was dword18)
    /* +0x1C */ unsigned short voipServerPort;       // 0 (was word1C)
    /* +0x1E */ unsigned short voipBFClientPort;     // 0 (was word1E)
    /* +0x20 */ int           voipQuality;           // 0 (was dword20) - written signed (sign + 31)
    /* +0x24 */ int           radioSpamInterval;     // 6 - sv
    /* +0x28 */ int           radioMaxSpamFlagCount; // 6 - sv
    /* +0x2C */ int           radioBlockedDuration;  // 30 - sv
    /* +0x30 */ unsigned short gameSpyPort;          // 0 - sv
    /* +0x32 */ unsigned char useContentChecking;    // NOT stored by the block ctor (the object is
                                                     //   static storage in the ELF, so 0); GameServer::
                                                     //   init copies its bUseContentChecking (+0x150)
                                                     //   here at 0x80A6C86 [not modelled in the tree's
                                                     //   init yet]. Zeroed by the ctor here.
    /* +0x33 */ unsigned char votingEnabled;         // 0 - sv
    /* +0x34 */ unsigned short numPlayersNeededToStart;   // 0
    /* +0x36 */ unsigned char friendlyFireWithMines; // 0 - setFriendlyFireWithMines' mirror (+0x3A)
    /* +0x37 */ unsigned char teamVoteOnly;          // 1
    /* +0x38 */ unsigned char noVehicles;            // 0
    /* +0x39 */ unsigned char alwaysTrue;            // 0 (the tinfo's name; [UNRESOLVED] meaning)
    /* +0x3C */ int           roundsPerMap;          // 0
    /* +0x40 */ int           spawnTime;             // 0
    /* +0x44 */ int           timeLimit;             // 0
    /* +0x48 */ int           ticketRatio;           // 0
    /* +0x4C */ int           teamRatioPercent;      // 0
    /* +0x50 */ unsigned char autoBalanceTeam;       // 1
    /* +0x51 */ unsigned char useGlobalUnlocks;      // 1
    /* +0x54 */ int           coopBotRatio;          // 50
    /* +0x58 */ int           coopBotCount;          // 16
    /* +0x5C */ int           coopBotDifficulty;     // 50
};

class ServerInfo
{
public:
    static ServerInfo* instance();                   // the function-static object

    ServerInfo();
    ~ServerInfo();                                   // the members' own dtors

    const std::string& getWelcomeMsg(int index) const;
    void setWelcomeMsg(int index, const std::string& message);
    const std::string& getGameName() const;
    void setGameName(const std::string& name);                      // 32 chars max, dirty
    bool isDirty() const;
    void setDirty(bool dirty);
    void internalSetDirty();                                        // dirty = true
    void getDataBlock(const void** outData, unsigned int* outSize, bool full);   // regenerates first
    unsigned char getRequestResponse() const;
    void generateDataBlock(bool full);                              // (the wire, above)
    void updateFromDataBlock(const unsigned char* data, unsigned int size);
                                                                    //   the client's receive half (block 0)

    // The block is public: ServerSettings' mirror setters write its flags directly (the ELF's
    // `mov [edx+19h], al` on the singleton), and generateDataBlock reads all of it.
    /* +0x00 */ bool            m_dirty;             // ctor false; the byte isDirty / setDirty read and write
    /* +0x04 */ ServerInfoBlock m_block;
    /* +0x64 */ std::vector<unsigned char> m_dataBlock;   // begin/end/cap - generateDataBlock's output;
                                                     //   getDataBlock hands out begin and end - begin
};

extern ServerInfo* serverInfo;                       // ds:0x8AAAA60 - Core/Globals.cpp; = instance() at static init

} // namespace hfe
} // namespace dice

#endif // GAME_GAMESERVER_SERVERINFO_H
