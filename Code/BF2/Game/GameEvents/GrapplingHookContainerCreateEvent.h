// Replay the server's rope attachment by creating a ride container for a player.
// Default payload fields are uninitialized until deserialization. The eight-bit
// player id decodes to 0..255, so the -1 guard only applies to local construction.
// [UNRESOLVED] The trailing int/float attachment parameters retain unnamed roles.
// RW-38 tracks separate late-join gadget reconstruction.

#ifndef GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERCREATEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERCREATEEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class GrapplingHookContainerCreateEvent : public GameEvent   // 0x1C
{
public:
    // attested: (int, int, int, float). 0x8071850 (C2) / 0x8071890 (C1).
    GrapplingHookContainerCreateEvent(int playerId, int ropeNetId, int field14, float field18);
    GrapplingHookContainerCreateEvent() {}                   // the maker's form - initialises nothing
    virtual ~GrapplingHookContainerCreateEvent() {}

    virtual int  getType() const;                            // = 60
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // a no-op query
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int   m_playerId;
    /* +0x10 */ int   m_ropeNetId;
    /* +0x14 */ int   m_field14;
    /* +0x18 */ float m_field18;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_GRAPPLINGHOOKCONTAINERCREATEEVENT_H
