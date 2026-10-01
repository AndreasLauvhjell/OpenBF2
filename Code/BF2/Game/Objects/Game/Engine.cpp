// Path:     [INFERRED] - see the header.
// Contents: recovered from bf2.i64 (ELF, symbolised) and bf2_w32ded.exe.i64. The generic engine
// update, force application, feedback, gear, heat, tracked-steering and sleep/wake paths
// have been read in both shipped ports.

#include "Engine.h"

#include <algorithm>
#include <cmath>

#include "../../../Core/Ids.h"
#include "../../../Core/IBase.h"
#include "../../../Game/Utility/Vars.h"                      // Vars::getFloat("phy-engine-slope")
#include "../../../Math/LocalBoundingBox.h"
#include "../../../Math/Mat4.h"
#include "../../../Physics/BasicPhysicsSystem.h"             // basicPhysicsSystem->getAirDensityZeroAtHeight (+0x30)
#include "../../../Physics/HeightmapCluster.h"
#include "../../../Physics/ICollisionMesh.h"
#include "../../../Physics/IPhysicsNode.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../../Scene/Object/ObjectUtil.h"
#include "../EngineTemplate.h"
#include "../IObjectComponent.h"
#include "../IWindAffector.h"
#include "../ObjectLayoutAccess.h"
#include "../Visual/LightSource.h"
#include "../Visual/LightSourceTemplate.h"

namespace dice {
namespace hfe {
namespace world {

extern IBase* basicPhysicsSystem;                            // ds:0x8B2784C - Core/Globals.cpp
extern IBase* heightmapCluster;                              // ds:0x8ABAF24 - Core/Globals.cpp

namespace {
    // in image order. The debug/show pair and brake-only values are retained by the
    // static initialiser but are not read by this class's recovered bodies.
    int*   const g_debugEngine          = hfe::Vars::getInt("debug-engine", 0);
    int*   const g_showEngine           = hfe::Vars::getInt("show-engine", 0);
    float* const g_forceRegLimit        = hfe::Vars::getFloat("force-reg-limit", 20.0f);
    float* const g_phyGas               = hfe::Vars::getFloat("phy-gas", 1.0f);
    float* const g_phyTurn              = hfe::Vars::getFloat("phy-turn", 0.1f);
    float* const g_feedbackModifier     = hfe::Vars::getFloat("fb-mod", 1.0f);
    float* const g_phyBrakeMod          = hfe::Vars::getFloat("phy-break-mod", 0.95f);
    float* const g_phyHandBrakeRpm      = hfe::Vars::getFloat("phy-hand-break-rpm", 0.1f);
    float* const g_phyHandBrakeAdjust   = hfe::Vars::getFloat("phy-hand-break-adjust", 0.05f);
    int*   const g_phyHandBrakeRelease  = hfe::Vars::getInt("phy-hand-break-release", 10);
    float* const g_phyEngineSlope       = hfe::Vars::getFloat("phy-engine-slope", 1.0f);
    float* const g_phyEngineInertia     = hfe::Vars::getFloat("phy-engine-inertia", 0.05f);
    float* const g_phyTankEngineInertia = hfe::Vars::getFloat("phy-tank-engine-inertia", 0.07f);

    inline float clampFloat(float value, float low, float high)
    {
        return value < low ? low : (value > high ? high : value);
    }

    inline float length3(const Vec3& value)
    {
        return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    }

    inline float dot3(const Vec3& a, const Vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline Vec3 scaled(const Vec3& value, float factor)
    {
        Vec3 out = { value.x * factor, value.y * factor, value.z * factor };
        return out;
    }

    IPlayerControlObject* findParentPlayerControlObject(IObject* object)
    {
        while (object)
        {
            if (void* const pco = object->queryInterface(IID_IPlayerControlObject))
                return static_cast<IPlayerControlObject*>(pco);
            object = object->getParent();
        }
        return 0;
    }

    inline void windAffectorSetActive(IObjectComponent* windAffector, bool active)
    {
        static_cast<IWindAffector*>(windAffector)->setIsActive(active);
    }
}

// EngineBase(template); the vptrs; the four own words zeroed.
Engine::Engine(const EngineTemplate* objectTemplate)
    : EngineBase(objectTemplate)
    , m_dword210(0.0f), m_dword214(0), m_trackContact(0), m_slope(0.0f)
{
}

Engine::~Engine() {}                                                                 // ~EngineBase

// the own words, then EngineBase::reset.
void Engine::reset()
{
    m_dword214 = 0;
    m_dword210 = 0;
    m_trackContact = 0;
    m_slope = 0.0f;
    EngineBase::reset();
}

// handleUpdate - ELF 0x82A0A70, 2,720 B / PE 0x62A420, 2,011 B. The generic engine tick: update
// its authored rotational axes, turn wheel feedback into throttle/RPM, apply tracked steering,
// heat and gear changes, drive the optional engine light, then run updatePhysics and the sleep/
// wake policy. Both shipped ports were read; typed virtual calls replace their ABI-specific slots.
void Engine::handleUpdate(float dt, unsigned int flags)
{
    if ((flags & 4u) == 0 && m_float1F4 > 0.0f)
    {
        m_float1F4 -= dt;
        if (m_float1F4 <= 0.0f)
        {
            m_float1F4 = -1.0f;
            updateFlags(0u, 0x800u);
            return;
        }
    }

    IObject* const root = getRootParent(this);                                  // shipped: unguarded
    IPhysicsNode* const physicsNode = static_cast<IPhysicsNode*>(
        object_detail::LayoutAccess::physicsNode(root));

    float xRotationRatio = 0.0f;
    const int engineType = m_template->getEngineType();
    if ((engineType & 0x20) != 0 && m_engineRunning)
    {
        const float mappedInput = m_template->m_remoteEngineInput == io::PINone
                                ? m_rotationInput.z : m_rotationInput.x;
        const bool decrease = m_template->m_decreaseAngleToZero
                           && mappedInput < 0.07f
                           && std::fabs(physicsNode->getPositionalSpeed().y)
                                < m_template->m_decreaseAngleToZeroVerticalVel;
        const float direction = m_template->m_acceleration.z > 0.0f ? 1.0f : -1.0f;
        float wantedAngle;
        float attackSpeed;
        if (decrease)
        {
            wantedAngle = 0.0f;
            attackSpeed = m_template->m_decreaseAngleToZeroSpeed;
        }
        else
        {
            wantedAngle = mapInputToWantedAngle(direction * m_rotationInput.z,
                m_template->getMaxAngleOfAttack(), m_template->getDefaultAngleOfAttack());
            attackSpeed = m_template->m_attackSpeed;
        }
        if (attackSpeed > 0.0f)
            wantedAngle = mapWantedAngleToAngle(wantedAngle, dt, m_currentAngle, attackSpeed);
        m_currentAngle = wantedAngle;
        m_rotationInput.z = m_template->m_acceleration.z > 0.0f ? 1.0f : -1.0f;
    }

    if ((engineType & 0x10) != 0)
        m_rotationInput.z = 1.0f;

    if (!getParent())
        return;

    if (m_template->m_maxSpeed.x != 0.0f) calculateAndClipAngle(m_template, 0, dt);
    if (m_template->m_maxSpeed.y != 0.0f) calculateAndClipAngle(m_template, 1, dt);
    if (m_template->m_maxSpeed.z != 0.0f) calculateAndClipAngle(m_template, 2, dt);

    const float turnRatio = m_currentRotation.z / m_template->m_maxRotation.z;
    // ELF reads template +0x188 (m_maxRotation.x) in both the guard and divisor; PE reads its
    // MSVC-layout counterpart at +0x238. Using m_minRotation.x here made the authored positive
    // tank yaw range (`setMaxRotation 1/0/1`) normalize to zero and disabled both differential
    // track RPM and the direct tracked-turn acceleration below.
    if (m_template->m_maxRotation.x > 0.0f)
        xRotationRatio = m_currentRotation.x / m_template->m_maxRotation.x;

    if ((flags & 4u) == 0 && m_soundInput > 0.0f)
    {
        m_soundInput -= dt / m_template->m_gearChangeTime;
        if (m_soundInput < 0.0f)
            m_soundInput = 0.0f;
    }

    m_byte1DC = (m_rotationInput.z < -0.1f && m_input > 0.0f)
             || (m_rotationInput.z >  0.1f && m_input < 0.0f);

    const bool tracked = (engineType & 6) == 6;
    float feedback = m_float1C8;
    float nextInput;
    if ((engineType & 0x20) != 0 && m_template->m_constantRpm)
    {
        nextInput = 1.0f;
    }
    else
    {
        float inertia;
        if (tracked)
        {
            feedback *= *g_feedbackModifier;
            const float speed = length3(physicsNode->getPositionalSpeed());
            static float c1 = 0.75f * m_template->m_differential;
            feedback *= clampFloat(1.0f - speed / c1, 0.0f, 1.0f);
            inertia = *g_phyTankEngineInertia;
        }
        else
        {
            inertia = *g_phyEngineInertia;
        }
        nextInput = m_input + inertia * (turnRatio - feedback - m_input * 0.5f);
    }

    if (tracked && std::fabs(nextInput) > 0.01f && (m_trackContact & 3u) == 3u)
    {
        const int contactCount = m_dword214 < 1 ? 1 : m_dword214;
        const float meanFriction = m_dword210 / static_cast<float>(contactCount);
        if (meanFriction * m_template->m_trackTurnSpeed
            > std::fabs(physicsNode->getRotationalSpeed().y))
        {
            float turnAcceleration = meanFriction * m_template->m_trackTurnAcceleration
                                   * xRotationRatio;
            if (nextInput < 0.0f)
                turnAcceleration = -turnAcceleration;
            Vec3 acceleration = physicsNode->getRotationalAcceleration();
            acceleration.y += turnAcceleration;
            physicsNode->setRotationalAcceleration(acceleration);
        }
    }
    resetTrackFriction();

    if (!m_engineRunning)
    {
        if (m_windAffector)
            windAffectorSetActive(m_windAffector, false);
        nextInput = 0.0f;
        m_input = 0.0f;
        m_currentAngle = 0.0f;
    }

    // Retail takes the upper bound on an unordered comparison (ELF 0x82A0DD8..0x82A0DFC,
    // ..0x62A9BE). Stock transport-helicopter helper engines have a zero
    // maxRotation.z, so turnRatio can be 0/0 on entry. A conventional lower/upper
    // clamp preserves that NaN and poisons thrust and the root vehicle's position.
    nextInput = nextInput < 1.2f ? nextInput : 1.2f;
    if (nextInput < -1.0f)
        nextInput = -1.0f;
    if (m_template->m_heatIncrement > 0.0f)
    {
        m_heat += m_template->m_heatIncrement * std::fabs(nextInput);
        m_heat -= m_template->m_coolingFactor;
        if (m_heat >= 1.0f)
        {
            m_engineRunning = false;
            nextInput = 0.0f;
            m_heat = 1.0f;
        }
        else if (m_heat < 0.0f)
        {
            m_heat = 0.0f;
        }
    }
    if (std::fabs(nextInput) < 0.01f)
        nextInput = 0.0f;
    m_input = nextInput;
    m_float1D4 = xRotationRatio;

    if (m_input > m_template->m_gearUp && m_soundInput == 0.0f
        && m_gear < m_template->m_numberOfGears)
        ++m_gear;
    if (m_input < m_template->m_gearDown && m_gear > 1)
        --m_gear;

    if ((flags & 4u) == 0)
        updateEngineSound();

    if (m_lightSource)
    {
        const LightSourceTemplate* const lightTemplate =
            static_cast<const LightSourceTemplate*>(m_lightSource->getTemplate());
        if (lightTemplate->getAffectingType() == 2)
            static_cast<LightSource*>(m_lightSource)->setAttenuationIntensity(
                std::fabs(m_soundRpm) * 0.9f + 0.1f);
    }

    m_float1CC = m_float1C8;
    m_float1C8 = 0.0f;
    m_float1D0 = 0.0f;
    updatePhysics(dt);

    if (physicsNode->getIsSleeping())
    {
        bool clearUpdate = true;
        IObject* const rootObject = getRootParent(this);
        if (rootObject
            && rootObject->getTemplate()->getClassID() == CID_PlayerControlObjectTemplate)
        {
            IPlayerControlObject* const pco = static_cast<IPlayerControlObject*>(
                rootObject->queryInterface(IID_IPlayerControlObject));
            clearUpdate = !pco->isOccupied();
        }
        if (m_float1F4 > 0.0f)
            clearUpdate = false;
        if (clearUpdate)
            updateFlags(0u, 0x800u);
    }

    if (std::fabs(m_input) >= 0.01f)
    {
        if (physicsNode->getIsSleeping())
            physicsNode->setIsAwake();
        if (m_float1F4 > 0.0f && m_float1F4 < 1.0f)
            m_float1F4 = 1.0f;
    }
}

// updatePhysics - ELF 0x82A17E0, 3,704 B / PE 0x62AE10, 2,538 B. Applies the generic engine's
// propeller/jet force and the sprint boost used by engine types 2 and 9. The typed calls below
// preserve the shipped virtual operations while avoiding the different ELF/MSVC vtable offsets.
void Engine::updatePhysics(float dt)
{
    (void)dt;                                                                  // shipped parameter is unused

    IObject* const root = getRootParent(this);
    if ((object_detail::LayoutAccess::flags(root) & 1u) != 0)
        return;

    IPhysicsNode* const physicsNode = static_cast<IPhysicsNode*>(
        object_detail::LayoutAccess::physicsNode(root));
    if (!physicsNode)
        return;

    const int engineType = m_template->getEngineType();
    if ((engineType & 1) != 0)
    {
        const Vec3 enginePosition = getAbsolutePosition();
        const float waterLevel = static_cast<IHeightmapCluster*>(heightmapCluster)
            ->getWaterLevelInWorldCoords(enginePosition.x, enginePosition.z);

        bool skipPropulsion = false;
        if (waterLevel <= enginePosition.y)
        {
            if ((engineType & 8) != 0 && std::fabs(m_input) > 0.02f)
            {
                m_input = 1.0f;
                skipPropulsion = true;
            }
        }
        else if ((engineType & 8) == 0)
        {
            m_input = 0.0f;
            skipPropulsion = true;
        }

        if (!skipPropulsion)
        {
            if ((engineType & 0x20) != 0)
            {
                const Mat4& engineTransform = getAbsoluteTransformation();
                Vec3 forward = {
                    engineTransform.m[8], engineTransform.m[9], engineTransform.m[10]
                };
                Vec3 airFlow = physicsNode->getTangentSpeed(enginePosition);
                if (m_template->getPureRotational())
                {
                    const Vec3& positionalSpeed = physicsNode->getPositionalSpeed();
                    const float retainedAirFlow = 1.0f - m_template->m_airFlowAffect;
                    airFlow.x -= positionalSpeed.x * retainedAirFlow;
                    airFlow.y -= positionalSpeed.y * retainedAirFlow;
                    airFlow.z -= positionalSpeed.z * retainedAirFlow;
                }

                float forceScalar = -dot3(forward, airFlow)
                                  * (std::fabs(m_currentAngle) + 1.0f)
                                  / m_template->m_noPropellerEffectAtSpeed;
                forceScalar += forceIntoEngine(0.0f) * m_currentAngle;
                forward.y *= getAirDensity(enginePosition.y);

                const Vec3& positionalSpeed = physicsNode->getPositionalSpeed();
                Vec3 horizontalSpeed = { positionalSpeed.x, 0.0f, positionalSpeed.z };
                const float perpendicularRatio = clampFloat(
                    length3(horizontalSpeed) / m_template->m_noEffectAtPerpSpeed, 0.0f, 1.0f);
                forceScalar *= 1.0f - perpendicularRatio;

                if (m_input < 0.00000011920929f)
                {
                    forceScalar *= 0.05f;
                    const Mat4& nodeTransform = physicsNode->getAbsoluteTransformation();
                    const Vec3 sideways = {
                        nodeTransform.m[4], nodeTransform.m[5], nodeTransform.m[6]
                    };
                    const float sidewaysSpeed = dot3(physicsNode->getPositionalSpeed(), sideways);
                    if (m_template->m_spinWhenNoEngineInAir > 0.00000011920929f
                        && std::fabs(sidewaysSpeed) > 3.0f)
                        forceScalar += m_template->m_spinWhenNoEngineInAir * sidewaysSpeed;
                }

                if (m_template->getPureRotational())
                {
                    Vec3 force = scaled(forward, forceScalar * 0.5f);
                    physicsNode->addAccelerationAtAbsolutePosition(enginePosition, force);
                    force.x = -force.x;
                    force.y = -force.y;
                    force.z = -force.z;
                    const Vec3& centre = physicsNode->getAbsolutePosition();
                    const Vec3 oppositePosition = {
                        centre.x * 2.0f - enginePosition.x,
                        centre.y * 2.0f - enginePosition.y,
                        centre.z * 2.0f - enginePosition.z
                    };
                    physicsNode->addAccelerationAtAbsolutePosition(oppositePosition, force);
                }
                else
                {
                    if (forceScalar < *g_forceRegLimit && m_template->m_horizontalDampAngle > 0.0f)
                    {
                        const Vec3& speed3 = physicsNode->getPositionalSpeed();
                        Vec3 horizontal = { speed3.x, 0.0f, speed3.z };
                        const float speed = length3(horizontal);
                        if (speed < m_template->m_dampHorizontalVel)
                        {
                            if (speed > 0.01f)
                            {
                                const float inverseSpeed = 1.0f / speed;
                                Vec3 direction = scaled(horizontal, inverseSpeed);
                                float damping = m_template->m_dampHorizontalVelFactor * speed;
                                const float halfLimit = m_template->m_dampHorizontalVel * 0.5f;
                                if (speed > halfLimit)
                                    damping -= (speed - halfLimit)
                                             * (2.0f * m_template->m_dampHorizontalVelFactor);
                                physicsNode->addAccelerationAtAbsolutePosition(
                                    enginePosition, scaled(direction, damping));
                            }

                            const float angleLimit = 0.017453292f
                                                   * m_template->m_horizontalDampAngle;
                            const float up = clampFloat(
                                physicsNode->getAbsoluteTransformation().m[5], 0.0f, 1.0f);
                            const float angle = std::acos(up);
                            if (angle < angleLimit)
                            {
                                const float phase = std::fmod(
                                    angle * (1.5707964f / angleLimit
                                           / m_template->m_horizontalDampAngleFactor),
                                    6.2831855f);
                                const float horizontalFactor = std::sin(phase);
                                forward.x *= horizontalFactor;
                                forward.z *= horizontalFactor;
                            }
                        }
                    }

                    physicsNode->addAccelerationAtAbsolutePosition(
                        enginePosition, scaled(forward, forceScalar));
                    if (m_input > 0.00000011920929f)
                    {
                        const float extra = (m_template->getHorizontalSpeedMagnifier() - 1.0f)
                                          * forceScalar;
                        const Vec3 horizontalForce = {
                            forward.x * extra, 0.0f, forward.z * extra
                        };
                        physicsNode->addAccelerationAtRelativePosition(
                            Vec3::getZero(), horizontalForce);
                    }
                }
            }
            else
            {
                const Mat4& engineTransform = getAbsoluteTransformation();
                Vec3 forward = {
                    engineTransform.m[8], engineTransform.m[9], engineTransform.m[10]
                };
                const float density = getAirDensity(enginePosition.y);
                const float relativeInput = m_input
                    - density * dot3(physicsNode->getPositionalSpeed(), forward)
                    / m_template->m_noPropellerEffectAtSpeed;
                const float scalar = std::fabs(m_input) * 0.1f
                                   + std::fabs(relativeInput) * relativeInput;
                feedbackLoop(scaled(forward, scalar), forward);

                const Mat4& updatedTransform = getAbsoluteTransformation();
                forward.x = updatedTransform.m[8];
                forward.y = updatedTransform.m[9];
                forward.z = updatedTransform.m[10];
                Vec3 force = scaled(forward, getCurrentRatio() * scalar);
                IPlayerControlObject* const pco = findParentPlayerControlObject(this);
                if (pco && pco->getIsSprinting())
                    force = scaled(force, pco->getSprintFactor());
                physicsNode->addAccelerationAtAbsolutePosition(enginePosition, force);
            }
        }
    }

    if (engineType == 2 || engineType == 9)
    {
        IPlayerControlObject* const pco = findParentPlayerControlObject(this);
        if (pco && pco->getIsSprinting())
        {
            IPhysicsNode* const rootPhysicsNode = static_cast<IPhysicsNode*>(
                object_detail::LayoutAccess::physicsNode(root));
            if (rootPhysicsNode)
            {
                const Mat4& engineTransform = getAbsoluteTransformation();
                Vec3 sprintForce = {
                    engineTransform.m[8], engineTransform.m[9], engineTransform.m[10]
                };
                float gearDampen = 0.0f;
                switch (m_gear)
                {
                    case 1: gearDampen = m_template->m_sprintGear1Dampen; break;
                    case 2: gearDampen = m_template->m_sprintGear2Dampen; break;
                    case 3: gearDampen = m_template->m_sprintGear3Dampen; break;
                    case 4: gearDampen = m_template->m_sprintGear4Dampen; break;
                    case 5: gearDampen = m_template->m_sprintGear5Dampen; break;
                    default: break;
                }
                const float sprintScale = gearDampen > 0.0f ? 12.0f / gearDampen : 0.0f;
                sprintForce = scaled(sprintForce, sprintScale * pco->getSprintFactor());
                if (m_gear == m_template->m_numberOfGears)
                    sprintForce = scaled(sprintForce, 1.0f - clampFloat(m_input, 0.0f, 1.0f));

                float inAirModifier = 1.0f;
                ICollisionMesh* const collisionMesh = static_cast<ICollisionMesh*>(
                    object_detail::LayoutAccess::collisionMesh(root));
                if (collisionMesh && collisionMesh->hasLod(1))
                {
                    const LocalBoundingBox* const box = collisionMesh->getBoundingBox(1);
                    const Vec3& rootPosition = root->getAbsolutePosition();
                    const float surface = engineType == 2
                        ? static_cast<IHeightmapCluster*>(heightmapCluster)
                            ->getHeightInWorldCoords(rootPosition.x, rootPosition.z)
                        : static_cast<IHeightmapCluster*>(heightmapCluster)
                            ->getWaterLevelInWorldCoords(rootPosition.x, rootPosition.z);
                    if (box->m_min.y + rootPosition.y - 0.25f > surface)
                        inAirModifier = m_template->m_dampenSprintRotationalForceInAirMod;
                }

                rootPhysicsNode->addPositionalAccelerationToRootParent(sprintForce);
                rootPhysicsNode->addRotationalAccelerationAtAbsolutePosition(
                    getAbsolutePosition(), scaled(sprintForce, inAirModifier));
            }
        }
    }
}

// 1 - clamp(height / airDensityZeroAtHeight, 0, 1); a negative ratio -> 1.
float Engine::getAirDensity(float height) const
{
    const float zeroAt = static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getAirDensityZeroAtHeight();   // +0x30
    const float ratio = height / zeroAt;
    if (ratio < 0.0f)
        return 1.0f;
    return 1.0f - (ratio <= 1.0f ? ratio : 1.0f);
}

// v = |input| * 0.1 + |arg + input| * (arg + input); the absolute transform's
// forward row (m[8..10]) scaled by v goes through feedbackLoop with the row itself; the answer is
// getCurrentRatio() * v.
float Engine::forceIntoEngine(float input)
{
    const Mat4& t = getAbsoluteTransformation();                                     // vslot +0x84
    const float sum = input + m_input;                                               // +0x1C4
    const float v = std::fabs(m_input) * 0.1f + std::fabs(sum) * sum;
    Vec3 forward; forward.x = t.m[8]; forward.y = t.m[9]; forward.z = t.m[10];
    Vec3 force;   force.x = t.m[8] * v; force.y = t.m[9] * v; force.z = t.m[10] * v;
    feedbackLoop(force, forward);
    return getCurrentRatio() * v;                                                    // vslot +0x1BC
}


float Engine::getCurrentRatio() const   { return 3.5f * m_template->m_differential / m_template->getGearRatio(m_gear); }   // +0x3CC +0x1E8
float Engine::getCurrentTorque() const  { return m_template->getTorque(m_input); }

void Engine::debugDraw() {}


void Engine::resetTrackFriction()
{
    m_dword210 = 0;
    m_dword214 = 0;
    m_trackContact = 0;
}

// Feeds force projected along direction back into engine load using torque and gear ratio.
// Engine flags select clamping and extreme-value or running-mean accumulation. force is passed by value.
void Engine::feedbackLoop(Vec3 force, const Vec3& direction)
{
    const float along = direction.y * force.y + direction.x * force.x + direction.z * force.z;
    float ratio = getCurrentRatio() * along / getCurrentTorque();
    const int type = m_template->getEngineType();                                    // +0x144
    if ((type & 2) != 0)
    {
        if (ratio > 1.0f)
        {
            const float k = 1.0f / ratio;
            force.x = force.x * k;
            force.y = force.y * k;
            force.z = k * force.z;
            ratio = 1.0f;
        }
        else if (ratio < -1.0f)
        {
            const float k = -1.0f / ratio;
            force.x = force.x * k;
            force.y = force.y * k;
            force.z = k * force.z;
            ratio = -1.0f;
        }
    }
    if ((m_template->getEngineType() & 4) != 0)
    {
        bool keep;
        if (m_input >= 0.0f)                                                         // +0x1C4
            keep = ratio <= m_float1C8;                                              // +0x1C8
        else
            keep = m_float1C8 <= ratio;
        if (!keep)
            m_float1C8 = ratio;
    }
    else
    {
        const float n = m_float1D0;                                                  // +0x1D0
        float smoothed;
        if (n >= 100.0f)
        {
            smoothed = 0.99000001f * m_float1C8;
        }
        else
        {
            const float next = n + 1.0f;
            m_float1D0 = next;
            smoothed = (m_float1C8 * n + ratio) * 0.99000001f / next;
        }
        m_float1C8 = smoothed;
        if (std::fabs(m_float1C8) < 0.001f)
            m_float1C8 = 0.0f;
    }
    if ((m_template->getEngineType() & 0x20) != 0)
    {
        float clamped;
        if (m_float1C8 < -1.25f)
        {
            clamped = -1.25f;
        }
        else
        {
            clamped = m_float1C8;
            if (clamped > 1.25f)
                clamped = 1.25f;
        }
        m_float1C8 = clamped;
    }
}

void Engine::setTrackHasContact(float side)
{
    if (side >= 0.0f)
    {
        if (side > 0.0f)
            m_trackContact |= 2u;
    }
    else
        m_trackContact |= 1u;
}

// the forward row's y (m[9]) smoothed into m_slope by the phy-engine-slope var.
float Engine::getSlope()
{
    const Mat4& t = getAbsoluteTransformation();                                     // vslot +0x84
    m_slope = m_slope + (t.m[9] - m_slope) * *g_phyEngineSlope;
    return m_slope;
}

} // namespace world
} // namespace hfe
} // namespace dice
