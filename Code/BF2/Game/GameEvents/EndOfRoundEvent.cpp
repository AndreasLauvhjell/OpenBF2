#include "EndOfRoundEvent.h"

#include "../IGameLogic.h"                       // gameLogic - isServer / resetPlayerFreeCameraPosition
#include "../GameLogic/ScoreSystem/ScoreManager.h"   // forceSetWinner / setVictoryType
#include "../../Core/Debug.h"                    // executeServer's abort
#include "../../Core/IBase.h"
#if !DEDICATED
#include "../../Core/ClassManager.h"             // PE-only SwiffHost lookup
#include "../../Menu/ISwiffHost.h"               // setIntF4C (rounds left)
#endif
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getLocalHumanPlayer

namespace dice {
namespace hfe {
namespace world { extern IBase* scoreManager; }  // ds:0x8AD1740 - Core/Globals.cpp (IBase*, the tree's form)
#if !DEDICATED
extern ClassManager* classManager;
#endif

namespace {
// .rodata dword GEEndOfRound (0x88892E8) = 48.
const int GEEndOfRound = 48;
} // namespace

//  (C2) / 0x8070990 (C1), 98 B. Three stores and the loopback - see the header.
EndOfRoundEvent::EndOfRoundEvent(int roundId, const TicketHistory& /*ticketHistory*/,
                                 int winnerTeamId, int victoryType)
    : GameEvent()
    , m_roundId(roundId)                                     // +0x0C
    , m_winnerTeamId(winnerTeamId)                           // +0x10
    , m_victoryType(victoryType)                             // +0x14
{
    // ticketHistory is accepted and discarded - it is not a member and never reaches the wire.

    if (gameLogic->isServer() && world::playerManager->getLocalHumanPlayer())   // +0x30
        executeClient();
}

int EndOfRoundEvent::getType() const
{
    return GEEndOfRound;
}

// Restores the local camera before publishing round results. Does nothing without
// game logic; a missing player manager skips only the camera reset. Requires the
// score manager. [PLATFORM] Clients also publish rounds remaining to the UI.
void EndOfRoundEvent::executeClient()
{
    if (!gameLogic)
        return;

    if (world::playerManager)
    {
        gameLogic->resetPlayerFreeCameraPosition(
            world::playerManager->getLocalHumanPlayer());                        // +0x30, then +0x214
    }

    // unguarded - see above. reinterpret_cast, not static_cast: the global is typed IBase* and
    // world::ScoreManager is a standalone class in this tree rather than an IBase subclass, so the
    // two are formally unrelated. Same cast Game/GameLogic/ServerGameLogic.cpp:1157 already uses on
    // this exact global.
    world::ScoreManager* scores = reinterpret_cast<world::ScoreManager*>(world::scoreManager);
    scores->forceSetWinner(static_cast<unsigned char>(m_winnerTeamId));          // +0x1C
    scores->setVictoryType(static_cast<unsigned char>(m_victoryType));           // +0x20

#if !DEDICATED
    // [PLATFORM] BF2.exe tail 0x542A30-0x542A5A. Unlike GameLogic's loading-state writes, this
    // lookup result IS null-checked. endOfRound.swf requires this value to be > 0 before it exposes
    // Join Game and Auto Ready.
    ui::ISwiffHost* host = static_cast<ui::ISwiffHost*>(
        classManager->getSingleton(std::string("SwiffHost")));
    if (host)
        host->setIntF4C(m_roundId);                                                // +0x35C
#endif
}

// executeServer - 0x8070BF0, 242 B. A hard stop - Debug type 6 aborts. The return below is
// unreachable in the shipped build; it is here because the slot returns bool.
bool EndOfRoundEvent::executeServer(int)
{
    Debug(DEBUGTYPE_6, "Game/GameEvents/EndOfRoundEvent.cpp", 120, "")
        << "Server shouldnt get endofroundevents from outside.";
    return true;
}

// deSerialize - 0x8070A00, 158 B. Three full-width ints, in field order.
bool EndOfRoundEvent::deSerialize(io::BitStream* stream)
{
    int roundId = 0;
    stream->readBits(&roundId, 32);
    m_roundId = roundId;

    int winnerTeamId = 0;
    stream->readBits(&winnerTeamId, 32);
    m_winnerTeamId = winnerTeamId;

    int victoryType = 0;
    stream->readBits(&victoryType, 32);
    m_victoryType = victoryType;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool EndOfRoundEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int roundId = m_roundId;
    stream->writeBits(&roundId, 32);

    int winnerTeamId = m_winnerTeamId;
    stream->writeBits(&winnerTeamId, 32);

    int victoryType = m_victoryType;
    stream->writeBits(&victoryType, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
