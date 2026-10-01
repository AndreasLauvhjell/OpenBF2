// Armor interface used by damage, supply, spawning and wreck logic.
// Preserves the complete 74-slot ELF surface, with the destructor last and getter constness unchanged.
// [UNRESOLVED] Original interface/concrete-class split; ArmorGLComp is the sole known implementer.
// Container and Vec3 getters return references/pointers to existing storage. Source placement is [INFERRED].

#ifndef GAME_OBJECTS_GAMELOGICCOMP_IARMORGLCOMP_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAMELOGICCOMP_IARMORGLCOMP_H

#include <list>
#include <string>

#include "../IObjectComponent.h"
#include "../../../Core/WeakPtr.h"
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

class IPlayer;

namespace world {

class IObject;

// The damage ledger record - dice::hfe::world::DamageFromPlayer, ctor 0x82173F0 / 0x8217430 (55 B):
// `player->getWeakPtr()` (IPlayer +0x1C) into +0x00, the float amount into +0x04, the double world
// time into +0x08. Sixteen bytes, which is what makes the ledger's list nodes 24 (allocate(24) in the
// component's ctor). worldTime is a double: WorldPref::getWorldTime() returns one, and narrowing
// it to float re-orders records made in the same frame.
struct DamageFromPlayer
{
    // (ELF symbol DamageFromPlayer(IPlayer*, float, double)): the player's weak
    // reference, the amount, the time. Defined in ArmorGLComp.cpp -
    DamageFromPlayer(IPlayer* player, float amount, double worldTime);

    WeakPtr<IPlayer> player;    // +0x00
    float            amount;    // +0x04
    double           worldTime; // +0x08
};

class IArmorGLComp : public IObjectComponent           // IID_IArmorGLComp = 50345 (Core/Ids.h)
{
public:
    // slots 14..25: the twelve the spawner sitting recovered first
    virtual void  setHitPoints(float hitPoints) = 0;                        // +0x38 slot 14
    virtual float getHitPoints() = 0;                                       // +0x3C slot 15
    virtual void  damage(float amount, IPlayer* perpetrator) = 0;           // +0x40 slot 16
    virtual void  heal(float amount) = 0;                                   // +0x44 slot 17
    virtual void  revive(float amount) = 0;                                 // +0x48 slot 18
    virtual void  setSoldierIsRevived() = 0;                                // +0x4C slot 19
    virtual bool  isSendingMessage() = 0;                                   // +0x50 slot 20
    virtual bool  isDestroyed() = 0;                                        // +0x54 slot 21 - THE
                                                                            // spawner's wreck test
    virtual bool  isCriticalDamaged() = 0;                                  // +0x58 slot 22
    virtual int   getStatus() const = 0;                                    // +0x5C slot 23
    virtual bool  collision() = 0;                                          // +0x60 slot 24 - the
                                                                            // accessor
    virtual bool  collision(int, int, IObject* other) = 0;                  // +0x64 slot 25 - the
                                                                            // handler

    // slots 26..29: the 16-entry collision ring
    virtual void      addColObject(IObject* object) = 0;                    // +0x68 slot 26
    virtual IObject** getColObjectList() = 0;                               // +0x6C slot 27 - the
                                                                            // array base (+0x68)
    virtual void      clearColObjectList() = 0;                             // +0x70 slot 28
    virtual bool      isInColList(IObject* object) = 0;                     // +0x74 slot 29

    // slots 30..39: template pass-throughs (several pick the wreck value when wrecked)
    virtual float getAngleMod() = 0;                                        // +0x78 slot 30
    virtual float getAttackDamage() = 0;                                    // +0x7C slot 31
    virtual bool  getDamageFromWater() = 0;                                 // +0x80 slot 32
    virtual float getMaxHitPoints() = 0;                                    // +0x84 slot 33
    virtual float getMaxWreckHitPoints() = 0;                               // +0x88 slot 34
    virtual float getSpeedMod() = 0;                                        // +0x8C slot 35
    virtual float getExplosionForceMod() = 0;                               // +0x90 slot 36
    virtual float getExplosionForceMax() = 0;                               // +0x94 slot 37
    virtual bool  getCollisionWithWater() = 0;                              // +0x98 slot 38
    virtual int   getDefaultMaterial() const = 0;                           // +0x9C slot 39

    // slots 40..60: the "last hit" record
    virtual void  setLastCollisionHeight(float height) = 0;                 // +0xA0 slot 40
    virtual float getLastCollisionHeight() = 0;                             // +0xA4 slot 41
    virtual void  setLastHitMaterialIndex(int index) = 0;                   // +0xA8 slot 42
    virtual int   getLastHitMaterialIndex() = 0;                            // +0xAC slot 43
    virtual int   getLastGroundHitMaterialIndex() = 0;                      // +0xB0 slot 44
    virtual void  setLastHitBoneIndex(int index, bool force) = 0;           // +0xB4 slot 45
    virtual int   getLastHitBoneIndex() const = 0;                          // +0xB8 slot 46
    virtual void  setLastHitDirection(const Vec3& direction, bool force) = 0;   // +0xBC slot 47
    virtual const Vec3& getLastHitDirection() const = 0;                    // +0xC0 slot 48
    virtual void  setLastHitPlace(Vec3 place) = 0;                          // +0xC4 slot 49 - by
                                                                            // value, as mangled
    virtual const Vec3& getLastHitPlace() = 0;                              // +0xC8 slot 50
    virtual void  setLastHitPlayer(IPlayer* player) = 0;                    // +0xCC slot 51
    virtual IPlayer* getLastHitPlayer() = 0;                                // +0xD0 slot 52
    virtual void  setLastHitCanTK(bool canTK) = 0;                          // +0xD4 slot 53
    virtual bool  getLastHitCanTK() = 0;                                    // +0xD8 slot 54
    virtual void  setLastHitWeapon(IObject* weapon) = 0;                    // +0xDC slot 55
    virtual IObject* getLastHitWeapon() = 0;                                // +0xE0 slot 56
    virtual void  setLastHitVehicle(IObject* vehicle) = 0;                  // +0xE4 slot 57
    virtual IObject* getLastHitVehicle() = 0;                               // +0xE8 slot 58
    virtual void  setLastDamageType(int type) = 0;                          // +0xEC slot 59
    virtual int   getLastDamageType() = 0;                                  // +0xF0 slot 60

    // slots 61..63: the three damage ledgers, returned BY address
    virtual std::list<DamageFromPlayer>*  getDamageFromPlayers() = 0;        // +0xF4 slot 61
    virtual std::list<DamageFromPlayer>*  getVehicleDamageFromPlayers() = 0; // +0xF8 slot 62
    virtual std::list<WeakPtr<IPlayer> >* getTeamDamagePlayers() = 0;        // +0xFC slot 63

    // slots 64..71
    virtual void  fullDestroy() = 0;                                        // +0x100 slot 64
    virtual bool  isInsideRepairRadius(const Vec3& position) = 0;           // +0x104 slot 65
    virtual bool  isInvunerable() = 0;                                      // +0x108 slot 66 -
                                                                            // dice's spelling
    virtual bool  isSpectacularWreckArmorEffect(const std::string& name) = 0;   // +0x10C slot 67
    virtual void  accumulateDamageNorm(float amount) = 0;                   // +0x110 slot 68
    virtual float getAccumulatedDamageNorm() const = 0;                     // +0x114 slot 69
    virtual void  resetAccumulatedDamageNorm() = 0;                         // +0x118 slot 70
    virtual void  ClearEffects() = 0;                                       // +0x11C slot 71 -
                                                                            // dice's capital C

    // slots 72/73: the destructor pair, last in the table
    virtual ~IArmorGLComp() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAMELOGICCOMP_IARMORGLCOMP_H
