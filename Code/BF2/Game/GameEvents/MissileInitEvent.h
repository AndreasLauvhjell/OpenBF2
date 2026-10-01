// Server-to-client projectile ownership and launcher information; server execution
// is fatal. Missing objects are ignored, but an existing non-projectile is fatal.
// [CHOSEN name] useDefaultVehicle selects the starting vehicle for weapon lookup.
// Player id is byte-truncated and weapon slot uses four wire bits.

#ifndef GAME_GAMEEVENTS_MISSILEINITEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_MISSILEINITEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class MissileInitEvent : public GameEvent                    // 0x14
{
public:
    // attested: (unsigned short, int, int, bool). 0x8076B30 (C2) / 0x8076B80 (C1).
    MissileInitEvent(unsigned short projectileNetId, int playerId, int weaponSlot,
                     bool useDefaultVehicle);
    MissileInitEvent() {}                                    // the maker's form - initialises nothing
    virtual ~MissileInitEvent() {}

    virtual int  getType() const;                            // GEMissileInit (43)
    virtual void logInfo(bool, int);                         // ITS OWN, empty
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // always aborts
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned short m_projectileNetId;
    /* +0x0E */ unsigned char  m_playerId;
    /* +0x0F */ unsigned char  m_weaponSlot;
    /* +0x10 */ bool           m_useDefaultVehicle;   // [CHOSEN] - see the header block
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_MISSILEINITEVENT_H
