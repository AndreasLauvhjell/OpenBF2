// Replicates a player's progress along a rope segment in both directions.
// Requires a valid segment index; indexing is unchecked and the interpolation parameter is unclamped.
// Clients skip their local player; servers apply and rebroadcast the update.
// [UNRESOLVED] m_field18 is stored in the container with no established meaning.

#ifndef GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERUPDATEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERUPDATEEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class GrapplingHookContainerUpdateEvent : public GameEvent   // 0x1C
{
public:
    // attested: (int, int, float, float). 0x8072200 (C2) / 0x8072240 (C1).
    GrapplingHookContainerUpdateEvent(int playerId, int segmentIndex, float t, float field18);
    GrapplingHookContainerUpdateEvent() {}                   // the maker's form - initialises nothing
    virtual ~GrapplingHookContainerUpdateEvent() {}

    virtual int  getType() const;                            // = 61
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // applies AND rebroadcasts
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int   m_playerId;
    /* +0x10 */ int   m_segmentIndex;
    /* +0x14 */ float m_t;
    /* +0x18 */ float m_field18;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERUPDATEEVENT_H
