// Client-to-server spotting event. The server notifies Python with the connection
// id, optionally notifies bots, then broadcasts the payload to playing clients.
// The sender-supplied HUD event id is forwarded unchanged. Only position is
// initialized by the default constructor. Keep team byte-sized; wire order is
// net ids, position, team, vehicle type, then HUD id.

#ifndef GAME_GAMEEVENTS_SPOTTEDEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_SPOTTEDEVENT_H

#include "../Common/GameEvent.h"
#include "../IGame.h"                      // world::VehicleType
#include "../../Math/Vec3.h"               // Vec2

namespace dice {
namespace hfe {

class SpottedEvent : public GameEvent                        // 0x24
{
public:
    // The only constructor. There is no parameterised form: sendSpottedMessage default-constructs
    // and then assigns the fields one at a time, which is what makes them public below.
    SpottedEvent();

    virtual ~SpottedEvent() {}

    virtual int  getType() const;                            // GESpotted (29)
    virtual void executeClient();                            // empty
    virtual bool executeServer(int connectionId);            // the whole class
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // public, and that is read rather than chosen: Game::sendSpottedMessage writes all six from
    // outside the class with no accessor in sight.
    /* +0x0C */ unsigned short     m_spotterVehicleNetId;
    /* +0x0E */ unsigned short     m_targetNetId;
    /* +0x10 */ unsigned char      m_team;          // ONE byte - see the header block
    /* +0x14 */ Vec2               m_position;      // the only field the constructor touches
    /* +0x1C */ world::VehicleType m_vehicleType;
    /* +0x20 */ int                m_hudEventId;    // 43 from the producer; trusted on receipt
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_SPOTTEDEVENT_H
