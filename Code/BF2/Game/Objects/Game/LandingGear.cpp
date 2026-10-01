// Path:     [INFERRED] - see LandingGear.h.
// Contents: recovered from the symbolised BF2 1.50 ELF (0x82A86D0..0x82A8C0E) and cross-checked
// against the Win32 dedicated server (0x62F070..0x62F4D1).

#include "LandingGear.h"

#include "../../../Core/Ids.h"
#include "../../../Core/System.h"
#include "../../../Physics/HeightmapCluster.h"
#include "../../../Physics/IPhysicsNode.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../LandingGearTemplate.h"
#include "../ObjectLayoutAccess.h"

namespace dice {
namespace hfe {
namespace world {

extern IBase* heightmapCluster;                                    // Core/Globals.cpp

// attested symbol and initial value: _ZN4dice3hfe5world21g_fullUpdateTickLimitE at ELF 0x898A004
// contains 5 and has LandingGear::handleUpdate as its sole code reference. The source-file home and
// unsigned type are [INFERRED]; the unsigned subtraction/comparison are explicit in the body.
unsigned int g_fullUpdateTickLimit = 5u;

namespace {

// The retail source indexes its Vec3 fields in a three-iteration loop. The recovered minimal Vec3
// has no operator[], so these accessors preserve that loop without widening the shared math API.
float vectorComponent(const Vec3& value, int axis)
{
    if (axis == 0)
        return value.x;
    if (axis == 1)
        return value.y;
    return value.z;
}

float& vectorComponent(Vec3& value, int axis)
{
    if (axis == 0)
        return value.x;
    if (axis == 1)
        return value.y;
    return value.z;
}

} // namespace


LandingGear::LandingGear(const LandingGearTemplate* objectTemplate)
    : RotationalBundle(objectTemplate)
    , m_gearUpDelayRemaining(0.0f)
    , m_gearDownDelayRemaining(0.0f)
    , m_gearUp(false)
    , m_savedRotation(m_currentRotation)
    , m_lastFullUpdateTick(0u)
{
    m_rotatesContinuously = true;
    updateFlags(0u, 0x800u);
}

LandingGear::~LandingGear() {}

// ; PE 0x62F110, 961 B. The two tick reads, strict comparisons, independent
// down/up tests, unclamped delay subtraction, sleep early-return, and unguarded height singleton
// all reproduce the shipped ordering.
void LandingGear::handleUpdate(float deltaTime, unsigned int flags)
{
    const unsigned int fullUpdateCutoff =
        static_cast<unsigned int>(WorldPref::getTickCount()) - g_fullUpdateTickLimit;
    const bool fullUpdate = m_lastFullUpdateTick < fullUpdateCutoff;
    m_lastFullUpdateTick = static_cast<unsigned int>(WorldPref::getTickCount());

    const LandingGearTemplate* objectTemplate =
        static_cast<const LandingGearTemplate*>(getTemplate());
    const Vec3 position = getAbsolutePosition();

    IHeightmapCluster* const cluster = static_cast<IHeightmapCluster*>(heightmapCluster);
    float clearance = position.y - cluster->getHeightInWorldCoords(position.x, position.z);
    const float waterClearance =
        position.y - cluster->getWaterLevelInWorldCoords(position.x, position.z);
    if (clearance > waterClearance)
        clearance = waterClearance;

    float speedSquared = 0.0f;
    IObject* const root = getRootParent(this);                          // shipped: unguarded result
    IPhysicsNode* const rootPhysics = static_cast<IPhysicsNode*>(
        object_detail::LayoutAccess::physicsNode(root));

    if (rootPhysics)
    {
        if (rootPhysics->getIsSleeping())
        {
            IPlayerControlObject* playerControlObject = 0;
            for (IObject* object = root; object; object = object->getParent())
            {
                playerControlObject = static_cast<IPlayerControlObject*>(
                    object->queryInterface(IID_IPlayerControlObject));
                if (playerControlObject)
                    break;
            }
            if (playerControlObject && !playerControlObject->isOccupied())
                updateFlags(0u, 0x800u);
        }

        IPhysicsNode* const ownPhysics = static_cast<IPhysicsNode*>(
            object_detail::LayoutAccess::physicsNode(this));
        if (ownPhysics)
        {
            ownPhysics->setSleepiness(rootPhysics->getSleepiness());
            if (ownPhysics->getIsSleeping())
                return;
        }

        const Vec3& speed = rootPhysics->getPositionalSpeed();
        speedSquared = speed.x * speed.x + speed.y * speed.y + speed.z * speed.z;
    }

    if (clearance < objectTemplate->m_gearDownHeight
        && speedSquared < objectTemplate->m_gearDownSpeed * objectTemplate->m_gearDownSpeed
        && !LandingGearTemplate::m_forceGearUp)
    {
        if (m_gearUp)
        {
            m_gearUp = false;
            m_gearDownDelayRemaining = objectTemplate->m_gearDownDelay;
        }
    }

    if ((clearance > objectTemplate->m_gearUpHeight
         && speedSquared > objectTemplate->m_gearUpSpeed * objectTemplate->m_gearUpSpeed)
        || LandingGearTemplate::m_forceGearUp)
    {
        if (!m_gearUp)
        {
            m_gearUp = true;
            m_gearUpDelayRemaining = objectTemplate->m_gearUpDelay;
        }
    }

    if (m_gearUp)
    {
        // Retail compares 0 against the timer and takes this arm for positive OR unordered values.
        // The negated <= form preserves that NaN quirk as well as ordinary finite behavior.
        if (!(m_gearUpDelayRemaining <= 0.0f))
        {
            m_gearUpDelayRemaining -= deltaTime;
        }
        else
        {
            const float input = objectTemplate->m_invertGear ? -1.0f : 1.0f;
            m_rotationInput.x = input;
            m_rotationInput.y = input;
            m_rotationInput.z = input;
        }
    }
    else
    {
        if (!(m_gearDownDelayRemaining <= 0.0f))
        {
            m_gearDownDelayRemaining -= deltaTime;
        }
        else
        {
            const float input = objectTemplate->m_invertGear ? 1.0f : -1.0f;
            m_rotationInput.x = input;
            m_rotationInput.y = input;
            m_rotationInput.z = input;
        }
    }

    if (fullUpdate)
    {
        for (int axis = 0; axis < 3; ++axis)
        {
            const float rateProduct = vectorComponent(objectTemplate->m_acceleration, axis)
                                    * vectorComponent(objectTemplate->m_maxSpeed, axis);
            if (rateProduct != 0.0f)
            {
                bool useMaximum = m_gearUp;
                if (objectTemplate->m_invertGear)
                    useMaximum = !useMaximum;
                if (rateProduct < 0.0f)
                    useMaximum = !useMaximum;

                vectorComponent(m_currentRotation, axis) = useMaximum
                    ? vectorComponent(objectTemplate->m_maxRotation, axis)
                    : vectorComponent(objectTemplate->m_minRotation, axis);
            }
        }
    }

    RotationalBundle::handleUpdate(deltaTime, flags);
}


void LandingGear::handleMessage(TemplateMessage message, IPlayer* player, bool flag)
{
    if (static_cast<int>(message) == 4)
        updateFlags(0x800u, 0u);
    RotationalBundle::handleMessage(message, player, flag);
}

} // namespace world
} // namespace hfe
} // namespace dice
