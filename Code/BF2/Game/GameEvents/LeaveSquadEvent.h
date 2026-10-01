// Removes a player from a squad, with server permission checks for self-removal or the leader.
// The additional clearInvite bit clears a pending invitation before removing membership.
// The inherited team/player/squad bytes retain their wire order.

#ifndef GAME_GAMEEVENTS_LEAVESQUADEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_LEAVESQUADEVENT_H

#include "JoinSquadEvent.h"

namespace dice {
namespace hfe {

class LeaveSquadEvent : public JoinSquadEvent                // 0x10
{
public:
    // [INFERRED] argument order - no ELF constructor exists for this class either.
    LeaveSquadEvent(int team, int playerId, int squadId, bool clearInvite);
    LeaveSquadEvent() : m_clearInvite(false) {}              // the maker's form
    virtual ~LeaveSquadEvent() {}

    virtual int  getType() const;                            // GELeaveSquad (17)
    virtual void executeClient();                            // empty, see the .cpp
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0F */ bool m_clearInvite;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_LEAVESQUADEVENT_H
