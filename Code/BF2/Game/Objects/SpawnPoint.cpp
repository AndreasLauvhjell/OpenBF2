// Spawn-point lifecycle and position selection.

#include "SpawnPoint.h"

#include "../../Core/Ids.h"
#include "../IGameLogic.h"                                    // gameLogic - isPrecaching (+0x228)
#include "../GameLogic/SpawnSystem/SpawnManager.h"            // ISpawnManager + the direct member
#include "../../Core/System.h"                                // WorldPref::getWorldTime
#include <map>
#include <vector>
#include "../../Scene/Object/ObjectManager.h"                 // getObjectTempVector / getObjectsFromGrid
#include "../../Scene/Object/ObjectPredicator.h"              // ObjectFlagPredicator
#include "../../Physics/HeightmapCluster.h"                   // getHeightInWorldCoords / getWaterLevel
#include "GameLogicComp/IArmorGLComp.h"                       // the +0x3C component's real interface
#include "IPlayer.h"                                          // getIsAlive
#include "../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // getOccupingPlayer
#include "Game/ControlPoint.h"                                // the control-point map's value type
#include "ObjectLayoutAccess.h"                              // LayoutAccess::armorComponent (+0x3C)
#include "Game/PlayerControlObject.h"                        // the concrete +0x180 dispatch
#include "Game/EntryPoint.h"                                 // BFfindEntryPoint
#include "ISoldier.h"
#include "../Utility/Vars.h"
#include "../../AI/AIDLL/AI/Main/BFPathfindingCourier.h"
#include "../../AI/AIDLL/AI/Main/CommonSingletonServer.h"
#include <cfloat>
#include <cmath>

namespace dice {
namespace hfe {

extern IBase* spawnManager;            // ds:0x8AD3A00 - filled by setSpawnManager at initModules

// ELF 81153EA / PE 581202: CommonSingletonServer's IAIPathfindingCourier,
// not a separate map singleton. The const bool query is ELF +14 / PE +0C
// (MSVC groups the two overloads in reverse order). The typed interface handles
// both that ordering and native pointer width. No null guard exists in either
// image; the caller gates this on checkAI. The concrete query remains chunk 04.
bool aiPathfindingMapContains(const Vec3& position)
{
    return static_cast<const ai::IAIPathfindingCourier*>(
        ai::CommonSingletonServer::m_iaiPathfindingCourier)->isValidPosition(position);
}


// Shared deterministic spawn-scatter seed, initialized from constructed spawn points.
// Scatter results depend on construction order and prior random draws.
static unsigned int s_scatterRandomSeed = 1973272912u;

// ELF initializer 0x81162D0 / PE 0x76B8F0..0x76B950. These are live Vars handles,
// not constants: defaults are 0x3F8147AE (1.01), 0x40400000 (3), 0x41100000 (9), and 2.
static float* s_terrainOffset = Vars::getFloat("spawnpoint-terrain-offset", 1.01f);
static float* s_scatterMinRadius = Vars::getFloat("spawnpoint-scatter-mindist", 3.0f);
static float* s_scatterMaxRadius = Vars::getFloat("spawnpoint-scatter-maxdist", 9.0f);
static int* s_scatterTestLod = Vars::getInt("spawnpoint-scatter-testlod", 2);

// Advances the Lehmer generator twice modulo 2^31-1 and converts the result to a unit float.
static float randomUnitFloat()
{
    s_scatterRandomSeed =
        (unsigned int)(((unsigned long long)24112u * s_scatterRandomSeed) % 0x7FFFFFFFu);
    s_scatterRandomSeed =
        (unsigned int)(((unsigned long long)26143u * s_scatterRandomSeed) % 0x7FFFFFFFu);
    return (float)(((s_scatterRandomSeed >> 7) | 1u) + 1u) * 0.000000059604645f;
}

SpawnPoint::SpawnPoint(const SpawnPointTemplate* objectTemplate)
    : Object(objectTemplate)
{
    m_spawnTemplate = objectTemplate;                // +0x134
    m_refCount = 1;                                  // +0x130
    m_spawnGroupId = 0;                              // +0x13C

    m_spawnPreventionDelay = objectTemplate->m_spawnPreventionDelay;   // +0x138 <- template +0x44
    // Seeded to minus the delay: isOccupied() compares (m_spawnPreventionDelay + m_lastSpawnTime)
    // against the world clock, so a fresh point reads "free" from tick zero.
    m_lastSpawnTime = -objectTemplate->m_spawnPreventionDelay;         // +0x140

    m_active = objectTemplate->m_active;             // +0x144 <- template +0x50

    m_lastSpawnPosition.x = 0.0f;                               // +0x148
    m_lastSpawnPosition.y = 0.0f;
    m_lastSpawnPosition.z = 0.0f;

    m_overrideEnterOnSpawn = false;                  // +0x154
    m_proneOnSpawn = false;                          // +0x155
    m_bool156 = false;                               // +0x156

    // The TU cursor - a virtual getId() through the template's vtable (+0x44), plus one, masked
    // positive. Last constructor wins, as shipped.
    s_scatterRandomSeed = (objectTemplate->getId() + 1) & 0x7FFFFFFF;
}

SpawnPoint::~SpawnPoint() {}

void SpawnPoint::addRef()  { ++m_refCount; }
int  SpawnPoint::getRef()  { return m_refCount; }
int  SpawnPoint::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

// , four cases - the ControlPoint shape with ISpawnPoint in the third slot.
void* SpawnPoint::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)         return const_cast<SpawnPoint*>(this);
    if (iid == world::IID_IObject) return const_cast<SpawnPoint*>(this);
    if (iid == IID_ISpawnPoint)
        return static_cast<ISpawnPoint*>(const_cast<SpawnPoint*>(this));   // this + 0x12C
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<SpawnPoint*>(this));   // this + 0x80
    return 0;
}

// init - 0x8115C50, 88 B. Registration with the spawn manager, twice-gated:
// * a child spawn point (m_parent set - one riding a vehicle) never registers itself; its
// parent's machinery owns it.
// * a precache probe never registers - the isPrecaching() test, IGameLogic slot +0x228, which
// GameLogic now implements for real (it was a bootmain stub returning false until this batch,
// which would have sent every probe into the manager).
// The -1 group means "unassigned"; addSpawnPoint's own logic files it.
void SpawnPoint::init()
{
    Object::init();

    if (m_parent)                                    // IObject +0x28, protected -> visible here
        return;
    if (gameLogic->isPrecaching())
        return;

    static_cast<SpawnManager*>(spawnManager)
        ->addSpawnPoint(static_cast<ISpawnPoint*>(this), -1);
}

// the mirror, same precache gate, then Object's teardown.
void SpawnPoint::destroy(bool force)
{
    if (!gameLogic->isPrecaching())
        static_cast<SpawnManager*>(spawnManager)
            ->remove(static_cast<const ISpawnPoint*>(this));

    Object::destroy(force);
}

// The two moved-or-changed overrides. Both end the same way: this point's group's cached average
// position is stale now, tell the manager. The group id is fetched virtually (self slot +0x1A0) -
// preserved: a subclass overriding getSpawnGroupId would redirect these too.
// makeAverageGroupPositionDirty is called on the manager unconditionally - no null check, no
// precache gate. Safe in both worlds: the manager exists from initModules on, and the function
// no-ops while its group map is empty, which during precache it always is.
void SpawnPoint::setAbsolutePosition(const Vec3& v)
{
    Object::setAbsolutePosition(v);
    static_cast<SpawnManager*>(spawnManager)->makeAverageGroupPositionDirty(getSpawnGroupId());
}

void SpawnPoint::updateFlags(unsigned int set, unsigned int clear)
{
    Object::updateFlags(set, clear);
    static_cast<SpawnManager*>(spawnManager)->makeAverageGroupPositionDirty(getSpawnGroupId());
}

// Object's emitter, result squashed to strict bool (the ELF's setnz) - a forward here.
bool SpawnPoint::makeScript(io::IStream* stream)
{
    return Object::makeScript(stream);
}

// teleport the object onto this point. The float parameter is ignored - the timestamp
// bookkeeping lives in the selection engine, not here.
bool SpawnPoint::spawn(world::IObject* obj, float)
{
    if (!obj)
        return false;
    obj->setAbsolutePosition(getAbsolutePosition());
    return true;
}

// occupied while the world clock has not yet passed (last spawn + prevention delay). The
// float parameter is ignored here too.
bool SpawnPoint::isOccupied(float) const
{
    return m_spawnPreventionDelay + m_lastSpawnTime > WorldPref::getWorldTime();
}

void SpawnPoint::setActive(bool v)               { m_active = v; }
bool SpawnPoint::getEnterOnSpawn()               { return m_spawnTemplate->m_enterOnSpawn
                                                        || m_overrideEnterOnSpawn; }
bool SpawnPoint::getAIEnterOnSpawn()             { return m_spawnTemplate->m_aiEnterOnSpawn; }
void SpawnPoint::setSpawnGroupId(int id)         { m_spawnGroupId = id; }

// a spawn point tied to a control point gets its group from the manager's
// (controlPointId, group) id allocator; a free-standing one keeps its own.
int SpawnPoint::getSpawnGroupId()
{
    if (m_spawnTemplate->m_controlPointId != 0xFFFFFFFFu)
        return static_cast<SpawnManager*>(spawnManager)->getMappedSpawnGroupId(
                   static_cast<int>(m_spawnTemplate->m_controlPointId),
                   static_cast<int>(m_spawnTemplate->m_group));
    return m_spawnGroupId;
}

void SpawnPoint::setOverrideEnterOnSpawn(bool v) { m_overrideEnterOnSpawn = v; }
bool SpawnPoint::getOverrideEnterOnSpawn()       { return m_overrideEnterOnSpawn; }
void SpawnPoint::setProneOnSpawn(bool v)         { m_proneOnSpawn = v; }
bool SpawnPoint::getProneOnSpawn()               { return m_proneOnSpawn || m_bool156; }
bool SpawnPoint::getOnlyForAI() const            { return m_spawnTemplate->getOnlyForAI(); }
bool SpawnPoint::getOnlyForHuman() const         { return m_spawnTemplate->getOnlyForHuman(); }

// [FRONTIER] - the spawn-selection engine: getActive (862 B), getObjectToEnter (224 B),
// getSpawnPosition (1,562 B), getSpawnRotation (649 B), findValidSpawnPos (1,283 B). One
// subsystem - height maps, scatter, occupancy sweeps, the s_scatterRandomSeed cursor - dispatched
// when a player picks a spawn, which no current drive reaches. Each faults by name.


// Returns whether this point accepts the requested player kind and team, checking ownership, damage, height and occupancy.
// Clears the temporary prone flag on every call. Scatter and close-to-vehicle modes bypass occupancy rejection.
// Occupancy ignores this point's own subtree; nearby entry-point search uses 15 units for AI and 5 for humans.
namespace {


// [INFERRED] name. ELF 0x8AAC800 is zero-filled .bss; PE 0x887258 is in the zero-filled .data
// tail. Each image has exactly one direct reference, the getActive read (ELF 0x811606C,
// ). No writer/address publication is present in either image's xrefs. The old extern
// had no definition and /force bound its read to image RVA 0 (the nonzero MZ header), vetoing all
// squad spawns. Preserve the retail zero-initialized storage and the read, not a fabricated veto.
static unsigned char g_spawnPointDynamicVeto = 0;

} // anonymous namespace

// world::heightmapCluster - ds:0x8ABAF24, owned by Core/Globals.cpp and declared IBase* there.
// Declared locally exactly as EngineBase.cpp does for the same global.
namespace world { extern IBase* heightmapCluster; }

bool SpawnPoint::getActive(bool isAI, int team)
{
    m_bool156 = false;                          // side effect, before any test

    if (!m_active)
        return false;

    // 2. dynamic (squad-leader) groups: ids above 191
    // gameLogic->getControlPointMap() - IGameLogic +0x174, whose values are ControlPoint* on
    // their primary vtable: the team getter used here is primary +0x1A0 (14 B, returns m_team at
    // +0x148), NOT IControlPoint's +0x1C. Two tables, two orders - the interface slot would have
    // been the wrong one.
    if (getSpawnGroupId() > 191)
    {
        typedef std::map<unsigned int, world::IObject*> ControlPointMap;
        ControlPointMap* points = gameLogic->getControlPointMap();

        bool sameTeam = false;
        for (ControlPointMap::const_iterator it = points->begin(); it != points->end(); ++it)
        {
            if (static_cast<world::ControlPoint*>(it->second)->getTeam() == team)
            {
                sameTeam = true;
                break;
            }
        }
        if (!sameTeam)
            return false;                       // the ELF spells this as a dead conditional

        if (g_spawnPointDynamicVeto)
            return false;
    }

    // 3. reserved for the other kind
    if (isAI ? m_spawnTemplate->m_onlyForHuman : m_spawnTemplate->m_onlyForAI)
        return false;

    // 4. the armour it hangs off
    // The first non-null m_armorComponent walking UP the parent chain from this point. +0x3C
    // holds a queryInterface(IID_IArmorGLComp) result, so these dispatch on the IArmorGLComp
    // sub-table: +0x5C is getStatus and +0x58 is isCriticalDamaged there.
    world::IArmorGLComp* armour = 0;
    for (world::IObject* o = this; o != 0; o = o->getParent())
    {
        // m_armorComponent is protected; LayoutAccess is the tree's own hatch for this
        // very walk ("the ELF's recurring obj+0x3C walk").
        armour = static_cast<world::IArmorGLComp*>(world::object_detail::LayoutAccess::armorComponent(o));
        if (armour)
            break;
    }
    if (armour)
    {
        if (armour->getStatus() != 0)
            return false;
        if (armour->isCriticalDamaged())
            return false;
    }

    // 5. height above the surface
    if (m_spawnTemplate->m_minSpawnHeight != -1.0f)             // ds:0x888B590, the ctor sentinel
    {
        const Vec3& p = getAbsolutePosition();
        world::IHeightmapCluster* hm =
            static_cast<world::IHeightmapCluster*>(world::heightmapCluster);

        const float ground = hm->getHeightInWorldCoords(p.x, p.z);      // +0x6C
        const float water  = hm->getWaterLevelInWorldCoords(p.x, p.z);  // +0x98
        const float surface = (ground > water) ? ground : water;        // fucomi/fcmovbe = max

        if ((p.y - surface) < m_spawnTemplate->m_minSpawnHeight)
            return false;
    }

    // 6/7. is anybody already here?
    const bool aiEntersOnSpawn = m_spawnTemplate->m_aiEnterOnSpawn && isAI;
    if (!aiEntersOnSpawn && !getEnterOnSpawn())
    {
        // either flag makes the point unconditionally available, not merely untested
        if (m_spawnTemplate->m_scatterSpawnPositions)
            return true;
        if (m_spawnTemplate->m_allowSpawnCloseToVehicle)
            return true;

        // the temp vector is fetched before the predicator is built, as shipped
        std::vector<world::IObject*>& hits = world::objectManager->getObjectTempVector();   // +0xA8
        world::ObjectFlagPredicator predicator(0x1000200u);
        world::objectManager->getObjectsFromGrid(getAbsolutePosition(), 1.0f, hits, &predicator);

        const world::IObject* myRoot = world::getRootParent(this);

        for (std::vector<world::IObject*>::const_iterator it = hits.begin(); it != hits.end(); ++it)
        {
            world::IObject* hit = *it;
            if (world::getRootParent(hit) == myRoot)
                continue;                       // my own subtree never blocks me

            const unsigned int classId = hit->getTemplate()->getClassID();

            if (classId == world::CID_SoldierTemplate)
            {
                // The class-ID check permits direct dispatch without querying another interface.
                world::IPlayer* occupant =
                    static_cast<world::PlayerControlObject*>(hit)->getOccupingPlayer();
                if (occupant && occupant->getIsAlive())
                    return false;               // somebody alive is standing here
            }
            if (classId == world::CID_PlayerControlObjectTemplate)
                return false;                   // a vehicle is parked here
        }
        return true;                            // nothing in the way
    }

    // Select the bot radius here: a null player bypasses BFfindEntryPoint's own bot override.
    const float radius = isAI ? 15.0f : 5.0f;
    return world::BFfindEntryPoint(getAbsolutePosition(), radius, team, 0,
                                   false, false, true) != 0;
}


// . See the note at the declaration.
ISpawnPoint* getSpawnPoint(world::IObject* object)
{
    return static_cast<ISpawnPoint*>(object->queryInterface(IID_ISpawnPoint));   // slot 3 (+0x0C)
}


// getSoldierYPosOffset - 0x8114E30, 11 B. One constant, no setter.
float getSoldierYPosOffset()
{
    return 0.875f;
}

// Writes the spawn position, using the authored offset or up to five scatter attempts.
// checkAI additionally requires a pathfinding-valid candidate. Failed scatter leaves the accumulated base position in out.
// Attempts rotate cumulatively; enter-on-spawn disables scattering.
bool SpawnPoint::getSpawnPosition(Vec3& out, bool checkAI)
{
    out = getAbsolutePosition();                                   // slot 27

    const SpawnPointTemplate* t = m_spawnTemplate;

    // enter-ON-spawn beats scatter - a player going straight into a vehicle lands on the marker.
    if (!t->m_scatterSpawnPositions || getEnterOnSpawn())           // +0x58 +0x194
    {
        out.x += t->m_spawnPositionOffset.x;                        // template +0x38
        out.y += t->m_spawnPositionOffset.y;
        out.z += t->m_spawnPositionOffset.z;
        m_lastSpawnPosition = out;                                  // +0x148
        return true;
    }

    // the scattered search
    world::IObject* root = world::getRootParent(this);
    if (root->queryInterface(world::IID_ISoldier))
        out.y -= getSoldierYPosOffset();                            // 0.875

    float zDeg, xDeg, yDeg;
    getRotationZXY(getAbsoluteTransformation(), zDeg, xDeg, yDeg);  // two of three are dead

    // THE first angle: a random bearing in [-70, +70] degrees, measured off the point's own yaw.
    float angle = randomUnitFloat() * 140.0f - 70.0f - yDeg;

    // AND A random radius between the TU's two float constants.
    const float minRadius = *s_scatterMinRadius;
    const float span      = *s_scatterMaxRadius - minRadius;
    const float radius    = minRadius + randomUnitFloat() * span;

    // The offset that gets rotated: straight back along -Z at `radius`.
    float offX = 0.0f;
    float offY = 0.0f;
    float offZ = -radius;

    for (int attempt = 0; ; ++attempt)
    {
        Vec3 candidate = out;

        angle += static_cast<float>(attempt) * 72.0f;

        Mat4 basis = Mat4::getIdentity();
        if (fabsf(angle) > 1.192092896e-07f)
        {
            Mat4 rot;
            for (int i = 0; i < 16; ++i) rot.m[i] = 0.0f;
            rot.m[0] = 1.0f; rot.m[5] = 1.0f; rot.m[10] = 1.0f; rot.m[15] = 1.0f;
            Vec3 up; up.x = 0.0f; up.y = 1.0f; up.z = 0.0f;
            setRotateAboutLine(rot, angle, up);
            const Mat4 was = basis;
            basis.rotMult(was, rot);
        }

        // THE offset IS rotated IN place AND carried into THE next attempt - each retry rotates
        // the already-rotated vector, which is what makes the 72 degrees cumulative.
        const float rx = basis.m[0] * offX + basis.m[4] * offY + basis.m[8]  * offZ;
        const float ry = basis.m[1] * offX + basis.m[5] * offY + basis.m[9]  * offZ;
        const float rz = basis.m[2] * offX + basis.m[6] * offY + basis.m[10] * offZ;
        offX = rx; offY = ry; offZ = rz;

        candidate.x += rx;
        candidate.y += ry;
        candidate.z += rz;

        if (findValidSpawnPos(out, candidate))
        {
            if (!checkAI || aiPathfindingMapContains(candidate))    // ds:0x8AC35B4 +0x14
            {
                out = candidate;
                m_lastSpawnPosition = candidate;                    // +0x148
                return true;
            }
        }

        if (attempt >= 4)                                           // five tries, then give up
            return false;
    }
}


// (1,283 B) / PE 0x580CF0. Test the full displacement, then half of it.
// `candidate` is written before the standing/prone tests, even if both reject the position;
// the second drop-height comparison therefore observes that write. The prone latch is only set.
bool SpawnPoint::findValidSpawnPos(const Vec3& base, Vec3& candidate)
{
    world::ObjectIgnorePredicator pred(0x100u, world::getRootParent(this));
    world::IObject* hitObject = 0;
    Vec3 hitPoint = {}, hitNormal = {};
    int hitPart;
    Vec3 direction = {candidate.x - base.x, candidate.y - base.y, candidate.z - base.z};
    const float lengthSquared = direction.x * direction.x + direction.y * direction.y
                              + direction.z * direction.z;
    const float distance = std::sqrt(lengthSquared);
    if (std::fabs(lengthSquared - 1.0f) >= FLT_EPSILON)
    {
        if (std::fabs(lengthSquared) >= FLT_EPSILON)
        {
            const float scale = 1.0f / std::sqrt(lengthSquared);
            direction.x *= scale;
            direction.y *= scale;
            direction.z *= scale;
        }
        else
            direction = Vec3::getZero();
    }

    world::objectManager->setOverrideLineTestLod(*s_scatterTestLod);
    for (int divisor = 1; divisor <= 2; ++divisor)
    {
        const float distanceToTry = distance / static_cast<float>(divisor);
        Vec3 position = {base.x + direction.x * distanceToTry,
                         base.y + direction.y * distanceToTry,
                         base.z + direction.z * distanceToTry};
        float ground = static_cast<world::IHeightmapCluster*>(world::heightmapCluster)
            ->getHeightInWorldCoords(position.x, position.z);
        const Vec3 down = {0.0f, -4.0f, 0.0f};
        if (world::objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                position, down, &pred, true, false, false, true, 0))
            ground = hitPoint.y > ground ? hitPoint.y : ground;
        else if (candidate.y - 4.0f > ground)
            continue;

        position.y = ground + *s_terrainOffset;
        world::IObject* rayHit = 0;
        const Vec3 feet = {position.x, position.y - getSoldierYPosOffset(), position.z};
        if (world::checkIfRayHitAnyObjectOrGround(base, feet, 0, rayHit, true, false, true, 0))
            continue;

        candidate = position;
        static Mat4 transform = Mat4::getIdentity();
        transform.m[12] = position.x;
        transform.m[13] = position.y;
        transform.m[14] = position.z;
        world::ISoldier* soldier = static_cast<world::ISoldier*>(
            world::getRootParent(this)->queryInterface(world::IID_ISoldier));
        if (soldier && !soldier->testIfValidTransformation(world::SoldierPose_Stand, transform, false))
        {
            if (!soldier->testIfValidTransformation(world::SoldierPose_Prone, transform, false))
                continue;
            m_bool156 = true;
        }
        world::objectManager->setOverrideLineTestLod(-1);
        return true;
    }
    world::objectManager->setOverrideLineTestLod(-1);
    return false;
}


// Returns marker orientation for ordinary or nearby spawns; scattered spawns face back toward the marker at level pitch and roll.
// Preserves the original conversion factor so facing matches the game.
const Vec3& SpawnPoint::getSpawnRotation()
{
    static Vec3 result;      // the ELF returns through an sret; this stands in for the caller's slot

    // no scatter (or entering a vehicle): the point's own orientation
    if (!m_spawnTemplate->m_scatterSpawnPositions || getEnterOnSpawn())      // +0x58 +0x194
    {
        float zDeg, xDeg, yDeg;
        getRotationZXY(getAbsoluteTransformation(), zDeg, xDeg, yDeg);       // slot 33
        result.x = -yDeg;
        result.y = -xDeg;
        result.z = -zDeg;
        return result;
    }

    // the marker this point nominally spawns on
    const Vec3& pos = getAbsolutePosition();                                 // slot 27
    const Vec3& offset = m_spawnTemplate->m_spawnPositionOffset;             // template +0x38

    Vec3 marker;
    marker.x = pos.x + offset.x;
    marker.y = pos.y + offset.y;
    marker.z = pos.z + offset.z;

    // and the direction from where the body actually landed, back to it
    Vec3 d;
    d.x = marker.x - m_lastSpawnPosition.x;                                  // +0x148
    d.y = marker.y - m_lastSpawnPosition.y;
    d.z = marker.z - m_lastSpawnPosition.z;

    // TOO close TO mean anything - fall back to the point's own orientation.
    if (sqrtf(d.x * d.x + d.y * d.y + d.z * d.z) < 1.0f)
    {
        float zDeg, xDeg, yDeg;
        getRotationZXY(getAbsoluteTransformation(), zDeg, xDeg, yDeg);
        result.x = -yDeg;
        result.y = -xDeg;
        result.z = -zDeg;
        return result;
    }

    // The same normalise-if-needed shape as FreeCamera's: |len2 - 1| against FLT_EPSILON to skip
    // the work, then |len2| against it to avoid dividing by zero.
    const float len2 = d.x * d.x + d.y * d.y + d.z * d.z;
    if (fabsf(len2 - 1.0f) >= 1.192092896e-07f)
    {
        if (fabsf(len2) >= 1.192092896e-07f)
        {
            const float inv = 1.0f / sqrtf(len2);
            d.x *= inv; d.y *= inv; d.z *= inv;
        }
        else
        {
            d.x = 0.0f; d.y = 0.0f; d.z = 0.0f;
        }
    }

    // ONE axis decides THE YAW, and d.z picks the quadrant.
    float s = d.x;
    if (s < -1.0f)      s = -1.0f;                 // written as two compares, not a clamp
    else if (s > 1.0f)  s =  1.0f;

    float yaw = asinf(s) * 57.29747f;              // NOT 180/pi - see the block above
    if (d.z < 0.0f)
        yaw = 180.0f - yaw;

    // pitch AND roll ARE hard zeros - a scattered spawn is always level.
    result.x = yaw;
    result.y = 0.0f;
    result.z = 0.0f;
    return result;
}

// (224 B), PE 0x580990 (153 B, ISpawnPoint face).
// The explicit EAX return and the AI caller prove an object pointer. Search misses
// return null directly; disabled entry still calls getRootParent(null), as shipped.
world::IObject* SpawnPoint::getObjectToEnter(bool isAI, int team)
{
    const SpawnPointTemplate* const objectTemplate =
        static_cast<const SpawnPointTemplate*>(m_template);
    world::IPlayerControlObject* entry = 0;
    if (getEnterOnSpawn() || (objectTemplate->m_aiEnterOnSpawn && isAI))
    {
        entry = world::BFfindEntryPoint(getAbsolutePosition(), isAI ? 15.0f : 5.0f,
                                        team, 0, false, false, true);
        if (!entry)
            return 0;
    }
    // Both ports pass the search result unchanged to getRootParent: no IObject query,
    // no secondary-face adjustment. BFfindEntryPoint returns a seat interface, so this
    // is a shipped interface/object mismatch on a non-null result. Do not silently fix it.
    return world::getRootParent(reinterpret_cast<world::IObject*>(entry));
}

} // namespace hfe
} // namespace dice
