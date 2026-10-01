// Reports a player's night-vision state to the server.
// Player IDs are integers in memory but narrow to eight bits on the wire.

#ifndef GAME_GAMEEVENTS_SETNIGHTVISIONEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_SETNIGHTVISIONEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class SetNightVisionEvent : public GameEvent                 // 0x14
{
public:
    SetNightVisionEvent(int playerId, bool nightVision);
    SetNightVisionEvent() : m_playerId(0), m_nightVision(false) {}   // the maker's form
    virtual ~SetNightVisionEvent() {}

    virtual int  getType() const;                            // GESetNightVisionEvent (66)
    virtual void executeClient();                            // empty, see the .cpp
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int  m_playerId;
    /* +0x10 */ bool m_nightVision;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_SETNIGHTVISIONEVENT_H
