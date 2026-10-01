// Voice-server routing and resources. Disabled voice is supported; enabled allocation and local-owner cleanup remain frontiers.

#include "VoipServerConnection.h"

#if defined(_WIN32)
# include <winsock2.h>
#else
# include <arpa/inet.h>
#endif

#include <cstdio>    // the InitEnabledVoip named stop
#include <cstdlib>   // rand - CreateClientSessionId
#include <cmath>     // powf - ditto
#include "ServerSettings.h"
#if !DEDICATED
#include "../GameServerSettings.h"          // Init's listen-server arm (BF2.exe 0x5B3820)
#include "../Main/Game.h"                   // isDedicated
#endif
#include "../../Core/ClassManager.h"
#include "../../Core/Ids.h"
#include "../../Core/SystemInfo.h"
#include "../../IO/Network/NetworkLayer/Socket/UDPSocket.h"
#include "../../Scene/Object/PlayerManager.h"
#include "../IGame.h"
#include "../Objects/IPlayer.h"

#include <cerrno>
#include <cstring>
#include <fstream>

namespace dice {
namespace hfe {

extern ClassManager* classManager;                    // Core/Globals.cpp; PE ds:0x892D58

// ds:0x8AAC980 - the lazily-created singleton pointer. The ELF global carries no symbol;
// [CHOSEN] a file-static here.
static VoipServerConnection* s_instance = 0;

namespace {
void clearFailedVoipSendError()
{
#if defined(_WIN32)
    // PE calls WSAGetLastError even though it discards the value, then clears it.
    (void)WSAGetLastError();
    WSASetLastError(0);
#else
    errno = 0;
#endif
}

void voipPacketInvariantFailure()
{
#if defined(_MSC_VER)
    // The PE's assertion arm is an int3 and may resume under a debugger. The caller therefore
    // repeats the test before copying, exactly as the shipped body does.
    __debugbreak();
#else
    std::abort();
#endif
}

void storeNativeWord(unsigned char* destination, unsigned short value)
{
    std::memcpy(destination, &value, sizeof(value));
}
} // namespace

// GetInstance - ELF 0x8126600. new(0x4418) + ctor on first use.
VoipServerConnection* VoipServerConnection::GetInstance()
{
    if (!s_instance)
        s_instance = new VoipServerConnection();
    return s_instance;
}

void VoipServerConnection::ReleaseInstance()
{
    if (s_instance)
    {
        delete s_instance;
        s_instance = 0;
    }
}

// Releases the local voice owner, routing log, and channel in that order when present.
// [CHOSEN, not recovered] Cleanup is split into a resource helper.
VoipServerConnection::~VoipServerConnection()
{
    const bool localResourceLive = !m_infoByte4315 && m_localVoipServer440C != 0;
    if (localResourceLive || m_routingLog4414 || m_channel)
        ReleaseEnabledVoipResources();
}

void VoipServerConnection::ReleaseEnabledVoipResources()
{
    if (!m_infoByte4315 && m_localVoipServer440C)
    {
        // [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: voip-enabled-init]
        // The local voice owner's stop/destruction contract is unresolved; abort before releasing it.
        std::fprintf(stderr, "[FRONTIER] dice::hfe::VoipServerConnection::ReleaseEnabledVoipResources "
                             "reached dice::hfe::VoipServerInternal::Stop/destructor\n");
        std::fflush(stderr);
        std::abort();
    }

    if (m_routingLog4414)
    {
        m_routingLog4414->close();
        delete m_routingLog4414;
        m_routingLog4414 = 0;
    }

    if (m_channel)
    {
        m_channel->close();
        delete m_channel;
        m_channel = 0;
    }
}

// Starts with no channel or shared secret, voice quality 3, and a twenty-space text buffer.
VoipServerConnection::VoipServerConnection()
{
    m_channel = 0;                                     // +0x04
    memset(m_packet, 0, sizeof(m_packet));             // covers the ELF's +0x4208 loop+memset zeroing
    m_infoByte4314  = 0;                               // +0x4314
    m_infoByte4315  = 0;                               // +0x4315
    memset(_pad4316, 0, sizeof(_pad4316));
    m_infoDword4318 = 0;                               // +0x4318
    memset(_pad431C, 0, sizeof(_pad431C));
    m_infoWord4400  = 0;                               // +0x4400
    m_infoDword4404 = 0;                               // +0x4404
    m_infoWord4408  = 0;                               // +0x4408
    m_localVoipServer440C  = 0;                        // +0x440C
    m_infoDword4410 = 3;                               // Default voice quality.
    m_routingLog4414 = 0;                              // +0x4414

    m_sharedSecret  = "";                              // ELF +0x43FC from c_defaultSharedSecret = ""

    // The twenty-space fill (ELF +0x438C buffer, +0x43F4 counter), verbatim.
    m_textCount = 0;
    memset(m_textBuffer, 0, sizeof(m_textBuffer));
    for (int i = 19; i >= 0; --i)
    {
        unsigned int c = m_textCount;
        if (c <= 0x63)
        {
            m_textBuffer[c] = ' ';
            m_textCount = ++c;
            m_textBuffer[c] = 0;
        }
    }

    m_versionCount = 0;                                // +0x4384; populated by enabled Init
    memset(m_versionBuffer, 0, sizeof(m_versionBuffer));
}

// Copies voice settings; returns false only for a null settings pointer.
// Local voice advertises address 1, while remote voice converts its address with inet_addr.
bool VoipServerConnection::GetVoipServerSettings(ServerSettings* value)
{
    if (!value)
        return false;

    m_infoByte4314  = (value->m_voipEnabled == true);
    m_infoByte4315  = (value->m_voipServerRemote == true);
    m_infoDword4410 = value->m_voipQuality;

    if (m_infoByte4315)
        m_infoDword4318 = (int)inet_addr(value->m_voipServerRemoteIP.c_str());
    else
        m_infoDword4318 = 1;

    m_infoWord4400  = value->m_voipServerPort;
    m_infoDword4404 = value->m_voipBFServerPort;
    m_infoWord4408  = value->m_voipBFClientPort;
    m_sharedSecret  = value->m_voipSharedPassword;
    return true;
}

#if !DEDICATED
// Copies listen-server voice settings through the settings interface in the same order as the record overload.
bool VoipServerConnection::GetVoipServerSettings(IGameServerSettings* value)
{
    if (!value)
        return false;

    m_infoByte4314  = (value->getVoipEnabled() == 1);
    m_infoByte4315  = (value->getVoipServerRemote() == 1);
    m_infoDword4410 = value->getVoipQuality();

    if (!m_infoByte4315)
        m_infoDword4318 = 1;
    else
        m_infoDword4318 = (int)inet_addr(value->getVoipServerRemoteIP().c_str());

    m_infoWord4400  = value->getVoipServerPort();
    m_infoDword4404 = value->getVoipBFServerPort();
    m_infoWord4408  = value->getVoipBFClientPort();
    m_sharedSecret  = value->getVoipSharedPassword();
    return true;
}
#endif

// Copies settings after the singleton lookup. Disabled voice succeeds without allocating resources.
// [FRONTIER] Enabled voice stops before the unfinished allocation path.
bool VoipServerConnection::Init()
{
    // Both binaries discard this exact call's return. ELF: classManager ds:0x8AC85A0, vslot
    // +0x4C at 0x8126C86; PE: ds:0x892D58, vslot +0x48 at 0x54B160. The one-slot difference is
    // the MSVC/Itanium destructor-pair ABI split. The global serverSettings below is a separate
    // load in both binaries; substituting the lookup result would be an infidelity.
#if !DEDICATED
    // [PLATFORM] BF2.exe 0x5B3C50 (): the client keeps the lookup's answer - a listen
    // server copies its voice settings out of that GameServerSettings object (0x5B3820), a dedicated
    // one out of the ServerSettings record (0x5B3920), picked by isDedicated().
    IBase* gameServerSettings = classManager->getSingleton("GameServerSettings");
    if (isDedicated())
    {
        if (!GetVoipServerSettings(serverSettings))
            return false;
    }
    else if (!GetVoipServerSettings(static_cast<IGameServerSettings*>(gameServerSettings)))
    {
        return false;
    }
#else
    classManager->getSingleton("GameServerSettings");

    if (!GetVoipServerSettings(serverSettings))
        return false;
#endif

    if (!m_infoByte4314)
        return true;

    return InitEnabledVoip();                          // [FRONTIER - only when voice is enabled]
}

// [FRONTIER] Voice-enabled initialization stops until UDP voice setup and the
// in-process voice server are implemented. sv.voipEnabled 0 avoids this branch.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: voip-enabled-init]
bool VoipServerConnection::InitEnabledVoip()
{
    std::fprintf(stderr, "[FRONTIER] dice::hfe::VoipServerConnection::InitEnabledVoip reached "
                         "(sv.voipEnabled is 1; the in-process voice server is not recovered)\n");
    std::fflush(stderr);
    //std::abort();
    return false;
}

// Appends 24 version bytes and 20 session bytes, excluding terminators.
// [PLATFORM] Length assertions abort on ELF; PE may resume and skip the invalid copy.
void VoipServerConnection::PacketAppendVersionAndSessionId(int& cursor)
{
    if (m_versionCount != 24)
        voipPacketInvariantFailure();
    if (m_versionCount == 24)
    {
        std::memcpy(m_packet + cursor, m_versionBuffer, 24);
        cursor += m_versionCount;
    }

    if (m_textCount != 20)
        voipPacketInvariantFailure();
    if (m_textCount == 20)
    {
        std::memcpy(m_packet + cursor, m_textBuffer, 20);
        cursor += m_textCount;
    }
}

// Sends team/commander/leader/squad/session/player routing as message 100, or
// message 101 to remove a player outside squads 1..10. Words use native little
// endian. Retains the server-interface query and optional routing debug dump.
bool VoipServerConnection::UpdatePlayerRouting(world::IPlayer* player)
{
    if (!m_channel || !world::playerManager)
        return false;

    unsigned char routing[8];
    std::memset(routing, 0, sizeof(routing));

    unsigned short playerId = 255;
    if (player->getId() != -1)
        playerId = static_cast<unsigned short>(player->getId());
    storeNativeWord(routing + 6, playerId);
    routing[0] = static_cast<unsigned char>(player->getTeam());

    // Both ports make this query and discard the result. Neither checks `game` for null.
    (void)game->queryInterface(IID_IGameServer);
    std::string unusedAddress;
    (void)unusedAddress;

    const unsigned short sessionId = player->getVoipSessionId();
    int cursor = 0;

    if (player->getIsCommander())
    {
        routing[1] = 1;
        routing[2] = 0;
        routing[3] = 0;
        storeNativeWord(routing + 4, sessionId);
    }
    else if (player->getIsSquadLeader()
          && static_cast<unsigned int>(player->getSquadId()) != 0
          && static_cast<unsigned int>(player->getSquadId()) <= 10u)
    {
        routing[1] = 0;
        routing[2] = 1;
        routing[3] = static_cast<unsigned char>(player->getSquadId());
        storeNativeWord(routing + 4, sessionId);
    }
    else
    {
        if (static_cast<unsigned int>(player->getSquadId()) == 0
         || static_cast<unsigned int>(player->getSquadId()) > 10u)
        {
            storeNativeWord(m_packet, sessionId);
            cursor = 2;
            PacketAppendVersionAndSessionId(cursor);
            m_packet[cursor] = 101;
            goto packet_ready;
        }

        routing[1] = 0;
        routing[2] = 0;
        routing[3] = static_cast<unsigned char>(player->getSquadId());
        storeNativeWord(routing + 4, sessionId);
    }

    std::memcpy(m_packet, routing, sizeof(routing));
    cursor = 8;
    PacketAppendVersionAndSessionId(cursor);
    m_packet[cursor] = 100;

packet_ready:
    ++cursor;
    if (!m_channel->send(m_packet, cursor))
        clearFailedVoipSendError();

    if (m_routingLog4414)
    {
        *m_routingLog4414 << std::endl
                          << "-----------------------------------------------------------------------------------------"
                          << std::endl;
        *m_routingLog4414 << systemInfo()->getDateTimeString().c_str() << std::endl;

        const std::list<world::IPlayer*>& players = world::playerManager->getPlayers();
        for (std::list<world::IPlayer*>::const_iterator it = players.begin();
             it != players.end(); ++it)
        {
            world::IPlayer* const current = *it;
            if (!current->getIsAIPlayer())
            {
                *m_routingLog4414 << "Player: " << current->getId() << ", "
                                  << current->getName()
                                  << "  Commander: " << current->getIsCommander()
                                  << "  SquadLeader: " << current->getIsSquadLeader()
                                  << "  SquadID: "
                                  << static_cast<unsigned int>(current->getSquadId())
                                  << std::endl;
            }
        }
    }

    return true;
}

// ResetPlayerRouting - ELF 0x8126BB0, 129 bytes.
//   * null channel -> return 0 (the boot state; nothing sent).
//   * otherwise: header via PacketAppendVersionAndSessionId, then message byte 102, then one
//     channel send; a failed send clears errno (yes - clears, not reads; preserved).
int VoipServerConnection::ResetPlayerRouting()
{
    if (!m_channel)
        return 0;

    int cursor = 0;
    PacketAppendVersionAndSessionId(cursor);
    m_packet[cursor] = 102;                            // *(this + 8 + cursor) = 102
    ++cursor;
    if (!m_channel->send(m_packet, cursor))
        clearFailedVoipSendError();
    return 1;
}

// [CHOSEN, not recovered] Updates currently return immediately.
// The original enabled path updates routing, debug logging, and packet reception.
void VoipServerConnection::Update(float dt)
{
    return;
    if (!m_infoByte4314)
        return;
    FullRoutingUpdate(dt);                             // [FRONTIER]
    UpdateDebugLogging();                              // [FRONTIER]
    Receive();                                         // [FRONTIER]
}

// Chooses a random voice-session ID independently of playerId, restarting the player scan on collision.
// Retries have no limit. Collision increments can exceed 16 bits; later storage narrows
// the result, so collisions introduced by that narrowing are not detected.
int VoipServerConnection::CreateClientSessionId(int playerId)
{
    (void)playerId;                                     // deliberate - see above

    if (!world::playerManager)
        return 0;

    const int range = (int)powf(256.0f, 2.0f);          // 65536, computed not folded
    int id = rand() % range;

    const std::list<world::IPlayer*>& players = world::playerManager->getPlayers();

    bool clash = true;
    while (clash)
    {
        clash = false;
        for (std::list<world::IPlayer*>::const_iterator it = players.begin();
             it != players.end(); ++it)
        {
            if ((*it)->getVoipSessionId() == id)        // vslot 624 = +0x270
            {
                id += 1 + (rand() % 500);               // random bump, not +1
                clash = true;                           // and rescan from the head
                break;
            }
        }
    }
    return id;
}

} // namespace hfe
} // namespace dice
