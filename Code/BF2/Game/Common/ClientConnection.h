// Client-facing stream managers, packet status and demo connection state.
// [INFERRED] Header placement beside attested Game/Common/ClientConnection.cpp.
// Demo mode controls buffer sizes and stream-manager configuration.

#ifndef GAME_COMMON_CLIENTCONNECTION_H        // [INFERRED] guard style
#define GAME_COMMON_CLIENTCONNECTION_H

#include <deque>
#include <vector>

#include "IClientConnection.h"

namespace dice {
namespace hfe {

namespace io { class BitStream; }
namespace world { class IPlayer; }

class PlayerActionManager;
class GameEventManager;
class GhostManager;
// Connection demo modes: 0 normal, 1 recording, 2 playback. Recording uses a larger stream.
// [UNRESOLVED] Enumerator names. ConnectionType indexes the six-row bandwidth table.
enum ConnectionType : int;          // Game/IGameServer.h owns the definition; `: int` = MSVC's default

enum ConnectionDemoMode
{
    CONNECTIONDEMOMODE_0 = 0,      // [UNRESOLVED name] - a normal connection
    CONNECTIONDEMOMODE_1 = 1,      // [UNRESOLVED name] - recording
    CONNECTIONDEMOMODE_2 = 2       // [UNRESOLVED name] - playback
};

// Per-packet delivery bookkeeping with event count, block index, and a stream-manager
// notification mask. Remaining fields are [UNRESOLVED]; preserve the 48-byte layout.
// [RETIRE-WHEN: packetstatus-real] Resolve remaining fields before replacing the opaque storage.
struct PacketStatus
{
    // Packet event count, send-block index, and contributing stream-manager mask.
    // Unidentified portions retain opaque storage.
    unsigned char m_opaque00[16];        // +0x00  [UNRESOLVED]
    unsigned int  m_eventCount;          // +0x10
    unsigned int  m_blockIndex;          // +0x14
    unsigned char m_opaque18[4];         // +0x18  [UNRESOLVED]
    unsigned int  m_streamManagerMask;   // +0x1C
    unsigned char m_opaque20[16];        // +0x20  [UNRESOLVED]
};

// Shared connection profiles. Column 1 is server update rate; column 3 is packet
// size. Other column names remain unresolved. Rows 0 and 1 are identical.
extern const int g_connectionTypes[6][6];

class ClientConnection : public IClientConnection
{
public:
    explicit ClientConnection(ConnectionDemoMode demoMode);
    virtual ~ClientConnection();

    // the thirteen vtable slots, in order
    virtual void  addStreamManager(IStreamManager* manager);     // +0x08
    virtual IStreamManager* getStreamManager(int managerId);     // +0x0C
    virtual DataBlockManager* getDataBlockManager();             // +0x10
    virtual StringManager*    getStringManager();                // +0x14
    virtual world::IPlayer*   getPlayer();                       // +0x18
    virtual void  setPlayer(world::IPlayer* player);             // +0x1C
    virtual bool  getIsRecordingConnection() const;              // +0x20
    virtual int   debugGetNumActiveGhosts();                     // +0x24
    virtual void  debugShowActiveGhosts();                       // +0x28
    virtual void  setContentValid(bool valid);                   // +0x2C
    virtual bool  getContentValid() const;                       // +0x30

    // NON-virtual. None of these is in the table - checked slot by slot.
    void setConnectionId(int connectionId);
    bool processReceivedPacket(io::BitStream* stream);
    bool handlePacketStatusNotification(bool& delivered);
    void newPacket(int sizeBytes);
    void resetStreamManagerStats();
    void setConnectionType(ConnectionType type);
    ConnectionType getConnectionType() const;
    void setServerUpdateRate(int rate);
    int  getServerUpdateRate() const;
    void setServerPacketSize(int size);
    int  getServerPacketSize() const;
    int  getDemoMode() const;

    // Builds outgoing messages. False causes the server to disconnect with reason 28.
    bool transmitMsgs();
    // Fills m_challengeString (+0x13C) and returns a pointer into the
    // connection. The two arms differ by a NUL byte - see the .cpp.
    const char* initChallengeString(const char* fixed);
    void outputStreamManagerStats();
                                                                 // the destructor calls this one

    // the per-stream-manager accounting the two stats functions keep. sixteen bytes: the
    // vector's element stride is `sar eax, 4` in resetStreamManagerStats and the destructor,
    // and addStreamManager pushes four zeroed dwords. Contents [UNRESOLVED].
    struct StreamManagerStats
    {
        unsigned int m_opaque[4];
    };

    // A static, not a member - `dice::hfe::ClientConnection::m_useAutoPingAdjust`, read by
    // handlePacketStatusNotification's auto-ping arm.
    static bool m_useAutoPingAdjust;

public:
    // Names are [INFERRED] (the IDB's where it had one); offsets and seed values are read. See the
    // layout block above for the instruction behind each one.
    /* +0x008 */ world::IPlayer*      m_player;
    /* +0x00C */ int                  m_connectionState;
    /* +0x010 */ bool                 m_bReset;
    /* +0x014 */ int                  m_dword14;
    /* +0x018 */ int                  m_packetSizeOverride;
    /* +0x01C */ float                m_timer1C;
    /* +0x020 */ float                m_timer20;
    /* +0x024 */ PlayerActionManager* m_playerActionManager;
    /* +0x028 */ GameEventManager*    m_gameEventManager;
    /* +0x02C */ GhostManager*        m_ghostManager;
    /* +0x030 */ StringManager*       m_stringManager;
    /* +0x034 */ DataBlockManager*    m_dataBlockManager;
    /* +0x038 */ std::deque<PacketStatus>            m_packetStatuses;
    /* +0x060 */ std::vector<IStreamManager*>        m_streamManagers;
    /* +0x06C */ std::vector<StreamManagerStats>     m_streamManagerStats;
    /* +0x078 */ int                  m_dword78;
    /* +0x07C */ int                  m_dword7C;
    // Send-buffer back pressure: a packet that did not fit suppresses building the next packet.
    /* +0x080 */ bool                 m_sentData;
    /* +0x084 */ io::BitStream*       m_bitStream;
    /* +0x088 */ int                  m_connectionId;
    /* +0x08C */ bool                 m_byte8C;
    /* +0x090 */ float                m_pingSamples[32];      // NOT zeroed by the constructor
    /* +0x110 */ int                  m_curPingIndex;
    /* +0x114 */ int                  m_dword114;
    /* +0x118 */ double               m_pingValue;
    /* +0x120 */ int                  m_connectionType;
    /* +0x124 */ int                  m_profile[6];           // +0x128 = update rate, +0x130 = size
    // Ten-byte challenge storage; the update timestamp is a separate field.
    /* +0x13C */ char                 m_challengeString[10];
    /* +0x146 */ bool                 m_byte146;
    /* +0x147 */ bool                 m_byte147;
    /* +0x148 */ int                  m_lastPingMs;
    /* +0x14C */ double               m_lastUpdateTime;       // attested - `fstp qword [ebx+14Ch]`
    /* +0x154 */ int                  m_nbProcessedPackets;
    /* +0x158 */ int                  m_dword158;
    /* +0x15C */ int                  m_dword15C;
    /* +0x160 */ int                  m_nbConnections;
    /* +0x164 */ bool                 m_byte164;
    /* +0x168 */ float                m_float168;
    /* +0x16C */ int                  m_dword16C;
    /* +0x170 */ bool                 m_executeServer;        // ServerConnection seeds it true
    /* +0x174 */ int                  m_demoMode;
    /* +0x178 */ bool                 m_contentValid;
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_CLIENTCONNECTION_H
