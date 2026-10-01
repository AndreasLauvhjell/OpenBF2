// Registers projectile collision, detonation, seek/follow and penetration console properties.
// Collision siblings share names and are selected through IID/CID-qualified active-component queries.
// The authored Default collision property is bouncing, while the accessor is getIsBouncing.
// [CHOSEN] ObjectTemplate access/category defaults are inherited from the sibling component bindings;
// original registration-unit placement is [INFERRED].

#include "../../../Core/ConsoleObjectMacro.h"
#include "../../../Core/Ids.h"                            // the two IIDs and their CIDs
#include "../../../Scene/Object/ObjectTemplateUtil.h"     // world::getActiveTemplateComponent
#include "GenericProjectileTemplate.h"
#include "ProjectileCompTemplates.h"
#include "../../../Core/StringUtil.h"
#include <cstdlib>
#include <sstream>

namespace dice {
namespace hfe {
namespace world {

// (writer), ELF 0x8581100 / PE 0x4800A0 (reader).
// Full instructions ConsoleClass663::setArgFromString (ELF 0x8608E80)
// calls this enum reader for the +0x70 trigger type; stock MTYPco must not use integer extraction.
std::ostream& operator<<(std::ostream& stream, RadiusTriggerType type)
{
    switch (type)
    {
    case MTYNone:      stream << "MTYNone"; break;
    case MTYPersonnel: stream << "MTYPersonnel"; break;
    case MTYVehicle:   stream << "MTYVehicle"; break;
    case MTYPco:       stream << "MTYPco"; break;
    default: break;
    }
    return stream;
}

std::istream& operator>>(std::istream& stream, RadiusTriggerType& type)
{
    std::string token;
    stream >> token;
    std::ostringstream normalized;
    if (token.length() == 1)
    {
        normalized << static_cast<RadiusTriggerType>(std::atoi(token.c_str()));
        token = normalized.str();
    }
    if (equalsIgnoreCase(token, "MTYNone"))
        type = MTYNone;
    // Shipped defect in both images: reads RTAmmo for value 1, while the writer emits
    // MTYPersonnel. Consequently "1" and "MTYPersonnel" leave the destination unchanged.
    else if (equalsIgnoreCase(token, "RTAmmo"))
        type = MTYPersonnel;
    else if (equalsIgnoreCase(token, "MTYVehicle"))
        type = MTYVehicle;
    else if (equalsIgnoreCase(token, "MTYPco"))
        type = MTYPco;
    return stream;
}

namespace {
// Retail inserts these generated objects into the same list as GameLogicObjTemplBFModule and the
// weapon vocabulary (ELF list 0x8AD54F8). A private projectile-only tag produces an orphan list
// that no module ever registers, so the properties parse and silently disappear.
const char* const kProjectileCompConsoleTag = "world.objTemplGame";

GenericProjectileTemplate* activeGenericProjectile()
{
    return static_cast<GenericProjectileTemplate*>(
        getActiveTemplate(CID_GenericProjectileTemplate));
}

void setGenericProjectileTimeToLive(const RandomDistribution& value)
{
    activeGenericProjectile()->m_timeToLive = value;
}

void setGenericProjectileMinDamage(float value)
{
    activeGenericProjectile()->m_minDamage = value;
}

// armingDelay's SET arm goes through the accessor because setArmingDelay does two stores - the
// seconds at +0x1CC and the int(seconds * 30) tick count at +0x1D0 - while its GET arm reads
// +0x1CC raw. Writing the field directly here would leave the tick count stale, which is the whole
// reason retail's executor is asymmetric.
void setGenericProjectileArmingDelay(float value)
{
    activeGenericProjectile()->setArmingDelay(value);
}

// `3dMapIcon` begins with a digit, so it can only be bound through the _NAMED property macro, and
// that macro takes a setter call. Retail's SET arm is a plain store (ELF 0x85FBC60 writes +0x1FC,
// BF2.exe 0x51BDB9 writes +0x2DC), so this is one too.
void setGenericProjectile3dMapIcon(int value)
{
    activeGenericProjectile()->m_field1FC = value;
}

// Returns the active collision component only if it implements the Default class ID.
// Checking IID alone would accept sibling layouts and make the concrete cast unsafe.
DefaultCollisionCompTemplate* activeCollision()
{
    return static_cast<DefaultCollisionCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileCollisionCompTemplate,      // 50507
                                   CID_DefaultCollisionCompTemplate));        // 38202
}

ReplenishCollisionCompTemplate* activeReplenishCollision()
{
    return static_cast<ReplenishCollisionCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileCollisionCompTemplate,
                                   CID_ReplenishCollisionCompTemplate));
}

ResurrectCollisionCompTemplate* activeResurrectCollision()
{
    return static_cast<ResurrectCollisionCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileCollisionCompTemplate,
                                   CID_ResurrectCollisionCompTemplate));
}

StickyCollisionCompTemplate* activeStickyCollision()
{
    return static_cast<StickyCollisionCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileCollisionCompTemplate,
                                   CID_StickyCollisionCompTemplate));
}

// All three penetration executors use retail's two-id form. The interface id selects the
// projectile-penetration component and the class id prevents a future sibling implementation from
// being cast to DefaultPenetrateCompTemplate. PE 0x4A15D0 and ELF 0x8601D90/0x8602200/0x8602680
// agree on IID 50531 and CID 38206.
DefaultPenetrateCompTemplate* activePenetrate()
{
    return static_cast<DefaultPenetrateCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectilePenetrateCompTemplate,     // 50531
                                   CID_DefaultPenetrateCompTemplate));       // 38206
}

// Requires the DefaultDetonationCompTemplate class as well as the component interface.
// invisibleAtEndEffect and stopAtEndEffect have inert setters and false getters, so are omitted.
// waterEndEffectTemplate is serialized without a parser binding.
DefaultDetonationCompTemplate* activeDetonation()
{
    return static_cast<DefaultDetonationCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileDetonationCompTemplate,     // 50511
                                   CID_DefaultDetonationCompTemplate));       // 38203
}

// effectRotation is one Vec3 console argument over the template's three-float array. The generated
// executor copies x/y/z to +0x74/+0x78/+0x7C in both ports (ELF 0x8609E50, PE 0x4A4660).
Vec3 detonationEffectRotationGet()
{
    DefaultDetonationCompTemplate* component = activeDetonation();
    Vec3 value;
    value.x = component->getEffectRotation(0);
    value.y = component->getEffectRotation(1);
    value.z = component->getEffectRotation(2);
    return value;
}

void detonationEffectRotationSet(const Vec3& value)
{
    activeDetonation()->setEffectRotation(value.x, value.y, value.z);
}

// The replenishing leaf inherits the complete DefaultDetonation surface, so the base receiver above
// deliberately continues to answer for its common properties through queryImplementation(38203).
// Its two additional fields use the concrete CID, matching both ports' generated executors.
ReplenishDetonationCompTemplate* activeReplenishDetonation()
{
    return static_cast<ReplenishDetonationCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileDetonationCompTemplate,
                                   CID_ReplenishDetonationCompTemplate));
}

// Retail's two FixedTarget executors use this exact two-id receiver: the shared seek interface is
// not enough, because SeekClosestTargetCompTemplate implements it as well.
FixedTargetCompTemplate* activeFixedTarget()
{
    return static_cast<FixedTargetCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileSeekCompTemplate,           // 50519
                                   CID_FixedTargetCompTemplate));             // 38209
}

SeekClosestTargetCompTemplate* activeSeekClosestTarget()
{
    return static_cast<SeekClosestTargetCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileSeekCompTemplate,
                                   CID_SeekClosestTargetCompTemplate));
}

DefaultFollowCompTemplate* activeDefaultFollow()
{
    return static_cast<DefaultFollowCompTemplate*>(
        getActiveTemplateComponent(IID_IProjectileFollowCompTemplate,
                                   CID_DefaultFollowCompTemplate));
}
} // namespace

BF2_CONSOLE_OBJECT("ObjectTemplate", 2, 0)

// GenericProjectile's motor scalars are distinct from RotationalBundle's same-named Vec3
// properties. Retail selects between them with the active-template CID. Both ports' generated
// executors are direct field reads/writes: startDelay is ELF +0x1D4 / PE +0x2B4
// (0x85FB370 / 0x49EC40), acceleration is +0x1EC / +0x2CC (0x85FC0C0 / 0x49F160), and maxSpeed
// is +0x1F0 / +0x2D0 (0x85FC520 / 0x49F320). startDelay controls the launch effect only; the
// recovered GenericProjectile::handleUpdate motor consumes acceleration and maxSpeed directly.
BF2_CONSOLE_FIELD(startDelay,   float, activeGenericProjectile(), m_field1D4, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(acceleration, float, activeGenericProjectile(), m_field1EC, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(maxSpeed,     float, activeGenericProjectile(), m_field1F0, kProjectileCompConsoleTag)

// The remaining stock TOW lifetime/direct-impact surface. timeToLive is a full five-word
// RandomDistribution (ELF executor 0x85F5540 / PE 0x49C690), not a scalar. minDamage has a raw SET
// but its GET calls getMinDamage so the shipped -1 sentinel falls back to damage. damage,
// hasOnTimeEffect and damageOwner are plain direct fields in both images.
BF2_CONSOLE_PROPERTY_FN(timeToLive, RandomDistribution,
                        getActiveTemplate(CID_GenericProjectileTemplate) != 0,
                        activeGenericProjectile()->getTimeToLive(),
                        setGenericProjectileTimeToLive, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(damage, float, activeGenericProjectile(), m_projectileDamage, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY_FN(minDamage, float,
                        getActiveTemplate(CID_GenericProjectileTemplate) != 0,
                        activeGenericProjectile()->getMinDamage(),
                        setGenericProjectileMinDamage, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(hasOnTimeEffect, bool, activeGenericProjectile(), m_hasOnTimeEffect, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(damageOwner, bool, activeGenericProjectile(), m_byte1F8, kProjectileCompConsoleTag)

// ConsoleClass569 (ELF 0x85F6750 / PE 0x49CE50) dispatches the template's bool
// setter/getter. The stock defibrillator enables this property so its point projectile
// reaches a disabled soldier's collision skeleton (ELF template byte +0x1F9 / PE +0x2D9).
BF2_CONSOLE_PROPERTY(damageMandownSoldiers, bool, activeGenericProjectile(), setDamageMandownSoldiers, getDamageMandownSoldiers, kProjectileCompConsoleTag)

// Generic projectile properties use the active-template class gate and zero/one arguments for get/set.
// armingDelay's setter also updates ticks; tracerTemplate's setter invalidates its cache.
// The trailEffectTemplate store is known, but its wet/dry label conflicts with activation's selector;
// verify that pairing before relying on the labels.
BF2_CONSOLE_FIELD(distToStartLoseDamage, float, activeGenericProjectile(), m_distToStartLoseDamage, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(distToMinDamage,       float, activeGenericProjectile(), m_distToMinDamage,       kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY_FN(armingDelay, float, getActiveTemplate(CID_GenericProjectileTemplate) != 0,
                        activeGenericProjectile()->m_armingDelay,
                        setGenericProjectileArmingDelay, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(timeToRemoveTK,        float, activeGenericProjectile(), m_timeToRemoveTK,        kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(canPickup,             bool,  activeGenericProjectile(), m_canPickup,             kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(invisible,             bool,  activeGenericProjectile(), m_invisible,             kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(addToProjectileList,   bool,  activeGenericProjectile(), m_addToProjectileList,   kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(useProjectileCamera,   bool,  activeGenericProjectile(), m_useProjectileCamera,   kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(detonateAfterProjectileStopped, bool, activeGenericProjectile(),
                  m_detonateAfterProjectileStopped, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(detonateAfterProjectileStoppedMinSpeed, float, activeGenericProjectile(),
                  m_detonateAfterProjectileStoppedMinSpeed, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(localPredictOnClient, bool, activeGenericProjectile(),
                     setLocalPredictOnClient, getLocalPredictOnClient, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(maxTracerScaler,        float, activeGenericProjectile(), m_maxTracerScaler,        kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(minTracerScaler,        float, activeGenericProjectile(), m_minTracerScaler,        kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(tracerScaler,           float, activeGenericProjectile(), m_tracerScaler,           kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(tracerSizeModifier,     float, activeGenericProjectile(), m_tracerSizeModifier,     kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(tracerConvergeDistance, float, activeGenericProjectile(), m_tracerConvergeDistance, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(tracerInterval, int, activeGenericProjectile(),
                     setTracerInterval, getTracerInterval, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(tracerTemplate, std::string, activeGenericProjectile(),
                     setTracerTemplateName, getTracerTemplateName, kProjectileCompConsoleTag)
BF2_CONSOLE_FIELD(trailEffectTemplate, std::string, activeGenericProjectile(), m_string208, kProjectileCompConsoleTag)

// Projectile map-marker properties share their spelling with SupplyObject properties.
// The active-template class gate selects the correct receiver; both bindings are required.
BF2_CONSOLE_FIELD(visibleOn3dMap, bool, activeGenericProjectile(), m_byte1FA, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY_FN_NAMED("3dMapIcon", int,
                              getActiveTemplate(CID_GenericProjectileTemplate) != 0,
                              activeGenericProjectile()->m_field1FC,
                              setGenericProjectile3dMapIcon, kProjectileCompConsoleTag)

// name                        T     receiver            setter                  getter                  tag
BF2_CONSOLE_PROPERTY(collision.bouncing,           bool, activeCollision(), setIsBouncing,          getIsBouncing,          kProjectileCompConsoleTag)   // +0x0D
BF2_CONSOLE_PROPERTY(collision.hasCollisionEffect, bool, activeCollision(), setHasCollisionEffect,  getHasCollisionEffect,  kProjectileCompConsoleTag)   // +0x0C

// The three sibling collision templates are independent implementations rather than subclasses of
// DefaultCollision. Retail therefore registers separate active-CID executors for their shared two
// names; duplicate console spellings are selected by isObjectActive, as in the fire-component table.
BF2_CONSOLE_PROPERTY(collision.bouncing, bool, activeReplenishCollision(), setIsBouncing, getIsBouncing, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.hasCollisionEffect, bool, activeReplenishCollision(), setHasCollisionEffect, getHasCollisionEffect, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.replenishingStrength, float, activeReplenishCollision(), setReplenishingStrength, getReplenishingStrength, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.replenishingType, ReplenishingType, activeReplenishCollision(), setReplenishingType, getReplenishingType, kProjectileCompConsoleTag)

BF2_CONSOLE_PROPERTY(collision.bouncing, bool, activeResurrectCollision(), setIsBouncing, getIsBouncing, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.hasCollisionEffect, bool, activeResurrectCollision(), setHasCollisionEffect, getHasCollisionEffect, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.resurrectDamage, float, activeResurrectCollision(), setResurrectDamage, getResurrectDamage, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.restoreHP, float, activeResurrectCollision(), setRestoreHP, getRestoreHP, kProjectileCompConsoleTag)

BF2_CONSOLE_PROPERTY(collision.bouncing, bool, activeStickyCollision(), setIsBouncing, getIsBouncing, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.hasCollisionEffect, bool, activeStickyCollision(), setHasCollisionEffect, getHasCollisionEffect, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.stickToTerrain, bool, activeStickyCollision(), setStickToTerrain, getStickToTerrain, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.stickToStaticObjects, bool, activeStickyCollision(), setStickToStaticObjects, getStickToStaticObjects, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.stickToVehicles, bool, activeStickyCollision(), setStickToVehicles, getStickToVehicles, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(collision.maxStickAngle, float, activeStickyCollision(), setMaxStickAngle, getMaxStickAngleDegrees, kProjectileCompConsoleTag)

// PE/ELF executors write the solid and liquid bytes at +0x0C/+0x0D. neverPenetrate is a shipped
// dead property: SET resolves the receiver but stores nothing, while GET always returns true.
BF2_CONSOLE_PROPERTY(penetrate.neverPenetrate, bool, activePenetrate(), setNeverPenetrate, getNeverPenetrate, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(penetrate.allowSolidPenetration, bool, activePenetrate(), setAllowSolidPenetration, getAllowSolidPenetration, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(penetrate.allowLiquidPenetration, bool, activePenetrate(), setAllowLiquidPenetration, getAllowLiquidPenetration, kProjectileCompConsoleTag)

// FixedTarget ConsoleClass715/714 in retail. targetType writes +0x0C directly; trackingDelay calls
// setTrackingDelay so the authored seconds and derived 30 Hz tick count change atomically.
BF2_CONSOLE_PROPERTY(seek.targetType, TargetType, activeFixedTarget(), setTargetType, getTargetType, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(seek.trackingDelay, float, activeFixedTarget(), setTrackingDelay, getTrackingDelay, kProjectileCompConsoleTag)

// SeekClosest has its own generated bindings for the two shared names plus four leaf properties.
// The angle setter converts degrees to the stored radians; both time setters refresh 30 Hz caches.
BF2_CONSOLE_PROPERTY(seek.targetType, TargetType, activeSeekClosestTarget(), setTargetType, getTargetType, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(seek.trackingDelay, float, activeSeekClosestTarget(), setTrackingDelay, getTrackingDelay, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(seek.maxAngleLock, float, activeSeekClosestTarget(), setMaxAngleLock, getMaxAngleLock, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(seek.maxDistLock, float, activeSeekClosestTarget(), setMaxTargetDistance, getMaxTargetDistance, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(seek.reLockTime, float, activeSeekClosestTarget(), setReLockDelay, getReLockDelay, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(seek.directionBonus, float, activeSeekClosestTarget(), setDirectionBonus, getDirectionBonus, kProjectileCompConsoleTag)

BF2_CONSOLE_PROPERTY(follow.maxYaw, float, activeDefaultFollow(), setMaxYaw, getMaxYaw, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(follow.maxPitch, float, activeDefaultFollow(), setMaxPitch, getMaxPitch, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(follow.changePitch, float, activeDefaultFollow(), setChangePitch, getChangePitch, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(follow.changeYaw, float, activeDefaultFollow(), setChangeYaw, getChangeYaw, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(follow.minDist, float, activeDefaultFollow(), setMinDist, getMinDist, kProjectileCompConsoleTag)

BF2_CONSOLE_PROPERTY(detonation.explosionMaterial, int, activeDetonation(), setExplosionMaterial, getExplosionMaterial, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionRadius, float, activeDetonation(), setExplosionRadius, getExplosionRadius, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionInnerConeRadius, float, activeDetonation(), setExplosionInnerConeRadius, getExplosionInnerConeRadius, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionSoldierLineOfSight, float, activeDetonation(), setExplosionSoldierLineOfSight, getExplosionSoldierLineOfSight, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionSoldierLineOfSightTimer, float, activeDetonation(), setExplosionSoldierLineOfSightTimer, getExplosionSoldierLineOfSightTimer, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionSoldierLineOfSightHealRate, float, activeDetonation(), setExplosionSoldierLineOfSightHealRate, getExplosionSoldierLineOfSightHealRate, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionLineOfSightMinimumDamage, float, activeDetonation(), setExplosionLineOfSightMinimumDamage, getExplosionLineOfSightMinimumDamage, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionYMod, float, activeDetonation(), setExplosionYMod, getExplosionYMod, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionForce, float, activeDetonation(), setExplosionForce, getExplosionForce, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.explosionDamage, float, activeDetonation(), setExplosionDamage, getExplosionDamage, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.maxDepthForExplosion, float, activeDetonation(), setMaxDepthForExplosion, getMaxDepthForExplosion, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.detonateDistanceToTarget, float, activeDetonation(), setDetonateDistanceToTarget, getDetonateDistanceToTarget, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.minDetonationSpeed, float, activeDetonation(), setMinDetonationSpeed, getMinDetonationSpeed, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.detonateOnWaterCollision, bool, activeDetonation(), setDetonateOnWaterCollision, getDetonateOnWaterCollision, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.useMMOnEndEffect, bool, activeDetonation(), setUseMMOnEndEffect, getUseMMOnEndEffect, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.triggerRadius, float, activeDetonation(), setTriggerRadius, getTriggerRadius, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY_FN(detonation.triggerType, RadiusTriggerType, activeDetonation() != 0,
    static_cast<RadiusTriggerType>(activeDetonation()->getTriggerType()),
    activeDetonation()->setTriggerType, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.triggerVictimMinSpeed, float, activeDetonation(), setTriggerVictimMinSpeed, getTriggerVictimMinSpeed, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.radiusDetonateWithAirVehicles, bool, activeDetonation(), setRadiusDetonateWithAirVehicles, getRadiusDetonateWithAirVehicles, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY_FN(detonation.effectRotation, Vec3, activeDetonation() != 0, detonationEffectRotationGet(), detonationEffectRotationSet, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.useCollisionNormal, bool, activeDetonation(), setUseCollisionNormal, getUseCollisionNormal, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.playMaterialEffectAtSurface, bool, activeDetonation(), setPlayMaterialEffectAtSurface, getPlayMaterialEffectAtSurface, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangRadiusWithNightVision, float, activeDetonation(), setFlashbangRadiusWithNightVision, getFlashbangRadiusWithNightVision, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.timeUntilCanDetonate, float, activeDetonation(), setTimeUntilCanDetonate, getTimeUntilCanDetonate, kProjectileCompConsoleTag)

// The two that clamp - see the defect note on setExplosionConeAngle in DefaultDetonationComp.cpp:
// an authored angle of 180 or more is stored as zero, not clamped to 180.
BF2_CONSOLE_PROPERTY(detonation.explosionConeAngle, float, activeDetonation(), setExplosionConeAngle, getExplosionConeAngle, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.triggerAngle, float, activeDetonation(), setTriggerAngle, getTriggerAngle, kProjectileCompConsoleTag)

// The two virtual accessors retail really has (+0x30/+0x34 and +0x38/+0x3C).
BF2_CONSOLE_PROPERTY(detonation.detectionRadius, float, activeDetonation(), setDetectionRadius, getDetectionRadius, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.showMineIndicationIcon, bool, activeDetonation(), setShowMineIndicationIcon, getShowMineIndicationIcon, kProjectileCompConsoleTag)

// The two settled by their executors this sitting.
BF2_CONSOLE_PROPERTY(detonation.triggerTime, float, activeDetonation(), setTriggerTime, getTriggerTime, kProjectileCompConsoleTag)                                        // +0x84
BF2_CONSOLE_PROPERTY(detonation.allowFriendlyFireDetonation, bool, activeDetonation(), setAllowFriendlyFireDetonation, getAllowFriendlyFireDetonation, kProjectileCompConsoleTag)  // +0xDC

// The registered effect-template name. Setting it clears its resolved-pointer cache, which the
// component constructor refills lazily; without that a renamed effect would keep the old target.
// waterEndEffectTemplate is makeScript-only in retail; see the shipped-asymmetry note above.
BF2_CONSOLE_PROPERTY(detonation.endEffectTemplate, std::string, activeDetonation(), setEndEffectName, getEndEffectName, kProjectileCompConsoleTag)

// ReplenishDetonationCompTemplate's leaf fields. Type is the shared RTHeal/RTAmmo vocabulary used
// by ReplenishingAmmoComp; makeScript emits replenishingType unconditionally and strength when nonzero.
BF2_CONSOLE_PROPERTY(detonation.replenishingStrength, float, activeReplenishDetonation(), setReplenishingStrength, getReplenishingStrength, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.replenishingType, ReplenishingType, activeReplenishDetonation(), setReplenishingType, getReplenishingType, kProjectileCompConsoleTag)

// The 21 flashbang channels.
BF2_CONSOLE_PROPERTY(detonation.flashbangBillboardMinAlpha, float, activeDetonation(), setFlashbangBillboardMinAlpha, getFlashbangBillboardMinAlpha, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangBillboardMaxAlpha, float, activeDetonation(), setFlashbangBillboardMaxAlpha, getFlashbangBillboardMaxAlpha, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangBillboardMaxRampTime, float, activeDetonation(), setFlashbangBillboardMaxRampTime, getFlashbangBillboardMaxRampTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangBillboardMinHoldTime, float, activeDetonation(), setFlashbangBillboardMinHoldTime, getFlashbangBillboardMinHoldTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangBillboardMaxHoldTime, float, activeDetonation(), setFlashbangBillboardMaxHoldTime, getFlashbangBillboardMaxHoldTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangBillboardMinHealTime, float, activeDetonation(), setFlashbangBillboardMinHealTime, getFlashbangBillboardMinHealTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangBillboardMaxHealTime, float, activeDetonation(), setFlashbangBillboardMaxHealTime, getFlashbangBillboardMaxHealTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangAdditiveGlowMinAlpha, float, activeDetonation(), setFlashbangAdditiveGlowMinAlpha, getFlashbangAdditiveGlowMinAlpha, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangAdditiveGlowMaxAlpha, float, activeDetonation(), setFlashbangAdditiveGlowMaxAlpha, getFlashbangAdditiveGlowMaxAlpha, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangAdditiveGlowMaxRampTime, float, activeDetonation(), setFlashbangAdditiveGlowMaxRampTime, getFlashbangAdditiveGlowMaxRampTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangAdditiveGlowMinHoldTime, float, activeDetonation(), setFlashbangAdditiveGlowMinHoldTime, getFlashbangAdditiveGlowMinHoldTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangAdditiveGlowMaxHoldTime, float, activeDetonation(), setFlashbangAdditiveGlowMaxHoldTime, getFlashbangAdditiveGlowMaxHoldTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangAdditiveGlowMinHealTime, float, activeDetonation(), setFlashbangAdditiveGlowMinHealTime, getFlashbangAdditiveGlowMinHealTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangAdditiveGlowMaxHealTime, float, activeDetonation(), setFlashbangAdditiveGlowMaxHealTime, getFlashbangAdditiveGlowMaxHealTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangMotionBlurMinAlpha, float, activeDetonation(), setFlashbangMotionBlurMinAlpha, getFlashbangMotionBlurMinAlpha, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangMotionBlurMaxAlpha, float, activeDetonation(), setFlashbangMotionBlurMaxAlpha, getFlashbangMotionBlurMaxAlpha, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangMotionBlurMaxRampTime, float, activeDetonation(), setFlashbangMotionBlurMaxRampTime, getFlashbangMotionBlurMaxRampTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangMotionBlurMinHoldTime, float, activeDetonation(), setFlashbangMotionBlurMinHoldTime, getFlashbangMotionBlurMinHoldTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangMotionBlurMaxHoldTime, float, activeDetonation(), setFlashbangMotionBlurMaxHoldTime, getFlashbangMotionBlurMaxHoldTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangMotionBlurMinHealTime, float, activeDetonation(), setFlashbangMotionBlurMinHealTime, getFlashbangMotionBlurMinHealTime, kProjectileCompConsoleTag)
BF2_CONSOLE_PROPERTY(detonation.flashbangMotionBlurMaxHealTime, float, activeDetonation(), setFlashbangMotionBlurMaxHealTime, getFlashbangMotionBlurMaxHealTime, kProjectileCompConsoleTag)

} // namespace world
} // namespace hfe
} // namespace dice
