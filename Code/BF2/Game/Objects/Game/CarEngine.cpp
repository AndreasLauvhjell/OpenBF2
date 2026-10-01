// Path:     [INFERRED] - see the header.
// Contents: recovered from bf2.i64 (ELF, symbolised) and bf2_w32ded.exe.i64. The complete geared
// drivetrain tick, its brake/clutch/gear state machine and its sprint-force tail have
// been read in both shipped ports.

#include "CarEngine.h"

#include <cmath>
#include <cstring>
#include <limits>

#include "../../../Core/IBase.h"
#include "../../../Core/Ids.h"
#include "../../../Game/Utility/Vars.h"
#include "../../../Math/LocalBoundingBox.h"
#include "../../../Math/Mat4.h"
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

extern IBase* heightmapCluster;                              // ds:0x8ABAF24 - Core/Globals.cpp

namespace {
    // The static gear table flt_8989EA0 (EngineBase.cpp has the same six words) - the non-car
    // branch of getGearRatio reads it 1..5.
    const float g_gearRatios[6] = { 0.0f, 4.0f, 2.0f, 1.5f, 1.25f, 1.0f };

    // CarEngine's image-order statics. The debug handle is registered but never read by this TU.
    float* const g_engineInertia  = hfe::Vars::getFloat("engine-inertia", 0.05f);
    int*   const g_debugEngine    = hfe::Vars::getInt("debug-engine", 0);
    float* const g_engineInertia3 = hfe::Vars::getFloat("engine-inertia3", 0.96f);
    float* const g_engineInertia2 = hfe::Vars::getFloat("engine-inertia2", 0.99f);
    const float g_upshiftSlope = 0.174f;                    // ELF ds:0x8AB07B8, exact bits 3E322D0E

    inline bool signedLowByteIsNegative(int value)
    {
        // Retail uses `test al, al`: c_ETNewCar2 is stored as the positive int 0xC2, but the
        // drivetrain deliberately interprets the sign of its low byte.
        return (static_cast<unsigned int>(value) & 0x80u) != 0;
    }

    inline float clampFloat(float value, float low, float high)
    {
        // [PLATFORM] The shipped PE helper checks the upper edge first, then uses an
        // unordered-inclusive lower test. In particular, NaN maps to the low bound. The ELF
        // compiler inlines these clamps with different unordered behavior at several call sites.
        if (value >= high)
            value = high;
        if (!(value >= low))
            value = low;
        return value;
    }

    inline float dot3(const Vec3& a, const Vec3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline Vec3 scaled(const Vec3& value, float factor)
    {
        Vec3 result = { value.x * factor, value.y * factor, value.z * factor };
        return result;
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

// EngineBase(template); the vptrs; the two own words zeroed.
CarEngine::CarEngine(const EngineTemplate* objectTemplate)
    : EngineBase(objectTemplate)
    , m_gearChange(0), m_gearChangeRatio(0.0f)
{
}

CarEngine::~CarEngine() {}                                                           // ~EngineBase

void CarEngine::reset()
{
    m_gearChange = 0;
    m_gearChangeRatio = 0.0f;
    EngineBase::reset();
}

// , 277 B. Decay an RPM value only while the requested drive input
// is drawing it back toward zero. Braking has its own immediate 0.9 loss; the old-car model also
// blends the pending gear ratio during clutch travel.
void CarEngine::revDownEngine(float targetInput, float dt)
{
    if (!((m_input > targetInput && m_input > 0.0f)
       || (targetInput > m_input && m_input < 0.0f)))
        return;

    float inertia = *g_engineInertia2;
    if (m_byte1DC == true)
        m_input *= 0.9f;

    if (!signedLowByteIsNegative(m_template->getEngineType())
        && !(std::fabs(m_gearChangeRatio - 1.0f)
             <= std::numeric_limits<float>::epsilon()))
    {
        m_input *= std::pow(m_gearChangeRatio, dt / m_template->m_gearChangeTime);
    }

    if (std::fabs(m_rotationInput.z) < 0.05f)
        inertia = *g_engineInertia3;
    m_input *= inertia;
}

// , 117 B. Both values are classified into reverse, neutral or
// forward. Opposing directions and the neutral/neutral case engage the brake latch.
void CarEngine::applyBrakes(float throttle, float threshold)
{
    const int requested = throttle < -threshold ? -1 : (throttle > threshold ? 1 : 0);
    const int current = m_input < -threshold ? -1 : (m_input > threshold ? 1 : 0);
    m_byte1DC = requested + current == 0;
}

// handleUpdate - ELF 0x829F620, 4,007 B / PE 0x62BAF0, 3,191 B. The complete geared-vehicle
// drivetrain: rotational input, wheel-RPM feedback, old/new car integration, braking, clutch and
// automatic gears, heat/sound/light state, wake policy, and the amphibious sprint-force tail.
// Both ports use signed LOW-byte engine-type tests for the newCar2 model; 0xC2 is not a negative
// int, so spelling those tests as ordinary signed-int comparisons changes the model completely.
void CarEngine::handleUpdate(float dt, unsigned int flags)
{
    const bool skipSound = (flags & 4u) != 0;
    if (!skipSound && m_float1F4 > 0.0f)
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

    if (!m_byte208)
    {
        if ((m_template->getEngineType() & 0x20) != 0 && m_engineRunning)
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
                const float defaultAngle = m_template->getDefaultAngleOfAttack();
                const float maxAngle = m_template->getMaxAngleOfAttack();
                wantedAngle = mapInputToWantedAngle(
                    direction * m_rotationInput.z, maxAngle, defaultAngle);
                attackSpeed = m_template->m_attackSpeed;
            }
            if (attackSpeed > 0.0f)
                wantedAngle = mapWantedAngleToAngle(wantedAngle, dt, m_currentAngle, attackSpeed);
            m_currentAngle = wantedAngle;
            // The shipped body reads acceleration.z again here rather than retaining direction.
            m_rotationInput.z = m_template->m_acceleration.z > 0.0f ? 1.0f : -1.0f;
        }

        // Retail calls getEngineType again here rather than retaining the value above.
        if ((m_template->getEngineType() & 0x10) != 0)
            m_rotationInput.z = 1.0f;
    }

    if (!getParent())
        return;

    if (m_byte208)
    {
        m_rotationInput.x = 0.0f;
        m_rotationInput.y = 0.0f;
        // EngineNetworkable stores +/-1.0f and +/-0.0f as raw ieee bits in this int field.
        std::memcpy(&m_rotationInput.z, &m_dword204, sizeof(m_rotationInput.z));
    }

    const float requestedInput = m_rotationInput.z;
    m_rotationInput.z *= m_soundInput;

    if (m_template->m_maxSpeed.x != 0.0f) calculateAndClipAngle(m_template, 0, dt);
    if (m_template->m_maxSpeed.y != 0.0f) calculateAndClipAngle(m_template, 1, dt);
    if (m_template->m_maxSpeed.z != 0.0f) calculateAndClipAngle(m_template, 2, dt);

    const float nextRotationalInput = m_rotationInput.z;
    float xRotationRatio = 0.0f;
    if (m_template->m_maxRotation.x > 0.0f)
        xRotationRatio = m_currentRotation.x / m_template->m_maxRotation.x;
    m_rotationInput.z = requestedInput;

    applyBrakes(requestedInput,
        signedLowByteIsNegative(m_template->getEngineType()) ? 0.04f : 0.1f);

    float nextInput = nextRotationalInput;
    if (!m_byte208)
    {
        if ((m_template->getEngineType() & 0x20) != 0 && m_template->m_constantRpm)
        {
            nextInput = 1.0f;
        }
        else
        {
            const float averageRpm = m_dword1F0 > 0
                ? m_dword1EC / static_cast<float>(m_dword1F0) : 0.0f;
            m_dword1EC = 0.0f;
            m_dword1F0 = 0;

            // Retail calls getEngineType once more after consuming/resetting wheel feedback.
            if (signedLowByteIsNegative(m_template->getEngineType()))
            {
                revDownEngine(nextRotationalInput, dt);

                if (std::fabs(m_input) < 0.05f)
                {
                    if (m_gear == 1 && requestedInput < -0.5f)
                        m_gearChange = -1;
                    else if (m_gear == 0 && requestedInput > -0.05f)
                        m_gearChange = 1;
                }

                float wheelInput = (std::fabs(averageRpm) - m_template->m_newCar2MinRpm)
                                 / (m_template->m_newCar2MaxRpm
                                    - m_template->m_newCar2MinRpm);
                if (!(wheelInput > 0.0f))
                    wheelInput = 0.0f;                              // shipped: no upper clamp
                nextInput = wheelInput * m_soundInput
                          + std::fabs(m_input) * (1.0f - m_soundInput);
                if (m_gear == 0)
                    nextInput = -nextInput;
            }
            else
            {
                revDownEngine(nextRotationalInput, dt);

                // Keep the source-level expression and all four ratio calls. [PLATFORM] GCC/ELF
                // evaluates torque before the current-gear ratios; MSVC/PE evaluates those two
                // ratios first. Both shipped images repeat the calls rather than caching them.
                const float factor = getCurrentTorque() * *g_engineInertia
                                   * getGearRatio(m_gear) * getGearRatio(m_gear)
                                   / (getGearRatio(1) * getGearRatio(1)) / 600.0f;
                nextInput = clampFloat(
                    m_input + (nextRotationalInput - averageRpm) * factor, -1.0f, 1.0f);
            }
        }
    }

    if (!m_engineRunning)
    {
        if (m_windAffector)
            windAffectorSetActive(m_windAffector, false);
        nextInput = 0.0f;
        if (!m_byte208)
        {
            m_input = 0.0f;
            m_currentAngle = 0.0f;
        }
    }

    nextInput = clampFloat(nextInput, -1.0f, 1.2f);
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

    if (!m_byte208)
    {
        m_input = nextInput;
        m_float1D4 = xRotationRatio; // [PLATFORM] ELF schedules this store before m_input; PE after
    }

    // The compiler retains this pre-adjustment value through the clutch update and pending-change
    // application. A newly queued automatic change below is therefore not applied until next tick.
    const int pendingChange = m_gearChange;
    if (m_soundInput < 1.0f || pendingChange != 0 || m_byte1DC == true)
    {
        const float clutchStep = 2.0f * dt / m_template->m_gearChangeTime;
        if (pendingChange != 0 || m_byte1DC == true)
        {
            m_soundInput -= clutchStep;
            if (m_soundInput < 0.0f)
                m_soundInput = 0.0f;
        }
        else
        {
            m_soundInput += clutchStep;
            if (!(m_soundInput < 1.0f))
                m_soundInput = 1.0f;
            if (m_soundInput > 0.95f)
                m_gearChangeRatio = 1.0f;
        }
    }

    if (!m_byte208 && m_byte1DC == true
        && m_soundInput < 0.05f && m_input < 0.1f)
        m_gear = 1;

    if (pendingChange != 0 && m_soundInput <= 0.05f)
    {
        m_gear += pendingChange;
        m_gearChange = 0;
    }

    bool skipDownshift = false;
    if (m_input > m_template->m_gearUp
        && m_gear < m_template->m_numberOfGears
        && m_soundInput > 0.95f)
    {
        // Once this arm is entered, retail never considers a downshift during the same tick,
        // including when slope/velocity suppress the actual upshift request.
        skipDownshift = true;
        IPhysicsNode* const rootPhysicsNode = static_cast<IPhysicsNode*>(
            object_detail::LayoutAccess::physicsNode(getRootParent(this)));
        const Mat4& transform = getAbsoluteTransformation();
        const Vec3 forward = { transform.m[8], transform.m[9], transform.m[10] };
        if (forward.y < g_upshiftSlope
            || dot3(rootPhysicsNode->getPositionalSpeed(), forward) <= 0.0f)
        {
            m_gearChange = 1;
            // [PLATFORM] ELF evaluates target first; PE evaluates current first.
            m_gearChangeRatio = getGearRatio(m_gear + 1) / getGearRatio(m_gear);
        }
    }

    if (!skipDownshift)
    {
        // [PLATFORM] The same pre-C++17 operand-order split exists here and in both assignments.
        const float downProbe = getGearRatio(m_gear - 1) * m_input / getGearRatio(m_gear);
        if (downProbe < 0.55f && m_gear > 1 && m_soundInput > 0.95f)
        {
            m_gearChange = -1;
            m_gearChangeRatio = getGearRatio(m_gear - 1) / getGearRatio(m_gear);
        }
    }

    if (!skipSound)
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

    if (std::fabs(m_rotationInput.z) > 0.001f || std::fabs(m_input) >= 0.05f)
    {
        if (physicsNode->getIsSleeping() == true)
            physicsNode->setIsAwake();
    }

    IPlayerControlObject* const pco = findParentPlayerControlObject(this);
    if (!pco || !pco->getIsSprinting())
        return;

    IHeightmapCluster* const heights = static_cast<IHeightmapCluster*>(heightmapCluster);
    // Retail invokes getAbsolutePosition independently for x and z in each query.
    const float waterZ = getAbsolutePosition().z;
    const float waterX = getAbsolutePosition().x;
    const float water = heights->getWaterLevelInWorldCoords(waterX, waterZ);
    const float terrainZ = getAbsolutePosition().z;
    const float terrainX = getAbsolutePosition().x;
    const float terrain = heights->getHeightInWorldCoords(terrainX, terrainZ);
    IObject* const sprintRoot = getRootParent(this);
    IPhysicsNode* const sprintNode = static_cast<IPhysicsNode*>(
        object_detail::LayoutAccess::physicsNode(sprintRoot));
    if (!sprintNode || !(terrain > water))
        return;

    const Mat4& transform = getAbsoluteTransformation();
    Vec3 sprintForce = { transform.m[8], transform.m[9], transform.m[10] };
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
    sprintForce = scaled(sprintForce, sprintScale);
    sprintForce = scaled(sprintForce, pco->getSprintFactor());

    if (m_gear == m_template->m_numberOfGears)
        sprintForce = scaled(sprintForce, 1.0f - clampFloat(m_input, 0.0f, 1.0f));

    float inAirModifier = 1.0f;
    ICollisionMesh* const collisionMesh = static_cast<ICollisionMesh*>(
        object_detail::LayoutAccess::collisionMesh(getRootParent(this)));
    if (collisionMesh && collisionMesh->hasLod(1))
    {
        const LocalBoundingBox* const box = collisionMesh->getBoundingBox(1);
        const Vec3& rootPosition = getRootParent(this)->getAbsolutePosition();
        if (box->m_min.y + rootPosition.y - 1.0f > terrain)
            inAirModifier = m_template->m_dampenSprintRotationalForceInAirMod;
    }

    sprintNode->addPositionalAccelerationToRootParent(sprintForce);
    sprintNode->addRotationalAccelerationAtAbsolutePosition(
        getAbsolutePosition(), scaled(sprintForce, inAirModifier));
}

int CarEngine::getGearChange() const   { return m_gearChange; }

// the change is stored and the ratio between the target gear and the current one
// remembered (both through the getGearRatio slot).
void CarEngine::setGearChange(int change)
{
    m_gearChange = change;
    // [PLATFORM] ELF evaluates the target first; PE evaluates the current gear first.
    m_gearChangeRatio = getGearRatio(m_gear + change) / getGearRatio(m_gear);         // vslot +0x1A4
}

// , 113 B: a non-negative signed LOW byte uses the static table;
// bit 0x80 selects the template curve. Gear 0 is the first gear's ratio times 1.2.
float CarEngine::getGearRatio(int gear) const
{
    if (!signedLowByteIsNegative(m_template->getEngineType()))                       // vslot +0x144
    {
        int i = 1;
        if (gear > 0)
            i = gear >= 6 ? 5 : gear;
        return g_gearRatios[i];
    }
    if (gear != 0)
        return m_template->getGearRatio(gear);
    return m_template->getGearRatio(1) * 1.2f;
}

// , 189 B: a non-negative signed LOW byte answers the template's
// torque word; bit 0x80 scales the curve by throttle (and optionally by clutch/sound input).
float CarEngine::getCurrentTorque() const
{
    if (!signedLowByteIsNegative(m_template->getEngineType()))
        return m_template->m_torque;                                                 // +0x3D0
    // The rotational input's Z component supplies engine throttle.
    float throttle = m_rotationInput.z;                                                     // +0x160
    if (m_gear <= 0)
    {
        if (throttle > 0.0f) throttle = 0.0f;
    }
    else
    {
        if (!(throttle > 0.0f)) throttle = 0.0f;
    }
    float factor = throttle;
    if (static_cast<float>(m_template->m_newCar2UseClutchedTorque) != 0.0f)                           // +0x628
        factor = throttle * m_soundInput;                                            // +0x1E4
    return m_template->getTorque(m_input) * factor;
}

} // namespace world
} // namespace hfe
} // namespace dice
