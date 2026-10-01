// Persistent explosion effect interface owned and deleted by its manager; it does not derive from IBase.
// updateAndCull returns true to retain the effect, false to remove and delete it.
// Preserve the three methods before the destructor. Server TearGas/Molotov behavior is intentionally empty;
// their original objects hold only a vptr, unlike FlashBang. Placement is [INFERRED].

#ifndef GAME_OBJECTS_FX_IEXPLOSIONEFFECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_FX_IEXPLOSIONEFFECT_H

namespace dice {
namespace hfe {
namespace world {

class IPlayer;
class IProjectileObject;

class IExplosionEffect
{
public:
    virtual void handleImmediateEffect(IPlayer* causedBy,
                                       const IProjectileObject* projectile) = 0;   // +0x00
    virtual void handlePersistentEffect(IPlayer* player) = 0;                      // +0x04
    virtual bool updateAndCull(float deltaTime) = 0;                               // +0x08
    virtual ~IExplosionEffect() {}                                                 // +0x0C +0x10
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_FX_IEXPLOSIONEFFECT_H
