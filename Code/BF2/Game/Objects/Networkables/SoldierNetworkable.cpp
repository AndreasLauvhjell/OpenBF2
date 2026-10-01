// Soldier replication, prediction and ragdoll state. Ragdoll payload replaces the
// normal state tail; reduced precision omits several owner-only fields.
// Ladder and zipline ids must resolve: map lookup inserts a null entry for unknown
// ids and state access does not guard that result. The send ring advances directly;
// only receive updates maintain the count used by prediction.

#include "Networkables.h"

#include <cmath>
#include <cstdio>
#include <map>
#include <vector>

#include "../Game/Soldier.h"                  // world::Soldier (a friend of this class), ISoldier
#include "../Game/SoldierTemplate.h"          // getInventorySize - the itemIndex wire width
#include "../Game/Ladder.h"                   // world::ladderMap + the state pair
#include "../Game/Zipline.h"                  // world::ziplineMap + recovered state pair
#include "../GameLogicComp/ArmorGLComp.h"     // hit points / damage type
#include "../IItemObject.h"                   // the remote item-switch apply arm
#include "../IWeaponObject.h"
#include "../IPlayer.h"                       // setFlags/getIsRemote/getCamera + g_playerFlagBit3
#include "../ICameraObject.h"                 // the on-ladder look override reads the camera
#include "../ObjectLayoutAccess.h"            // the sanctioned raw reads (+0x28/+0x3C/+0x44/+0x48)
#include "../../../Physics/SoldierPhysicsNode.h"   // the four replicated physics vectors
#include "../../../Physics/SoldierResponse.h"      // m_byteFA (a friend grants the raw access)
#include "../../../Animation/BoneAnimation/RagDollTemplate.h"   // anim::RagDoll - the bit-15 arm
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../../IO/BitStream.h"
#include "../../../IO/Network/NetworkManager.h"    // BaseLineData, StatePrecisionFull, networkManager
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"         // formatString - the Debug payload, as shipped
#include "../../../Math/Mat4.h"               // getIdentity, get/setRotationZXY
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

// Zero reference vectors for soldier speed compression and per-component network statistics.
namespace {

Vec3 zeroVec3()
{
    Vec3 v; v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
    return v;
}

Vec3 s_unusedVec924 = zeroVec3();               // ds:0x8AAE924 [UNRESOLVED] - kept, unread
Vec3 s_speedCompressionVector = zeroVec3();     // ds:0x8AAE930 - the zero reference

Vec3 soldierStatsColour()
{
    Vec3 c; c.x = 250.0f; c.y = 0.0f; c.z = 0.0f;
    return c;
}

} // namespace

io::RegistredNetworkStats SoldierNetworkable::m_stats("SoldierNetworkable", soldierStatsColour());

// Typed weapon accessors; callers guard the active weapon before using them.
namespace {
inline int  weaponObjectGetZoom(void* w)        { return static_cast<world::IWeaponObject*>(w)->getZoom(); }
inline int  weaponObjectGetWeaponState(void* w) { return static_cast<world::IWeaponObject*>(w)->getWeaponState(); }
inline void weaponObjectSetZoom(void* w, int zoom)              { static_cast<world::IWeaponObject*>(w)->setZoom(zoom); }
inline void weaponObjectSetWeaponState(void* w, unsigned int s) { static_cast<world::IWeaponObject*>(w)->setWeaponState(s); }
} // namespace

// The two anonymous-namespace transform helpers. The ELF's mangled names put both inside
// dice::hfe's anonymous namespace in this TU (_ZN4dice3hfe52_GLOBAL__N_...), keyed to m_stats.
// setSoldierTransformation(soldier, m, updatePrev) - 0x82476F0, 113 B: relative when parented,
// absolute otherwise; when updatePrev is set, the physics node's previous transformation is
// pinned too so the interpolator does not lerp across a teleport.
// predict_setSoldierTransformation(soldier, m) - 0x8247770, 86 B: the same without the flag -
// prediction always pins the previous transformation.
namespace {

void setSoldierTransformation(world::Soldier* soldier, const Mat4& m, bool updatePrev)
{
    typedef world::object_detail::LayoutAccess L;
    if (L::parent(soldier))
        soldier->setRelativeTransformation(m);
    else
        soldier->setAbsoluteTransformation(m);
    if (updatePrev)
    {
        world::SoldierPhysicsNode* const phys =
            static_cast<world::SoldierPhysicsNode*>(L::physicsNode(soldier));
        if (phys)
            phys->setPrevTransformation(m);
    }
}

void predict_setSoldierTransformation(world::Soldier* soldier, const Mat4& m)
{
    typedef world::object_detail::LayoutAccess L;
    if (L::parent(soldier))
        soldier->setRelativeTransformation(m);
    else
        soldier->setAbsoluteTransformation(m);
    world::SoldierPhysicsNode* const phys =
        static_cast<world::SoldierPhysicsNode*>(L::physicsNode(soldier));
    if (phys)
        phys->setPrevTransformation(m);
}

// The identity-with-translation matrix every teleport/predict arm builds: sixteen zeroed dwords,
// the diagonal seeded from Mat4::getIdentity, translation overwritten.
Mat4 identityAt(const Vec3& translation)
{
    Mat4 m = Mat4::getIdentity();
    m.m[12] = translation.x;
    m.m[13] = translation.y;
    m.m[14] = translation.z;
    return m;
}

} // namespace

// Leaves sprinting, skydiving, swimming, weapon-stowed, and ammo-status fields uninitialized.
SoldierNetworkable::UPDATE::UPDATE()
{
    time = 0.0f;
    used = 0;
    absoluteTrans.x = 0.0f;          absoluteTrans.y = 0.0f;          absoluteTrans.z = 0.0f;
    positionalSpeed.x = 0.0f;        positionalSpeed.y = 0.0f;        positionalSpeed.z = 0.0f;
    positionalAcceleration.x = 0.0f; positionalAcceleration.y = 0.0f; positionalAcceleration.z = 0.0f;
    positionalFriction.x = 0.0f;     positionalFriction.y = 0.0f;     positionalFriction.z = 0.0f;
    localLinearSpeed.x = 0.0f;       localLinearSpeed.y = 0.0f;       localLinearSpeed.z = 0.0f;
    facingDegrees = 0.0f;
    lookLeftRightAngle = 0.0f;
    pendingTurn = 0.0f;
    lookUpDownAngle = 0.0f;
    pose = 0;
    requestedPose = 0;
    hitPoints = 0;
    lastDamageType = 0;
    smoothedForward = 0.0f;
    smoothedStrafe = 0.0f;
    itemIndex = 0;
    poseInputLatch = 0;
    currentAbility = 0.0f;
    sprintStamina = 0.0f;
    // +0x7C..+0x7F deliberately untouched - see the block above.
    weaponZoom = 0;
    weaponState = 0;
    ladderId = 0;
    ladderState = 0.0f;
    ziplineId = 0;
    ziplineState = 0.0f;
    responseByteFA = 0;
    // +0x9C (ammoStatus) deliberately untouched.
}

// ctor - 0x8245F90/0x8245FF0, 82 B. NetworkableBase's ctor, the second base's four fields, both
// vptrs, refcount 1, null ring. The ELF initialises the second base's +0x48 with a byte store
// (the other fifteen classes use a dword there) - dice hand-wrote these four stores rather than
// delegating, and treated +0x48 as a bool. Under this tree's NetworkableObjectBase() the slot is
// a dword write of 0; the three high bytes retail leaves as heap residue are zeroed here instead.
// Nothing recovered reads them.
SoldierNetworkable::SoldierNetworkable()
{
    m_refCount = 1;                                  // +0x50
    m_updates  = 0;                                  // +0x54
}

// create - 0x8779BA0, 89 B (the registry TU's factory, kept beside its class like the others).
IBase* SoldierNetworkable::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    SoldierNetworkable* self = new SoldierNetworkable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// D2/D1 0x8246050/90 (58 B), D0 0x82460D0: revector, delete[] the ring (null-tested, NOT nulled -
// unlike DestroyableObjectNetworkable's, which stores 0 back), chain ~NetworkableBase.
SoldierNetworkable::~SoldierNetworkable()
{
    if (m_updates)
        delete[] m_updates;
}

void  SoldierNetworkable::addRef() { ++m_refCount; }
int   SoldierNetworkable::getRef() { return m_refCount; }
int   SoldierNetworkable::release()
{
    const bool last = (m_refCount == 1);
    const int  r    = --m_refCount;
    if (last)
        delete this;
    return r;
}

// . THE check order IS THE ELF'S: IBase, then INetworkableObject, then
// INetworkable - not DestroyableObjectNetworkable's merged first test.
void* SoldierNetworkable::queryInterface(unsigned int iid) const
{
    SoldierNetworkable* self = const_cast<SoldierNetworkable*>(this);
    if (iid == IID_IBase)
        return static_cast<io::INetworkable*>(self);
    if (iid == io::IID_INetworkableObject)
        return static_cast<io::INetworkableObject*>(self);
    if (iid == io::IID_INetworkable)
        return static_cast<io::INetworkable*>(self);
    return 0;
}

// init - 0x82461A0, 119 B. Bind the soldier (+0x44), cache the +0x18 interface, build the ring:
// operator new[](0x280) and four explicit UPDATE constructions (stride 0xA0). No reset() call,
// no field priming beyond the ctor's partial write set, and the return is a constant 1.
bool SoldierNetworkable::init(void* object)
{
    m_object = static_cast<world::IObject*>(object);                              // +0x44
    m_networkableObject =
        static_cast<io::INetworkableObject*>(queryInterface(io::IID_INetworkableObject));   // +0x18
    m_updates = new UPDATE[4];                                                    // +0x54 0x280 B
    return true;
}

// Invalidates all four records before base reset; a null ring skips both operations.
void SoldierNetworkable::reset()
{
    if (!m_updates)
        return;
    for (unsigned int slot = 0; slot <= 3; ++slot)
        m_updates[slot].used = 0;
    io::NetworkableBase::reset();
}

// Tests baseline capacity without writing any data.
bool SoldierNetworkable::getBaseLine(io::BaseLineData& baseLineData)
{
    return static_cast<unsigned int>(baseLineData.m_cursor) + 1 < 513;
}

// bits 0..7, 12, 14, 16, 19, 20.
unsigned int SoldierNetworkable::getGhostStateMask() const
{
    return 0x1950FF;
}

unsigned int SoldierNetworkable::getMinimumBitSize() const
{
    return 100;
}

// newest minus oldest over the count; the ring pointer is NOT null-tested
// (m_count is only ever nonzero after setNetUpdate ran, which implies init did).
float SoldierNetworkable::getAverageUpdateDeltaTime() const
{
    if (m_count == 0)
        return 0.0f;
    return (m_updates[m_head].time - m_updates[m_tail].time) / static_cast<float>(m_count);
}

// / 0x8246140, 47 B: the ring accessors the wire pair goes through
// (NetworkableBase.h's "helper shape"). Null ring or slot > 3 answer 0; MarkUsed also raises the
// slot's valid flag.
SoldierNetworkable::UPDATE* SoldierNetworkable::getUpdate(unsigned int slot)
{
    if (!m_updates || slot > 3)
        return 0;
    return &m_updates[slot];
}

SoldierNetworkable::UPDATE* SoldierNetworkable::getUpdateAndMarkUsed(unsigned int slot)
{
    if (!m_updates || slot > 3)
        return 0;
    UPDATE* const u = &m_updates[slot];
    u->used = 1;
    return u;
}

// . NO guards AT ALL - a null ring or an out-of-range slot dereferences anyway,
// exactly as shipped.
bool SoldierNetworkable::getIsUpdateValid(unsigned int slot)
{
    return m_updates[slot].used != 0;
}

// findEmbracingIndices - 0x824BFF0, 151 B - the house ring search (RotationalBundleNetworkable and
// SimpleObjectNetworkable carry byte-identical logic over their strides): walk forward from the
// oldest slot until a record time reaches t; *lo trails, *hi leads. The second `count <= 1`
// test after seeding lo/hi is the ELF's own redundancy, kept.
bool SoldierNetworkable::findEmbracingIndices(float t, unsigned int* lo, unsigned int* hi) const
{
    if (static_cast<unsigned int>(m_count) <= 1u)
        return false;
    const unsigned int tail = static_cast<unsigned int>(m_tail);
    if (m_updates[tail].time > t)
        return false;
    *lo = tail;
    *hi = tail;
    if (static_cast<unsigned int>(m_count) <= 1u)
        return false;
    for (unsigned int step = 1;;)
    {
        const unsigned int idx = (step + static_cast<unsigned int>(m_tail)) & 3;
        *hi = idx;
        if (m_updates[idx].time >= t)
            return true;
        ++step;
        *lo = idx;
        if (step >= static_cast<unsigned int>(m_count))
            return false;
    }
}

// dice'S OWN name, mangled and all. Two direct (non-virtual) Soldier calls.
void SoldierNetworkable::setSoldierStateHACK()
{
    world::Soldier* const soldier = static_cast<world::Soldier*>(m_object);
    soldier->updateTransformation();
    soldier->updateCameraTransformation();
}

// Sets this owner ID and propagates it to every weapon networkable.
// Requires non-null weapon entries; preserves repeated networkable lookups.
void SoldierNetworkable::setOwnerID(int ownerId)
{
    m_ownerId = ownerId;                                                          // +0x40
    world::Soldier* const soldier = static_cast<world::Soldier*>(m_object);
    std::vector<world::IObject*>* const weapons =
        static_cast<std::vector<world::IObject*>*>(soldier->getWeapons());
    for (std::vector<world::IObject*>::iterator it = weapons->begin(); it != weapons->end(); ++it)
    {
        world::IObject* const weapon = *it;
        if (weapon->getNetworkable())                                             // call one
        {
            io::INetworkable* const net = weapon->getNetworkable();               // call two
            io::INetworkableObject* const obj = net->m_networkableObject;         // +0x18 raw
            if (obj)
                obj->setOwnerID(ownerId);
        }
    }
}

// Snapshots soldier state and marks fields changed since the previous baseline.
// An invalid previous record marks all fields dirty; ladder look state comes from the occupant's camera.
void SoldierNetworkable::updateStateMask(unsigned int* stateMask)
{
    typedef world::object_detail::LayoutAccess L;
    world::Soldier* const soldier = static_cast<world::Soldier*>(m_object);
    const float eps = 1.1920929e-7f;                       // FLT_EPSILON, the ELF's constant

    m_prev = m_head;                                       // +0x34
    m_head = (m_head + 1) & 3;                             // +0x28
    m_next = m_head;                                       // +0x38

    UPDATE* const rec  = getUpdateAndMarkUsed(static_cast<unsigned int>(m_head));
    UPDATE* const prev = getUpdate(static_cast<unsigned int>(m_prev));

    // the root-only block: transform and physics
    if (!L::parent(soldier))
    {
        const Mat4& abs = soldier->getAbsoluteTransformation();
        bool same = false;
        if (prev->used)
            same = std::fabs(prev->absoluteTrans.x - abs.m[12]) <= eps
                && std::fabs(prev->absoluteTrans.y - abs.m[13]) <= eps
                && std::fabs(prev->absoluteTrans.z - abs.m[14]) <= eps;
        if (same)
        {
            rec->absoluteTrans = prev->absoluteTrans;
        }
        else
        {
            *stateMask |= 0x1;
            rec->absoluteTrans.x = abs.m[12];
            rec->absoluteTrans.y = abs.m[13];
            rec->absoluteTrans.z = abs.m[14];
        }

        world::SoldierPhysicsNode* const phys =
            static_cast<world::SoldierPhysicsNode*>(L::physicsNode(soldier));
        if (phys)
        {
            const Vec3& speed = phys->getPositionalSpeed();
            same = false;
            if (prev->used)
                same = std::fabs(prev->positionalSpeed.x - speed.x) <= eps
                    && std::fabs(prev->positionalSpeed.y - speed.y) <= eps
                    && std::fabs(prev->positionalSpeed.z - speed.z) <= eps;
            if (same) rec->positionalSpeed = prev->positionalSpeed;
            else      { *stateMask |= 0x80; rec->positionalSpeed = speed; }

            const Vec3& accel = phys->getPositionalAcceleration();
            same = false;
            if (prev->used)
                same = std::fabs(prev->positionalAcceleration.x - accel.x) <= eps
                    && std::fabs(prev->positionalAcceleration.y - accel.y) <= eps
                    && std::fabs(prev->positionalAcceleration.z - accel.z) <= eps;
            if (same) rec->positionalAcceleration = prev->positionalAcceleration;
            else      { *stateMask |= 0x100; rec->positionalAcceleration = accel; }

            const Vec3& friction = phys->getPositionalFriction();
            same = false;
            if (prev->used)
                same = std::fabs(prev->positionalFriction.x - friction.x) <= eps
                    && std::fabs(prev->positionalFriction.y - friction.y) <= eps
                    && std::fabs(prev->positionalFriction.z - friction.z) <= eps;
            if (same) rec->positionalFriction = prev->positionalFriction;
            else      { *stateMask |= 0x200; rec->positionalFriction = friction; }

            const Vec3& linear = phys->getLocalLinearSpeed();
            same = false;
            if (prev->used)
                same = std::fabs(prev->localLinearSpeed.x - linear.x) <= eps
                    && std::fabs(prev->localLinearSpeed.y - linear.y) <= eps
                    && std::fabs(prev->localLinearSpeed.z - linear.z) <= eps;
            if (same) rec->localLinearSpeed = prev->localLinearSpeed;
            else      { *stateMask |= 0x40000; rec->localLinearSpeed = linear; }
        }

        if (prev->used && prev->smoothedForward == soldier->m_smoothedForward)
            rec->smoothedForward = prev->smoothedForward;
        else { *stateMask |= 0x400; rec->smoothedForward = soldier->m_smoothedForward; }

        if (prev->used && prev->smoothedStrafe == soldier->m_smoothedStrafe)
            rec->smoothedStrafe = prev->smoothedStrafe;
        else { *stateMask |= 0x800; rec->smoothedStrafe = soldier->m_smoothedStrafe; }

        world::SoldierResponsePhysics* const response =
            static_cast<world::SoldierResponsePhysics*>(L::responsePhysics(soldier));
        if (response)
            rec->responseByteFA = response->m_byteFA;      // no bit - always carried
    }

    // sprint (through the PCO interface)
    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        soldier->queryInterface(world::IID_IPlayerControlObject));
    if (pco)
    {
        const float stamina = pco->getSprintStamina();
        if (prev->used && prev->sprintStamina == stamina)
            rec->sprintStamina = prev->sprintStamina;
        else { *stateMask |= 0x4000; rec->sprintStamina = stamina; }

        const unsigned char sprinting = pco->getIsSprinting() ? 1 : 0;
        if (prev->used && sprinting == prev->isSprinting)
            rec->isSprinting = prev->isSprinting;
        else { *stateMask |= 0x4000; rec->isSprinting = sprinting; }
    }

    // the look/pose scalars
    if (prev->used && prev->facingDegrees == soldier->m_facingDegrees)
        rec->facingDegrees = prev->facingDegrees;
    else { *stateMask |= 0x2; rec->facingDegrees = soldier->m_facingDegrees; }

    if (prev->used && prev->lookLeftRightAngle == soldier->m_lookLeftRightAngle)
        rec->lookLeftRightAngle = prev->lookLeftRightAngle;
    else { *stateMask |= 0x4; rec->lookLeftRightAngle = soldier->m_lookLeftRightAngle; }

    if (prev->used && prev->pendingTurn == soldier->m_float214)
        rec->pendingTurn = prev->pendingTurn;
    else { *stateMask |= 0x8; rec->pendingTurn = soldier->m_float214; }

    if (prev->used && prev->lookUpDownAngle == soldier->m_lookUpDownAngle)
        rec->lookUpDownAngle = prev->lookUpDownAngle;
    else { *stateMask |= 0x10; rec->lookUpDownAngle = soldier->m_lookUpDownAngle; }

    if (prev->used && prev->pose == soldier->m_pose)
        rec->pose = prev->pose;
    else { *stateMask |= 0x20; rec->pose = soldier->m_pose; }

    if (prev->used && prev->requestedPose == soldier->m_requestedPose)
        rec->requestedPose = prev->requestedPose;
    else { *stateMask |= 0x20; rec->requestedPose = soldier->m_requestedPose; }

    // weapon zoom/state - always snapshotted, never a bit. pco used unguarded.
    world::IObject* const activeWeapon = pco->getActiveWeapon(0);
    void* weaponObject = 0;
    if (activeWeapon)
        weaponObject = activeWeapon->queryInterface(world::IID_IWeaponObject);
    if (activeWeapon && weaponObject)
    {
        rec->weaponZoom  = weaponObjectGetZoom(weaponObject);          // IWeaponObject +0x108
        rec->weaponState = weaponObjectGetWeaponState(weaponObject);   // IWeaponObject +0x164
    }
    else
    {
        rec->weaponZoom  = 0;
        rec->weaponState = 0;
    }

    rec->isSkyDiving = soldier->m_isSkyDiving ? 1 : 0;     // no bit - always carried
    rec->isSwimming  = soldier->m_isSwimming ? 1 : 0;

    if (prev->used && soldier->m_weaponStowed == (prev->weaponStowed != 0))
        rec->weaponStowed = prev->weaponStowed;
    else { *stateMask |= 0x1000; rec->weaponStowed = soldier->m_weaponStowed ? 1 : 0; }

    // armor: quantised hit points + damage type
    world::ArmorGLComp* const armor =
        static_cast<world::ArmorGLComp*>(L::armorComponent(soldier));
    if (armor)
    {
        float hp = armor->getHitPoints();
        if (hp <= 0.0f)
            hp = 0.0f;
        const int quantised = static_cast<int>(
            std::ceil(hp) / armor->getMaxHitPoints() * 255.0f);
        if (prev->used && prev->hitPoints == quantised)
            rec->hitPoints = prev->hitPoints;
        else { *stateMask |= 0x40; rec->hitPoints = quantised; }
        rec->lastDamageType = armor->getLastDamageType();  // no bit - rides on bit 6
    }
    else
    {
        rec->hitPoints = 0;
        rec->lastDamageType = 0;
    }

    // item index (shares bit 12 with weaponStowed)
    if (prev->used && prev->itemIndex == soldier->m_activeItemIndex)
        rec->itemIndex = prev->itemIndex;
    else { *stateMask |= 0x1000; rec->itemIndex = soldier->m_activeItemIndex; }

    if (prev->used && prev->poseInputLatch == soldier->m_poseInputLatch)
        rec->poseInputLatch = prev->poseInputLatch;
    else { *stateMask |= 0x2000; rec->poseInputLatch = soldier->m_poseInputLatch; }

    // ladder
    const unsigned int ladderId = soldier->getLadderId();
    if (prev->used && prev->ladderId == ladderId)
        rec->ladderId = prev->ladderId;
    else { *stateMask |= 0x10000; rec->ladderId = ladderId; }

    if (soldier->getLadderId())
    {
        // operator[] inserts a null Ladder* for an unknown id, and getCurrentState is then
        // called on that null pointer with no guard. Shipped shape.
        world::Ladder* const ladder = world::ladderMap[static_cast<int>(rec->ladderId)];
        const float state = ladder->getCurrentState(soldier);
        if (prev->used && prev->ladderState == state)
            rec->ladderState = prev->ladderState;
        else { *stateMask |= 0x10000; rec->ladderState = state; }

        // On ladders, derive pending turn from the occupant's camera rotation.
        // Preserve repeated occupant/camera lookups.
        if (soldier->getOccupingPlayer())
        {
            world::IPlayer* const occ1 = soldier->getOccupingPlayer();
            if (occ1->getCamera())
            {
                world::IPlayer* const occ2 = soldier->getOccupingPlayer();
                world::ICameraObject* const camera = occ2->getCamera();
                world::IObject* const cameraObject = static_cast<world::IObject*>(
                    camera->queryInterface(world::IID_IObject));
                // Use the camera's relative transformation.
                const Mat4& cameraRel = cameraObject->getRelativeTransformation();
                float zDeg, xDeg, yDeg;
                getRotationZXY(cameraRel, zDeg, xDeg, yDeg);
                const float negY = -yDeg;
                const float negZ = -zDeg;                  // dead in the ELF - kept for the record
                const float negX = -xDeg;                  // dead in the ELF
                (void)negZ; (void)negX;
                if (prev->used && prev->pendingTurn == negY)
                    rec->pendingTurn = prev->pendingTurn;
                else { *stateMask |= 0x8; rec->pendingTurn = negY; }
            }
        }
    }

    // zipline
    const unsigned int ziplineId = soldier->getZiplineId();
    if (prev->used && prev->ziplineId == ziplineId)
        rec->ziplineId = prev->ziplineId;
    else { *stateMask |= 0x100000; rec->ziplineId = ziplineId; }

    if (soldier->getZiplineId())
    {
        world::Zipline* const zipline = world::ziplineMap[static_cast<int>(rec->ziplineId)];
        const float state = zipline->getCurrentState(soldier);        // null-this as shipped
        if (prev->used && prev->ziplineState == state)
            rec->ziplineState = prev->ziplineState;
        else { *stateMask |= 0x100000; rec->ziplineState = state; }
    }

    rec->currentAbility = soldier->getCurrentAbility();    // no bit - always carried

    // the force-full latch (soldier +0x37C): OR the whole ghost mask in, once
    if (soldier->m_byte37C)
    {
        *stateMask |= getGhostStateMask();                 // a virtual call in the ELF
        soldier->m_byte37C = 0;
    }

    // ammo
    const int ammo = soldier->getAmmoStatus();
    if (prev->used && prev->ammoStatus == ammo)
        rec->ammoStatus = prev->ammoStatus;
    else { *stateMask |= 0x80000; rec->ammoStatus = ammo; }

}

// Awake ragdolls on occupied soldiers force mask 0x8040; otherwise strip the ragdoll bit.
// Baseline data is unused.
bool SoldierNetworkable::getNetUpdate(io::BitStream& stream, io::BaseLineData* /*baseline*/,
                                      unsigned int stateMask, int precision)
{
    typedef world::object_detail::LayoutAccess L;
    world::Soldier* const soldier = static_cast<world::Soldier*>(m_object);

    m_updateStartBitPos = stream.getBitPosition();                    // +0x30
    m_next = m_head;                                                  // pin the record to send
    UPDATE* const rec = getUpdate(static_cast<unsigned int>(m_head));

    anim::RagDoll* const ragdoll = soldier->m_ragDoll;
    unsigned int mask = stateMask;
    if (ragdoll && ragdoll->m_awake && soldier->isOccupied())         // RagDoll +0x19: simulating
        mask = 0x8040;
    else
        mask &= ~0x8000u;

    stream.writeBits(&mask, 21);

    if (L::armorComponent(soldier))
    {
        if (mask & 0x40)
        {
            unsigned int hitPoints = static_cast<unsigned int>(rec->hitPoints);
            stream.writeBits(&hitPoints, 8);
        }
        if (mask & 0x40)
        {
            unsigned int lastDamageType = static_cast<unsigned int>(rec->lastDamageType);
            stream.writeBits(&lastDamageType, 1);
        }
    }

    // pose / requestedPose - two separate ifs on the same bit, each the inlined BitStream.inl:59
    // ranged write with bounds 0..4 (three bits; the house width loop).
    if (mask & 0x20)
    {
        const unsigned int pose = static_cast<unsigned int>(rec->pose);
        if (pose > 4)
        {
            stream.setError(4);
            Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
                << "(fatal error) Value out of bounds val: " << static_cast<int>(pose)
                << " lower " << 0 << " upper: " << 4;
        }
        else
        {
            unsigned int bits = 0;
            do
                ++bits;
            while ((1u << bits) - 1 < 5);
            unsigned int scratch = pose;
            stream.writeBits(&scratch, bits);
        }
    }
    if (mask & 0x20)
    {
        const unsigned int requestedPose = static_cast<unsigned int>(rec->requestedPose);
        if (requestedPose > 4)
        {
            stream.setError(4);
            Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
                << "(fatal error) Value out of bounds val: " << static_cast<int>(requestedPose)
                << " lower " << 0 << " upper: " << 4;
        }
        else
        {
            unsigned int bits = 0;
            do
                ++bits;
            while ((1u << bits) - 1 < 5);
            unsigned int scratch = requestedPose;
            stream.writeBits(&scratch, bits);
        }
    }

    if (mask & 0x8000u)
    {
        ragdoll->writeCurrentState(&stream);               // replaces the whole movement payload
    }
    else
    {
        // position
        if (mask & 0x1)
        {
            if (stream.hasCompressionVector())
            {
                stream.writeCompressedVector(rec->absoluteTrans, stream.getCompressionVector(),
                                             0.001f);
            }
            else
            {
                unsigned int selector = 0;                 // the hand-written selector-0 escape
                stream.writeBits(&selector, 2);
                stream.writeBits(&rec->absoluteTrans.x, 32);
                stream.writeBits(&rec->absoluteTrans.y, 32);
                stream.writeBits(&rec->absoluteTrans.z, 32);
            }
        }

        // the physics vectors - full precision sends four, reduced sends one coarser
        if (precision == io::StatePrecisionFull)
        {
            if (mask & 0x80)
                stream.writeCompressedVector(rec->positionalSpeed, s_speedCompressionVector, 0.001f);
            if (mask & 0x100)
                stream.writeCompressedVector(rec->positionalAcceleration, s_speedCompressionVector,
                                             0.000099999997f);
            if (mask & 0x200)
                stream.writeCompressedVector(rec->positionalFriction, s_speedCompressionVector,
                                             0.000099999997f);
            if (mask & 0x40000)
                stream.writeCompressedVector(rec->localLinearSpeed, s_speedCompressionVector, 0.001f);
        }
        else if (mask & 0x80)
        {
            stream.writeCompressedVector(rec->positionalSpeed, s_speedCompressionVector,
                                         0.010000001f);
        }

        // the look quartet: raw floats at full precision, 12-bit quantised otherwise
        if (precision == io::StatePrecisionFull)
        {
            if (mask & 0x2)
            {
                float v = rec->facingDegrees;
                stream.writeBits(&v, 32);
            }
            if (mask & 0x4)
            {
                float v = rec->lookLeftRightAngle;
                stream.writeBits(&v, 32);
            }
            if (mask & 0x8)
            {
                float v = rec->pendingTurn;
                stream.writeBits(&v, 32);
            }
            if (mask & 0x10)
            {
                float v = rec->lookUpDownAngle;
                stream.writeBits(&v, 32);
            }
        }
        else
        {
            if (mask & 0x2)
            {
                float n = (rec->facingDegrees / 360.0f + 1.0f) * 0.5f;
                if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
                unsigned int q = static_cast<unsigned int>(n * 4095.0f);
                stream.writeBits(&q, 12);
            }
            if (mask & 0x4)
            {
                float n = (rec->lookLeftRightAngle / 90.0f + 1.0f) * 0.5f;
                if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
                unsigned int q = static_cast<unsigned int>(n * 4095.0f);
                stream.writeBits(&q, 12);
            }
            if (mask & 0x8)
            {
                float n = (rec->pendingTurn / 180.0f + 1.0f) * 0.5f;
                if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
                unsigned int q = static_cast<unsigned int>(n * 4095.0f);
                stream.writeBits(&q, 12);
            }
            if (mask & 0x10)
            {
                float n = (rec->lookUpDownAngle / 90.0f + 1.0f) * 0.5f;
                if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
                unsigned int q = static_cast<unsigned int>(n * 4095.0f);
                stream.writeBits(&q, 12);
            }
        }

        // weapon zoom/state - unconditional ranged writes
        {
            const unsigned int weaponZoom = static_cast<unsigned int>(rec->weaponZoom);
            if (weaponZoom > 3)
            {
                stream.setError(4);
                Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
                    << "(fatal error) Value out of bounds val: " << static_cast<int>(weaponZoom)
                    << " lower " << 0 << " upper: " << 3;
            }
            else
            {
                unsigned int bits = 0;
                do
                    ++bits;
                while ((1u << bits) - 1 < 4);              // -> 3 bits, the house rounding
                unsigned int scratch = weaponZoom;
                stream.writeBits(&scratch, bits);
            }
        }
        {
            const unsigned int weaponState = static_cast<unsigned int>(rec->weaponState);
            if (weaponState > 1)
            {
                stream.setError(4);
                Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
                    << "(fatal error) Value out of bounds val: " << static_cast<int>(weaponState)
                    << " lower " << 0 << " upper: " << 1;
            }
            else
            {
                unsigned int bits = 0;
                do
                    ++bits;
                while ((1u << bits) - 1 < 2);              // -> 2 bits
                unsigned int scratch = weaponState;
                stream.writeBits(&scratch, bits);
            }
        }

        // the three unconditional flags
        {
            unsigned int flag = rec->isSkyDiving ? 1u : 0u;
            stream.writeBits(&flag, 1);
        }
        {
            unsigned int flag = rec->isSwimming ? 1u : 0u;
            stream.writeBits(&flag, 1);
        }
        {
            unsigned int flag = rec->weaponStowed ? 1u : 0u;
            stream.writeBits(&flag, 1);
        }

        // the smoothed movement axes, /50 range
        if (precision == io::StatePrecisionFull)
        {
            if (mask & 0x400)
            {
                float n = (rec->smoothedForward / 50.0f + 1.0f) * 0.5f;
                if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
                unsigned int q = static_cast<unsigned int>(n * 65535.0f);
                stream.writeBits(&q, 16);
            }
            if (mask & 0x800)
            {
                float n = (rec->smoothedStrafe / 50.0f + 1.0f) * 0.5f;
                if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
                unsigned int q = static_cast<unsigned int>(n * 65535.0f);
                stream.writeBits(&q, 16);
            }
        }
        else
        {
            if (mask & 0x400)
            {
                float n = (rec->smoothedForward / 50.0f + 1.0f) * 0.5f;
                if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
                unsigned int q = static_cast<unsigned int>(n * 1023.0f);
                stream.writeBits(&q, 10);
            }
            if (mask & 0x800)
            {
                float n = (rec->smoothedStrafe / 50.0f + 1.0f) * 0.5f;
                if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
                unsigned int q = static_cast<unsigned int>(n * 1023.0f);
                stream.writeBits(&q, 10);
            }
        }

        {
            unsigned int flag = rec->responseByteFA ? 1u : 0u;
            stream.writeBits(&flag, 1);
        }

        // sprint
        if (precision == io::StatePrecisionFull)
        {
            if (mask & 0x4000)
            {
                float stamina = rec->sprintStamina;
                if (stamina < 0.0f) stamina = 0.0f; else if (stamina > 1.0f) stamina = 1.0f;
                unsigned int q = static_cast<unsigned int>(stamina * 127.0f);
                stream.writeBits(&q, 7);
            }
            if (mask & 0x4000)                             // the same bit, tested again - shipped
            {
                unsigned int flag = rec->isSprinting ? 1u : 0u;
                stream.writeBits(&flag, 1);
            }
        }
        else if (mask & 0x4000)
        {
            float stamina = rec->sprintStamina;
            if (stamina < 0.0f) stamina = 0.0f; else if (stamina > 1.0f) stamina = 1.0f;
            unsigned int q = static_cast<unsigned int>(stamina * 31.0f);
            stream.writeBits(&q, 5);
        }

        // item index: value+1 ranged over the template's inventory
        const int inventorySize =
            static_cast<const world::SoldierTemplate*>(soldier->getTemplate())->getInventorySize();
        if (mask & 0x1000)
        {
            const unsigned int value = static_cast<unsigned int>(rec->itemIndex + 1);
            const unsigned int upper = static_cast<unsigned int>(inventorySize + 1);
            if (value > upper)
            {
                stream.setError(4);
                Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
                    << "(fatal error) Value out of bounds val: " << static_cast<int>(value)
                    << " lower " << 0 << " upper: " << static_cast<int>(upper);
            }
            else
            {
                unsigned int bits = 0;
                if (inventorySize != -2)
                {
                    do
                        ++bits;
                    while (static_cast<unsigned int>(inventorySize + 2) > (1u << bits) - 1);
                }
                unsigned int scratch = value;
                stream.writeBits(&scratch, bits);
            }
        }

        if (mask & 0x2000)
        {
            unsigned int latch = static_cast<unsigned int>(rec->poseInputLatch);
            stream.writeBits(&latch, 2);
        }

        // ability - full precision only
        if (precision == io::StatePrecisionFull && (mask & 0x20000))
        {
            float n = (rec->currentAbility + 1.0f) * 0.5f;
            if (n < 0.0f) n = 0.0f; else if (n > 1.0f) n = 1.0f;
            unsigned int q = static_cast<unsigned int>(n * 127.0f);
            stream.writeBits(&q, 7);
        }

        // ladder
        if (mask & 0x10000)
        {
            const unsigned int ladderId = rec->ladderId;
            if (ladderId > 0xFF)
            {
                stream.setError(4);
                Debug(DEBUGTYPE_7, "IO/BitStream.inl", 59, "")
                    << "(fatal error) Value out of bounds val: " << static_cast<int>(ladderId)
                    << " lower " << 0 << " upper: " << 255;
            }
            else
            {
                unsigned int bits = 0;
                do
                    ++bits;
                while ((1u << bits) - 1 < 0x100);          // -> nine bits for 0..255
                unsigned int scratch = ladderId;
                stream.writeBits(&scratch, bits);
            }
            if (rec->ladderId)
            {
                float state = rec->ladderState;
                if (state < 0.0f) state = 0.0f; else if (state > 1.0f) state = 1.0f;
                unsigned int q = static_cast<unsigned int>(state * 4095.0f);
                stream.writeBits(&q, 12);
            }
        }

        // zipline
        if (mask & 0x100000)
        {
            unsigned int ziplineId = rec->ziplineId;
            stream.writeBits(&ziplineId, 16);
            if (rec->ziplineId)
            {
                float state = rec->ziplineState;
                if (state < 0.0f) state = 0.0f; else if (state > 1.0f) state = 1.0f;
                unsigned int q = static_cast<unsigned int>(state * 4095.0f);
                stream.writeBits(&q, 12);
            }
        }

        // ammo: THE record IS masked IN place before the bit test - shipped side effect
        rec->ammoStatus &= 0xF;
        if (mask & 0x80000)
        {
            unsigned int bits = 0;
            do
                ++bits;
            while ((1u << bits) - 1 < 0x10);               // -> 5 bits for 0..15
            unsigned int scratch = static_cast<unsigned int>(rec->ammoStatus);
            stream.writeBits(&scratch, bits);
        }
    }

    if (stream.hasError())
    {
        stream.clearError();
        return false;
    }
    m_stats.m_writeBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);

    const bool fullControl =
        stateMask == 0xFFFFFFFFu && precision == io::StatePrecisionFull;

    return true;
}

// Decodes remote soldier movement, equipment and ragdoll state, carrying omitted values forward.
// Applies accepted absolute updates to the soldier. Read errors report a fatal diagnostic.
void SoldierNetworkable::setNetUpdate(io::BitStream& stream, float updateTime, bool ghostActive,
                                      io::BaseLineData* /*baseline*/, int precision)
{
    typedef world::object_detail::LayoutAccess L;
    world::Soldier* const soldier = static_cast<world::Soldier*>(m_object);

    world::ArmorGLComp* const armor =
        static_cast<world::ArmorGLComp*>(L::armorComponent(soldier));

    const bool isFullUpdate = makeRoomForNextUpdate(ghostActive);
    UPDATE* const rec = getUpdateAndMarkUsed(static_cast<unsigned int>(m_next));
    if (m_prev != m_next)
        *rec = *getUpdate(static_cast<unsigned int>(m_prev));          // the 160-byte carry-forward
    rec->time = updateTime;

    m_updateStartBitPos = stream.getBitPosition();                     // +0x30

    unsigned int mask = 0;
    stream.readBits(&mask, 21);
    if (stream.hasError())
    {
        // attested AND fatal - Debug type 6 aborts; the clears below are the ELF's dead tail.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/SoldierNetworkable.cpp", 760, __FUNCTION__)
            << formatString("error reading data");
        stream.clearError();
        return;
    }

    if (mask & 0x40)
    {
        unsigned int hitPoints = 0;
        stream.readBits(&hitPoints, 8);
        rec->hitPoints = static_cast<int>(hitPoints);
        if (mask & 0x40)
        {
            unsigned int lastDamageType = 0;
            stream.readBits(&lastDamageType, 1);
            rec->lastDamageType = static_cast<int>(lastDamageType);
        }
        if (armor)
        {
            armor->setLastDamageType(rec->lastDamageType);
            armor->setHitPoints(armor->getMaxHitPoints()
                                * (static_cast<float>(rec->hitPoints) / 255.0f));
        }
    }

    if (mask & 0x20)
    {
        unsigned int bits = 0;
        do
            ++bits;
        while ((1u << bits) - 1 < 5);
        unsigned int pose = 0;
        stream.readBits(&pose, bits);
        rec->pose = static_cast<int>(pose);
    }
    if (mask & 0x20)
    {
        unsigned int bits = 0;
        do
            ++bits;
        while ((1u << bits) - 1 < 5);
        unsigned int requestedPose = 0;
        stream.readBits(&requestedPose, bits);
        rec->requestedPose = static_cast<int>(requestedPose);
    }

    if (static_cast<short>(mask) < 0)                      // bit 15, the 16-bit sign test
    {
        if (soldier->m_ragDoll)
            soldier->m_ragDoll->readCurrentState(&stream);
        else
            anim::RagDoll::skipCurrentState(&stream);
    }
    else
    {
        // position
        if (mask & 0x1)
        {
            rec->absoluteTrans =
                stream.readCompressedVector(stream.getCompressionVector(), 0.001f);
        }

        // the physics vectors
        if (precision == io::StatePrecisionFull)
        {
            if (mask & 0x80)
                rec->positionalSpeed =
                    stream.readCompressedVector(s_speedCompressionVector, 0.001f);
            if (mask & 0x100)
                rec->positionalAcceleration =
                    stream.readCompressedVector(s_speedCompressionVector, 0.000099999997f);
            if (mask & 0x200)
                rec->positionalFriction =
                    stream.readCompressedVector(s_speedCompressionVector, 0.000099999997f);
            if (mask & 0x40000)
                rec->localLinearSpeed =
                    stream.readCompressedVector(s_speedCompressionVector, 0.001f);
        }
        else if (mask & 0x80)
        {
            rec->positionalSpeed =
                stream.readCompressedVector(s_speedCompressionVector, 0.010000001f);
        }

        // the look quartet
        if (precision == io::StatePrecisionFull)
        {
            if (mask & 0x2)
                stream.readBits(&rec->facingDegrees, 32);
            if (mask & 0x4)
                stream.readBits(&rec->lookLeftRightAngle, 32);
            if (mask & 0x8)
                stream.readBits(&rec->pendingTurn, 32);
            if (mask & 0x10)
                stream.readBits(&rec->lookUpDownAngle, 32);
        }
        else
        {
            // q/4095 -> [0..1] -> *2-1 -> a 2/4095 deadzone -> the field's range. Per field.
            if (mask & 0x2)
            {
                unsigned int q = 0;
                stream.readBits(&q, 12);
                float centred = static_cast<float>(q) / 4095.0f;
                centred = centred + centred - 1.0f;
                if (std::fabs(centred) <= 0.00048840052f)
                    centred = 0.0f;
                rec->facingDegrees = centred * 360.0f;
            }
            if (mask & 0x4)
            {
                unsigned int q = 0;
                stream.readBits(&q, 12);
                float centred = static_cast<float>(q) / 4095.0f;
                centred = centred + centred - 1.0f;
                if (std::fabs(centred) <= 0.00048840052f)
                    centred = 0.0f;
                rec->lookLeftRightAngle = centred * 90.0f;
            }
            if (mask & 0x8)
            {
                unsigned int q = 0;
                stream.readBits(&q, 12);
                float centred = static_cast<float>(q) / 4095.0f;
                centred = centred + centred - 1.0f;
                if (std::fabs(centred) <= 0.00048840052f)
                    centred = 0.0f;
                rec->pendingTurn = centred * 180.0f;
            }
            if (mask & 0x10)
            {
                unsigned int q = 0;
                stream.readBits(&q, 12);
                float centred = static_cast<float>(q) / 4095.0f;
                centred = centred + centred - 1.0f;
                if (std::fabs(centred) <= 0.00048840052f)
                    centred = 0.0f;
                rec->lookUpDownAngle = centred * 90.0f;
            }
        }

        // weapon zoom/state, flags - unconditional
        {
            unsigned int bits = 0;
            do
                ++bits;
            while ((1u << bits) - 1 < 4);
            unsigned int weaponZoom = 0;
            stream.readBits(&weaponZoom, bits);
            rec->weaponZoom = static_cast<int>(weaponZoom);
        }
        {
            unsigned int bits = 0;
            do
                ++bits;
            while ((1u << bits) - 1 < 2);
            unsigned int weaponState = 0;
            stream.readBits(&weaponState, bits);
            rec->weaponState = static_cast<int>(weaponState);
        }
        {
            unsigned int flag = 0;
            stream.readBits(&flag, 1);
            rec->isSkyDiving = (flag == 1);
        }
        {
            unsigned int flag = 0;
            stream.readBits(&flag, 1);
            rec->isSwimming = (flag == 1);
        }
        {
            unsigned int flag = 0;
            stream.readBits(&flag, 1);
            rec->weaponStowed = (flag == 1);
        }

        // the smoothed movement axes
        if (precision == io::StatePrecisionFull)
        {
            if (mask & 0x400)
            {
                unsigned int q = 0;
                stream.readBits(&q, 16);
                float centred = static_cast<float>(q) / 65535.0f;
                centred = centred + centred - 1.0f;
                if (std::fabs(centred) <= 0.000030518044f)
                    centred = 0.0f;
                rec->smoothedForward = centred * 50.0f;
            }
            if (mask & 0x800)
            {
                unsigned int q = 0;
                stream.readBits(&q, 16);
                float centred = static_cast<float>(q) / 65535.0f;
                centred = centred + centred - 1.0f;
                if (std::fabs(centred) <= 0.000030518044f)
                    centred = 0.0f;
                rec->smoothedStrafe = centred * 50.0f;
            }
        }
        else
        {
            if (mask & 0x400)
            {
                unsigned int q = 0;
                stream.readBits(&q, 10);
                float centred = static_cast<float>(q) / 1023.0f;
                centred = centred + centred - 1.0f;
                if (std::fabs(centred) <= 0.0019550342f)
                    centred = 0.0f;
                rec->smoothedForward = centred * 50.0f;
            }
            if (mask & 0x800)
            {
                unsigned int q = 0;
                stream.readBits(&q, 10);
                float centred = static_cast<float>(q) / 1023.0f;
                centred = centred + centred - 1.0f;
                if (std::fabs(centred) <= 0.0019550342f)
                    centred = 0.0f;
                rec->smoothedStrafe = centred * 50.0f;
            }
        }

        {
            unsigned int flag = 0;
            stream.readBits(&flag, 1);
            rec->responseByteFA = (flag == 1);
        }

        // sprint
        if (precision == io::StatePrecisionFull)
        {
            if (mask & 0x4000)
            {
                unsigned int q = 0;
                stream.readBits(&q, 7);
                rec->sprintStamina = static_cast<float>(q) / 127.0f;
            }
            if (mask & 0x4000)
            {
                unsigned int flag = 0;
                stream.readBits(&flag, 1);
                rec->isSprinting = (flag == 1);
            }
        }
        else if (mask & 0x4000)
        {
            unsigned int q = 0;
            stream.readBits(&q, 5);
            rec->sprintStamina = static_cast<float>(q) / 31.0f;
        }

        // item index
        const int inventorySize =
            static_cast<const world::SoldierTemplate*>(soldier->getTemplate())->getInventorySize();
        if (mask & 0x1000)
        {
            unsigned int bits = 0;
            if (inventorySize != -2)
            {
                do
                    ++bits;
                while (static_cast<unsigned int>(inventorySize + 2) > (1u << bits) - 1);
            }
            unsigned int value = 0;
            stream.readBits(&value, bits);
            rec->itemIndex = static_cast<int>(value) - 1;
        }

        if (mask & 0x2000)
        {
            unsigned int latch = 0;
            stream.readBits(&latch, 2);
            rec->poseInputLatch = static_cast<int>(latch);
        }

        if (precision == io::StatePrecisionFull && (mask & 0x20000))
        {
            unsigned int q = 0;
            stream.readBits(&q, 7);
            float centred = static_cast<float>(q) / 127.0f;
            centred = centred + centred - 1.0f;
            if (std::fabs(centred) <= 0.015748031f)
                centred = 0.0f;
            rec->currentAbility = centred;
        }

        // ladder / zipline
        if (mask & 0x10000)
        {
            unsigned int bits = 0;
            do
                ++bits;
            while ((1u << bits) - 1 < 0x100);
            unsigned int ladderId = 0;
            stream.readBits(&ladderId, bits);
            rec->ladderId = ladderId;
            if (rec->ladderId)
            {
                unsigned int q = 0;
                stream.readBits(&q, 12);
                rec->ladderState = static_cast<float>(q) / 4095.0f;
            }
        }
        if (mask & 0x100000)
        {
            unsigned int ziplineId = 0;
            stream.readBits(&ziplineId, 16);
            rec->ziplineId = ziplineId;
            if (rec->ziplineId)
            {
                unsigned int q = 0;
                stream.readBits(&q, 12);
                rec->ziplineState = static_cast<float>(q) / 4095.0f;
            }
        }

        if (mask & 0x80000)
        {
            unsigned int bits = 0;
            do
                ++bits;
            while ((1u << bits) - 1 < 0x10);
            unsigned int ammo = 0;
            stream.readBits(&ammo, bits);
            rec->ammoStatus = static_cast<int>(ammo);
        }
    }

    // stats and the tail error arm
    m_stats.m_readBits += static_cast<int>(stream.getBitPosition() - m_updateStartBitPos);
    if (stream.hasError())
    {
        // attested AND fatal - see the header block.
        Debug(DEBUGTYPE_6, "Game/Objects/Networkables/SoldierNetworkable.cpp", 870, __FUNCTION__)
            << formatString("error reading data");
        stream.clearError();
        return;
    }

    if (static_cast<short>(mask) < 0)
        return;                                            // a ragdoll update applies nothing else

    // THE apply

    // ladder transition: detach from the old, teleport, attach to the new
    const unsigned int currentLadderId = soldier->getLadderId();
    if (rec->ladderId != currentLadderId)
    {
        if (currentLadderId)
        {
            world::Ladder* const oldLadder = world::ladderMap[static_cast<int>(currentLadderId)];
            if (oldLadder)
                oldLadder->detachSoldier(soldier);
        }
        setSoldierTransformation(soldier, identityAt(rec->absoluteTrans), isFullUpdate);
        if (rec->ladderId)
        {
            world::Ladder* const newLadder = world::ladderMap[static_cast<int>(rec->ladderId)];
            if (newLadder)
                newLadder->attachSoldier(soldier);
        }
    }

    // zipline transition - same shape, but the teleport skips the prev-transform pin
    const unsigned int currentZiplineId = soldier->getZiplineId();
    if (rec->ziplineId != currentZiplineId)
    {
        if (currentZiplineId)
        {
            world::Zipline* const oldZipline = world::ziplineMap[static_cast<int>(currentZiplineId)];
            if (oldZipline)
                oldZipline->detachSoldier(soldier);
        }
        {
            const Mat4 m = identityAt(rec->absoluteTrans);
            if (L::parent(soldier))
                soldier->setRelativeTransformation(m);
            else
                soldier->setAbsoluteTransformation(m);
        }
        if (rec->ziplineId)
        {
            world::Zipline* const newZipline = world::ziplineMap[static_cast<int>(rec->ziplineId)];
            if (newZipline)
                newZipline->attachSoldier(soldier);
        }
    }

    soldier->m_byte315 = 1;                                // "applying a network update" latch

    world::SoldierPhysicsNode* const phys =
        static_cast<world::SoldierPhysicsNode*>(L::physicsNode(soldier));

    if (isFullUpdate)
    {
        // the state setters go through the maps unguarded - a null entry is called anyway.
        if (soldier->getLadderId())
        {
            world::Ladder* const ladder = world::ladderMap[static_cast<int>(rec->ladderId)];
            ladder->setCurrentState(soldier, rec->ladderState);
        }
        if (soldier->getZiplineId())
        {
            world::Zipline* const zipline = world::ziplineMap[static_cast<int>(rec->ziplineId)];
            zipline->setCurrentState(soldier, rec->ziplineState);
        }
        if (!L::parent(soldier) && !soldier->m_ragDoll)
            setSoldierTransformation(soldier, identityAt(rec->absoluteTrans), true);

        if (mask & 0x2)
            soldier->m_facingDegrees = rec->facingDegrees;
        if (mask & 0x4)
            soldier->m_lookLeftRightAngle = rec->lookLeftRightAngle;
        if (mask & 0x8)
            soldier->m_float214 = rec->pendingTurn;
        if (mask & 0x10)
            soldier->m_lookUpDownAngle = rec->lookUpDownAngle;
        if (mask & 0x20)
        {
            soldier->m_pose = rec->pose;
            soldier->m_requestedPose = rec->requestedPose;
        }

        if (phys)
        {
            if (phys->getIsMobile())
            {
                // yaw rate from the previous record's facing angle - record times are milliseconds
                float yawRate = 0.0f;
                if (static_cast<unsigned int>(m_count) > 1u)
                {
                    UPDATE* const before =
                        getUpdate((static_cast<unsigned int>(m_next) + 3) & 3);
                    if (before)
                    {
                        const float dt = rec->time - before->time;
                        if (dt > 0.0f)
                            yawRate = (rec->facingDegrees - before->facingDegrees) / (dt / 1000.0f);
                    }
                }
                phys->setPositionalSpeed(rec->positionalSpeed);
                Vec3 rotational;
                rotational.x = 0.0f; rotational.y = yawRate; rotational.z = 0.0f;
                phys->setRotationalSpeed(rotational);
            }
            if (mask & 0x100)
                phys->setPositionalAcceleration(rec->positionalAcceleration);
            if (mask & 0x200)
                phys->setPositionalFriction(rec->positionalFriction);
            if (mask & 0x40000)
                phys->setLocalLinearSpeed(rec->localLinearSpeed);

            // unconditional zeroing - and the second call overwrites the yaw rate set just
            // above. Shipped ordering, reproduced.
            Vec3 zero;
            zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
            phys->setRotationalFriction(zero);
            phys->setRotationalSpeed(zero);
            phys->setRotationalAcceleration(zero);
        }

        if (mask & 0x400)
            soldier->m_smoothedForward = rec->smoothedForward;
        if (mask & 0x800)
            soldier->m_smoothedStrafe = rec->smoothedStrafe;

        world::SoldierResponsePhysics* const response =
            static_cast<world::SoldierResponsePhysics*>(L::responsePhysics(soldier));
        if (response)
            response->m_byteFA = rec->responseByteFA;

        world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
            soldier->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
        {
            pco->setSprintStamina(rec->sprintStamina);
            if (precision == io::StatePrecisionFull)
                pco->setIsSprinting(rec->isSprinting != 0);
        }

        if (mask & 0x20000)
            soldier->setCurrentAbility(rec->currentAbility);

        setSoldierStateHACK();
    }

    soldier->setIsSkyDiving(rec->isSkyDiving != 0);        // the non-virtual member
    soldier->m_isSwimming = rec->isSwimming != 0;

    // the remote item switch (bit 12)
    if (mask & 0x1000)
    {
        // Raise the player's bit-3 flag pair around the switch, and put the inverse back after -
        // set/clear swapped, which is exactly the undo. See IPlayer.cpp for the flag's evidence.
        world::IPlayer* const occupant =
            static_cast<world::ISoldier*>(soldier)->getOccupingPlayer();
        if (occupant)
            occupant->setFlags(g_playerFlagBit3);

        soldier->m_weaponStowed = rec->weaponStowed != 0;

        if (rec->itemIndex != -1)
        {
            // walk up the parent chain to the first object that answers the PCO query
            world::IPlayerControlObject* pco = 0;
            for (world::IObject* holder = soldier; holder;
                 holder = L::parent(holder))
            {
                pco = static_cast<world::IPlayerControlObject*>(
                    holder->queryInterface(world::IID_IPlayerControlObject));
                if (pco)
                    break;
            }
            std::vector<world::IObject*>* const weapons =
                static_cast<std::vector<world::IObject*>*>(pco->getWeapons(0));
            for (std::vector<world::IObject*>::iterator it = weapons->begin();
                 it != weapons->end(); ++it)
            {
                world::IObject* const firearm = *it;
                world::IItemObject* itemFace = 0;
                if (firearm)
                    itemFace = static_cast<world::IItemObject*>(
                        firearm->queryInterface(world::IID_IItemObject));
                // itemFace dereferenced unguarded - a null element or a non-item weapon
                // crashes retail identically.
                if (itemFace->getItemIndex() == rec->itemIndex)
                {
                    if (pco->getActiveWeapon(0) != firearm)
                        pco->setActiveWeapon(firearm, 0);
                    break;
                }
            }
        }

        soldier->enableItem(static_cast<char>(rec->itemIndex));   // the index truncated to a char

        const int activeIndex = soldier->m_activeItemIndex;
        soldier->m_weaponStowed = rec->weaponStowed != 0;         // the second store - shipped
        if (activeIndex != -1)
        {
            world::IObject* const item = soldier->getItem(activeIndex);
            world::IItemObject* itemFace = 0;
            if (item)
                itemFace = static_cast<world::IItemObject*>(
                    item->queryInterface(world::IID_IItemObject));
            // unguarded again, as shipped.
            if (rec->weaponStowed)
                itemFace->disableItem();
            else
                itemFace->enableItem();
        }

        if (occupant)
        {
            Flag32Bits<world::IPlayer> inverse;
            inverse.set   = g_playerFlagBit3.clear;
            inverse.clear = g_playerFlagBit3.set;
            occupant->setFlags(inverse);
        }
    }

    if (mask & 0x2000)
        soldier->m_poseInputLatch = rec->poseInputLatch;

    // weapon zoom/state apply - only for unoccupied or remote-owned soldiers
    {
        world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
            soldier->queryInterface(world::IID_IPlayerControlObject));
        world::IObject* const activeWeapon = pco->getActiveWeapon(0);  // pco unguarded, shipped
        void* weaponObject = 0;
        if (activeWeapon)
            weaponObject = activeWeapon->queryInterface(world::IID_IWeaponObject);
        if (weaponObject)
        {
            world::IPlayer* occupant = 0;
            if (soldier)
                occupant = soldier->getOccupingPlayer();
            if (!soldier || !occupant || occupant->getIsRemote())
            {
                weaponObjectSetZoom(weaponObject, rec->weaponZoom);              // +0x58
                weaponObjectSetWeaponState(weaponObject,
                                           static_cast<unsigned int>(rec->weaponState));   // +0x5C
            }
        }
    }

    if (mask & 0x80000)
        soldier->setAmmoStatus(rec->ammoStatus);

    soldier->m_byte315 = 0;
}

// predict - 0x82477D0, 1,488 B - the client-side extrapolator/interpolator (a real body, one of
// only seven among the sixteen). Gated off entirely while m_forceFullUpdate is pending. Three
// arms, stamping the second base's predict-state field (+0x4C):
// 0 = snapped to the newest record (past the extrapolation window, or no bracketing pair)
// 1 = extrapolated along the newest record's speed
// 2 = interpolated between the two records bracketing (now - interpolationTime)
void SoldierNetworkable::predict(float now)
{
    typedef world::object_detail::LayoutAccess L;

    if (m_forceFullUpdate)
        return;

    world::Soldier* const soldier = static_cast<world::Soldier*>(m_object);
    m_next = m_head;
    world::SoldierPhysicsNode* const phys =
        static_cast<world::SoldierPhysicsNode*>(L::physicsNode(soldier));

    UPDATE* const newest = getUpdate(static_cast<unsigned int>(m_head));
    if (!newest)
        return;
    UPDATE* const previous = getUpdate((static_cast<unsigned int>(m_head) + 3) & 3);
    if (!previous)
        return;

    float ringDt = 0.0f;
    if (phys)
    {
        float yawRate = 0.0f;
        if (static_cast<unsigned int>(m_count) > 1u)
        {
            ringDt = newest->time - previous->time;
            if (ringDt > 0.0f)
                yawRate = (newest->facingDegrees - previous->facingDegrees) / (ringDt / 1000.0f);
        }
        if (phys->getIsMobile())
        {
            phys->setPositionalSpeed(newest->positionalSpeed);
            Vec3 rotational;
            rotational.x = 0.0f; rotational.y = yawRate; rotational.z = 0.0f;
            phys->setRotationalSpeed(rotational);
        }
    }

    const float t = now - static_cast<float>(io::networkManager->getInterpolationTime());

    if (t > newest->time)
    {
        if (static_cast<float>(io::networkManager->getExtrapolationTime()) < t - newest->time)
        {
            // snap: too far past the newest record
            if (!L::parent(soldier))
            {
                if (!soldier->m_ragDoll)
                    predict_setSoldierTransformation(soldier, identityAt(newest->absoluteTrans));
                m_dword10 = 0;
            }
            setPredictedState(soldier, 0, newest, 0.0f);
            return;
        }

        // extrapolate along the newest speed
        float span = ringDt;
        if (span == 0.0f)
            span = static_cast<float>(io::networkManager->getExtrapolationTime());
        if (!L::parent(soldier) && !soldier->m_ragDoll)
        {
            const float fraction = (t - newest->time) / span;
            const float seconds  = span / 1000.0f;
            Vec3 step;
            step.x = newest->positionalSpeed.x * seconds;
            step.y = newest->positionalSpeed.y * seconds;
            step.z = newest->positionalSpeed.z * seconds;
            Vec3 position;
            position.x = step.x * fraction + newest->absoluteTrans.x;
            position.y = step.y * fraction + newest->absoluteTrans.y;
            position.z = step.z * fraction + newest->absoluteTrans.z;
            if (!soldier->m_ragDoll)
                predict_setSoldierTransformation(soldier, identityAt(position));
            m_dword10 = 1;
        }
        setPredictedState(soldier, 0, newest, 0.0f);
        return;
    }

    // interpolate between the bracketing records
    unsigned int lo = 0, hi = 0;
    UPDATE* hiRec = 0;
    UPDATE* loRec = 0;
    bool bracketed = findEmbracingIndices(t, &lo, &hi);
    if (bracketed)
    {
        hiRec = getUpdate(hi);
        loRec = getUpdate(lo);
        bracketed = (hiRec != 0) && (loRec != 0);
    }
    float span2 = 0.0f;
    if (bracketed)
        span2 = hiRec->time - loRec->time;
    if (!bracketed || span2 <= 0.0f || span2 >= 500.0f)
    {
        // the snap arm again (no usable bracket)
        if (!L::parent(soldier))
        {
            if (!soldier->m_ragDoll)
                predict_setSoldierTransformation(soldier, identityAt(newest->absoluteTrans));
            m_dword10 = 0;
        }
        setPredictedState(soldier, 0, newest, 0.0f);
        return;
    }

    float fraction = 1.0f;
    if (span2 > 1.0f)
        fraction = (t - loRec->time) / span2;

    Mat4 m = Mat4::getIdentity();
    const float facing   = fraction * (hiRec->facingDegrees - loRec->facingDegrees)
                         + loRec->facingDegrees;
    const float lookUpDown = loRec->lookUpDownAngle
                           + (hiRec->lookUpDownAngle - loRec->lookUpDownAngle) * fraction;
    setRotationZXY(m, facing, lookUpDown, 0.0f);
    Vec3 position;
    position.x = (hiRec->absoluteTrans.x - loRec->absoluteTrans.x) * fraction + loRec->absoluteTrans.x;
    position.y = (hiRec->absoluteTrans.y - loRec->absoluteTrans.y) * fraction + loRec->absoluteTrans.y;
    position.z = (hiRec->absoluteTrans.z - loRec->absoluteTrans.z) * fraction + loRec->absoluteTrans.z;
    m.m[12] = position.x;
    m.m[13] = position.y;
    m.m[14] = position.z;

    if (!L::parent(soldier) && !soldier->m_ragDoll)
    {
        predict_setSoldierTransformation(soldier, m);
        m_dword10 = 2;
    }
    setPredictedState(soldier, loRec, hiRec, fraction);
}

// setPredictedState - 0x8247280, 1,133 B - predict's apply. update2's pose/look land on the
// soldier outright; sprint stamina and the ladder/zipline states are lerped between update1 and
// update2 when update1 is present (the interpolation arm); with update1 null the ladder/zipline
// state comes from the current ring record at m_next instead. Ends in setSoldierStateHACK.
// The map setCurrentState calls are unguarded null-this, as everywhere else in this TU.
void SoldierNetworkable::setPredictedState(world::Soldier* soldier, const UPDATE* update1,
                                           const UPDATE* update2, float fraction)
{
    typedef world::object_detail::LayoutAccess L;

    UPDATE* const current = getUpdate(static_cast<unsigned int>(m_next));

    world::SoldierResponsePhysics* const response =
        static_cast<world::SoldierResponsePhysics*>(L::responsePhysics(soldier));

    soldier->m_facingDegrees      = update2->facingDegrees;
    soldier->m_lookLeftRightAngle = update2->lookLeftRightAngle;
    soldier->m_float214           = update2->pendingTurn;
    soldier->m_lookUpDownAngle    = update2->lookUpDownAngle;
    soldier->m_pose               = update2->pose;
    soldier->m_requestedPose      = update2->requestedPose;
    if (response)
        response->m_byteFA = update2->responseByteFA;

    world::Soldier* const ownSoldier = static_cast<world::Soldier*>(m_object);
    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        ownSoldier->queryInterface(world::IID_IPlayerControlObject));
    if (pco)
    {
        float stamina = update2->sprintStamina;
        if (update1)
            stamina = update1->sprintStamina + (stamina - update1->sprintStamina) * fraction;
        pco->setSprintStamina(stamina);
    }

    const unsigned int ladderId = soldier->getLadderId();
    if (ladderId)
    {
        world::Ladder* const ladder = world::ladderMap[static_cast<int>(ladderId)];
        if (update1)
        {
            const float state = update1->ladderState
                              + (update2->ladderState - update1->ladderState) * fraction;
            ladder->setCurrentState(soldier, state);
        }
        else
        {
            ladder->setCurrentState(soldier, current->ladderState);
        }
    }

    const unsigned int ziplineId = soldier->getZiplineId();
    if (ziplineId)
    {
        world::Zipline* const zipline = world::ziplineMap[static_cast<int>(ziplineId)];
        if (update1)
        {
            const float state = update1->ziplineState
                              + (update2->ziplineState - update1->ziplineState) * fraction;
            zipline->setCurrentState(soldier, state);
        }
        else
        {
            zipline->setCurrentState(soldier, current->ziplineState);
        }
    }

    setSoldierStateHACK();
}

} // namespace hfe
} // namespace dice
