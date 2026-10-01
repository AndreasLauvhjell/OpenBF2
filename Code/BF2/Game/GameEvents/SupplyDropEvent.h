// Requests a team supply drop at a two-component map position.
// The wire sends a 32-bit team value before the coordinates, but execution reads only its low byte.
// The request constructor also initializes only that byte.
// [UNRESOLVED] The position components' original names are unknown.

#ifndef GAME_GAMEEVENTS_SUPPLYDROPEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_SUPPLYDROPEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class SupplyDropEvent : public GameEvent                     // 0x18
{
public:
    // [INFERRED] argument order - no ELF constructor exists.
    SupplyDropEvent(float positionX, float positionZ, int team);
    SupplyDropEvent() : m_team(0) { m_position.x = 0.0f; m_position.y = 0.0f; }
    virtual ~SupplyDropEvent() {}

    virtual int  getType() const;                            // GESupplyDrop (37)
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeClient is deliberately NOT declared - the slot is inherited.

protected:
    /* +0x0C */ Math::Vec2 m_position;
    /* +0x14 */ int   m_team;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_SUPPLYDROPEVENT_H
