// Projectile lifecycle, launch, steering, collision and effects, with Bundle, IProjectileObject and ISoundNode bases.
// Networkable templates pre-store sixteen instances; construction must consume their network IDs and resources.
// [PLATFORM] Original ELF size/base offsets are 0x238/+0x13C/+0x144; current MSVC x86 layouts differ.
// Use typed bases and members to account for those differences.
// Header placement is [INFERRED].

#ifndef GAME_OBJECTS_PROJECTILES_GENERICPROJECTILE_H     // [INFERRED] guard style
#define GAME_OBJECTS_PROJECTILES_GENERICPROJECTILE_H

#include <list>

#include "../Base/Bundle.h"
#include "IProjectileObject.h"
#include "../../../Sound/ISoundNode.h"
#include "../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {

class GenericProjectileNetworkable;   // the wire wrapper - a friend below; see Networkables.h

namespace world {

class GenericProjectileTemplate;
class SimpleObjectTemplate;
class IObjectTemplateComponent;
class IObjectTemplate;
class IPlayer;
class ICameraObject;
enum TargetType : int;

// The shared launch/update face at the start of every projectile-component sub-interface.
// Both shipped ports place these three slots immediately after the IBase quartet; the component
// destructor follows (one MSVC slot, the Itanium D1/D0 pair), and the concrete
// collision/detonation/seek/follow/sound interfaces append their specialised operations after it.
// IID_IProjectileComp attests the interface name, while the component vtables attest this order.
class IProjectileComp : public IBase
{
public:
    virtual bool hasUpdate() const = 0;                    // +0x10
    virtual void updateComponent(float, unsigned int) = 0; // +0x14
    virtual void activate() = 0;                           // +0x18

protected:
    virtual ~IProjectileComp() {}                          // PE +0x1C; ELF +0x1C/+0x20
};

// Collision component interface. Keep resetProjectile before preResetProjectile and all following
// stick/query/state slots in order. Typed dispatch handles the platform destructor-slot difference.
class IProjectileCollisionComp : public IProjectileComp
{
public:
    virtual void resetProjectile(bool) = 0;                 // ELF +0x24
    virtual void preResetProjectile(bool) = 0;              // ELF +0x28
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, int,
                                 CollisionFaceInfo*, CollisionFaceInfo*) = 0;
                                                            // ELF +0x2C
    virtual int  getDefaultCollisionType(int) const = 0;    // ELF +0x30
    virtual void stick(IObject*, const Mat4&) = 0;          // ELF +0x34
    virtual void stickAbsolute(const Mat4&) = 0;            // ELF +0x38
    virtual IObject* getHostObject() const = 0;             // ELF +0x3C
    virtual const Mat4& getStickTransformation() const = 0; // ELF +0x40
    virtual bool hasCollisionEffect() const = 0;            // ELF +0x44
    virtual bool getIsBouncing() const = 0;                 // ELF +0x48
    // Returns the replenishment amount. The empty Default/Sticky implementations materialise
    // 0.0f on the x87 stack (ELF 0x82895A0/0x828DA70); ReplenishCollision supplies the value.
    virtual float getHealEffect(Vec3&) const = 0;           // ELF +0x4C
};

// _ZTV...IProjectileDetonationComp: after the shared face and ABI-specific destructor slots,
// the detonation operation and its state accessors have this exact source order. The by-value
// Vec3 parameters are fixed by the symbol
// `detonate(IObject*,int,Vec3,unsigned int,Vec3,CollisionFaceInfo*)`.
class IProjectileDetonationComp : public IProjectileComp
{
public:
    virtual void detonate(IObject*, int, Vec3, unsigned int, Vec3,
                          CollisionFaceInfo*) = 0;           // PE +0x20 / ELF +0x24
    virtual void setTargetObject(IObject*) = 0;
    virtual bool getDetonate() const = 0;
    virtual void setDetonate(bool) = 0;
    virtual bool getPlayMaterialEffectAtSurface() const = 0;
    virtual bool hasExplosion() const = 0;
    virtual int getExplosionMaterial() const = 0;
    virtual float getExplosionRadius() const = 0;
    virtual float getExplosionInnerConeRadius() const = 0;
    virtual float getFlashbangRadiusWithNightVision() const = 0;
    virtual float getExplosionSoldierLineOfSight() const = 0;
    virtual float getExplosionSoldierLineOfSightTimer() const = 0;
    virtual float getExplosionSoldierLineOfSightHealRate() const = 0;
    virtual float getExplosionLineOfSightMinimumDamage() const = 0;
    virtual float getExplosionConeAngle() const = 0;
    virtual float getExplosionYMod() const = 0;
    virtual float getExplosionForce() const = 0;
    virtual float getExplosionDamage() const = 0;
    virtual float getFlashbangBillboardMinAlpha() const = 0;
    virtual float getFlashbangBillboardMaxAlpha() const = 0;
    virtual float getFlashbangBillboardMaxRampTime() const = 0;
    virtual float getFlashbangBillboardMinHoldTime() const = 0;
    virtual float getFlashbangBillboardMaxHoldTime() const = 0;
    virtual float getFlashbangBillboardMinHealTime() const = 0;
    virtual float getFlashbangBillboardMaxHealTime() const = 0;
    virtual float getFlashbangAdditiveGlowMinAlpha() const = 0;
    virtual float getFlashbangAdditiveGlowMaxAlpha() const = 0;
    virtual float getFlashbangAdditiveGlowMaxRampTime() const = 0;
    virtual float getFlashbangAdditiveGlowMinHoldTime() const = 0;
    virtual float getFlashbangAdditiveGlowMaxHoldTime() const = 0;
    virtual float getFlashbangAdditiveGlowMinHealTime() const = 0;
    virtual float getFlashbangAdditiveGlowMaxHealTime() const = 0;
    virtual float getFlashbangMotionBlurMinAlpha() const = 0;
    virtual float getFlashbangMotionBlurMaxAlpha() const = 0;
    virtual float getFlashbangMotionBlurMaxRampTime() const = 0;
    virtual float getFlashbangMotionBlurMinHoldTime() const = 0;
    virtual float getFlashbangMotionBlurMaxHoldTime() const = 0;
    virtual float getFlashbangMotionBlurMinHealTime() const = 0;
    virtual float getFlashbangMotionBlurMaxHealTime() const = 0;
    virtual float getDetectionRadius() const = 0;
    virtual bool getShowMineIndicationIcon() const = 0;
    virtual int getRadiusTriggerType() const = 0;
    virtual IObjectTemplate* getEndEffect() const = 0;
    virtual float getTimeUntilCanDetonate() const = 0;
};

class IProjectileSoundComp : public IProjectileComp
{
public:
    virtual void initSoundComp(const SimpleObjectTemplate*) = 0; // PE +0x20 / ELF +0x24
    virtual void reset() = 0;                                // PE +0x24 / ELF +0x28
};

// The secondary table at ELF 0x88BF6DC / PE 0x7DCDEC contains the shared projectile-component
// face, the ABI-specific destructor slot(s), and this single appended operation. Keeping the
// complete interface typed avoids freezing the one-slot PE / two-slot Itanium destructor delta.
class IProjectileRicochetComp : public IProjectileComp
{
public:
    virtual bool ricochet(const Vec3& impactOffset,
                          const Vec3& incomingVelocity,
                          const Vec3& surfaceNormal,
                          float retainedFraction,
                          float baseProjectileDamage,
                          float sampledLifetime,
                          float& lastRicochetTime,
                          Vec3& ricochetWorldPosition,
                          float& currentDamage) = 0;          // PE +0x20 / ELF +0x24
};

// _ZTV...IProjectilePenetrateComp has exactly one operation after the shared projectile-component
// face and the ABI-specific destructor slot(s). The complete argument list is fixed by
// DefaultPenetrateComp::penetrate's ELF mangling; PE's secondary table places it at +0x20 while
// Itanium's D1/D0 pair moves the same source slot to +0x24.
class IProjectilePenetrateComp : public IProjectileComp
{
public:
    virtual bool penetrate(IObject*, const Vec3&, int, int,
                           float, float, float,
                           float&, Vec3&, float&) = 0;        // PE +0x20 / ELF +0x24
};

// _ZTV...IProjectileFollowComp - complete. Exactly two methods are appended to the shared face, and
// both are pinned by argument type at the one site that dispatches them,
// GenericProjectile::handleUpdate (ELF 0x8281860): +0x28 takes an integer tick, +0x24 takes an
// object pointer. Two independent classes implement it - DefaultFollowComp (0x8295AB0) and
// PreDestinedFollowComp (0x8297C90) - and their _ZThn16_ thunk sets agree exactly on these two.
// THE shipped call order IS setDelayTick first, then setTargetObject. setTargetObject is 159
// bytes and does real work; the delay must already be in place when it runs. Do not reorder them.
class IProjectileFollowComp : public IProjectileComp
{
public:
    virtual void setTargetObject(IObject*) = 0;              // PE +0x20 / ELF +0x24
    virtual void setDelayTick(unsigned int) = 0;             // PE +0x24 / ELF +0x28
};

// _ZTV...IProjectileSeekComp - complete. FixedTargetComp's secondary vtable at ELF 0x88BEB50 and
// fixes all six appended slots. The ELF/PE offset difference is solely the extra
// Itanium destructor slot; the source order is identical in both images.
class IProjectileSeekComp : public IProjectileComp
{
public:
    virtual void setTargetObject(IObject*, bool) = 0;        // PE +0x20 / ELF +0x24
    virtual IObject* getTargetObject() const = 0;            // PE +0x24 / ELF +0x28
    virtual int getTrackingDelayTick() const = 0;            // PE +0x28 / ELF +0x2C
    virtual float getMaxTargetDistance() const = 0;          // PE +0x2C / ELF +0x30
    virtual float getMaxTargetAngle() const = 0;             // PE +0x30 / ELF +0x34
    virtual TargetType getTargetType() const = 0;            // PE +0x34 / ELF +0x38
};

class GenericProjectile : public Bundle,
                          public IProjectileObject,
                          public ISoundNode
{
    // GenericProjectileNetworkable's retail ELF bodies document projectile
    // +0x150/+0x1C4..+0x1D4/+0x1C8/+0x1E4. [PLATFORM] The rebuilt wrapper
    // accesses the corresponding typed members because the current MSVC x86 layout differs as
    // documented above. The friend line reproduces that access without widening the public
    // surface - the same argument as Soldier.h's.
    friend class ::dice::hfe::GenericProjectileNetworkable;

    // GenericProjectileTemplate::preCache writes the throwaway projectile's +0x1FD byte directly
    // between turnIntoTracer(false) and forceDestroy(). Both retail ports emit a plain field store,
    // not a setter call; keep that source-level access narrow instead of widening the field.
    friend class GenericProjectileTemplate;

    // FixedTargetComp::updateComponent reads the launch tick directly in both shipped images
    // (ELF projectile +0x1BC, PE +0x1DC). The friend preserves that direct typed access without
    // widening GenericProjectile's public surface. [INFERRED source access arrangement]
    friend class FixedTargetComp;
    friend class SeekClosestTargetComp;
    friend class ReplenishCollisionComp;

public:
    explicit GenericProjectile(const GenericProjectileTemplate* objectTemplate);
                                                     // C2 0x827F890, 845 B / C1 0x827FBE0 - read in full
    virtual ~GenericProjectile();

    // the quartet, its OWN refcount at +0x148
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// four answers

    // overrides of inherited slots
    virtual void init();                             // +0x020
    virtual void disableCachedObject();              // +0x024
    virtual void enableCachedObject();               // +0x028
    virtual void destroy(bool force);                // +0x034
    // updateTracer() then SimpleObject::handleVisualUpdate with both
    // floats forwarded. Both calls are NON-virtual in the shipped body - a direct call and a
    // qualified base call (via Bundle, which is the SimpleObject in this hierarchy).
    virtual void handleVisualUpdate(float, float);   // +0x0A8
    // The authoritative componentless and component-bearing paths are recovered, including
    // seek/follow, associated-tracer convergence and authored acceleration.
    virtual void handleUpdate(float, unsigned int);  // +0x0B0
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
                                                     // +0x0B4 ; common/typed collision arm
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
                                                     // +0x104
    virtual void initNetworkableComp();              // +0x178

    // THE retail ELF additions, +0x180..+0x2EC, in the primary vtable's own order (walked from
    // _ZTVN4dice3hfe5world17GenericProjectileE @ 0x88BC340). [P] documents a corresponding retail
    // IProjectileObject slot (thunks _ZThn316_); that interface remains deliberately undeclared
    // past its destructor in this tree because a known-prefix-only declaration produces a hybrid
    // MSVC vtable. getTransform/getVelocity are the two ISoundNode slots (_ZThn324_).
    // Return types on unread accessors are [CHOSEN] from body size and the template field behind
    // them; parameter lists come from the mangled names.
    virtual void initComponents();                   // +0x180
    // The stock authoritative no-component detonation/teardown arms are recovered. The material
    // impact callback and optional AI collision-handler callbacks remain branch-local named stops.
    virtual void detonate(bool);                     // +0x184 [P]
    virtual void resetProjectile(bool);              // +0x188 [P]
    // The common stock-server launch arm is recovered. Non-null client trail-template creation
    // retains a branch-local named stop until its specialised effect interface lands.
    virtual void activate();                         // +0x18C [P]
    virtual void initOnLaunch();                     // +0x190 [P]
    virtual void postInit();                         // +0x194 [P]
    virtual void useProjectileCamera();              // +0x198 [P]
    virtual bool isInProjectileCamera();             // +0x19C [P] PE 0x5B2E60
    virtual void disableProjectileCamera();          // +0x1A0 [P]
    virtual bool getIsActive() const;                // +0x1A4 [P]
    virtual void setPlayer(IPlayer*);                // +0x1A8 [P]
    // the weak read on +0x174. It is also
    // IProjectileObject subtable slot +0x54 - the world::projectileOwner seam Camera::setViewMode
    // and ResponsePhysicsManager dispatch; the TV-missile drain builds reached it the day the
    // projectileTemplate binding landed.
    virtual IPlayer* getPlayer() const;              // +0x1AC [P]
    virtual void setAssistPlayers(std::list<WeakPtr<IPlayer> >);
                                                     // +0x1B0 [P]
    virtual std::list<WeakPtr<IPlayer> >* getAssistPlayers();
                                                     // +0x1B4 [P]
    virtual void setTeam(int);                       // +0x1B8 [P]
    virtual int  getTeam();                          // +0x1BC [P]
    virtual void setLauncherWeapon(IObject*);        // +0x1C0 [P]
    virtual IObject* getLauncherWeapon() const;      // +0x1C4 [P]
    virtual void setLauncherVehicle(IObject*);       // +0x1C8 [P]
    virtual IObject* getLauncherVehicle() const;     // +0x1CC [P]
    virtual void setStartPos(Vec3);                  // +0x1D0 [P]
    virtual Vec3 getStartPos();                      // +0x1D4 [P]
    virtual float getMaxTargetDistance() const;      // +0x1D8 [P]
    virtual float getMaxTargetAngle() const;         // +0x1DC [P]
    virtual void setTargetObject(IObject*);          // +0x1E0 [P]
    virtual IObject* getTargetObject();              // +0x1E4 [P]
    virtual bool canTK();                            // +0x1E8 [P]
    virtual bool hasCollisionEffect() const;         // +0x1EC [P]
    virtual float getHealEffect(Vec3&) const;        // +0x1F0 [P]
    virtual bool hasExplosion() const;               // +0x1F4 [P]
    virtual int  getExplosionMaterial() const;       // +0x1F8 [P]
    virtual float getExplosionRadius() const;        // +0x1FC [P]
    virtual float getExplosionInnerConeRadius() const;          // +0x200 [P]
    virtual float getFlashbangRadiusWithNightVision() const;    // +0x204 [P]
    virtual float getExplosionSoldierLineOfSight() const;       // +0x208 [P]
    virtual float getExplosionSoldierLineOfSightTimer() const;  // +0x20C [P]
    virtual float getExplosionSoldierLineOfSightHealRate() const;   // +0x210 [P]
    virtual float getExplosionLineOfSightMinimumDamage() const; // +0x214 [P]
    virtual float getExplosionConeAngle() const;     // +0x218 [P]
    virtual float getExplosionYMod() const;          // +0x21C [P]
    virtual float getExplosionForce() const;         // +0x220 [P]
    virtual float getExplosionDamage() const;        // +0x224 [P]
    virtual int  getExplosionType() const;           // +0x228 [P]
    virtual bool getDamageMandownSoldiers() const;   // +0x22C [P]
    virtual float getDamage(Vec3);                   // +0x230 [P]
    // float, not int. All three
    // shipped bodies leave the remaining lifetime on the x87 stack with no integer conversion
    // (ELF 0x82843F0 ends `fsubr`/`fst`/ret; BF2.exe 0x611F30 and bf2_w32ded.exe 0x5B10E0, 45 B
    // each, end `fsubr`/ret), and the one recovered caller - HudAmmo::updateBombFuel, BF2.exe
    // 7A7A3A - `fdiv`s st0 straight after the call. IProjectileObject subtable slot ELF +0xB8 /
    // PE +0xB4: the world::projectileTimeToLive seam (GenericProjectile.cpp).
    virtual float getTimeToLive();                   // +0x234 [P]
    virtual const Mat4& getTransform();              // +0x238 [ISoundNode] 0x82843B0, 24 B
    virtual const Math::Vec3* getVelocity();         // +0x23C [ISoundNode] 0x82843D0, 30 B
    virtual int  getTracerInterval() const;          // +0x240 [P]
    virtual bool hasTracer() const;                  // +0x244 [P]
    virtual void turnIntoTracer(bool);               // +0x248 [P]
    virtual void setAssociatedTracer(IProjectileObject*);       // +0x24C [P]
    virtual int  getDefaultCollisionType(int) const; // +0x250 [P]
    // bool, not void: the ELF returns 1 on both bounce arms and the
    // component's own answer otherwise. Its one recovered caller discards it.
    virtual bool ricochet(const Vec3&, const Vec3&, const Vec3&, float);
                                                     // +0x254 [P]
    virtual float getMinDetonationSpeed() const;     // +0x258 [P]
    // bool, not void: the ELF returns the component's answer.
    virtual bool penetrate(IObject*, const Vec3&, int, int, float);
                                                     // +0x25C [P]
    virtual void setUnderWater(float);               // +0x260 [P]
    virtual float getUnderWater();                   // +0x264 [P]
    virtual bool getPlayMaterialEffectAtSurface() const;        // +0x268 [P]
    virtual float getWaterVelocityDecreaseMod() const;          // +0x26C [P]
    virtual void setLastCountedHitAtTick(unsigned int);         // +0x270 [P]
    virtual unsigned int getLastCountedHitAtTick();  // +0x274 [P]
    virtual void setCountHit(bool);                  // +0x278 [P]
    virtual bool getCountHit();                      // +0x27C [P]
    virtual void setLocalPredict(bool);              // +0x280 [P]
    virtual bool getLocalPredict() const;            // +0x284 [P]
    virtual void setLocalPredictedProjectile(IObject*);         // +0x288 [P]
    virtual IObject* getLocalPredictedProjectile() const;       // +0x28C [P]
    virtual unsigned int getPlayedImpactEffectAtTick() const;   // +0x290 [P]
    virtual void setPlayedImpactEffectAtTick(unsigned int);     // +0x294 [P]
    // the 21-parameter flashbang farm, three channels x seven - all template forwards
    virtual float getFlashbangBillboardMinAlpha() const;        // +0x298 [P]
    virtual float getFlashbangBillboardMaxAlpha() const;        // +0x29C [P]
    virtual float getFlashbangBillboardMaxRampTime() const;     // +0x2A0 [P]
    virtual float getFlashbangBillboardMinHoldTime() const;     // +0x2A4 [P]
    virtual float getFlashbangBillboardMaxHoldTime() const;     // +0x2A8 [P]
    virtual float getFlashbangBillboardMinHealTime() const;     // +0x2AC [P]
    virtual float getFlashbangBillboardMaxHealTime() const;     // +0x2B0 [P]
    virtual float getFlashbangAdditiveGlowMinAlpha() const;     // +0x2B4 [P]
    virtual float getFlashbangAdditiveGlowMaxAlpha() const;     // +0x2B8 [P]
    virtual float getFlashbangAdditiveGlowMaxRampTime() const;  // +0x2BC [P]
    virtual float getFlashbangAdditiveGlowMinHoldTime() const;  // +0x2C0 [P]
    virtual float getFlashbangAdditiveGlowMaxHoldTime() const;  // +0x2C4 [P]
    virtual float getFlashbangAdditiveGlowMinHealTime() const;  // +0x2C8 [P]
    virtual float getFlashbangAdditiveGlowMaxHealTime() const;  // +0x2CC [P]
    virtual float getFlashbangMotionBlurMinAlpha() const;       // +0x2D0 [P]
    virtual float getFlashbangMotionBlurMaxAlpha() const;       // +0x2D4 [P]
    virtual float getFlashbangMotionBlurMaxRampTime() const;    // +0x2D8 [P]
    virtual float getFlashbangMotionBlurMinHoldTime() const;    // +0x2DC [P]
    virtual float getFlashbangMotionBlurMaxHoldTime() const;    // +0x2E0 [P]
    virtual float getFlashbangMotionBlurMinHealTime() const;    // +0x2E4 [P]
    virtual float getFlashbangMotionBlurMaxHealTime() const;    // +0x2E8 [P]
    virtual bool isCreatedByLocalPlayer() const;     // +0x2EC [P]

    // NON-virtual members
    void determineExplosionType();                   // the strstr trio
    bool getDetonate() const;                        // detonation comp or 0
    void setDetonate(bool);                          // detonation comp or no-op
    void updateTracer();
    void stopEffects();
    void destroyTrailEffect();
    void playTrailEffect();                          // Restarts an existing trail effect.
    void startStartEffect();
    void hideProjectile();
    void sendMissileInitEvent();
    void enableProjectileCamera();
    void reduceVelocityUnderwater();
    void stick(IObject*, const Mat4&);
    void stickAbsolute(const Mat4&);
    IObject* getStickOnParent() const;
    const Mat4& getStickTransformation() const;
    bool getDetonateAfterProjectileStopped() const;
    float getDetonateAfterProjectileStoppedMinSpeed() const;
    // float, not int: the ELF returns it on the x87 stack.
    float getProjectileDamage() const;

    // Inline field view with DefaultCollisionComp, which reads the launch
    // stamp directly off the projectile (ELF `[projectile + 0x190]`) for its self-hit window.
    float getWorldLaunchTime() const { return m_worldLaunchTime; }
    // Despite living in this helper group in retail source, this exactly overrides Object's live
    // visual-bounds slot. Active tracers use m_maxTracerScaler; all other arms delegate to Object.
    virtual float getVisualBoundingRadius() const;
    bool getIsTracer() const;

private:
    // THE retail ELF layout, +0x148..+0x237, read from the 845-byte constructor. Bundle occupies
    // 0..0x13B; the two interface sub-objects sit at +0x13C and +0x144. These offsets are
    // documentary only; the current MSVC x86 positions are recorded at the top of this file. A
    // field with no recovered reader keeps its ELF offset as its name. five gaps ARE THE ELF
    // ctor'S OWN - +0x184..+0x194, +0x1FE/F, +0x208, +0x20D..F and +0x220 stay operator-new
    // residue, as shipped.
    /* +0x148 */ int   m_refCount;                   // ctor 1 - its OWN, like Soldier's
    /* +0x14C */ const GenericProjectileTemplate* m_template;
    // the seven components, re-queried by initComponents, zeroed by disableCachedObject.
    // Each recovered specialised face is typed so the compiler accounts for the PE/ELF destructor
    // slot difference; ricochet remains on the shared prefix until its own interface is walked.
    /* +0x150 */ IProjectileCollisionComp* m_collisionComp;
    /* +0x154 */ IProjectileDetonationComp* m_detonationComp;
    /* +0x158 */ IProjectileSeekComp* m_seekComp;
    /* +0x15C */ IProjectileFollowComp* m_followComp;
    /* +0x160 */ IProjectileRicochetComp* m_ricochetComp;
    /* +0x164 */ IProjectilePenetrateComp* m_penetrateComp;
    /* +0x168 */ IProjectileSoundComp* m_soundComp;
    /* +0x16C */ std::list<WeakPtr<IPlayer> > m_assistPlayers;   // under this tree's libstdc++
    // Weak references to the launcher weapon, player, and vehicle.
    /* +0x170 */ WeakPtr<IObject> m_launcherWeapon;  // dtor drops; reset uses target
    /* +0x174 */ WeakPtr<IPlayer> m_player;          // dtor drops; getPlayer's read
    /* +0x178 */ WeakPtr<IObject> m_launcherVehicle; // dtor drops
    /* +0x17C */ int   m_team;                       // ctor 0 - set/getTeam
    /* +0x180 */ bool  m_isActive;                   // ctor 0 - activate raises, resetProjectile clears
    /* +0x181 */ bool  m_byte181;                    // ctor 0 - init: "has a networkable"; reset: 1
    /* +0x184 */ int   m_dword184;                   // NOT ctor-written [UNRESOLVED]
    /* +0x188 */ int   m_dword188;                   // NOT ctor-written [UNRESOLVED]
    /* +0x18C */ float m_exactLaunchTime;             // activate = System::getExactTime()
    /* +0x190 */ float m_worldLaunchTime;             // activate = float(WorldPref::getWorldTime())
    /* +0x194 */ int   m_dword194;                   // NOT ctor-written [UNRESOLVED]
    /* +0x198 */ int   m_dword198;                   // ctor -1 [UNRESOLVED - hit-tick family?]
    /* +0x19C */ Vec3  m_vec19C;                     // ctor 0 [UNRESOLVED]
    /* +0x1A8 */ Vec3  m_startPos;                   // set/getStartPos
    /* +0x1B4 */ int   m_dword1B4;                   // ctor 0 [UNRESOLVED]
    /* +0x1B8 */ int   m_dword1B8;                   // ctor 0 [UNRESOLVED]
    /* +0x1BC */ int   m_dword1BC;                   // ctor 0 [UNRESOLVED]
    /* +0x1C0 */ float m_damage;                     // ctor = template +0x1DC (the default damage)
    /* +0x1C4 */ int   m_dword1C4;                   // ctor 0 - replicated (the networkable's +0x28)
    /* +0x1C8 */ int   m_tick1C8;                    // ctor -1000; setNetUpdate stamps getTickCount()
    /* +0x1CC */ Vec3  m_vector1CC;                  // ctor 0 - replicated (the networkable's +0x2C)
    /* +0x1D8 */ Vec3  m_vec1D8;                     // ctor 0 [UNRESOLVED]
    /* +0x1E4 */ void* m_targetObjectWeak;           // weak block - setNetUpdate stores the target's
                                                     // weak ptr; resetProjectile drops it
    /* +0x1E8 */ ICameraObject* m_projectileCamera;  // init's CID_CameraTemplate child hunt
    /* +0x1EC */ int   m_dword1EC;                   // ctor -1 [UNRESOLVED]
    /* +0x1F0 */ void* m_associatedTracerWeak;       // WeakPtr<IProjectileObject> control block
    /* +0x1F4 */ void* m_localPredictedWeak;         // weak block - reset destroys its IObject target,
                                                     // setLocalPredictedProjectile also destroys the previous stand-in before adoption.
                                                     // The interpolation proxy below is weakly held by the stand-in; reset destroys its target and
                                                     // destruction drops the control block.
    /* +0x1F8 */ void* m_interpolationProxyWeak;     // weak block - see above
    /* +0x1FC */ bool  m_byte1FC;                    // ctor 0 - reset clears
    /* +0x1FD */ bool  m_byte1FD;                    // ctor 0 - retail preCache's throwaway sets 1
    /* +0x200 */ int   m_dword200;                   // last-counted-hit tick; ctor/reset clear it
    /* +0x204 */ bool  m_countHit;                   // ctor/activate 0; set/getCountHit
    /* +0x205 */ bool  m_localPredict;               // ctor 0; suppresses networkable attachment;
                                                     // enableCachedObject clears it
    /* +0x206 */ bool  m_byte206;                    // ctor 1; reset sets 1; cleared ON THE
                                                     // setLocalPredictedProjectile clears the adopted object's byte above, stopping its predicted self-drive.
                                                     // Its original name remains [UNRESOLVED]. The impact tick below throttles effects to one per ten ticks;
                                                     // activate clears it, while the constructor leaves it unwritten.
    /* +0x208 */ unsigned int m_playedImpactEffectAtTick;
    /* +0x20C */ bool  m_byte20C;                    // ctor 0 [UNRESOLVED]
    /* +0x210 */ Vec3  m_vec210;                     // ctor 0 [UNRESOLVED]
    /* +0x21C */ IObject* m_trailEffectObject;        // ctor 0; effect-template createObject result
    /* +0x220 */ int   m_explosionType;              // NOT ctor-written - init's
                                                     // determineExplosionType seeds it every time
                                                     // (0 other / 1 flashbang / 2 molotov / 3 teargas)
    /* +0x224 */ IBase* m_geometrySwap224;           // ctor 0 - destroy releases when != m_geometry
    /* +0x228 */ IBase* m_geometrySwap228;           // ctor 0 - destroy releases when != m_geometry
    /* +0x22C */ Vec3  m_vec22C;                     // ctor 0 [UNRESOLVED]

    // static init - dice::hfe::world::GenericProjectile::m_random, attested name.
    // [UNRESOLVED shape] - no recovered reader yet; deliberately NOT declared until one lands.
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_GENERICPROJECTILE_H
