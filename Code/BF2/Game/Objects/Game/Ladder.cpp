// Ladder registry, traversal, bounds and collision handling. [INFERRED] Source placement.
// The additional initialized file-static words have [UNRESOLVED] meaning.

#include "Ladder.h"
#include "LadderContainer.h"
#include "Soldier.h"

#include "../../../Core/Ids.h"
#include "../../../Game/IGame.h"
#include "../../../Game/IGameLogic.h"
#include "../IObject.h"
#include "../IObjectTemplate.h"
#include "../IPlayer.h"
#include "../ISoldier.h"
#include "../ICameraObject.h"
#include "../ObjectLayoutAccess.h"
#include "../TemplateLeaves.h"                    // LadderTemplate
#include "../../../Math/Mat4.h"
#include "../../../Math/Vec3.h"
#include "../../../Math/LocalBoundingBox.h"
#include "../../../Physics/HeightmapCluster.h"
#include "../../../Physics/ICollisionMesh.h"
#include "../../../Physics/PhysicsUtil.h"
#include "../../../Scene/Object/ObjectManager.h"
#include "../../../Scene/Object/ObjectPredicator.h"
#include "../../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../../Scene/Object/ObjectUtil.h"    // getRootParent
#include "../../Utility/Vars.h"

#include <cfloat>
#include <cmath>

namespace dice {
namespace hfe {
namespace world {

extern IBase* heightmapCluster;                    // ds:0x8ABAF24, Core/Globals.cpp

namespace {

float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

void normalizeLikeEngine(Vec3& v)
{
    const float lenSq = dot(v, v);
    if (std::fabs(lenSq - 1.0f) >= FLT_EPSILON)
    {
        if (std::fabs(lenSq) >= FLT_EPSILON)
        {
            const float invLength = 1.0f / std::sqrt(lenSq);
            v.x *= invLength;
            v.y *= invLength;
            v.z *= invLength;
        }
        else
        {
            v.x = v.y = v.z = 0.0f;
        }
    }
}

Vec3 getObjectRotation(IObject* object)
{
    const bool hasParent = object_detail::LayoutAccess::parent(object) != 0;
    const Mat4& transform = hasParent
        ? object->getRelativeTransformation()
        : object->getAbsoluteTransformation();
    float z, x, y;
    getRotationZXY(transform, z, x, y);
    Vec3 out = { -y, -x, -z };
    return out;
}

void setObjectRotation(IObject* object, const Vec3& rotation)
{
    const bool hasParent = object_detail::LayoutAccess::parent(object) != 0;
    Mat4 transform = hasParent
        ? object->getRelativeTransformation()
        : object->getAbsoluteTransformation();
    setRotationZXY(transform, -rotation.z, -rotation.y, -rotation.x);
    if (hasParent)
        object->setRelativeTransformation(transform);
    else
        object->setAbsoluteTransformation(transform);
}

} // anonymous namespace

// .data 0x8987B64, initialised to 1 - the id the first ladder of a level gets.
int Ladder::globalLadderId = 1;

// ds:0x8AAD220 - the static init constructs it (an SGI header node; count at +4).
std::map<int, Ladder*> ladderMap;

namespace ladder_detail {
// The Vars family the TU registers at static init (Vars::getFloat returns the tunable's slot; the
// second argument is the default, bit patterns read from the init). Named ds:0x8AAD2B8..0x8AAD300.
// All 17 floats and both debug ints are consumed by the recovered Ladder/LadderContainer bodies.
float* ladderOffsetTop        = Vars::getFloat("ladder-offset-top",           0.36f);
float* ladderSoldierStartGetoff = Vars::getFloat("ladder-soldier-start-getoff", 1.8f);
float* ladderTopExitOffset    = Vars::getFloat("ladder-top-exit-offset",      0.75f);
float* ladderExitOffset       = Vars::getFloat("ladder-exit-offset",          0.75f);
float* ladderBottomMarginal   = Vars::getFloat("ladder-bottom-marginal",      0.05f);
float* ladderStepSize         = Vars::getFloat("ladder-step-size",            0.3f);
float* ladderSkidSpeedMod     = Vars::getFloat("ladder-skid-speed-mod",       4.0f);
float* ladderClimbSpeedMod    = Vars::getFloat("ladder-climb-speed-mod",      0.045f);
float* ladderEndCameraOffset  = Vars::getFloat("ladder-end-camera-offset",    0.4f);
float* ladderWaterLimit       = Vars::getFloat("ladder-water-limit",          1.2f);
float* ladderGetoffSpeedMod   = Vars::getFloat("ladder-getoff-speed-mod",     1.15f);
float* ladderSoldierDist      = Vars::getFloat("ladder-soldier-dist",         0.6f);
float* ladderDownAngleLimit   = Vars::getFloat("ladder-down-angle-limit",     35.0f);
float* ladderDownRayMag       = Vars::getFloat("ladder-down-ray-mag",         0.2f);
float* ladderDownRayMagSkid   = Vars::getFloat("ladder-down-ray-mag-skid",    0.3f);
float* ladderUpRayMagSkid     = Vars::getFloat("ladder-up-ray-mag-skid",      1.7f);
float* ladderGetoffRayMin     = Vars::getFloat("ladder-getoff-ray-min",       0.3f);
int*   ladderRayDebug         = Vars::getInt  ("ladder-ray-debug",            0);
int*   ladderRayDebug2        = Vars::getInt  ("ladder-ray-debug2",           0);
} // namespace ladder_detail

using namespace ladder_detail;

// (C2) / 0x81884F0 (C1), 170 B, read in full: SimpleObject, then (the ctor's own store
// order) template, height 2.0f, refcount 1, the two vptrs, dirty true, top/bottom/bottomReal 0,
// id = globalLadderId++, and the map insert of (id, this).
Ladder::Ladder(const LadderTemplate* objectTemplate)
    : SimpleObject(objectTemplate)
{
    m_template = objectTemplate;                     // +0x13C
    m_ladderHeight = 2.0f;                           // +0x148
    m_refCount = 1;                                  // +0x138
    m_boundsDirty = true;                            // +0x144
    m_absoluteTop = 0.0f;                            // +0x150
    m_absoluteBottom = 0.0f;                         // +0x154
    m_absoluteBottomReal = 0.0f;                     // +0x158
    m_ladderId = globalLadderId++;                   // +0x140
    ladderMap.insert(std::make_pair(m_ladderId, this));
}

// (D2, 262 B) / 0x81886B0 (D1, 193 B) / 0x8188780 (D0, 201 B): vptr restores, erase
// this ladder's id from the map (find + rebalance-for-erase + free, inlined), and if the map is now
// empty reset globalLadderId to 1 - so a fresh level's first ladder is id 1 again; then
// SimpleObject's dtor. (The 262-byte D2 is the same body with the allocator's free-list path
// inlined instead of the deallocate call.)
Ladder::~Ladder()
{
    ladderMap.erase(m_ladderId);
    if (ladderMap.empty())
        globalLadderId = 1;
}

// The quartet - refcount at +0x138.
void Ladder::addRef()  { ++m_refCount; }
int  Ladder::getRef()  { return m_refCount; }
int  Ladder::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

// , TWO cases only: IBase / IObject -> this. No IConsoleSaveable arm - see the header.
void* Ladder::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)   return const_cast<Ladder*>(this);
    if (iid == IID_IObject) return const_cast<Ladder*>(this);
    return 0;
}

// `ret`. A ladder overrides the networkable hook to do nothing - it is never replicated as an
// object; soldiers carry the ladder id instead (SoldierNetworkable reads ladderMap).
void Ladder::initNetworkableComp() {}

// each: recalc-if-dirty, then the field.
float Ladder::getLadderHeight()       { if (m_boundsDirty) recalcBounds(); return m_ladderHeight; }
float Ladder::getAbsoluteTop()        { if (m_boundsDirty) recalcBounds(); return m_absoluteTop; }
float Ladder::getAbsoluteBottom()     { if (m_boundsDirty) recalcBounds(); return m_absoluteBottom; }
float Ladder::getAbsoluteBottomReal() { if (m_boundsDirty) recalcBounds(); return m_absoluteBottomReal; }

// the soldier's root parent's y, mapped onto the ladder as (y - bottom) / height
// and clamped to [0, 1]; 0 when the soldier has no root. (The ISoldier is first turned into its
// IObject through queryInterface(IID_IObject).)
float Ladder::getCurrentState(ISoldier* soldier)
{
    IObject* object = static_cast<IObject*>(
        reinterpret_cast<IBase*>(soldier)->queryInterface(IID_IObject));
    IObject* root = getRootParent(object);
    if (!root)
        return 0.0f;
    const float y = root->getAbsolutePosition().y;
    const float bottom = getAbsoluteBottom();
    const float state = (y - bottom) / getLadderHeight();
    if (state < 0.0f) return 0.0f;
    if (state > 1.0f) return 1.0f;
    return state;
}

// put the soldier's root at (x, bottom + height * state, z) - x/z kept.
void Ladder::setCurrentState(ISoldier* soldier, float state)
{
    IObject* object = static_cast<IObject*>(
        reinterpret_cast<IBase*>(soldier)->queryInterface(IID_IObject));
    IObject* root = getRootParent(object);
    if (!root)
        return;
    const float bottom = getAbsoluteBottom();
    const float y = getLadderHeight() * state + bottom;
    const Vec3& current = root->getAbsolutePosition();      // +0x6C
    Vec3 position;
    position.x = current.x;
    position.y = y;
    position.z = current.z;
    root->setAbsolutePosition(position);                     // +0x70
}

// . The ladder object itself is not entered. It creates the transient
// LadderContainer PCO, gives it the ladder transform, and seats the soldier through `_enter`.
void Ladder::attachSoldier(ISoldier* soldier)
{
    IPlayer* player = soldier->getOccupingPlayer();
    if (!player)
        return;

    if (soldier->getOccupingPlayer()->getDefaultVehicle() !=
        soldier->getOccupingPlayer()->getVehicle())
        return;

    const unsigned int oldId = soldier->getLadderId();
    if (oldId == static_cast<unsigned int>(m_ladderId))
        return;

    if (oldId)
        ladderMap[static_cast<int>(oldId)]->detachSoldier(soldier); // operator[] and no null guard

    IObjectTemplate* objectTemplate =
        objectTemplateManager->getTemplate(std::string("LadderContainer"));
    objectTemplate->queryInterface(IID_IPlayerControlObjectTemplate); // result discarded, as shipped
    LadderContainer* container = static_cast<LadderContainer*>(objectTemplate->createObject());
    container->setAbsoluteTransformation(getAbsoluteTransformation());
    container->m_ladder = this;

    if (!game->queryInterface(IID_IGameClient) || !soldier->getOccupingPlayer()->getIsRemote())
        disablePhysics(container, false);

    container->init();
    container->updateFlags(0x100, 0);
    container->_enter(soldier->getOccupingPlayer());
    soldier->setLadderId(static_cast<unsigned int>(m_ladderId));
}

// . On a client, preserve the camera's Z/X rotation and replace only its
// script-facing X component with Soldier's pending-turn accumulator before destroying the mount.
// Both ports then call disablePhysics(soldierObject, false) after container->destroy(true): ELF
// and PE 0x59C590 -> 0x54A740. This surprising order/callee is a shipped quirk.
void Ladder::detachSoldier(ISoldier* soldier)
{
    if (!soldier)
        return;

    const unsigned int id = soldier->getLadderId();

    if (!gameLogic->isServer())
    {
        if (soldier->getOccupingPlayer() && soldier->getOccupingPlayer()->getCamera())
        {
            IObject* camera = static_cast<IObject*>(
                soldier->getOccupingPlayer()->getCamera()->queryInterface(IID_IObject));
            Vec3 rotation = getObjectRotation(camera);
            rotation.x = static_cast<Soldier*>(soldier)->m_float214;
            setObjectRotation(camera, rotation);
        }
    }

    if (id == static_cast<unsigned int>(m_ladderId))
    {
        IObject* object = static_cast<IObject*>(soldier->queryInterface(IID_IObject));
        LadderContainer* container = static_cast<LadderContainer*>(
            object_detail::LayoutAccess::parent(object));
        container->_exit();
        container->destroy(true);

        if (!game->queryInterface(IID_IGameClient) || !soldier->getOccupingPlayer()->getIsRemote())
            disablePhysics(object, false);
        soldier->setLadderId(0);
    }
}

// copy the position, then clamp its y into [bottom + ladder-bottom-marginal,
// top] after dropping it by 1.0 (the soldier's eye-to-feet) - `min(max(y - 1, bottom + m), top)`,
// evaluated as the ELF does: upper = min(y - 1, top); result = max(upper, bottom + m).
Vec3 Ladder::snapSoldierPosToLadder(const Vec3& position)
{
    Vec3 out = position;
    const float lower = getAbsoluteBottom() + *ladderBottomMarginal;
    const float dropped = position.y - 1.0f;
    const float top = getAbsoluteTop();
    const float upper = (dropped > top) ? top : dropped;
    out.y = (upper > lower) ? upper : lower;
    return out;
}

// . Geometry LOD 2 supplies the vertical span. Terrain, water and a line
// test may only raise the usable bottom; bottomReal deliberately keeps the mesh-derived value.
void Ladder::recalcBounds()
{
    const LocalBoundingBox* bounds = static_cast<const LocalBoundingBox*>(
        static_cast<ICollisionMesh*>(m_collisionMesh)->getBoundingBox(2));
    const Vec3& position = getAbsolutePosition();

    m_ladderHeight = bounds->m_max.y - bounds->m_min.y;
    m_absoluteTop = position.y + bounds->m_max.y;
    m_absoluteBottomReal = getAbsolutePosition().y + bounds->m_min.y;
    m_absoluteBottom = m_absoluteBottomReal;

    IHeightmapCluster* cluster = static_cast<IHeightmapCluster*>(heightmapCluster);
    const float terrain = cluster->getHeightInWorldCoords(position.x, position.z);
    const float water = cluster->getWaterLevelInWorldCoords(position.x, position.z) - *ladderWaterLimit;
    float floorHeight = water > terrain ? water : terrain;
    if (floorHeight < m_absoluteBottom)
        floorHeight = m_absoluteBottom;
    m_absoluteBottom = floorHeight;

    const Mat4& transform = getAbsoluteTransformation();
    Vec3 offset = { transform.m[8] * *ladderSoldierDist,
                    transform.m[9] * *ladderSoldierDist,
                    transform.m[10] * *ladderSoldierDist };
    Vec3 from = { transform.m[12] - offset.x, m_absoluteTop,
                  transform.m[14] - offset.z };
    Vec3 to = { transform.m[12] - offset.x, m_absoluteBottom,
                transform.m[14] - offset.z };
    Vec3 direction = { to.x - from.x, to.y - from.y, to.z - from.z };

    ObjectFlagPredicatorEx predicator(0x100, 8);
    IObject* hitObject = 0;
    Vec3 hitPoint = { 0.0f, 0.0f, 0.0f };
    Vec3 hitNormal = { 0.0f, 0.0f, 0.0f };
    int hitPart = 0;
    objectManager->setOverrideLineTestLod(2);
    objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                                 from, direction, &predicator, true, false, false, true, 0);
    objectManager->setOverrideLineTestLod(-1);

    if (hitObject)
    {
        if ((hitObject->getTemplate()->getSpecialMaterials() & 2) != 0)
        {
            const float candidate = hitPoint.y - *ladderWaterLimit;
            if (candidate > m_absoluteBottom)
                m_absoluteBottom = candidate;
        }
        else
        {
            m_absoluteBottom = hitPoint.y + 0.01f;
        }
    }
    m_boundsDirty = false;
}

// . Values 1/2/3 are consumed directly as LadderExitType by the container.
LadderExitType Ladder::checkLimits(const Mat4& transform, bool climbingUp, bool climbingDown)
{
    LadderExitType result = LadderExitNone;
    if (transform.m[13] > getAbsoluteTop() - *ladderOffsetTop)
        result = climbingUp ? LadderExitTop : LadderExitNone;
    if (getAbsoluteBottom() - *ladderBottomMarginal > transform.m[13] && climbingDown)
        result = LadderExitBottom;

    IHeightmapCluster* cluster = static_cast<IHeightmapCluster*>(heightmapCluster);
    const float terrain = cluster->getHeightInWorldCoords(transform.m[12], transform.m[14]);
    float surface = cluster->getWaterLevelInWorldCoords(transform.m[12], transform.m[14])
                  - *ladderWaterLimit;
    if (surface <= terrain)
        surface = terrain;
    if (surface > transform.m[13])
        return LadderExitSide;
    return result;
}

// Named class and inheritance are emitted in the ELF RTTI/vtable. It excludes soldiers from the
// 0x108 collision mask before applying ObjectFlagPredicator's ordinary flag test.
class LadderCollisionPredicator : public ObjectFlagPredicator
{
public:
    explicit LadderCollisionPredicator(unsigned int flags) : ObjectFlagPredicator(flags) {}
    virtual bool predicate(IObject* object)
    {
        if (object->queryInterface(IID_ISoldier))
            return false;
        return ObjectFlagPredicator::predicate(object);
    }
};

// . Two short probes protect downward/upward travel; near the top, a stand-pose test may
// suppress the fallback clearance ray. The function-static outputs/matrix are intentional retail
// state (including the matrix capturing the first ladder's rotation only once).
int Ladder::checkCollision(const Mat4& transform, LadderContainer* container)
{
    static Vec3 hitPoint = { 0.0f, 0.0f, 0.0f };
    static Vec3 hitNormal = { 0.0f, 0.0f, 0.0f };

    int result = 0;
    Vec3 down = { 0.0f,
                  -*(container && container->m_skidding ? ladderDownRayMagSkid : ladderDownRayMag),
                  0.0f };
    const Vec3 start = { transform.m[12], transform.m[13], transform.m[14] };
    Vec3 away = { -transform.m[8], -transform.m[9], -transform.m[10] };
    normalizeLikeEngine(away);
    away.x *= *ladderExitOffset;
    away.y *= *ladderExitOffset;
    away.z *= *ladderExitOffset;

    LadderCollisionPredicator predicator(0x108);
    IObject* hitObject = 0;
    int hitPart = 0;
    objectManager->setOverrideLineTestLod(2);

    if (!container || container->m_climbingDown || container->m_skidding || *ladderRayDebug)
    {
        Vec3 from = { start.x + away.x, start.y + away.y, start.z + away.z };
        if (objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                                         from, down, &predicator,
                                         true, false, false, true, 0) ||
            objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                                         start, away, &predicator,
                                         true, false, false, true, 0))
            result = 1;

        if (*ladderRayDebug)
        {
            Vec3 debugEnd = { from.x + down.x, from.y + down.y, from.z + down.z };
            (void)debugEnd;
        }
    }

    if (!container || container->m_climbingUp || *ladderRayDebug)
    {
        Vec3 rayStart = { start.x + away.x, start.y + away.y, start.z + away.z };
        if (container)
            rayStart.y += *ladderUpRayMagSkid;
        Vec3 target = { start.x + away.x,
                        start.y + away.y + *ladderUpRayMagSkid + 0.2f,
                        start.z + away.z };
        Vec3 direction = { target.x - rayStart.x, target.y - rayStart.y,
                           target.z - rayStart.z };
        if (!objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                                          rayStart, direction, &predicator,
                                          true, false, false, true, 0))
        {
            Vec3 secondStart = { start.x, start.y + *ladderUpRayMagSkid, start.z };
            objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                                         secondStart, away, &predicator,
                                         true, false, false, true, 0);
        }

        if (hitObject)
        {
            result = 2;
        }
        else
        {
            const float distanceToTop = getAbsoluteTop() - start.y;
            if (*ladderSoldierStartGetoff > 0.0f &&
                *ladderSoldierStartGetoff > distanceToTop)
            {
                bool validTopExit = false;
                if (container && container->getOccupingObject())
                {
                    ISoldier* soldier = static_cast<ISoldier*>(
                        container->getOccupingObject()->queryInterface(IID_ISoldier));
                    static Mat4 topExitTransform = getAbsoluteTransformation();
                    const Vec3 topExit = LadderContainer::_getExitPosition(
                        this, container->getAbsolutePosition(), LadderExitTop);
                    topExitTransform.m[12] = topExit.x;
                    topExitTransform.m[13] = topExit.y;
                    topExitTransform.m[14] = topExit.z;
                    validTopExit = soldier->testIfValidTransformation(
                        SoldierPose_Stand, topExitTransform, true);
                }

                if (!validTopExit)
                {
                    Vec3 fallback = getAbsolutePosition();
                    float factor = 1.0f - distanceToTop / *ladderSoldierStartGetoff;
                    if (factor <= *ladderGetoffRayMin)
                        factor = *ladderGetoffRayMin;
                    fallback.y = getAbsoluteTop() + factor;

                    const Mat4& ladderTransform = getAbsoluteTransformation();
                    const float scale = factor * *ladderTopExitOffset;
                    Vec3 scaledForward = { ladderTransform.m[8] * scale,
                                           ladderTransform.m[9] * scale,
                                           ladderTransform.m[10] * scale };
                    Vec3 fallbackDirection = { scaledForward.x - away.x,
                                               scaledForward.y - away.y,
                                               scaledForward.z - away.z };
                    if (objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                                                     fallback, fallbackDirection, &predicator,
                                                     true, false, false, true, 0))
                        result = 2;

                    if (*ladderRayDebug)
                    {
                        Vec3 debugA = { fallback.x + away.x, fallback.y + away.y,
                                        fallback.z + away.z };
                        Vec3 debugB = { fallback.x + scaledForward.x,
                                        fallback.y + scaledForward.y,
                                        fallback.z + scaledForward.z };
                        (void)debugA; (void)debugB;
                    }
                }
            }
        }
    }

    objectManager->setOverrideLineTestLod(-1);
    return result;
}

// . A collision only mounts a soldier when facing, camera pitch, limits,
// clearance and stand-pose validation all agree. Every rejection delegates to SimpleObject.
bool Ladder::handleCollision(IObject* other, const Vec3& a, const Vec3& b, const Vec3& c,
                             int d, int e, CollisionFaceInfo* f, CollisionFaceInfo* g)
{
    ISoldier* soldier = static_cast<ISoldier*>(other->queryInterface(IID_ISoldier));
    if (!soldier || getAbsoluteBottom() > other->getAbsolutePosition().y)
        return SimpleObject::handleCollision(other, a, b, c, d, e, f, g);

    Mat4 candidate = other->getAbsoluteTransformation();
    Vec3 snappedInput = { candidate.m[12], candidate.m[13], candidate.m[14] };
    const Vec3 snapped = snapSoldierPosToLadder(snappedInput);
    candidate.m[12] = getAbsolutePosition().x;
    candidate.m[13] = snapped.y;
    candidate.m[14] = getAbsolutePosition().z;

    IPlayerControlObject* pco = static_cast<IPlayerControlObject*>(
        other->queryInterface(IID_IPlayerControlObject));
    if (!pco->getActiveCamera() || soldier->getIsSkyDiving())
        return SimpleObject::handleCollision(other, a, b, c, d, e, f, g);

    IObject* cameraObject = static_cast<IObject*>(
        pco->getActiveCamera()->queryInterface(IID_IObject));
    const Mat4& ladderTransform = getAbsoluteTransformation();
    const Mat4& soldierTransform = other->getAbsoluteTransformation();
    Vec3 ladderForward = { ladderTransform.m[8], ladderTransform.m[9], ladderTransform.m[10] };
    Vec3 soldierForward = { soldierTransform.m[8], soldierTransform.m[9], soldierTransform.m[10] };

    float cameraZ, cameraX, cameraY;
    getRotationZXY(cameraObject->getAbsoluteTransformation(), cameraZ, cameraX, cameraY);
    const float cameraPitch = -cameraX;

    const Vec3& soldierPosition = other->getAbsolutePosition();
    const Vec3& ladderPosition = getAbsolutePosition();
    Vec3 toLadder = { ladderPosition.x - soldierPosition.x,
                      ladderPosition.y - soldierPosition.y,
                      ladderPosition.z - soldierPosition.z };
    normalizeLikeEngine(toLadder);

    const bool fromFront = dot(toLadder, ladderForward) > 0.0f;
    const float facing = dot(ladderForward, soldierForward);
    float pitchSign = cameraPitch > 0.0f ? 1.0f : 0.0f;
    if (cameraPitch < 0.0f)
        pitchSign = -1.0f;
    const bool nearTop = candidate.m[13] > getAbsoluteTop() - 1.2f;

    if ((facing < 0.5f && fromFront) || (facing > -0.5f && !fromFront))
        return SimpleObject::handleCollision(other, a, b, c, d, e, f, g);
    if (!nearTop)
    {
        if (!(-pitchSign > 0.0f))
            return SimpleObject::handleCollision(other, a, b, c, d, e, f, g);
    }
    else if (*ladderDownAngleLimit > cameraPitch)
    {
        return SimpleObject::handleCollision(other, a, b, c, d, e, f, g);
    }

    if (!pco->getOccupingPlayer() ||
        (!nearTop && checkLimits(candidate, false, false) != LadderExitNone))
        return SimpleObject::handleCollision(other, a, b, c, d, e, f, g);
    if (checkCollision(candidate, 0) > 0)
        return SimpleObject::handleCollision(other, a, b, c, d, e, f, g);

    const Vec3 exitPosition = LadderContainer::_getExitPosition(this,
        Vec3{ candidate.m[12], candidate.m[13], candidate.m[14] }, LadderExitSide);
    candidate.m[12] = exitPosition.x;
    candidate.m[13] = exitPosition.y;
    candidate.m[14] = exitPosition.z;
    if (!soldier->testIfValidTransformation(SoldierPose_Stand, candidate, true) ||
        *ladderRayDebug2 > 0)
        return SimpleObject::handleCollision(other, a, b, c, d, e, f, g);

    attachSoldier(soldier);
    return false;
}

} // namespace world
} // namespace hfe
} // namespace dice
