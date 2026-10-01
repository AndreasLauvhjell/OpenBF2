// Announces the completed round, winning team and victory type to clients.
// Ticket history is ignored; the winner and victory consumers narrow their values to bytes.
// Construction also applies the event on a server with a local human player.
// Receiving this event on the server is a fatal error.

#ifndef GAME_GAMEEVENTS_ENDOFROUNDEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_ENDOFROUNDEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

struct TicketHistory;   // Game/IGameLogic.h:52 - [UNRESOLVED] layout

class EndOfRoundEvent : public GameEvent                     // 0x18
{
public:
    // ticketHistory is accepted AND discarded - see the header block.
    EndOfRoundEvent(int roundId, const TicketHistory& ticketHistory,
                    int winnerTeamId, int victoryType);
    EndOfRoundEvent() : m_roundId(0), m_winnerTeamId(0), m_victoryType(0) {}
    virtual ~EndOfRoundEvent() {}

    virtual int  getType() const;                            // GEEndOfRound (48)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // aborts
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int m_roundId;          // server sender supplies roundsPerMap-roundsPlayed-1
    /* +0x10 */ int m_winnerTeamId;
    /* +0x14 */ int m_victoryType;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_ENDOFROUNDEVENT_H
