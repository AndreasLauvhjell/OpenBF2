// Initializes an already-created client rope with its owning player and endpoint data.
// Player IDs narrow to eight wire bits; the -1 check cannot match a decoded byte.
// Default construction initializes only the vectors; senders must fill the remaining fields.
// [UNRESOLVED] The scalar and two vectors retain unnamed roles in rope initialization.

#ifndef GAME_GAMEEVENTS_GRAPPLINGHOOKCREATEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_GRAPPLINGHOOKCREATEEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class GrapplingHookCreateEvent : public GameEvent            // 0x30
{
public:
    // attested: (unsigned short, int, float, Vec3, Vec3). 0x80727F0 (C2) / 0x8072880 (C1).
    GrapplingHookCreateEvent(unsigned short ropeNetId, int playerId, float field14,
                             Vec3 fieldA, Vec3 fieldB);
    GrapplingHookCreateEvent();                              // the maker's form - zeroes the two Vec3s
    virtual ~GrapplingHookCreateEvent() {}

    virtual int  getType() const;                            // = 63
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // a no-op query
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned short m_ropeNetId;   // NOT initialised by the default constructor
    /* +0x10 */ int            m_playerId;    // NOT initialised by the default constructor
    /* +0x14 */ float          m_field14;     // NOT initialised by the default constructor
    /* +0x18 */ Vec3           m_fieldA;
    /* +0x24 */ Vec3           m_fieldB;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_GRAPPLINGHOOKCREATEEVENT_H
