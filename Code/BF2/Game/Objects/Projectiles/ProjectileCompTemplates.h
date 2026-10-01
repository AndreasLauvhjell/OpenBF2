// Projectile component templates implement all twelve interface slots, including makeScript at +0x2C.
// Do not move makeScript into IObjectTemplateComponent: weapon templates put preCache at that slot instead.
// Preserve each class's destructor placement and the collision siblings' distinct method names/constness.
// Empty lifecycle callbacks are intentional. Shared-header placement is [INFERRED].

#ifndef GAME_OBJECTS_PROJECTILECOMPTEMPLATES_H     // [INFERRED] guard style
#define GAME_OBJECTS_PROJECTILECOMPTEMPLATES_H

#include <iosfwd>
#include <cmath>
#include <string>

#include "../CompTemplateInterfaces.h"
#include "../IObjectTemplateComponent.h"
#include "../../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace io { class IStream; }
namespace world {

class IObject;
class Object;
class ObjectTemplate;
class IObjectTemplate;
class IObjectComponent;
class SimpleObjectTemplate;
enum TargetType : int;
enum ReplenishingType : int;
// ELF RadiusTriggerType stream operators, cross-checked against the dedicated PE.
enum RadiusTriggerType : int { MTYNone = 0, MTYPersonnel = 1, MTYVehicle = 2, MTYPco = 3 };
std::ostream& operator<<(std::ostream&, RadiusTriggerType);
std::istream& operator>>(std::istream&, RadiusTriggerType&);

// Shared by the seek-component console bindings and makeScript writers. Both directions are
// recovered at ELF 0x857EFC0 / 0x857F030; unknown input deliberately leaves the destination alone.
std::ostream& operator<<(std::ostream&, TargetType);
std::istream& operator>>(std::istream&, TargetType&);
std::ostream& operator<<(std::ostream&, ReplenishingType);
std::istream& operator>>(std::istream&, ReplenishingType&);

// Shared projectile-template component and serialization declarations.
#define BF2_PROJECTILE_COMP_TEMPLATE(cls)                                   \
    virtual void  addRef();                                                 \
    virtual int   getRef();                                                 \
    virtual int   release();                                                \
    virtual void* queryInterface(unsigned int) const;                       \
    virtual unsigned int getClassID() const;                                \
    virtual unsigned int getInterfaceID();                                  \
    virtual IObjectComponent* createComponent();                            \
    virtual void  objectCreated(IObject*);                                  \
    virtual void  objectDestroyed(IObject*);                                \
    virtual void  setTemplate(IObjectTemplate*);                            \
    virtual void* queryImplementation(unsigned int);                        \
    virtual bool  makeScript(io::IStream*)

// The twelve direct implementors and ReplenishDetonation subclass. Byte sizes are from
// each factory's operator new call; slot counts are from the
// vtables.

// [UNRESOLVED] return type on all four get*CollisionType — each body is 7 bytes
// (`mov eax, imm; retn`), so it returns a small constant, but nothing pins the type. Almost
// certainly an enum shared with the collision system.
class DefaultCollisionCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable     // , 13 slots
{ public: BF2_PROJECTILE_COMP_TEMPLATE(DefaultCollisionCompTemplate);
          DefaultCollisionCompTemplate();
          virtual int getDefaultCollisionType() const;                      // +0x30

          // THE TWO bytes, from the constructor's five stores (vptr, vptr,
          // refCount 1, and these two false). They fill the class out to its proven 16 bytes, and
          // DefaultCollisionComp reads both: hasCollisionEffect() is +0x0C verbatim, getIsBouncing()
          // is +0x0D, and getDefaultCollisionType(int) short-circuits on +0x0D.
          bool getHasCollisionEffect() const { return m_hasCollisionEffect; }
          bool getIsBouncing() const         { return m_isBouncing; }

          // [CHOSEN, not recovered] - these TWO setters DO NOT exist IN THE shipped image. The
          // class has exactly 24 ELF symbols (ctor, createComponent, makeScript and the 13 vtable
          // slots plus their _ZThn4_ thunks) and no accessor pair among them, so retail's console
          // executors for `ObjectTemplate.collision.bouncing` and `.hasCollisionEffect` write the
          // two bytes directly, the way objectTemplate.physicsType writes SimpleObjectTemplate's.
          // They exist here only to keep the binding a plain BF2_CONSOLE_PROPERTY instead of
          // reaching into protected storage from a generated class; the stored value is identical.
          void setHasCollisionEffect(bool value) { m_hasCollisionEffect = value; }
          void setIsBouncing(bool value)         { m_isBouncing = value; }

  protected:
          /* +0x08 */ int  m_refCount;
          /* +0x0C */ bool m_hasCollisionEffect;
          /* +0x0D */ bool m_isBouncing;
};

class ReplenishCollisionCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable   // , 13 slots
{ public: BF2_PROJECTILE_COMP_TEMPLATE(ReplenishCollisionCompTemplate);
          ReplenishCollisionCompTemplate();
          virtual int getReplenishCollisionType() const;                    // +0x30

          bool getHasCollisionEffect() const { return m_hasCollisionEffect; }
          bool getIsBouncing() const { return m_isBouncing; }
          float getReplenishingStrength() const { return m_replenishingStrength; }
          ReplenishingType getReplenishingType() const { return m_replenishingType; }
          void setHasCollisionEffect(bool value) { m_hasCollisionEffect = value; }
          void setIsBouncing(bool value) { m_isBouncing = value; }
          void setReplenishingStrength(float value) { m_replenishingStrength = value; }
          void setReplenishingType(ReplenishingType value) { m_replenishingType = value; }

  protected:
          /* +0x08 */ int m_refCount;
          /* +0x0C */ bool m_hasCollisionEffect;
          /* +0x0D */ bool m_isBouncing;
          /* +0x10 */ float m_replenishingStrength;
          /* +0x14 */ ReplenishingType m_replenishingType;
};

class ResurrectCollisionCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable   // , 13 slots
{ public: BF2_PROJECTILE_COMP_TEMPLATE(ResurrectCollisionCompTemplate);
          ResurrectCollisionCompTemplate();
          virtual int getResurrectCollisionType() const;                    // +0x30

          bool getHasCollisionEffect() const { return m_hasCollisionEffect; }
          bool getIsBouncing() const { return m_isBouncing; }
          float getResurrectDamage() const { return m_resurrectDamage; }
          float getRestoreHP() const { return m_restoreHP; }
          void setHasCollisionEffect(bool value) { m_hasCollisionEffect = value; }
          void setIsBouncing(bool value) { m_isBouncing = value; }
          void setResurrectDamage(float value) { m_resurrectDamage = value; }
          void setRestoreHP(float value) { m_restoreHP = value; }

  protected:
          /* +0x08 */ int m_refCount;
          /* +0x0C */ bool m_hasCollisionEffect;
          /* +0x0D */ bool m_isBouncing;
          /* +0x10 */ float m_resurrectDamage;
          /* +0x14 */ float m_restoreHP;
};

// Its 13th slot is named getDefaultCollisionType, NOT getStickyCollisionType — see the header
// block. That is the real symbol; do not regularise it.
class StickyCollisionCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable      // , 13 slots
{ public: BF2_PROJECTILE_COMP_TEMPLATE(StickyCollisionCompTemplate);
          StickyCollisionCompTemplate();
          virtual int getDefaultCollisionType() const;                      // +0x30 returns 0

          // setMaxStickAngle takes degrees AND stores A cosine (clamped to [-180,180] first), so
          // the runtime never sees an angle - StickyCollisionComp::handleCollision compares the
          // stored value straight against the collision normal's Y.
          void setMaxStickAngle(float degrees);

          bool  getHasCollisionEffect() const { return m_hasCollisionEffect; }
          bool  getIsBouncing() const         { return m_isBouncing; }
          // Runtime compares the stored cosine directly. The generated console GET performs the
          // inverse conversion (clamp, acos, radians-to-degrees), so keep the two views explicit.
          float getMaxStickAngle() const      { return m_maxStickAngle; }
          float getMaxStickAngleDegrees() const
          {
              const float clamped = m_maxStickAngle < -1.0f ? -1.0f
                                  : m_maxStickAngle > 1.0f ? 1.0f
                                  : m_maxStickAngle;
              return std::acos(clamped) * 57.2957763671875f;
          }
          bool  getStickToTerrain() const     { return m_stickToTerrain; }
          bool  getStickToStaticObjects() const     { return m_stickToStaticObjects; }
          bool  getStickToVehicles() const    { return m_stickToVehicles; }
          void  setHasCollisionEffect(bool value) { m_hasCollisionEffect = value; }
          void  setIsBouncing(bool value) { m_isBouncing = value; }
          void  setStickToTerrain(bool value) { m_stickToTerrain = value; }
          void  setStickToStaticObjects(bool value) { m_stickToStaticObjects = value; }
          void  setStickToVehicles(bool value) { m_stickToVehicles = value; }

  protected:
          // from the constructor's eight stores; they fill the class out to its
          // proven 24 bytes. The angle starts at -1.0f - a cosine that accepts every surface -
          // and all three permission bytes start true.
          /* +0x08 */ int   m_refCount;
          /* +0x0C */ bool  m_hasCollisionEffect;
          /* +0x0D */ bool  m_isBouncing;
          /* +0x10 */ float m_maxStickAngle;      // stored as cos(radians)
          /* +0x14 */ bool  m_stickToTerrain;     // [CHOSEN] - permits the no-target case
          /* +0x15 */ bool  m_stickToStaticObjects;     // [CHOSEN] - permits the target case
          /* +0x16 */ bool  m_stickToVehicles;    // [CHOSEN] - checked against the host's root
};

// the detonation pair -
// DefaultDetonationCompTemplate had NO definition anywhere in the tree (only a forward decl in
// ModuleRegistry.cpp) despite being an RTTI-proven direct implementor. 228 B - by far the largest
// class in this family; the mine/tripwire detonation logic. Vtable order: makeScript first among
// its new slots, then the four accessors (getters const, unlike most of this layer).
class DefaultDetonationCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable  // , 16 slots
{ public: BF2_PROJECTILE_COMP_TEMPLATE(DefaultDetonationCompTemplate);
          DefaultDetonationCompTemplate();

          // The two console setters that do work beyond a store.
          void setExplosionConeAngle(float degrees);
          void setTriggerAngle(float degrees);

          // THE four virtual slots this class appends - and they are the
          // reason its table is sixteen entries where every sibling's is twelve or thirteen. They
          // were previously declared as ordinary inline getters, which left the rebuilt class with a
          // 12-slot vtable against retail's 16 - a silent ABI divergence in a class nothing had
          // dispatched through yet.
          // Read from the primary table at off_88BDF48; the order below IS the table order.
          virtual void  setDetectionRadius(float);                          // +0x30
          virtual float getDetectionRadius() const;                         // +0x34
          virtual void  setShowMineIndicationIcon(bool);                    // +0x38
          virtual bool  getShowMineIndicationIcon() const;                  // +0x3C

          // [CHOSEN, not recovered] field accessors expose original direct-store console behavior.
          // The real radius/icon and angle setters retain their side effects and clamps.
          void  setExplosionMaterial(int v) { m_explosionMaterial = v; }
          void  setExplosionRadius(float v) { m_explosionRadius = v; }
          void  setExplosionInnerConeRadius(float v) { m_explosionInnerConeRadius = v; }
          void  setExplosionSoldierLineOfSight(float v) { m_explosionSoldierLineOfSight = v; }
          void  setExplosionSoldierLineOfSightTimer(float v) { m_explosionSoldierLineOfSightTimer = v; }
          void  setExplosionSoldierLineOfSightHealRate(float v) { m_explosionSoldierLineOfSightHealRate = v; }
          void  setExplosionLineOfSightMinimumDamage(float v) { m_explosionLineOfSightMinimumDamage = v; }
          void  setExplosionYMod(float v) { m_explosionYMod = v; }
          void  setExplosionForce(float v) { m_explosionForce = v; }
          void  setExplosionDamage(float v) { m_explosionDamage = v; }
          void  setMaxDepthForExplosion(float v) { m_maxDepthForExplosion = v; }
          void  setDetonateDistanceToTarget(float v) { m_detonateDistanceToTarget = v; }
          void  setMinDetonationSpeed(float v) { m_minDetonationSpeed = v; }
          float getMinDetonationSpeed() const { return m_minDetonationSpeed; }
          void  setDetonateOnWaterCollision(bool v) { m_detonateOnWaterCollision = v; }
          bool  getDetonateOnWaterCollision() const { return m_detonateOnWaterCollision; }
          void  setUseMMOnEndEffect(bool v) { m_useMMOnEndEffect = v; }
          void  setTriggerRadius(float v) { m_triggerRadius = v; }
          void  setTriggerType(int v) { m_radiusTriggerType = v; }
          int   getTriggerType() const { return m_radiusTriggerType; }
          void  setTriggerVictimMinSpeed(float v) { m_triggerVictimMinSpeed = v; }
          float getTriggerVictimMinSpeed() const { return m_triggerVictimMinSpeed; }
          void  setRadiusDetonateWithAirVehicles(bool v) { m_radiusDetonateWithAirVehicles = v; }
          bool  getRadiusDetonateWithAirVehicles() const { return m_radiusDetonateWithAirVehicles; }
          void  setUseCollisionNormal(bool v) { m_useCollisionNormal = v; }
          void  setPlayMaterialEffectAtSurface(bool v) { m_playMaterialEffectAtSurface = v; }
          void  setTriggerTime(float v) { m_triggerTime = v; }
          void  setAllowFriendlyFireDetonation(bool v) { m_allowFriendlyFireDetonation = v; }
          bool  getAllowFriendlyFireDetonation() const { return m_allowFriendlyFireDetonation; }
          void  setFlashbangRadiusWithNightVision(float v) { m_flashbangRadiusWithNightVision = v; }
          void  setTimeUntilCanDetonate(float v) { m_timeUntilCanDetonate = v; }

          // The 21 flashbang channels - client presentation only, but every one is authored.
          void  setFlashbangBillboardMinAlpha(float v) { m_flashbang[0] = v; }
          void  setFlashbangBillboardMaxAlpha(float v) { m_flashbang[1] = v; }
          void  setFlashbangBillboardMaxRampTime(float v) { m_flashbang[2] = v; }
          void  setFlashbangBillboardMinHoldTime(float v) { m_flashbang[3] = v; }
          void  setFlashbangBillboardMaxHoldTime(float v) { m_flashbang[4] = v; }
          void  setFlashbangBillboardMinHealTime(float v) { m_flashbang[5] = v; }
          void  setFlashbangBillboardMaxHealTime(float v) { m_flashbang[6] = v; }
          void  setFlashbangAdditiveGlowMinAlpha(float v) { m_flashbang[7] = v; }
          void  setFlashbangAdditiveGlowMaxAlpha(float v) { m_flashbang[8] = v; }
          void  setFlashbangAdditiveGlowMaxRampTime(float v) { m_flashbang[9] = v; }
          void  setFlashbangAdditiveGlowMinHoldTime(float v) { m_flashbang[10] = v; }
          void  setFlashbangAdditiveGlowMaxHoldTime(float v) { m_flashbang[11] = v; }
          void  setFlashbangAdditiveGlowMinHealTime(float v) { m_flashbang[12] = v; }
          void  setFlashbangAdditiveGlowMaxHealTime(float v) { m_flashbang[13] = v; }
          void  setFlashbangMotionBlurMinAlpha(float v) { m_flashbang[14] = v; }
          void  setFlashbangMotionBlurMaxAlpha(float v) { m_flashbang[15] = v; }
          void  setFlashbangMotionBlurMaxRampTime(float v) { m_flashbang[16] = v; }
          void  setFlashbangMotionBlurMinHoldTime(float v) { m_flashbang[17] = v; }
          void  setFlashbangMotionBlurMaxHoldTime(float v) { m_flashbang[18] = v; }
          void  setFlashbangMotionBlurMinHealTime(float v) { m_flashbang[19] = v; }
          void  setFlashbangMotionBlurMaxHealTime(float v) { m_flashbang[20] = v; }

          // The two effect-template names. The resolved pointers are filled lazily by the
          // component constructor, so setting a name must clear the cache alongside it.
          void setEndEffectName(const std::string& v)      { m_endEffectName = v; m_endEffect = 0; }
          void setWaterEndEffectName(const std::string& v) { m_waterEndEffectName = v; m_waterEndEffect = 0; }

          // effectRotation arrives as three floats; the console binds it as a Vec3.
          void setEffectRotation(float x, float y, float z)
          { m_effectRotation[0] = x; m_effectRotation[1] = y; m_effectRotation[2] = z; }

          // The clamped pair's getters - retail has the setters as real symbols but no
          // getters; these are [CHOSEN] read-backs so the bindings can round-trip.
          float getTriggerAngle() const { return m_triggerAngle; }

          // ALL 45 fields AND their defaults, from the constructor's stores
          // and confirmed field-by-field against the pass-through getters, makeScript reads and
          // generated console executors. The registered vocabulary contains 56 names; two are
          // shipped-dead no-op commands and the writer-only waterEndEffectTemplate name has no
          // console registration.
          // TWO object templates ARE held BY name AND resolved lazily - not here, but in
          // DefaultDetonationComp's constructor, through objectTemplateManager.
          const std::string& getEndEffectName() const { return m_endEffectName; }
          IObjectTemplate*   getEndEffect() const     { return m_endEffect; }
          void setEndEffect(IObjectTemplate* t)       { m_endEffect = t; }
          const std::string& getWaterEndEffectName() const { return m_waterEndEffectName; }
          IObjectTemplate*   getWaterEndEffect() const  { return m_waterEndEffect; }
          void setWaterEndEffect(IObjectTemplate* t)    { m_waterEndEffect = t; }

          bool  getPlayMaterialEffectAtSurface() const { return m_playMaterialEffectAtSurface; }
          int   getExplosionMaterial() const           { return m_explosionMaterial; }
          float getExplosionForce() const              { return m_explosionForce; }
          float getExplosionYMod() const               { return m_explosionYMod; }
          float getExplosionRadius() const             { return m_explosionRadius; }
          float getExplosionInnerConeRadius() const    { return m_explosionInnerConeRadius; }
          float getFlashbangRadiusWithNightVision() const { return m_flashbangRadiusWithNightVision; }
          float getExplosionSoldierLineOfSight() const { return m_explosionSoldierLineOfSight; }
          float getExplosionSoldierLineOfSightTimer() const { return m_explosionSoldierLineOfSightTimer; }
          float getExplosionSoldierLineOfSightHealRate() const { return m_explosionSoldierLineOfSightHealRate; }
          float getExplosionLineOfSightMinimumDamage() const { return m_explosionLineOfSightMinimumDamage; }
          float getExplosionConeAngle() const          { return m_explosionConeAngle; }
          float getExplosionDamage() const             { return m_explosionDamage; }
          int   getRadiusTriggerType() const           { return m_radiusTriggerType; }
          float getTimeUntilCanDetonate() const        { return m_timeUntilCanDetonate; }
          float getTriggerTime() const                     { return m_triggerTime; }
          float getMaxDepthForExplosion() const        { return m_maxDepthForExplosion; }
          float getTriggerRadius() const               { return m_triggerRadius; }
          float getDetonateDistanceToTarget() const    { return m_detonateDistanceToTarget; }
          bool  getUseMMOnEndEffect() const                      { return m_useMMOnEndEffect; }
          // detonate reads both of these. +0x71 selects whether the end effect is oriented by the
          // collision normal or by the projectile's own facing; makeScript names that byte
          // `useCollisionNormal`. `effectRotation` is the extra Vec3 handed to doImpact, and both
          // ports' generated executor copies it directly to +0x74/+0x78/+0x7C.
          bool  getUseCollisionNormal() const                      { return m_useCollisionNormal; }
          float getEffectRotation(int i) const                  { return m_effectRotation[i]; }

          float getFlashbangBillboardMinAlpha() const     { return m_flashbang[0]; }
          float getFlashbangBillboardMaxAlpha() const     { return m_flashbang[1]; }
          float getFlashbangBillboardMaxRampTime() const  { return m_flashbang[2]; }
          float getFlashbangBillboardMinHoldTime() const  { return m_flashbang[3]; }
          float getFlashbangBillboardMaxHoldTime() const  { return m_flashbang[4]; }
          float getFlashbangBillboardMinHealTime() const  { return m_flashbang[5]; }
          float getFlashbangBillboardMaxHealTime() const  { return m_flashbang[6]; }
          float getFlashbangAdditiveGlowMinAlpha() const    { return m_flashbang[7]; }
          float getFlashbangAdditiveGlowMaxAlpha() const    { return m_flashbang[8]; }
          float getFlashbangAdditiveGlowMaxRampTime() const { return m_flashbang[9]; }
          float getFlashbangAdditiveGlowMinHoldTime() const { return m_flashbang[10]; }
          float getFlashbangAdditiveGlowMaxHoldTime() const { return m_flashbang[11]; }
          float getFlashbangAdditiveGlowMinHealTime() const { return m_flashbang[12]; }
          float getFlashbangAdditiveGlowMaxHealTime() const { return m_flashbang[13]; }
          float getFlashbangMotionBlurMinAlpha() const    { return m_flashbang[14]; }
          float getFlashbangMotionBlurMaxAlpha() const    { return m_flashbang[15]; }
          float getFlashbangMotionBlurMaxRampTime() const { return m_flashbang[16]; }
          float getFlashbangMotionBlurMinHoldTime() const { return m_flashbang[17]; }
          float getFlashbangMotionBlurMaxHoldTime() const { return m_flashbang[18]; }
          float getFlashbangMotionBlurMinHealTime() const { return m_flashbang[19]; }
          float getFlashbangMotionBlurMaxHealTime() const { return m_flashbang[20]; }

  protected:
          /* +0x08 */ int   m_refCount;                      // 1
          /* +0x0C */ std::string m_endEffectName;           // ""
          /* +0x10 */ IObjectTemplate* m_endEffect;          // 0 - resolved by the component ctor
          /* +0x14 */ std::string m_waterEndEffectName;        // ""
          /* +0x18 */ IObjectTemplate* m_waterEndEffect;       // 0 - resolved by the component ctor
          /* +0x1C */ bool  m_playMaterialEffectAtSurface;   // false
          /* +0x20 */ int   m_explosionMaterial;             // -1  hasExplosion() is `!= -1`
          /* +0x24 */ float m_explosionForce;                // 150.0f
          /* +0x28 */ float m_explosionYMod;                 // 1.0f
          /* +0x2C */ float m_explosionRadius;               // 10.0f
          /* +0x30 */ float m_explosionInnerConeRadius;      // 0.0f
          /* +0x34 */ float m_flashbangRadiusWithNightVision;// 0.0f
          /* +0x38 */ float m_explosionSoldierLineOfSight;   // 0.0f
          /* +0x3C */ float m_explosionSoldierLineOfSightTimer;    // 0.0f
          /* +0x40 */ float m_explosionSoldierLineOfSightHealRate; // 0.0005f
          /* +0x44 */ float m_explosionLineOfSightMinimumDamage;   // 0.3f
          /* +0x48 */ float m_explosionConeAngle;            // 0.0f - setExplosionConeAngle writes it
          /* +0x4C */ float m_explosionDamage;               // 500.0f
          // Underwater depth limit and the distances that enable detonation updates.
          /* +0x50 */ float m_maxDepthForExplosion;          // 2.0f
          /* +0x54 */ float m_triggerRadius;                 // 0.0f
          /* +0x58 */ float m_detonateDistanceToTarget;      // 0.0f
          /* +0x5C */ float m_minDetonationSpeed;            // 0.0f  named (was m_float5C)
          /* +0x60 */ float m_detectionRadius;               // 1.5f
          /* +0x64 */ bool  m_radiusDetonateWithAirVehicles; // false named (was m_byte64)
          /* +0x65 */ bool  m_showMineIndicationIcon;        // false
          /* +0x66 */ bool  m_useMMOnEndEffect;              // false named (was m_byte66) -
                                            // detonate's impact-arm gate; MM = MaterialManager
          /* +0x68 */ float m_triggerVictimMinSpeed; // Minimum victim speed for triggering.
          /* +0x6C */ int   m_radiusTriggerType;             // 0  the console name is
                                            // `triggerType`; the member keeps its accessor spelling
          /* +0x70 */ bool  m_detonateOnWaterCollision;      // true  named (was m_byte70)
          /* +0x71 */ bool  m_useCollisionNormal;            // true  named (was m_byte71) -
                                            // RW-70's first open question, settled
          /* +0x74 */ float m_effectRotation[3];             // 0,0,0 named (was m_vec74) -
                                            // RW-70's second; makeScript writes x/y/z joined by '/'
          /* +0x80 */ float m_triggerAngle; // Angle in degrees; retains the setter's clamp behavior.
          /* +0x84 */ float m_triggerTime; // Initial component countdown.
          // Flashbang groups: billboard, additive glow, motion blur; each has min/max alpha,
          // ramp, min/max hold, and min/max heal.
          /* +0x88 */ float m_flashbang[21];
          // defaults: 0.5 1.0 0.12 2.0 6.0 2.0 6.0 | 0.5 1.0 0.02 1.0 5.0 1.0 5.0
          // | 0.96 1.0 0.02 2.0 10.0 2.0 10.0
          /* +0xDC */ bool  m_allowFriendlyFireDetonation; // Console setting; no runtime reader is known.
          /* +0xE0 */ float m_timeUntilCanDetonate;          // 0.0f
};

// Subclass, not a direct implementor: RTTI base = DefaultDetonationCompTemplate. Inherits
// getInterfaceID, objectCreated, objectDestroyed, setTemplate and all four accessors; overrides
// the quartet, getClassID, createComponent, queryImplementation and makeScript.
class ReplenishDetonationCompTemplate : public DefaultDetonationCompTemplate
{ public:
    ReplenishDetonationCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int);
    virtual bool  makeScript(io::IStream*);

    // Retail's generated console executors store these two leaf fields directly. These trivial
    // wrappers preserve that operation while keeping ModuleProjectileComp.cpp out of the layout.
    void setReplenishingStrength(float value) { m_replenishingStrength = value; }
    float getReplenishingStrength() const { return m_replenishingStrength; }
    void setReplenishingType(ReplenishingType value) { m_replenishingType = value; }
    ReplenishingType getReplenishingType() const { return m_replenishingType; }

protected:
    // The subclass starts immediately after DefaultDetonationCompTemplate: +0xE4 in the retail
    // ELF and +0x114 in the Win32 PE. Both ports allocate twelve more bytes in this order.
    int m_refCount;
    float m_replenishingStrength;
    ReplenishingType m_replenishingType;
};

// the remaining template families
class DefaultFollowCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable        // , 12 slots
{
public:
    BF2_PROJECTILE_COMP_TEMPLATE(DefaultFollowCompTemplate);
    DefaultFollowCompTemplate();

    void setMaxYaw(float value) { m_maxYaw = value; }
    float getMaxYaw() const { return m_maxYaw; }
    void setMaxPitch(float value) { m_maxPitch = value; }
    float getMaxPitch() const { return m_maxPitch; }
    void setChangeYaw(float value) { m_changeYaw = value; }
    float getChangeYaw() const { return m_changeYaw; }
    void setChangePitch(float value) { m_changePitch = value; }
    float getChangePitch() const { return m_changePitch; }
    void setMinDist(float value) { m_minDist = value; }
    float getMinDist() const { return m_minDist; }

private:
    /* +0x08 */ int m_refCount;
    /* +0x0C */ float m_maxYaw;
    /* +0x10 */ float m_maxPitch;
    /* +0x14 */ float m_changeYaw;
    /* +0x18 */ float m_changePitch;
    /* +0x1C */ float m_minDist;
};

class DefaultPenetrateCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable     // , 12 slots
{ public: BF2_PROJECTILE_COMP_TEMPLATE(DefaultPenetrateCompTemplate);
          DefaultPenetrateCompTemplate();

          // THE TWO permission bytes, from the constructor's five stores.
          // both default TO true, unlike DefaultCollisionCompTemplate's pair which both start
          // false. DefaultPenetrateComp::penetrate gates on them only when the surface material does
          // not set overrideNeverPenetrate.
          bool getCanPenetrateSolid() const { return m_canPenetrateSolid; }
          bool getCanPenetrateWater() const { return m_canPenetrateWater; }

          void setAllowSolidPenetration(bool value) { m_canPenetrateSolid = value; }
          bool getAllowSolidPenetration() const { return m_canPenetrateSolid; }
          void setAllowLiquidPenetration(bool value) { m_canPenetrateWater = value; }
          bool getAllowLiquidPenetration() const { return m_canPenetrateWater; }

          // shipped dead property. Both ports' console executor parse the SET value and resolve
          // the active component, but store nothing; GET resolves it and returns literal true.
          // Preserve that quirk rather than treating the name as "clear both permission bytes".
          void setNeverPenetrate(bool) {}
          bool getNeverPenetrate() const { return true; }

  protected:
          /* +0x08 */ int  m_refCount;
          /* +0x0C */ bool m_canPenetrateSolid;
          /* +0x0D */ bool m_canPenetrateWater;
};

class DefaultProjectileSoundCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable  // , 12 slots
{
public:
    BF2_PROJECTILE_COMP_TEMPLATE(DefaultProjectileSoundCompTemplate);
    DefaultProjectileSoundCompTemplate();

    // setTemplate calls this with zero for the second argument. The uint is present in the ELF
    // mangling and unused in the body; retain it rather than folding the helper into setTemplate.
    void initTemplates(SimpleObjectTemplate*, unsigned int);

protected:
    /* +0x08 */ int m_refCount;
};

class DefaultRicochetCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable      // , 12 slots
{ public: BF2_PROJECTILE_COMP_TEMPLATE(DefaultRicochetCompTemplate);
          DefaultRicochetCompTemplate();
          // NO tuning AT ALL - two vptrs and a reference count is the whole class, which is
          // consistent with DefaultRicochetComp never reading its template and with the bounce cone
          // being a hard-coded 0.2.
  protected:
          /* +0x08 */ int m_refCount;
};

// The fixed-target seek template: exactly 24 bytes in both shipped x86 images. Unlike the empty
// declaration that used to stand here, every one of its twelve vtable entries and both authored
// fields are now represented.
class FixedTargetCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable          // , 12 slots
{
public:
    BF2_PROJECTILE_COMP_TEMPLATE(FixedTargetCompTemplate);
    FixedTargetCompTemplate();

    void setTrackingDelay(float seconds);

    TargetType getTargetType() const       { return m_targetType; }
    float getTrackingDelay() const         { return m_trackingDelay; }
    int getTrackingDelayTick() const       { return m_trackingDelayTick; }

    // [CHOSEN source wrapper, recovered effect] Retail's generated seek.targetType executor writes
    // +0x0C directly. Keeping that write behind a trivial setter lets the reconstructed console
    // macro reproduce the same stored value without exposing the field.
    void setTargetType(TargetType type)     { m_targetType = type; }

private:
    /* +0x08 */ int        m_refCount;          // ctor 1
    /* +0x0C */ TargetType m_targetType;        // ctor TTHeat (2)
    /* +0x10 */ float      m_trackingDelay;     // seconds; ctor 0.0f
    /* +0x14 */ int        m_trackingDelayTick; // trunc(seconds * 30.0f)
};

// The predestined artillery follow template carries no authored state: both shipped x86 images
// prove that its complete object is just two vptrs and this leaf reference count (12 bytes).
class PreDestinedFollowCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable    // , 12 slots
{
public:
    BF2_PROJECTILE_COMP_TEMPLATE(PreDestinedFollowCompTemplate);
    PreDestinedFollowCompTemplate();

private:
    /* +0x08 */ int m_refCount;
};

class SeekClosestTargetCompTemplate : public IProjectileCompTemplate, public io::IConsoleSaveable    // , 12 slots
{
public:
    BF2_PROJECTILE_COMP_TEMPLATE(SeekClosestTargetCompTemplate);
    SeekClosestTargetCompTemplate();

    void setTrackingDelay(float seconds);
    void setReLockDelay(float seconds);
    void setMaxAngleLock(float degrees);

    // (17 B), PE 0x5B9D30 (10 B). The console setter independently attests
    // m_maxAngleLock at +0x0C. The stored value is radians; the weapon-facing API uses degrees.
    float getMaxAngleLock() const { return m_maxAngleLock * 57.2957763671875f; }
    // Runtime seek math consumes the stored radians directly (ELF 0x8294D10 / 0x8294A9C).
    float getMaxTargetAngle() const { return m_maxAngleLock; }

    // TargetComp::getMaxTargetDistance/getTargetType read these two fields directly after
    // resolving CID_SeekClosestTargetCompTemplate (ELF 0x8278CB0/0x8278C70; PE twins). Keep the
    // source call typed even while the rest of this independently gated template remains dark.
    float getMaxTargetDistance() const { return m_maxTargetDistance; }
    TargetType getTargetType() const { return m_targetType; }
    float getTrackingDelay() const { return m_trackingDelay; }
    int getTrackingDelayTick() const { return m_trackingDelayTick; }
    float getReLockDelay() const { return m_reLockDelay; }
    int getReLockDelayTick() const { return m_reLockDelayTick; }
    float getDirectionBonus() const { return m_directionBonus; }

    void setMaxTargetDistance(float value) { m_maxTargetDistance = value; }
    void setTargetType(TargetType value) { m_targetType = value; }
    void setDirectionBonus(float value) { m_directionBonus = value; }

private:
    /* +0x08 */ int m_refCount;
    /* +0x0C */ float m_maxAngleLock;          // radians
    /* +0x10 */ float m_maxTargetDistance;
    /* +0x14 */ TargetType m_targetType;
    /* +0x18 */ float m_trackingDelay;
    /* +0x1C */ int m_trackingDelayTick;       // trunc(seconds * 30)
    /* +0x20 */ float m_reLockDelay;
    /* +0x24 */ int m_reLockDelayTick;         // trunc(seconds * 30)
    /* +0x28 */ float m_directionBonus;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILECOMPTEMPLATES_H
