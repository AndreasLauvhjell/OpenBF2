// Updates a rope segment's endpoints; the server applies and rebroadcasts the event.
// Endpoints use the stream's compression reference at scale 1000.
// Requires a valid segment index: both sides index the inline segment array unchecked.
// [UNRESOLVED] m_flag28 is transmitted and stored, but its meaning is unknown.

#ifndef GAME_GAMEEVENTS_GRAPPLINGHOOKUPDATEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_GRAPPLINGHOOKUPDATEEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class GrapplingHookUpdateEvent : public GameEvent            // 0x2C
{
public:
    GrapplingHookUpdateEvent();                              // the only constructor - zeroes everything
    virtual ~GrapplingHookUpdateEvent() {}

    virtual int  getType() const;                            // = 64
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // applies AND rebroadcasts
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // public - both producers assign these from outside the class.
    /* +0x0C */ unsigned short m_ropeNetId;
    /* +0x0E */ unsigned char  m_segmentIndex;
    /* +0x10 */ Vec3           m_start;
    /* +0x1C */ Vec3           m_end;
    /* +0x28 */ bool           m_flag28;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_GRAPPLINGHOOKUPDATEEVENT_H
