// Armor damage, repair and destruction state.
// Companion Game/Objects/GameLogicComp/ArmorGLComp.cpp is attested by __FILE__.

#ifndef GAME_OBJECTS_GAMELOGICCOMP_ARMORGLCOMP_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAMELOGICCOMP_ARMORGLCOMP_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "IArmorGLComp.h"
#include "../../../Core/WeakPtr.h"
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

class IPlayer;

namespace world {

class ArmorGLCompTemplate;
class DamageEffects;
class IObject;

class ArmorGLComp : public IArmorGLComp
{
public:
    // Tracked collision with an expiry timer, other object, and collision category.
    struct HandledCollision
    {
        IObject* object;    // +0x00 the other object (0 for land / water)
        float    timer;     // +0x04 decremented by dt in handleUpdate, erased below zero
        int      first;     // +0x08 the caller's first int; part of the "already handled" key
    };

    explicit ArmorGLComp(ArmorGLCompTemplate* objectTemplate);
    virtual ~ArmorGLComp();                                                 // (D2), 484 B - slots 72/73

    // IObjectComponent, slots 0..13
    virtual void  addRef();
    virtual int   getRef();                                                 // (+0x04)
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;
    virtual void  init(IObject* object);
    virtual void  destroy();
    virtual void  setObject(IObject* object);
    virtual IObject* getObject() const;
    virtual bool  hasUpdate() const;                                        // constant true
    virtual void  handleUpdate(float deltaTime);                            // THE tick
    virtual void  handleMessage(TemplateMessage message, IPlayer* player, bool flag);

    // IArmorGLComp, slots 14..71
    virtual void  setHitPoints(float hitPoints);
    virtual float getHitPoints();
    virtual void  damage(float amount, IPlayer* perpetrator);
    virtual void  heal(float amount);
    virtual void  revive(float amount);
    virtual void  setSoldierIsRevived();
    virtual bool  isSendingMessage();
    virtual bool  isDestroyed();
    virtual bool  isCriticalDamaged();
    // Returns 0 while alive, 1 while destroyed, and 2 at or below the negative wreck-health threshold.
    virtual int   getStatus() const;
    virtual bool  collision();
    virtual bool  collision(int, int, IObject* other);                      // ; returns bool (see the .cpp)
    virtual void      addColObject(IObject* object);
    virtual IObject** getColObjectList();
    virtual void      clearColObjectList();
    virtual bool      isInColList(IObject* object);
    virtual float getAngleMod();                                            // each - template
    virtual float getAttackDamage();                                        // pass-throughs
    virtual bool  getDamageFromWater();
    virtual float getMaxHitPoints();
    virtual float getMaxWreckHitPoints();
    virtual float getSpeedMod();
    virtual float getExplosionForceMod();                                   // wreck-aware
    virtual float getExplosionForceMax();                                   // wreck-aware
    virtual bool  getCollisionWithWater();
    virtual int   getDefaultMaterial() const;
    virtual void  setLastCollisionHeight(float height);
    virtual float getLastCollisionHeight();
    virtual void  setLastHitMaterialIndex(int index);                       // ?, 38 B
    virtual int   getLastHitMaterialIndex();
    virtual int   getLastGroundHitMaterialIndex();
    virtual void  setLastHitBoneIndex(int index, bool force);
    virtual int   getLastHitBoneIndex() const;
    virtual void  setLastHitDirection(const Vec3& direction, bool force);
    virtual const Vec3& getLastHitDirection() const;
    virtual void  setLastHitPlace(Vec3 place);
    virtual const Vec3& getLastHitPlace();
    virtual void  setLastHitPlayer(IPlayer* player);                        // ; WeakPtr take, no-op on a wreck
    virtual IPlayer* getLastHitPlayer();
    virtual void  setLastHitCanTK(bool canTK);
    virtual bool  getLastHitCanTK();
    virtual void  setLastHitWeapon(IObject* weapon);                        // ; WeakPtr take
    virtual IObject* getLastHitWeapon();
    virtual void  setLastHitVehicle(IObject* vehicle);                      // ; WeakPtr take
    virtual IObject* getLastHitVehicle();
    virtual void  setLastDamageType(int type);
    virtual int   getLastDamageType();
    virtual std::list<DamageFromPlayer>*  getDamageFromPlayers();
    virtual std::list<DamageFromPlayer>*  getVehicleDamageFromPlayers();
    virtual std::list<WeakPtr<IPlayer> >* getTeamDamagePlayers();
    virtual void  fullDestroy();
    virtual bool  isInsideRepairRadius(const Vec3& position);
    virtual bool  isInvunerable();                                          // dice's spelling
    virtual bool  isSpectacularWreckArmorEffect(const std::string& name);
    virtual void  accumulateDamageNorm(float amount);
    virtual float getAccumulatedDamageNorm() const;
    virtual void  resetAccumulatedDamageNorm();
    virtual void  ClearEffects();                                           // dice's capital C

    // non-virtual
    // signatures from THE demangled symbols, not short names: updateStatus takes TWO arguments,
    // and `collision` is overloaded (above). Written ones say so; the rest carry their ELF size and
    // fault BY name if a live component reaches them.
    void  updateStatus(float hitPoints, bool notify);    // the state machine
    void  damageCheckPerputrator(IPlayer*, float);       // dice's spelling
    void  playEffect();                                  // / BF2.exe 0x6880A0, 1,188 B -
    void  updateDestroy();
    void  onDestroyedObjectAI();
    // THE same lookup AS ResetStrategicObject, true bumps the holder's
    // destroyed-piece count, false drops it. Unlike Reset it aborts on a missing key -
    // Debug(DEBUGTYPE_6) "Couldn't update number of destroyed parts!" at ArmorGLComp.cpp:1744,
    // which is the only attested line number this file has. Nothing clamps the count, so an
    // unpaired false would drive it negative.
    void  updateStrategicObject(bool destroyed);
    bool  checkIfSpectacularHit(float amount);           // ; returns bool (0/1 in eax, damage() stores it)
    void  updateWreckStatus();
    // the DamageEffects of
    // the damage state just entered, 0 without a state change. `state` is read by neither image.
    DamageEffects* getEffect(int state);
    // dead IN both images: the ELF body (0x821AD50, 349 B) has zero references and BF2.exe has
    // no copy at all - the client never reads m_wreckEffects. Declared only, deliberately unwritten.
    DamageEffects* getWreckEffect(int state);            // [FRONTIER] - no caller
    // NO caller IN either image (ELF 0x821B780: zero references; BF2.exe carries no copy). It is
    // the only writer that raises m_byte25C, so playEffect's m_byte25C test never fires on a
    // reset in practice - only on the constructor's leftover byte.
    void  reset();                                       // written (needs updateStatus)
    void  setUseDestroyedCollision(IObject*, bool);      // written; the soldier push-out is a ghost
    void  setUseDestroyedLodRecursive(IObject*, bool);   // written
    void  setUseDestroyedLod(bool, bool);                // written
    void  UpdateWaterDamage(float);                      // written  (capital U - dice's)
    void  UpdateUpSideDownDamage(float);                 // written  (capital U - dice's)
    void  UpdateInWaterStatus();                         // written
    bool  isCloseToGround(Vec3& normal);                 // written
    // the inner block is no longer a ghost. Zeroes the destroyed-
    // piece count on the holder that owns this object's strategic-object group.
    void  ResetStrategicObject();
    void  destroyOrResetObject();                        // written
    void  StopAllEffects();                              // / BF2.exe 0x6862C0 - written; ~ArmorGLComp and getEffect call it
    void  _setHitPoints(float hitPoints);                // the underscore is dice's

    // Not an ELF member: ObjectSpawner::spawnObject reads the concrete component's +0x08 (m_template)
    // straight off the object to ask getCanBeDestroyed(). This build spells that read as an inline
    // accessor rather than a friend; same load, same value. [REBUILD CONVENIENCE]
    ArmorGLCompTemplate* getTemplateForSpawner() const { return m_template; }

    friend class ArmorGLCompTemplate;
    // [CHOSEN, not recovered] access route for LightSource's proven direct damage-field reads.
    friend class LightSource;

protected:
    // layout, from the constructor. ELF offsets; see the note above.
    /* +0x04 */ int                  m_refCount;             // ctor 1
    /* +0x08 */ ArmorGLCompTemplate* m_template;
    /* +0x0C */ IObject*             m_object;               // NOT written by the ctor - setObject
                                                             // (via Object::setComponent) and init
                                                             // both store it. Uninitialised until then,
                                                             // as shipped.
    /* +0x10 */ float m_hitPoints;                           // ctor <- template hitPoints. Negative ==
                                                             // wreck health, floored at -wreckHitPoints
    /* +0x14 */ bool  m_sendingMessage;                      // 0 - see the header note; a re-entrancy guard
    /* +0x15 */ bool  m_destroyed;                           // 0 - isDestroyed(); a stored flag
    /* +0x16 */ bool  m_criticallyDamaged;                   // 0
    /* +0x18 */ float m_lastCollisionHeight;                 // -1.0
    /* +0x1C */ int   m_lastHitMaterialIndex;                // -1
    /* +0x20 */ int   m_lastGroundHitMaterialIndex;          // -1
    /* +0x24 */ WeakPtr<IPlayer> m_lastHitPlayer;            // null block
    /* +0x28 */ WeakPtr<IObject> m_lastHitWeapon;            // null block
    /* +0x2C */ WeakPtr<IObject> m_lastHitVehicle;           // null block
    /* +0x30 */ Vec3  m_lastHitPlace;                        // (0,0,0) - returned BY address
    /* +0x3C */ float m_lastHitTimeout;                      // NOT written by ctor. handleUpdate: -= dt;
                                                             // below 0 -> 0, drop m_lastHitPlayer, and
                                                             // m_lastHitCanTK = true. [CHOSEN name]
    /* +0x40 */ bool  m_lastHitCanTK;                        // 1
    /* +0x44 */ int   m_lastHitBoneIndex;                    // -1
    /* +0x48 */ Vec3  m_lastHitDirection;                    // (0,0,0) - returned BY address
    /* +0x54 */ bool  m_inDeepWater;                         // 0 - UpdateInWaterStatus; gates the deep arm
    /* +0x55 */ bool  m_wasInWater;                          // 0 - forces a water check every tick while set
    /* +0x56 */ bool  m_collisionWithWater;                  // 0 - getCollisionWithWater
    /* +0x57 */ bool  m_byte57;                              // 0 - handleUpdate clears it after the height
                                                             // test; SET elsewhere [UNRESOLVED role]
    /* +0x58 */ bool  m_byte58;                              // 0 - updateStatus sets it   [UNRESOLVED]
    /* +0x59 */ bool  m_byte59;                              // 0 } toggled as a pair by updateStatus,
    /* +0x5A */ bool  m_byte5A;                              // 0 } never both               [UNRESOLVED]
    /* +0x5C */ int   m_status;                              // ctor <- getStatus(); the cached status
    /* +0x60 */ float m_wreckTime;                           // 0.0  - world time it became a wreck
    /* +0x64 */ float m_destroyedTime;                       // -1.0 - world time it was destroyed
    /* +0x68 */ IObject* m_colObjects[16];                   // NOT zeroed by ctor - the ring's slots
    /* +0xA8 */ float    m_colValues[16];                    // NOT zeroed - parallel to the slots
    /* +0xE8 */ int   m_colHead;                             // 0
    /* +0xEC */ int   m_colTail;                             // 0
    /* +0xF0 */ float m_criticalDamageTimer;                 // 0.0 - handleUpdate accumulates dt, fires
                                                             // the critical-damage bleed each 1.0 s
    /* +0xF4 */ float m_deepWaterDamageTimer;                // 0
    /* +0xF8 */ float m_waterDamageTimer;                    // 0
    // THE effect block, from playEffect / getEffect / StopAllEffects and both
    // constructors. +0xFC..+0x218 sit at the same offsets in BF2.exe (no STL member precedes them);
    // the client constructor 0x6891E0 zeroes exactly the count, the two map pointers, +0x184 and
    // +0x188 (and the wreck four), and neither id table.
    /* +0xFC */ unsigned int m_effectIds[32];                // NOT written by ctor - the object ids of the
                                                             // attached effects playEffect made for the
                                                             // current state; StopAllEffects force-destroys
                                                             // the owner's children that carry them. The
                                                             // 32nd id raises the Debug(Error) at line 1082.
    /* +0x17C */ int  m_effectCount;                         // 0 - playEffect rewrites it per played state
    /* +0x180 */ std::map<int, DamageEffects*>* m_effects;   // 0; init -> &template->m_damageEffects
    /* +0x184 */ DamageEffects* m_lastDamageEffects;         // 0 -
                                                             // the DamageEffects* getEffect last answered.
                                                             // playEffect compares it (0x68811E / 0x821A275)
                                                             // and stores it (0x688538 / 0x821A5CD) as a
                                                             // pointer; 0 when no state changed.
                                                             // [INFERRED name]
    /* +0x188 */ bool m_byte188;                             // 0 - role true while the
                                                             // getEffect marks destruction states 0/-100; playEffect places those effects untracked in world space
                                                             // and skips them while m_byte25C is set. Repair/revival clears the flag.
                                                             // The wreck-effect mirror below is not used by playEffect; its ELF getter has no observed caller.
    /* +0x18C */ unsigned int m_wreckEffectIds[32];          // NOT written by ctor - never used
    /* +0x20C */ int  m_wreckEffectCount;                    // 0
    /* +0x210 */ std::map<int, DamageEffects*>* m_wreckEffects;   // 0; init -> &template->m_wreckDamageEffects
    /* +0x214 */ int  m_dword214;                            // 0 [UNRESOLVED]
    /* +0x218 */ bool m_byte218;                             // 0 [UNRESOLVED]
    /* +0x21C */ int  m_lastDamageType;                      // 0
    /* +0x220 */ std::list<DamageFromPlayer>  m_damageFromPlayers;
    /* +0x224 */ std::list<DamageFromPlayer>  m_vehicleDamageFromPlayers;
    /* +0x228 */ std::list<WeakPtr<IPlayer> > m_teamDamagePlayers;
    /* +0x22C */ float m_dword22C;                           // NOT written by ctor. damage() accumulates (a float
                                                             // add) the amounts of hits on the forward half that
                                                             // land left (local x <= 0); LightSource tests < 10
    /* +0x230 */ float m_dword230;                           // ... and right (local x > 0); projector mask's z
    /* +0x234 */ unsigned int m_invulnerableUntilTick;       // 0; init/reset -> tickCount + 30 x armor-invunerable-time
    /* +0x238 */ bool m_byte238;                             // 0 - handleUpdate clears it every tick;
                                                             // Selects spectacular destruction effects.
    /* +0x239 */ bool m_pendingDestroy;                      // 0 - THE ONE-frame deferral: updateStatus sets
                                                             // it, the next handleUpdate plays the effect,
                                                             // destroys or resets the object, clears it.
                                                             // (The IDB calls it spectacularDeath.)
    /* +0x23C */ float m_previousHitPoints;                  // ctor <- template hitPoints; revive writes it
    /* +0x240 */ float m_upSideDownTimer;                    // 0; reloads with 1.0
    /* +0x244 */ bool  m_upSideDownLatch;                    // 0 - "already damaged for this flip"
    /* +0x248 */ float m_accumulatedDamageNorm;              // 0.0
    /* +0x24C */ float m_healAccumulated;                    // 0 - reset zeroes it too.
                                                             // by heal(): the hit points restored since the
                                                             // damage-from-player list was last cleared; past
                                                             // 50 (or at full health) the list is cleared and
                                                             // it restarts. [INFERRED name] (was m_dword24C)
    /* +0x250 */ std::map<int, std::vector<HandledCollision*>*> m_handledCollisions;   // +0x250/+0x254
    /* +0x258 */ int   m_dword258;                           // NOT written by ctor [UNRESOLVED]
    /* +0x25C */ bool  m_byte25C;                            // NOT written by ctor in either image (heap
                                                             // leftovers - bf_malloc does not zero).
                                                             // setHitPoints clears it after its own
                                                             // playEffect; reset sets it, but reset has no
                                                             // caller in either image. playEffect's client
                                                             // arm skips a destroyed state while it is set
                                                             // [UNRESOLVED name]
    /* +0x260 */ float m_lastWaterCheckTime;                 // 0; UpdateInWaterStatus's jittered stamp
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAMELOGICCOMP_ARMORGLCOMP_H
