// Owns persistent effects for PlayerManager and dispatches them during player updates.
// Deletes effects when updateAndCull returns false; isObjectAffected intentionally always returns false.
// Client construction registers file-scope HUD variables; dedicated construction only creates the list.
// The sole member's STL layout differs: original ELF four bytes, PE twelve. Placement is [INFERRED].

#ifndef GAME_OBJECTS_FX_PERSISTENTEXPLOSIONEFFECTMANAGER_H     // [INFERRED] guard style
#define GAME_OBJECTS_FX_PERSISTENTEXPLOSIONEFFECTMANAGER_H

#include <list>

// DO NOT forward-declare Vec3 - `dice::hfe::Vec3` is a typedef (Math/Vec3.h:114), so `class Vec3;`
// is C2371. The trap is recorded in IO/BitStream.h, which was the third file to spring it.
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class IExplosionEffect;
class IObject;
class IPlayer;
class IProjectileObject;

class CPersistentExplosionEffectManager
{
public:
    CPersistentExplosionEffectManager();
    ~CPersistentExplosionEffectManager();

    // The entry point from the damage side: a projectile has gone off. Queries the object for
    // IID_IProjectileObject, builds the matching effect and fires its immediate half. Silent when
    // the object is null or is not a projectile.
    void handleExplosionEffect(IPlayer* causedBy, const Vec3& position, const IObject* projectile);

    // THE PER-player tick'S first statement. Player::update hands every player to this, every
    // fixed step, before anything else it does. With no live effects the walk is empty - which is
    // why an unbuilt manager was survivable right up until a player existed.
    void handleObject(IPlayer* player);

    // The per-frame half, driven only by the local human player's update (see Player::update's
    // gate): age every effect and destroy the ones that answer false.
    void updateExplosionEffects(float deltaTime);

    void killAllEffects();

    IExplosionEffect* createExplosionEffect(const Vec3& position,
                                           const IProjectileObject* projectile);

    bool isObjectAffected(const IPlayer* player) const;   // always false

private:
    template<class Effect>
    IExplosionEffect* initExplosionEffect(const Vec3&, const IProjectileObject*);
    /* +0x00 */ std::list<IExplosionEffect*> m_effects;   // the entire object
};

// The two client knobs the constructor binds to the HUD information layer: ds:0x9FDC3C (bool) and
// ds:0x9FDC40 (float), adjacent in .data with the flag at the lower address. Names [INFERRED] from
// the registered strings. ExplosionEffectFlashBang writes this shared storage; the manager's
// registration and the effect's three-channel fade state therefore observe the same values.
extern bool  blindingLightDamageShow;    // ds:0x9FDC3C, "BlindingLightDamageShow"
extern float blindingLightDamageAlpha;   // ds:0x9FDC40, "BlindingLightDamageAlpha"

// ds:0x8AB08E0 - defined in Core/Globals.cpp. Created by PlayerManager's constructor, deleted and
// nulled by its destructor, and read by Player::update and by nothing else.
extern CPersistentExplosionEffectManager* explosionEffectManager;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_FX_PERSISTENTEXPLOSIONEFFECTMANAGER_H
