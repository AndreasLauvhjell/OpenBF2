// Join-squad request and shared payload base for leave/leader events. Wire order
// is team, player, squad, one byte each. The field-setting constructor's argument
// order is [INFERRED]; deserialization supplies maker-created payloads.

#ifndef GAME_GAMEEVENTS_JOINSQUADEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_JOINSQUADEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class JoinSquadEvent : public GameEvent                     // 0x10
{
public:
    // [INFERRED] - see the warning above. No ELF constructor exists to read the order from.
    JoinSquadEvent(int team, int playerId, int squadId);
    JoinSquadEvent() : m_team(0), m_playerId(0), m_squadId(0) {}   // the maker's form
    virtual ~JoinSquadEvent() {}

    virtual int  getType() const;                           // GEJoinSquad (16)
    virtual void executeClient();                           // empty, see the .cpp
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned char m_team;
    /* +0x0D */ unsigned char m_playerId;
    /* +0x0E */ unsigned char m_squadId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_JOINSQUADEVENT_H
