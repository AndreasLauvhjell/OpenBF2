#include "SetSquadLeaderEvent.h"

#include "../../Core/IBase.h"
#include "../GameLogic/SquadManager/ISquadManager.h" // typed SquadManager dispatch

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GESetSquadLeader (0x888A684) = 28.
const int GESetSquadLeader = 28;

// [INFERRED] The squad setter's final flag suppresses broadcast. Client execution
// suppresses an echo; server execution broadcasts the authoritative change.
} // namespace

int SetSquadLeaderEvent::getType() const
{
    return GESetSquadLeader;
}

// executeClient - 0x807B5E0, 77 B. Apply the server's decision locally, suppressing the echo.
// Argument order taken from the raw pushes at 0x807B60D..0x807B628 - see the header's warning.
void SetSquadLeaderEvent::executeClient()
{
    if (squadManager)
        static_cast<ISquadManager*>(squadManager)->setSquadLeader(
            m_team, m_squadId, m_playerId, true);                                      // +0x38
}

// Requests leadership for the sending connection, ignoring the packet's player ID.
// Allows an absent leader or a leader matching either the full connection ID or its low byte.
// Returns true even when the request is ignored or the squad manager is absent.
bool SetSquadLeaderEvent::executeServer(int connectionId)
{
    if (!squadManager)
        return true;

    const int leader = static_cast<ISquadManager*>(squadManager)->getSquadLeader(
        m_team, m_squadId);                                                        // +0x3C

    const bool allowed = (leader == -1)
                      || (leader == connectionId)
                      || (leader == static_cast<unsigned char>(connectionId));   // byte-truncated, see above

    if (allowed)
        static_cast<ISquadManager*>(squadManager)->setSquadLeader(
            m_team, m_squadId, connectionId, false);                           // +0x38

    return true;
}

} // namespace hfe
} // namespace dice
