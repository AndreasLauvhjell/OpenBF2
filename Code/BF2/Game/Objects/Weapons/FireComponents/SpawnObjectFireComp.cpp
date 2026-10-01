// Firing component that spawns an object.

#include "SpawnObjectFireComp.h"

#include <cfloat>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "../../SpawnObjectFireCompTemplate.h"
#include "../GenericFireArm.h"
#include "../../IObjectTemplate.h"
#include "../../IPlayer.h"
#include "../../ObjectLayoutAccess.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../Physics/CollisionManager.h"
#include "../../../../Physics/HeightmapCluster.h"
#include "../../../../Physics/ICollisionMesh.h"
#include "../../../../Physics/IPhysicsNode.h"
#include "../../../../Scene/Object/ObjectManager.h"
#include "../../../../Scene/Object/ObjectPredicator.h"

namespace dice {
namespace hfe {
namespace world {

extern IBase* heightmapCluster;

namespace {

// Both dedicated-server images indirect through a TU-local int pointer here.  Its backing int is
// initialized to zero and has no writer or external xref in either image.  The source name and its
// client-build role are [UNRESOLVED]; the observable dedicated-server contract is retained without
// inventing a console variable or a mutable production seam.
int s_spawnObjectSelectionOverrideStorage = 0;
int* const s_spawnObjectSelectionOverride = &s_spawnObjectSelectionOverrideStorage;

void normaliseLikeRetail(Vec3& value)
{
    const float lengthSquared =
        value.x * value.x + value.y * value.y + value.z * value.z;

    if (std::fabs(lengthSquared - 1.0f) < FLT_EPSILON)
        return;

    if (std::fabs(lengthSquared) >= FLT_EPSILON)
    {
        const float inverseLength = 1.0f / std::sqrt(lengthSquared);
        value.x *= inverseLength;
        value.y *= inverseLength;
        value.z *= inverseLength;
        return;
    }

    value.x = 0.0f;
    value.y = 0.0f;
    value.z = 0.0f;
}

} // namespace

// SpawnObjectFireComp runtime

// ELF C2/C1 0x826DB60/0x826DBC0; PE 0x5AE150.  The collision-mesh retry flag starts true and is
// permanently cleared only when the projectile template names no loadable collision mesh.
SpawnObjectFireComp::SpawnObjectFireComp(SpawnObjectFireCompTemplate* objectTemplate)
    : SingleFireComp(objectTemplate)
    , m_refCount(1)
    , m_spawnObjectTemplate(objectTemplate)
    , m_collisionMesh(0)
    , m_tryCollisionMesh(true)
{
}

// /0x826DC50; PE 0x5AE070.  Retail deliberately does not dispose a collision mesh
// cached after an intersection failure here; only canSpawn's successful scan deletes it.
SpawnObjectFireComp::~SpawnObjectFireComp()
{
}

void SpawnObjectFireComp::addRef()
{
    ++m_refCount;
}

int SpawnObjectFireComp::getRef()
{
    return m_refCount;
}

int SpawnObjectFireComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// .  The two weapon interfaces return the adjusted secondary face.
void* SpawnObjectFireComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return const_cast<SpawnObjectFireComp*>(this);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponFireComp)
    {
        return static_cast<IWeaponFireComp*>(
            const_cast<SpawnObjectFireComp*>(this));
    }
    return 0;
}

unsigned int SpawnObjectFireComp::getInterfaceID() const
{
    return IID_IWeaponFireComp;
}

// .  This empty override is intentional: the parachute selector must not
// run SingleFireComp's delayed-fire/time-event update arm.
void SpawnObjectFireComp::updateComponent(float, unsigned int)
{
}

// .  Preserve the repeated vehicle fetch and dead transformation read:
// the former can clear stale weak storage and the latter is an observable virtual dispatch even
// though retail ultimately compares world-Y falling speed.
bool SpawnObjectFireComp::isSelectable() const
{
    IPlayer* const player = firearm()->getPlayerByForce();
    if (!player)
        return false;

    IObject* const vehicle = player->getVehicle();
    if (!vehicle->queryInterface(IID_ISoldier))
        return false;

    const Vec3 vehiclePosition = player->getVehicle()->getAbsolutePosition();
    (void)vehiclePosition;

    SpawnObjectFireComp* const self = const_cast<SpawnObjectFireComp*>(this);
    if (*s_spawnObjectSelectionOverride <= 0 && !self->checkHeight())
        return false;

    IObject* const defaultVehicle = player->getDefaultVehicle();
    IPhysicsNode* const physicsNode = static_cast<IPhysicsNode*>(
        object_detail::LayoutAccess::physicsNode(defaultVehicle));

    const Mat4& unusedTransformation =
        getRootParent(static_cast<IObject*>(firearm()))->getAbsoluteTransformation();
    const Vec3 unusedForward = {
        unusedTransformation.m[8],
        unusedTransformation.m[9],
        unusedTransformation.m[10]
    };
    (void)unusedForward;

    Vec3 direction = physicsNode->getPositionalSpeed();
    const float speed = std::sqrt(
        direction.x * direction.x
        + direction.y * direction.y
        + direction.z * direction.z);
    normaliseLikeRetail(direction);

    const float verticalSpeed = speed * direction.y;
    if (*s_spawnObjectSelectionOverride > 0 || verticalSpeed <= -10.0f)
        return self->canSpawn();
    return false;
}

// .  A chute cannot open within two metres of the higher of terrain and
// water, nor when the two-metre downward object ray finds anything accepted by the 0x100 filter.
bool SpawnObjectFireComp::checkHeight()
{
    IPlayer* const player = firearm()->getPlayerByForce();
    IObject* const defaultVehicle = player->getDefaultVehicle();
    const Vec3 position = defaultVehicle->getAbsolutePosition();

    IHeightmapCluster* const cluster =
        static_cast<IHeightmapCluster*>(heightmapCluster);
    const float terrainHeight = cluster->getHeightInWorldCoords(position.x, position.z);
    const float waterHeight = cluster->getWaterLevelInWorldCoords(position.x, position.z);
    const float surfaceHeight = terrainHeight < waterHeight ? waterHeight : terrainHeight;
    if (surfaceHeight + 2.0f > position.y)
        return false;

    ObjectIgnorePredicator predicator(0x100u, player->getDefaultVehicle());
    IObject* hitObject = 0;
    Vec3 hitPoint = { 0.0f, 0.0f, 0.0f };
    Vec3 hitNormal = { 0.0f, 0.0f, 0.0f };
    int hitPart;
    const Vec3 down = { 0.0f, -2.0f, 0.0f };
    objectManager->intersectLine(
        hitObject, hitPoint, hitNormal, hitPart, position, down,
        &predicator, true, false, false, true, 0u);
    return hitObject == 0;
}

// .  The collision mesh is a temporary handle.  An intersecting object
// keeps it cached for the next attempt; a clean attempt deletes it, while a missing mesh disables
// this test permanently and is accepted thereafter.
bool SpawnObjectFireComp::canSpawn()
{
    if (!m_tryCollisionMesh)
        return true;

    if (!m_collisionMesh)
    {
        IObjectTemplate* const projectileTemplate = firearm()->getProjectileTemplate();
        if (!projectileTemplate)
            return false;

        const std::string collisionMeshName = projectileTemplate->getCollisionMesh();
        m_collisionMesh = collisionManager->getCollisionMeshPart(collisionMeshName, 0);
        if (!m_collisionMesh)
        {
            m_tryCollisionMesh = false;
            return true;
        }
    }

    const Mat4& transform = getProjectileOrigin();
    ObjectIgnorePredicator predicator(
        0x100u, getRootParent(static_cast<IObject*>(firearm())));
    std::vector<IObject*> objects;
    const Vec3 centre = { transform.m[12], transform.m[13], transform.m[14] };
    objectManager->getObjectsFromGrid(centre, 15.0f, objects, &predicator);

    for (std::vector<IObject*>::iterator it = objects.begin(); it != objects.end(); ++it)
    {
        IObject* const object = *it;
        if (object->getTemplate()->getClassID() != CID_SoldierTemplate
            && checkIfObjectIntersects(m_collisionMesh, transform, object) == true)
        {
            return false;
        }
    }

    delete m_collisionMesh;
    m_collisionMesh = 0;
    return true;
}

// .  Unlike ordinary firearms this uses the soldier vehicle's absolute
// transform, then applies the authored local projectile-start offset along its three basis rows.
const Mat4& SpawnObjectFireComp::getProjectileOrigin() const
{
    SpawnObjectFireComp* const self = const_cast<SpawnObjectFireComp*>(this);
    IPlayer* const player = firearm()->getPlayerByForce();
    IObject* const soldier = player->getVehicle();
    self->m_projectileOrigin = soldier->getAbsoluteTransformation();

    const Vec3& offset = m_spawnObjectTemplate->getProjectileStartPosition();
    self->m_projectileOrigin.m[12] +=
        self->m_projectileOrigin.m[0] * offset.x
        + self->m_projectileOrigin.m[4] * offset.y
        + self->m_projectileOrigin.m[8] * offset.z;
    self->m_projectileOrigin.m[13] +=
        self->m_projectileOrigin.m[1] * offset.x
        + self->m_projectileOrigin.m[5] * offset.y
        + self->m_projectileOrigin.m[9] * offset.z;
    self->m_projectileOrigin.m[14] +=
        self->m_projectileOrigin.m[2] * offset.x
        + self->m_projectileOrigin.m[6] * offset.y
        + self->m_projectileOrigin.m[10] * offset.z;
    return self->m_projectileOrigin;
}

// .  Selecting this component fires immediately and then returns false,
// preventing GenericFireArm's ordinary enable arm from running.
bool SpawnObjectFireComp::preEnable()
{
    fire(false, -1);
    return false;
}

// SpawnObjectFireCompTemplate

SpawnObjectFireCompTemplate::SpawnObjectFireCompTemplate()
    : SingleFireCompTemplate()
    , m_refCount(1)
{
}

SpawnObjectFireCompTemplate::~SpawnObjectFireCompTemplate()
{
}

void SpawnObjectFireCompTemplate::addRef()
{
    ++m_refCount;
}

int SpawnObjectFireCompTemplate::getRef()
{
    return m_refCount;
}

int SpawnObjectFireCompTemplate::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* SpawnObjectFireCompTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase
        || iid == IID_IObjectTemplateComponent
        || iid == IID_IWeaponCompTemplate)
    {
        return const_cast<SpawnObjectFireCompTemplate*>(this);
    }
    if (iid == io::IID_IConsoleSaveable)
    {
        return static_cast<io::IConsoleSaveable*>(
            const_cast<SpawnObjectFireCompTemplate*>(this));
    }
    return 0;
}

unsigned int SpawnObjectFireCompTemplate::getClassID() const
{
    return CID_SpawnObjectFireCompTemplate;
}

IObjectComponent* SpawnObjectFireCompTemplate::createComponent()
{
    return new SpawnObjectFireComp(this);
}

// Retail deliberately recognizes the concrete leaf and FireCompTemplate, but not the intermediate
// SingleFireCompTemplate class id.
void* SpawnObjectFireCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_SpawnObjectFireCompTemplate || classId == CID_FireCompTemplate)
        return this;
    return 0;
}

// .  This calls FireCompTemplate directly (not SingleFireCompTemplate)
// and appends an empty stream.  SpawnObject owns no console properties of its own.
bool SpawnObjectFireCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!FireCompTemplate::makeScript(stream))
        return false;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
