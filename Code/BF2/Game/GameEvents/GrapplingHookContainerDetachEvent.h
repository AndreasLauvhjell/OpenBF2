// Detaches a rope rider and places the soldier at the supplied position.
// Clients position before detaching; servers detach before positioning. Both destroy the container.
// Default construction zeroes only the position. Client execution includes the local player.

#ifndef GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERDETACHEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERDETACHEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class GrapplingHookContainerDetachEvent : public GameEvent   // 0x1C
{
public:
    // attested: (int, const Vec3&). 0x8071CD0 (C2) / 0x8071D30 (C1).
    GrapplingHookContainerDetachEvent(int playerId, const Vec3& position);
    GrapplingHookContainerDetachEvent();                     // the maker's form - zeroes the Vec3 only
    virtual ~GrapplingHookContainerDetachEvent() {}

    virtual int  getType() const;                            // = 62
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int  m_playerId;     // NOT initialised by the default constructor
    /* +0x10 */ Vec3 m_position;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERDETACHEVENT_H
