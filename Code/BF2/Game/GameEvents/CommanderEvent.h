// Commander requests and state notifications share this event. Id 3 updates clients;
// ids 0/1/2/4/6/7/8 serve server requests. Unsupported directions and id 5 are fatal.
// Player id uses signed-magnitude encoding; -1 is a sentinel. Type 8 uses teamId
// as a Boolean. [UNRESOLVED] Only CESoldierView and CEOverheadView names are attested.

#ifndef GAME_GAMEEVENTS_COMMANDEREVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_COMMANDEREVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class CommanderEvent : public GameEvent                      // 0x14
{
public:
    // Only these two enumerator names are identified; other event types remain numeric.
    enum CommanderEventTypes
    {
        CESoldierView  = 6,
        CEOverheadView = 7
    };

    // attested: _ZN4dice3hfe14CommanderEventC2ENS1_19CommanderEventTypesEii is
    // (CommanderEventTypes, int, int).
    CommanderEvent(CommanderEventTypes type, int teamId, int playerId);
    CommanderEvent() : m_commanderEventId(0), m_teamId(0), m_playerId(0) {}
    virtual ~CommanderEvent() {}

    virtual int  getType() const;                            // GECommander (19)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int           m_commanderEventId;
    /* +0x10 */ unsigned char m_teamId;
    /* +0x11 */ // one byte of alignment padding
    /* +0x12 */ short         m_playerId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_COMMANDEREVENT_H
