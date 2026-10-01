// Renames a squad using an inline name capped at 31 characters plus a terminator.
// The wire sends squad, team, player, length and name bytes in that order.

#ifndef GAME_GAMEEVENTS_CHANGESQUADNAMEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_CHANGESQUADNAMEEVENT_H

#include <string>

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class ChangeSquadNameEvent : public GameEvent                // 0x30
{
public:
    // The longest name that fits, from the constructor's own clamp. Named here so deSerialize's
    // bound and the constructor's clamp cannot drift apart.
    enum { MAX_NAME_LENGTH = 31 };

    ChangeSquadNameEvent(int team, int squadId, int playerId, const std::string& name);
    ChangeSquadNameEvent() : m_nameLength(0), m_team(0), m_squadId(0), m_playerId(0)
        { m_name[0] = '\0'; }                                // the maker's form
    virtual ~ChangeSquadNameEvent() {}

    virtual int  getType() const;                            // GEChangeSquadName (22)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ char          m_name[MAX_NAME_LENGTH + 1];
    /* +0x2C */ unsigned char m_nameLength;
    /* +0x2D */ unsigned char m_team;
    /* +0x2E */ unsigned char m_squadId;
    /* +0x2F */ unsigned char m_playerId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_CHANGESQUADNAMEEVENT_H
