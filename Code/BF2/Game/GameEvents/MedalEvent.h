// Announces a medal and applies it locally on a server with a local human player.
// Server execution only reports whether the game exposes IGameServer.
// [UNRESOLVED] m_field10 and m_field14 retain their unknown meanings and 4/8-bit wire widths.
// [UNRESOLVED] Medal is represented as an opaque enum; its original type category is unknown.

#ifndef GAME_GAMEEVENTS_MEDALEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_MEDALEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

namespace world {
// [UNRESOLVED] enumerators, and [UNRESOLVED] whether this is an enum at all - see the header block.
// It belongs to the award/score subsystem, not to this event; when that lands, this declaration
// moves there.
enum Medal : int;
}

class MedalEvent : public GameEvent                          // 0x18
{
public:
    // attested: (dice::hfe::world::Medal, int, int). The ELF stores the third argument at +0x10
    // and the second at +0x14 - see the .cpp.
    MedalEvent(world::Medal medal, int field14, int field10);
    MedalEvent() : m_medal(0), m_field10(0), m_field14(0) {}    // the maker's form
    virtual ~MedalEvent() {}

    virtual int  getType() const;                            // GEMedal (41)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // a no-op, see above
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int m_medal;        // a world::Medal, stored as its four bytes
    /* +0x10 */ int m_field10;
    /* +0x14 */ int m_field14;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_MEDALEVENT_H
