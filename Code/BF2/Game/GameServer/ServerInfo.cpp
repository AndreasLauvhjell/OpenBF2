// Server description and its connection payload; publishes the singleton during static initialization.

#include "ServerInfo.h"

#include "../../Core/StringUtil.h"                    // getEmptyString
#include "../IGameLogic.h"                             // gameLogic->getVoipServerInfo (generateDataBlock)
#include "ServerSettings.h"                            // serverSettings - the block's source
#include "../../IO/BitStream.h"                        // the wire

namespace dice {
namespace hfe {

// : the defaults, then the one seeded welcome message.
ServerInfoBlock::ServerInfoBlock()
    : gameName("bf2")
{
    requestResponse = 0;
    voipEnabled = 0;
    allowNoseCam = 0;
    hitIndicator = 0;
    tkPunishDefault = 1;
    allowFreeCam = 0;
    allowExternalViews = 0;
    voipServerAddress = 0;
    voipServerPort = 0;
    voipBFClientPort = 0;
    voipQuality = 0;
    radioSpamInterval = 6;
    radioMaxSpamFlagCount = 6;
    radioBlockedDuration = 30;
    gameSpyPort = 0;
    useContentChecking = 0;                          // NOT a ctor store in the ELF - static storage's 0
    votingEnabled = 0;
    numPlayersNeededToStart = 0;
    friendlyFireWithMines = 0;
    teamVoteOnly = 1;
    noVehicles = 0;
    alwaysTrue = 0;
    roundsPerMap = 0;
    spawnTime = 0;
    timeLimit = 0;
    ticketRatio = 0;
    teamRatioPercent = 0;
    autoBalanceTeam = 1;
    useGlobalUnlocks = 1;
    coopBotRatio = 50;
    coopBotCount = 16;
    coopBotDifficulty = 50;
    welcomeMessages.push_back(std::string("Welcome to yet another BF2 server"));
}

// : guard + storage + atexit = a function-static object.
ServerInfo* ServerInfo::instance()
{
    static ServerInfo s_instance;
    return &s_instance;
}

ServerInfo::ServerInfo()
    : m_dirty(false)
{
}

// : the data buffer, the welcome strings, the vector, the game name - i.e. the
// members' destructors, in reverse order.
ServerInfo::~ServerInfo()
{
}

const std::string& ServerInfo::getWelcomeMsg(int index) const
{
    if (static_cast<unsigned int>(index) >= m_block.welcomeMessages.size())
        return getEmptyString();
    return m_block.welcomeMessages[index];
}

// : indices 0..2 only; resize the vector to index+1 (empty strings fill), store
// the message truncated to 256 characters, mark dirty. Out-of-range indices do nothing.
void ServerInfo::setWelcomeMsg(int index, const std::string& message)
{
    if (static_cast<unsigned int>(index) > 2)
        return;
    const unsigned int count = static_cast<unsigned int>(index) + 1;
    if (count >= m_block.welcomeMessages.size())
        m_block.welcomeMessages.insert(m_block.welcomeMessages.end(),
                                       count - m_block.welcomeMessages.size(), std::string());
    else
        m_block.welcomeMessages.erase(m_block.welcomeMessages.begin() + count,
                                      m_block.welcomeMessages.end());
    m_block.welcomeMessages[index] = std::string(message, 0, 0x100);
    internalSetDirty();
}

const std::string& ServerInfo::getGameName() const { return m_block.gameName; }

// : 32 characters, dirty.
void ServerInfo::setGameName(const std::string& name)
{
    m_block.gameName = std::string(name, 0, 32);
    internalSetDirty();
}

bool ServerInfo::isDirty() const        { return m_dirty; }
void ServerInfo::setDirty(bool dirty)   { m_dirty = dirty; }
void ServerInfo::internalSetDirty()     { m_dirty = true; }

// : regenerate, then hand out the buffer.
void ServerInfo::getDataBlock(const void** outData, unsigned int* outSize, bool full)
{
    generateDataBlock(full);
    *outData = m_dataBlock.empty() ? 0 : &m_dataBlock[0];
    *outSize = static_cast<unsigned int>(m_dataBlock.size());
}

unsigned char ServerInfo::getRequestResponse() const { return m_block.requestResponse; }

// Server-info bit writers: Boolean uses one bit; signed values use a sign bit and 31 magnitude bits.
namespace {
inline void writeBool(io::BitStream& stream, bool value)
{
    int bit = value ? 1 : 0;
    stream.writeBits(&bit, 1);
}
inline void writeU16(io::BitStream& stream, unsigned short value)
{
    unsigned short v = value;
    stream.writeBits(&v, 16);
}
inline void writeSigned(io::BitStream& stream, int value)
{
    if (value < 0)
    {
        int sign = 1;
        stream.writeBits(&sign, 1);
        int magnitude = -value;
        stream.writeBits(&magnitude, 31);
    }
    else
    {
        int sign = 0;
        stream.writeBits(&sign, 1);
        int magnitude = value;
        stream.writeBits(&magnitude, 31);
    }
}
inline void writeString(io::BitStream& stream, const std::string& text)
{
    writeU16(stream, (unsigned short)text.size());
    stream.writeBits(text.c_str(), 8 * (unsigned int)text.size());   // the ELF re-terminates then writes
}

// the read half, for updateFromDataBlock - the mirrors of the four writers above.
// Every one reads into a zeroed int first, exactly as the ELF's unrolled readBits calls do.
inline bool readBool(io::BitStream& stream)
{
    int bit = 0;
    stream.readBits(&bit, 1);
    return bit == 1;
}
inline unsigned short readU16(io::BitStream& stream)
{
    int value = 0;
    stream.readBits(&value, 16);
    return static_cast<unsigned short>(value);
}
inline unsigned int readU32(io::BitStream& stream)
{
    int value = 0;
    stream.readBits(&value, 32);
    return static_cast<unsigned int>(value);
}
inline int readSigned(io::BitStream& stream)                    // 1 sign bit + 31 magnitude bits
{
    int sign = 0;
    stream.readBits(&sign, 1);
    int magnitude = 0;
    stream.readBits(&magnitude, 31);
    return sign == 1 ? -magnitude : magnitude;
}
// u16 length, then that many bytes read into a zero-filled scratch vector and assigned. A zero
// length resizes the string to 0 instead of assigning - both spellings are the ELF's, and they are
// equivalent; kept so the body reads like the binary.
inline void readString(io::BitStream& stream, std::string& out)
{
    const unsigned short length = readU16(stream);
    if (length)
    {
        std::vector<unsigned char> buffer(length, 0);
        stream.readBits(&buffer[0], 8u * length);
        out.assign(reinterpret_cast<const char*>(&buffer[0]), length);
    }
    else
        out.resize(0);
}
} // namespace

void ServerInfo::generateDataBlock(bool full)
{
    m_dataBlock.resize(895);                         // 0x37F: fill with 0 / erase past it

    // refresh the block from the live settings
    m_block.requestResponse = full ? 1 : 0;
    {
        bool enabled = m_block.voipEnabled != 0;
        gameLogic->getVoipServerInfo(enabled, m_block.voipServerAddress, m_block.voipServerPort,
                                     m_block.voipBFClientPort, m_block.voipQuality);   // IGameLogic +0x258
        m_block.voipEnabled = enabled ? 1 : 0;
    }
    m_block.allowFreeCam           = serverSettings->m_allowFreeCam;             // +0x50
    m_block.allowExternalViews     = serverSettings->m_allowExternalViews;       // +0x51
    m_block.allowNoseCam           = serverSettings->m_allowNoseCam;             // +0x52
    m_block.hitIndicator           = serverSettings->m_hitIndicator;             // +0x53
    m_block.tkPunishDefault        = serverSettings->m_tkPunishDefault;          // +0xA0
    m_block.gameSpyPort            = (unsigned short)serverSettings->m_gameSpyPort;   // +0xA8
    m_block.gameName.assign(serverSettings->m_serverName);                       // +0x04 - the server name goes out as "gameName"
    m_block.radioSpamInterval      = serverSettings->m_radioSpamInterval;        // +0xE8
    m_block.radioMaxSpamFlagCount  = serverSettings->m_radioMaxSpamFlagCount;    // +0xEC
    m_block.radioBlockedDuration   = serverSettings->m_radioBlockedDuration;     // +0xF0
    m_block.votingEnabled          = serverSettings->m_votingEnabled;            // +0x24
    m_block.numPlayersNeededToStart = (unsigned short)serverSettings->getNumPlayersNeededToStart();
    m_block.friendlyFireWithMines  = serverSettings->m_friendlyFireWithMines;    // +0x100
    m_block.teamVoteOnly           = serverSettings->m_teamVoteOnly;             // +0x30
    m_block.noVehicles             = serverSettings->getNoVehicles();            // forced false in gpm_coop
    m_block.alwaysTrue             = 1;
    m_block.roundsPerMap           = serverSettings->m_roundsPerMap;             // +0x74
    m_block.spawnTime              = (int)serverSettings->getSpawnTime();        // float -> int
    m_block.timeLimit              = serverSettings->m_timeLimit;                // +0x78
    m_block.ticketRatio            = serverSettings->getTicketRatio();
    m_block.teamRatioPercent       = (int)serverSettings->getTeamRatioPercent(); // float -> int
    m_block.autoBalanceTeam        = serverSettings->m_autoBalanceTeam;          // +0xC8
    m_block.useGlobalUnlocks       = serverSettings->getUseGlobalUnlocks();
    m_block.coopBotRatio           = (int)serverSettings->getCoopBotRatio();     // float -> int
    m_block.coopBotCount           = serverSettings->getCoopBotCount();
    m_block.coopBotDifficulty      = (int)serverSettings->getCoopBotDifficulty();   // float -> int

    // the wire
    io::BitStream stream(&m_dataBlock[0], (unsigned int)m_dataBlock.size());
    writeString(stream, m_block.gameName);
    setWelcomeMsg(0, serverSettings->m_welcomeMessage);   // +0x18 - marks the object dirty again
    writeU16(stream, (unsigned short)m_block.welcomeMessages.size());
    for (unsigned int i = 0; i < m_block.welcomeMessages.size(); ++i)
        writeString(stream, m_block.welcomeMessages[i]);
    writeBool(stream, m_block.requestResponse != 0);
    writeBool(stream, m_block.voipEnabled != 0);
    writeBool(stream, m_block.allowNoseCam != 0);
    writeBool(stream, m_block.hitIndicator != 0);
    writeBool(stream, m_block.tkPunishDefault != 0);
    writeBool(stream, m_block.allowFreeCam != 0);
    writeBool(stream, m_block.allowExternalViews != 0);
    {
        unsigned int address = m_block.voipServerAddress;
        stream.writeBits(&address, 32);
    }
    writeU16(stream, m_block.voipServerPort);
    writeU16(stream, m_block.voipBFClientPort);
    writeSigned(stream, m_block.voipQuality);
    writeU16(stream, m_block.gameSpyPort);
    writeSigned(stream, m_block.radioSpamInterval);
    writeSigned(stream, m_block.radioMaxSpamFlagCount);
    writeSigned(stream, m_block.radioBlockedDuration);
    writeBool(stream, m_block.useContentChecking != 0);
    writeBool(stream, m_block.votingEnabled != 0);
    writeU16(stream, m_block.numPlayersNeededToStart);
    writeBool(stream, m_block.friendlyFireWithMines != 0);
    writeBool(stream, m_block.teamVoteOnly != 0);
    writeBool(stream, m_block.noVehicles != 0);
    writeBool(stream, m_block.alwaysTrue != 0);
    writeSigned(stream, m_block.roundsPerMap);
    writeSigned(stream, m_block.spawnTime);
    writeSigned(stream, m_block.timeLimit);
    writeSigned(stream, m_block.ticketRatio);
    writeSigned(stream, m_block.teamRatioPercent);
    writeBool(stream, m_block.autoBalanceTeam != 0);
    writeBool(stream, m_block.useGlobalUnlocks != 0);
    writeSigned(stream, m_block.coopBotRatio);
    writeSigned(stream, m_block.coopBotCount);
    writeSigned(stream, m_block.coopBotDifficulty);

    // trim to the written bytes
    const unsigned int bytes = (stream.getBitPosition() + 7) >> 3;   // the read cursor when the switch says so
    m_dataBlock.resize(bytes);                        // grow with 0 / erase past it, as shipped

    // [CHOSEN, not recovered] Clear dirty after generating the join block so a periodic
    // non-response broadcast cannot replace its client-info request during joining.
    // [RETIRE-WHEN: real-server capture confirms whether retail rebroadcasts ServerInfo with byte10=0
    //  during a join, or relies on client timing to have already responded.]
    m_dirty = false;                                  // +0x00
}

// Reads server-info fields and publishes voice settings to game logic. Does not
// check stream errors, regenerate the local data block, or change the dirty flag.
// The received welcome-message count replaces the current vector size.
void ServerInfo::updateFromDataBlock(const unsigned char* data, unsigned int size)
{
    io::BitStream stream(const_cast<unsigned char*>(data), size);
    stream.setReadMode();                            // readWriteSwitch = 1
    stream.setBitSize(8 * size);

    readString(stream, m_block.gameName);

    const unsigned short count = readU16(stream);
    m_block.welcomeMessages.resize(count);
    for (unsigned short i = 0; i < count; ++i)
        readString(stream, m_block.welcomeMessages[i]);

    m_block.requestResponse         = readBool(stream) ? 1 : 0;
    m_block.voipEnabled             = readBool(stream) ? 1 : 0;
    m_block.allowNoseCam            = readBool(stream) ? 1 : 0;
    m_block.hitIndicator            = readBool(stream) ? 1 : 0;
    m_block.tkPunishDefault         = readBool(stream) ? 1 : 0;
    m_block.allowFreeCam            = readBool(stream) ? 1 : 0;
    m_block.allowExternalViews      = readBool(stream) ? 1 : 0;
    m_block.voipServerAddress       = readU32(stream);
    m_block.voipServerPort          = readU16(stream);
    m_block.voipBFClientPort        = readU16(stream);
    m_block.voipQuality             = readSigned(stream);
    m_block.gameSpyPort             = readU16(stream);
    m_block.radioSpamInterval       = readSigned(stream);
    m_block.radioMaxSpamFlagCount   = readSigned(stream);
    m_block.radioBlockedDuration    = readSigned(stream);
    m_block.useContentChecking      = readBool(stream) ? 1 : 0;
    m_block.votingEnabled           = readBool(stream) ? 1 : 0;
    m_block.numPlayersNeededToStart = readU16(stream);
    m_block.friendlyFireWithMines   = readBool(stream) ? 1 : 0;
    m_block.teamVoteOnly            = readBool(stream) ? 1 : 0;
    m_block.noVehicles              = readBool(stream) ? 1 : 0;
    m_block.alwaysTrue              = readBool(stream) ? 1 : 0;
    m_block.roundsPerMap            = readSigned(stream);
    m_block.spawnTime               = readSigned(stream);
    m_block.timeLimit               = readSigned(stream);
    m_block.ticketRatio             = readSigned(stream);
    m_block.teamRatioPercent        = readSigned(stream);
    m_block.autoBalanceTeam         = readBool(stream) ? 1 : 0;
    m_block.useGlobalUnlocks        = readBool(stream) ? 1 : 0;
    m_block.coopBotRatio            = readSigned(stream);
    m_block.coopBotCount            = readSigned(stream);
    m_block.coopBotDifficulty       = readSigned(stream);

    gameLogic->storeVoipServerInfo(m_block.voipEnabled != 0, m_block.voipServerAddress,
                                   m_block.voipServerPort, m_block.voipBFClientPort,
                                   m_block.voipQuality);                    // IGameLogic +0x254
}

// ds:0x8AAAA60 - seated before main by static-init _10, as here.
ServerInfo* serverInfo = ServerInfo::instance();

} // namespace hfe
} // namespace dice
