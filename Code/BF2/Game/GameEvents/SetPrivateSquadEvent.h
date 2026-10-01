// Sets squad privacy. Constructor arguments are team, squad and privacy;
// the wire sends the squad byte before the team byte, then the privacy bit.

#ifndef GAME_GAMEEVENTS_SETPRIVATESQUADEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_SETPRIVATESQUADEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class SetPrivateSquadEvent : public GameEvent                // 0x10
{
public:
    SetPrivateSquadEvent(int team, int squadId, bool isPrivate);
    SetPrivateSquadEvent() : m_team(0), m_squadId(0), m_isPrivate(false) {}   // the maker's form
    virtual ~SetPrivateSquadEvent() {}

    virtual int  getType() const;                            // GESetPrivateSquad (23)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned char m_team;
    /* +0x0D */ unsigned char m_squadId;
    /* +0x0E */ bool          m_isPrivate;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_SETPRIVATESQUADEVENT_H
