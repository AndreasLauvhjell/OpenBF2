// Replicate where a sticky projectile attached. Target id zero means the world;
// object attachments additionally carry target generation and child index.
// The transform sends three floats per row in order 2,0,1,3; the last column
// is implied. Player id 255 denotes an unowned projectile.

#ifndef GAME_GAMEEVENTS_STICKYPROJECTILEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_STICKYPROJECTILEEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace world { class GenericProjectile; class IObject; }

class StickyProjectileEvent : public GameEvent               // 0x54
{
public:
    // attested: (world::GenericProjectile*, world::IObject*, const Mat4&). 0x807C220 (C2) /
    //  (C1). The producer constructor is ; executeClient retains named frontiers.
    StickyProjectileEvent(world::GenericProjectile* projectile, world::IObject* target,
                          const Mat4& transform);
    StickyProjectileEvent();                                 // the maker's form - identity matrix only
    virtual ~StickyProjectileEvent() {}

    virtual int  getType() const;                            // GEStickyProjectile (32)
    virtual void logInfo(bool, int);                         // ITS OWN, empty
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // a no-op query
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ Mat4           m_transform;
    /* +0x4C */ unsigned short m_projectileNetId;
    /* +0x4E */ unsigned short m_targetNetId;
    /* +0x50 */ unsigned char  m_targetNetIdCount;   // conditional on m_targetNetId
    /* +0x51 */ unsigned char  m_playerId;           // defaults to -1 (255)
    /* +0x52 */ unsigned char  m_childIndex;         // conditional on m_targetNetId
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_STICKYPROJECTILEEVENT_H
