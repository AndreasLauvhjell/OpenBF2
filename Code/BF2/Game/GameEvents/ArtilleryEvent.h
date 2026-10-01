// Artillery request (type 1, client to server) or fired notification (type 2,
// server to client). Sending either to the wrong side is fatal; types 0/3 are inert.
// Team occupies one byte in memory but 32 bits on the wire. Only target is
// initialized by default construction; producers must fill their payload fields.

#ifndef GAME_GAMEEVENTS_ARTILLERYEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_ARTILLERYEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"                 // Vec2

namespace dice {
namespace hfe {

class ArtilleryEvent : public GameEvent                      // 0x24
{
public:
    // The type discriminator's two values. [CHOSEN names] - the ELF compares against bare 1 and 2
    // and no enum type appears in any mangling, unlike RankEvent's and UnlockEvent's nested enums.
    // Declared as constants rather than an enum for exactly that reason: inventing a type name here
    // would be inventing frozen surface.
    static const int ArtilleryTypeRequestFire = 1;           // client -> server
    static const int ArtilleryTypeDidFire     = 2;           // server -> client

    // Dedicated ELF default producer constructor; its producers assign fields afterward.
    ArtilleryEvent();
#if !DEDICATED
    // [INFERRED] int spelling for team. [CHOSEN, not recovered] The unused final
    // word preserves the client calling convention.
    // [RETIRE-WHEN: ARTILLERY-CLIENT-CTOR-UNUSED-WORD] Recover its source type before replacing it.
    ArtilleryEvent(int team, Vec2 target, unsigned int unusedWord);
#endif

    virtual ~ArtilleryEvent() {}

    virtual int  getType() const;                            // GEArtillery (30)
    virtual void executeClient();                            // aborts on type 1
    virtual bool executeServer(int connectionId);            // aborts on type 2
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    /* +0x0C */ Vec2           m_target;         // the only field the constructor touches
    /* +0x14 */ unsigned char  m_team;           // 1-based; indexes the artillery manager
    /* +0x18 */ int            m_fireTick;
    /* +0x1C */ int            m_artilleryType;
    /* +0x20 */ unsigned short m_weaponNetId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_ARTILLERYEVENT_H
