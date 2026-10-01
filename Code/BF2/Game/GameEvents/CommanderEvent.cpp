#include "CommanderEvent.h"

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../../Core/Debug.h"                    // the four abort sites
#include "../../Core/IBase.h"
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../GameLogic/SquadManager/ISquadManager.h"   // the nine slots below, typed now

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GECommander (0x8888920) = 19.
const int GECommander = 19;

// The wire width of m_commanderEventId: bitsToHold(9) = 4. Both halves compute it inline from the
// literal 9 with the same do/while loop ConnectionTypeEvent uses - so nine ids are provisioned,
// 0..8, which is exactly what the two jump tables cover.
unsigned int commanderEventIdBits()
{
    unsigned int bits = 0;
    do
        ++bits;
    while (((1u << bits) - 1u) < 9u);
    return bits;
}

inline ISquadManager* squadManagerIface()
{
    return static_cast<ISquadManager*>(squadManager);     // ds:0x8ACBFA4
}
} // namespace

//  (C2) / 0x806C370 (C1), 115 B. Three stores, then the discarded getCommander call - see
// the header block.
CommanderEvent::CommanderEvent(CommanderEventTypes type, int teamId, int playerId)
    : GameEvent()
    , m_commanderEventId(static_cast<int>(type))             // +0x0C
    , m_teamId(static_cast<unsigned char>(teamId))           // +0x10
    , m_playerId(static_cast<short>(playerId))               // +0x12
{
    // Preserve the commander lookup even though its result is discarded.
    if (m_commanderEventId == 3 && m_playerId == -1 && squadManager)
        squadManagerIface()->getCommander(m_teamId);                     // ISquadManager +0x70
}

int CommanderEvent::getType() const
{
    return GECommander;
}

// ID 3 sets commander. IDs 0, 1, 2, and 4 are no-ops; the remaining handled
// invalid client operations issue fatal diagnostics.
void CommanderEvent::executeClient()
{
    switch (m_commanderEventId)
    {
    case 0:
    case 1:
    case 2:
    case 4:
        break;                                   // deliberate no-ops on the client

    case 3:
        if (squadManager)
            squadManagerIface()->setCommander(m_teamId, m_playerId);   // ISquadManager +0x6C
        break;

    case CESoldierView:                          // 6
        Debug(DEBUGTYPE_6, "Game/GameEvents/CommanderEvent.cpp", 106, "")
            << "CESoldierView should not be executed on client";
        break;

    case CEOverheadView:                         // 7
        Debug(DEBUGTYPE_6, "Game/GameEvents/CommanderEvent.cpp", 111, "")
            << "CEOverheadView should not be executed on client";
        break;

    default:                                     // includes id 5 - see the header's table
        Debug(DEBUGTYPE_6, "Game/GameEvents/CommanderEvent.cpp", 116, "")
            << m_commanderEventId << " is an unknown type.";
        break;
    }
}

// Accepts commander actions only on a server with a squad manager and a matching
// connection/player id. Commander checks use the connection's low byte. Action 8
// interprets the team field as the zoom flag; actions 6–8 use the connection id.
bool CommanderEvent::executeServer(int connectionId)
{
    if (!game->queryInterface(IID_IGameServer))
        return false;

    if (!squadManager)
        return false;

    if (connectionId != m_playerId)
        return false;

    const bool isCommander =
        squadManagerIface()->getCommander(m_teamId)                    // ISquadManager +0x70
            == static_cast<unsigned char>(connectionId);                    // byte-truncated

    switch (m_commanderEventId)
    {
    case 0:
        if (squadManager)
            squadManagerIface()->acceptCommander(m_teamId, m_playerId);            // +0x60
        break;

    case 1:
        if (squadManager)
            squadManagerIface()->declineCommander(m_teamId, m_playerId);           // +0x64
        break;

    case 2:
        if (squadManager && isCommander)
            squadManagerIface()->resignCommander(m_teamId);                        // +0x68
        break;

    case 3:
        break;                                   // deliberate no-op on the server

    case 4:
        if (squadManager)
            squadManagerIface()->applyForCommander(m_teamId, m_playerId);          // +0x58
        break;

    case CESoldierView:                          // 6
        if (squadManager)
            squadManagerIface()->setCommanderToSoldierMode(connectionId);          // +0x78
        break;

    case CEOverheadView:                         // 7
        if (squadManager && isCommander)
            squadManagerIface()->setCommanderToOverheadCamMode(connectionId);      // +0x7C
        break;

    case 8:
        if (squadManager && isCommander)
        {
            squadManagerIface()->setCommanderOverheadCamZoomMode(connectionId,
                                                        m_teamId != 0);                 // +0x80
        }
        break;

    default:                                     // includes id 5 - see the header's table
        Debug(DEBUGTYPE_6, "Game/GameEvents/CommanderEvent.cpp", 197, "")
            << m_commanderEventId << " is an unknown type.";
        break;
    }

    return true;
}

// deSerialize - 0x806C3F0, 258 B. Four bits, a byte, then the sign-and-magnitude player id.
bool CommanderEvent::deSerialize(io::BitStream* stream)
{
    int commanderEventId = 0;
    stream->readBits(&commanderEventId, commanderEventIdBits());
    m_commanderEventId = commanderEventId;

    int teamId = 0;
    stream->readBits(&teamId, 8);
    m_teamId = static_cast<unsigned char>(teamId);

    int negative = 0;
    stream->readBits(&negative, 1);

    int magnitude = 0;
    stream->readBits(&magnitude, 15);
    m_playerId = static_cast<short>((negative == 1) ? -magnitude : magnitude);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool CommanderEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int commanderEventId = m_commanderEventId;
    stream->writeBits(&commanderEventId, commanderEventIdBits());

    int teamId = m_teamId;
    stream->writeBits(&teamId, 8);

    if (m_playerId < 0)
    {
        int negative = 1;
        stream->writeBits(&negative, 1);
        int magnitude = -m_playerId;
        stream->writeBits(&magnitude, 15);
    }
    else
    {
        int negative = 0;
        stream->writeBits(&negative, 1);
        int magnitude = m_playerId;
        stream->writeBits(&magnitude, 15);
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
