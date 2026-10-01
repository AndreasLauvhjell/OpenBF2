// Changes the squad leader using JoinSquadEvent's inherited three-byte payload and codec.
// The payload fields are team, player and squad; execution passes team, squad and player.

#ifndef GAME_GAMEEVENTS_SETSQUADLEADEREVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_SETSQUADLEADEREVENT_H

#include "JoinSquadEvent.h"

namespace dice {
namespace hfe {

class SetSquadLeaderEvent : public JoinSquadEvent            // 0x10
{
public:
    // [INFERRED] argument order - no ELF constructor exists for this class.
    SetSquadLeaderEvent(int team, int playerId, int squadId)
        : JoinSquadEvent(team, playerId, squadId) {}
    SetSquadLeaderEvent() {}                                 // the maker's form
    virtual ~SetSquadLeaderEvent() {}

    virtual int  getType() const;                            // GESetSquadLeader (28)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);

    // deSerialize / serialize are deliberately NOT declared - the base's are what the vtable
    // holds. Declaring overrides here would emit two symbols dice does not have and would put
    // this class's own functions in slots +0x2C/+0x30 where JoinSquadEvent's belong.
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_SETSQUADLEADEREVENT_H
