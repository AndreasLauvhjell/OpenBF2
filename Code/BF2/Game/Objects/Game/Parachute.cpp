// Parachute lifecycle and player interaction.

#include "Parachute.h"

#include <cmath>

#include "../../../Core/Ids.h"                     // IID_IGameServer + the queryInterface IIDs
#include "../../../Core/WeakPtr.h"                 // the raw Ctrl read - PCO's getOccupingPlayer idiom
#include "../../../Scene/Object/ObjectManager.h"   // objectManager->postMessage (+0xD0)
#include "../../../Game/IGame.h"                   // game - the IGameServer query + exitVehicle (+0xA8)
#include "../../../Game/Utility/Vars.h"            // the parachute-debug cvar handle
#include "../../../Animation/BoneAnimation/AnimationSystem.h" // active trigger-message system
#include "../../../Physics/HeightmapCluster.h"     // terrain clearance in updateAnimationState
#include "../../../Physics/IPhysicsNode.h"         // positional-speed transfers
#include "../../../Physics/Material.h"             // water-physics collision material
#include "../../../Physics/MaterialManager.h"      // material id lookup
#include "../../../Physics/PhysicsUtil.h"          // turnPhysicsOff - init's debug arm
#include "../../../Scene/Object/PlayerManager.h"   // local-human camera capture
#include "../ObjectLayoutAccess.h"                 // raw node/sibling reads used by retail
#include "../Base/FreeCamera.h"                    // detachSoldier's m_traceObject write
#include "../IPlayer.h"                            // exitVehicle's argument
#include "../TemplateLeaves.h"                     // ParachuteTemplate
#include "Soldier.h"                               // detachSoldier -> enableRagDoll

namespace dice {
namespace hfe {
namespace world {

extern IBase* materialManager;                       // ds:0x8AD2820, Core/Globals.cpp
extern IBase* heightmapCluster;                      // ds:0x8ABAF24, Core/Globals.cpp

// Same mangled world free function as Soldier.cpp's recovered 0x81A3AF0 body. The original
// defining TU is the PCO helper run; this tree currently houses the body beside Soldier.
void reenablePhysicsOnSoldier(IObject* object);

namespace {

// The retail TU's static-init (_0ii_166) caches Vars::getInt("parachute-debug", 0) in a file
// static (ds:0x8AAD5B8, initial 0xFFFFFFFF before init runs); Parachute::init and handleCollision
// read it. Same idiom as GenericProjectileNetworkable.cpp's two cvars.
// The same static-init seeds the gameplay constants at 0x8AAD598..0x8AAD5B4. Two independently
// stored values are both 0.05f; the material branch still selects between them in retail.
int* const g_parachuteDebug = Vars::getInt("parachute-debug", 0);
const float kOpenAnimationHeight = 8.0f;              // / PE dword_8885A0
const float kExitDisableDelay = 0.05f;                // / PE dword_8885A4
const float kCollisionDisableDelay = 0.05f;           // / PE dword_8885A8
const float kCollisionUpThreshold = 0.3f;             // / PE dword_8885AC

// The raw control-block drop - GenericProjectile.cpp's helper, byte for byte the discipline the
// ELF inlines (refcount at +0, delete on the 1->0 edge).
void dropWeakBlock(void* block)
{
    if (!block)
        return;
    int* refCount = static_cast<int*>(block);
    if ((*refCount)-- == 1)
        ::operator delete(block);
}

template <typename T>
T* weakTarget(int* block)
{
    return block ? reinterpret_cast<typename WeakPtr<T>::Ctrl*>(block)->target : 0;
}

// Assign a typed WeakPtr into the leaf's raw control-block slot with the exact retain-before-drop
// order emitted by both ports. The temporary returned by getWeakPtr releases its own reference
// after this helper returns, leaving the destination's retained reference alive.
void assignWeakBlock(int*& destination, const WeakPtr<IObject>& source)
{
    WeakPtr<IObject>::Ctrl* incoming = source.m_ctrl;
    if (incoming)
        ++incoming->refCount;
    dropWeakBlock(destination);
    destination = reinterpret_cast<int*>(incoming);
}

} // namespace

// ctor - 0x8194960 (C2), read in full: PlayerControlObject base (the template pointer upcasts -
// ParachuteTemplate : PlayerControlObjectTemplate), the compiler's revectors, then exactly three
// own stores in this order: m_refCount = 1, m_freeCameraWeak = 0, reset(). reset() writes the
// remaining three fields, so the whole 20-byte extension leaves the ctor deterministic.
Parachute::Parachute(const ParachuteTemplate* objectTemplate)
    : PlayerControlObject(objectTemplate)
{
    m_refCount = 1;
    m_freeCameraWeak = 0;
    reset();
}

// D1 - 0x8194xxx, 95 B: drop the weak block, then the base chain (the revectors are the
// compiler's).
Parachute::~Parachute()
{
    dropWeakBlock(m_freeCameraWeak);
}

// the quartet, against +0x3BC
void Parachute::addRef()  { ++m_refCount; }
int  Parachute::getRef()  { return m_refCount; }

int Parachute::release()
{
    const int r = --m_refCount;
    if (r == 0)
        delete this;                                  // the ELF's D0 dispatch via vtable +0x18
    return r;
}

// PlayerControlObject::queryInterface's four-interface table verbatim, re-emitted by the
// leaf (Itanium subobjects +0x80 / +0x13C / +0x140; the static_casts are the MSVC equivalents).
void* Parachute::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == world::IID_IObject)
        return const_cast<Parachute*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<Parachute*>(this));
    if (iid == world::IID_IPlayerControlObject)
        return static_cast<IPlayerControlObject*>(const_cast<Parachute*>(this));
    if (iid == world::IID_IAnimatedBundle)
        return static_cast<IAnimatedBundle*>(const_cast<Parachute*>(this));
    return 0;
}

// init - 0x8194A60, 99 B: post template message 32 to self at a third of a second, run the
// vehicle assembly line, then the debug arm - `parachute-debug 1` turns the fresh chute's physics
// off. 0x3EAAAAAB is float(1/3) rounded-to-nearest; the literal below reproduces the bit pattern.
void Parachute::init()
{
    objectManager->postMessage(this, static_cast<TemplateMessage>(32), 0.33333334f);   // +0xD0
    PlayerControlObject::init();
    if (*g_parachuteDebug == 1)
        turnPhysicsOff(this, true);
}

// back to the cache pristine - reset the extension, then the PCO park.
void Parachute::disableCachedObject()
{
    reset();
    PlayerControlObject::disableCachedObject();
}

// the same message-32 post init makes, then the PCO revive.
void Parachute::enableCachedObject()
{
    objectManager->postMessage(this, static_cast<TemplateMessage>(32), 0.33333334f);   // +0xD0
    PlayerControlObject::enableCachedObject();
}

// a pure chain, but the ELF gives the leaf its OWN slot for it. Preserved as an override.
void Parachute::destroy(bool force)
{
    PlayerControlObject::destroy(force);
}

// handleUpdate - 0x8194B70, 109 B: the disable countdown. While armed (> 0), tick it down; on the
// crossing, disable() fires and the timer re-arms to -1.0f (off). Collision, exit and exitPlayer
// are the three recovered arming paths.
void Parachute::handleUpdate(float dt, unsigned int flags)
{
    if (m_disableTimer > 0.0f)
    {
        m_disableTimer -= dt;
        if (m_disableTimer <= 0.0f)
        {
            disable();
            m_disableTimer = -1.0f;
        }
    }
    PlayerControlObject::handleUpdate(dt, flags);
}

// true - a parachutist may enter another vehicle mid-air (the IPlayerControlObject default
// says false).
bool Parachute::allowEnterOtherVehicle()
{
    return true;
}

// previous <- current, current <- message. replaces AnimatedBundle's animation-system
// forward - the ELF override does NOT chain, so a parachute's trigger messages never reach its
// animation systems. As shipped.
void Parachute::setTriggerMessage(int message)
{
    m_prevTriggerMessage = m_triggerMessage;
    m_triggerMessage = message;
}

// reset - non-virtual, TU-local in retail: the extension back to pristine. Called by the ctor and
// disableCachedObject; the recovered exit/collision paths arm the timer independently.
void Parachute::reset()
{
    m_disableTimer = -1.0f;
    m_triggerMessage = 0;
    m_prevTriggerMessage = 0;
    dropWeakBlock(m_freeCameraWeak);
    m_freeCameraWeak = 0;
}

// disable - 0x8194C10, 172 B, non-virtual, read in full. Server-only (the IID_IGameServer query
// gates the whole body): kick the occupying player out (exitVehicle, IGame +0xA8, both bools
// true), clear object flags 0x102 (updateFlags, IObject +0x60 - set none, clear bits 1 and 8),
// and post template message 33 to self at a tenth of a second. The ELF reads the occupant control
// block twice (its inlined weak-get, CSE'd); one source-level read is the same access.
void Parachute::disable()
{
    if (!game->queryInterface(IID_IGameServer))
        return;

    IPlayer* occupant = m_occupyingPlayer
        ? reinterpret_cast<WeakPtr<IPlayer>::Ctrl*>(m_occupyingPlayer)->target : 0;   // ctrl +4
    if (occupant)
        game->exitVehicle(occupant, true, true);                                      // +0xA8

    updateFlags(0, 0x102);                                                            // +0x60
    objectManager->postMessage(this, static_cast<TemplateMessage>(33), 0.1f);         // +0xD0
}

// handleCollision - ELF 0x8194CF0, 443 B / PE 0x5997C0, 326 B. On the server, an enabled chute
// collapses only for an upward-facing contact (or a null other object). Flag 8 on the other object
// suppresses the collapse except for a stationary physics-bearing root. The material lookup is
// deliberately retained even though both selected delays are 0.05f in BF2 1.50.
bool Parachute::handleCollision(IObject* other, const Vec3& position, const Vec3& normal,
                                const Vec3& speed, int collisionType, int materialId,
                                CollisionFaceInfo* ownFace, CollisionFaceInfo* otherFace)
{
    typedef object_detail::LayoutAccess L;

    if (game->queryInterface(IID_IGameServer) &&
        m_disableTimer < 0.0f &&
        *g_parachuteDebug != 1 &&
        // Both ports materialize the full dot with (0,1,0) only on this final non-null arm; keep
        // the zero products because NaNs in X/Z suppress the comparison in retail too.
        (!other || (normal.z * 0.0f + normal.x * 0.0f + normal.y) > kCollisionUpThreshold))
    {
        bool hasFlag8 = false;
        bool hasRootPhysics = false;
        bool rootIsMoving = false;

        if (other)
        {
            hasFlag8 = (other->getFlags() & 8u) != 0;

            // Asked twice in both ports. The first getRootParent may populate the object's cache;
            // retail then resolves and reads it again for the actual speed dispatch.
            if (L::physicsNode(getRootParent(other)))
            {
                hasRootPhysics = true;
                IPhysicsNode* const rootNode = static_cast<IPhysicsNode*>(
                    L::physicsNode(getRootParent(other)));
                if (rootNode)
                {
                    const Vec3& rootSpeed = rootNode->getPositionalSpeed();
                    rootIsMoving = std::sqrt(rootSpeed.x * rootSpeed.x +
                                             rootSpeed.y * rootSpeed.y +
                                             rootSpeed.z * rootSpeed.z) > 1.0f;
                }
            }
        }

        if (!hasFlag8 || (hasRootPhysics && !rootIsMoving))
        {
            Material* const material =
                static_cast<MaterialManager*>(materialManager)->getMaterialPtr(
                    static_cast<unsigned int>(materialId));
            if (material && material->getHasWaterPhysics() == true)
                m_disableTimer = kCollisionDisableDelay;
            else
                m_disableTimer = kExitDisableDelay;
        }
    }

    return PlayerControlObject::handleCollision(other, position, normal, speed,
                                                 collisionType, materialId, ownFace, otherFace);
}

// , 96 B. Message 32 refreshes and re-posts itself before chaining;
// message 33 detaches the body and destroys the chute without chaining.
void Parachute::handleMessage(TemplateMessage message, IPlayer* player, bool remote)
{
    if (message == static_cast<TemplateMessage>(32))
    {
        updateAnimationState();
        objectManager->postMessage(this, static_cast<TemplateMessage>(32), 0.33333334f);
        PlayerControlObject::handleMessage(message, player, remote);
    }
    else if (message == static_cast<TemplateMessage>(33))
    {
        detachSoldier();
        game->destroyObject(this);
    }
    else
    {
        PlayerControlObject::handleMessage(message, player, remote);
    }
}

// , 174 B. The chute inherits the player's team, keeps its current
// translation, and removes pitch/roll: only the negated Y component of getRotationZXY survives.
bool Parachute::enter(IPlayer* player)
{
    if (!PlayerControlObject::enter(player))
        return false;

    setTeam(player->getTeam());

    float zDegrees;
    float xDegrees;
    float yDegrees;
    getRotationZXY(getAbsoluteTransformation(), zDegrees, xDegrees, yDegrees);

    Mat4 transform = Mat4::getIdentity();
    const Vec3& absolutePosition = getAbsolutePosition();
    transform.m[12] = absolutePosition.x;
    transform.m[13] = absolutePosition.y;
    transform.m[14] = absolutePosition.z;

    Mat4 yaw = Mat4::getIdentity();
    setRotateYDeg(yaw, -yDegrees);
    const Mat4 beforeYaw = transform;
    transform.rotMult(beforeYaw, yaw);
    setAbsoluteTransformation(transform);
    return true;
}

// , 208 B. Preserve the released Soldier's exit position and the
// chute's velocity across the base exit, then schedule detachment on a server.
void Parachute::exit()
{
    typedef object_detail::LayoutAccess L;

    IObject* const occupyingObject = weakTarget<IObject>(m_occupyingObject);
    if (occupyingObject)
    {
        const Vec3& p = occupyingObject->getAbsolutePosition();
        m_altExitPosition[0] = p.x;
        m_altExitPosition[1] = p.y;
        m_altExitPosition[2] = p.z;
        _byte20C = true;
    }

    Vec3 positionalSpeed = Vec3::getZero();
    IPhysicsNode* const node = static_cast<IPhysicsNode*>(L::physicsNode(this));
    if (node)
        positionalSpeed = node->getPositionalSpeed();

    PlayerControlObject::exit();

    if (occupyingObject)
    {
        IPhysicsNode* const occupantNode =
            static_cast<IPhysicsNode*>(L::physicsNode(occupyingObject));
        if (occupantNode)
            occupantNode->setPositionalSpeed(positionalSpeed);
    }

    if (game->queryInterface(IID_IGameServer))
        objectManager->postMessage(this, static_cast<TemplateMessage>(33), 0.1f);
}

// , 222 B. The local human's free camera is retained for the later
// detachSoldier trace hand-off. No free-camera null guard exists in retail.
void Parachute::exitPlayer()
{
    if (weakTarget<IPlayer>(m_occupyingPlayer) &&
        weakTarget<IPlayer>(m_occupyingPlayer) == playerManager->getLocalHumanPlayer())
    {
        IObject* const freeCamera = weakTarget<IPlayer>(m_occupyingPlayer)->getFreeCamera();
        assignWeakBlock(m_freeCameraWeak, freeCamera->getWeakPtr());
    }

    PlayerControlObject::exitPlayer();
    if (m_disableTimer < 0.0f)
        m_disableTimer = kExitDisableDelay;
    setTriggerMessage(4);
    updateAnimationState();
}

// , 302 B. The server chooses the trigger from terrain clearance;
// every build then replaces messages 1/2/4 on the active system and each animated direct child.
// The active system is intentionally unguarded in retail.
void Parachute::updateAnimationState()
{
    if (game->queryInterface(IID_IGameServer))
    {
        const Vec3& p = getAbsolutePosition();
        const float clearance = p.y - static_cast<IHeightmapCluster*>(heightmapCluster)
                                            ->getHeightInWorldCoords(p.x, p.z);
        if (m_disableTimer > 0.0f)
            setTriggerMessage(4);
        else if (clearance >= kOpenAnimationHeight)
            setTriggerMessage(1);
        else
            setTriggerMessage(2);
    }

    anim::IAnimationSystem* const animationSystem =
        static_cast<anim::IAnimationSystem*>(m_activeAnimationSystem);
    animationSystem->clearTriggerMessage(1);
    animationSystem->clearTriggerMessage(2);
    animationSystem->clearTriggerMessage(4);
    animationSystem->setTriggerMessage(static_cast<unsigned int>(m_triggerMessage));

    for (IObject* child = getChild(); child; child = object_detail::LayoutAccess::sibling(child))
    {
        if (child->queryInterface(IID_IAnimatedBundle))
        {
            IAnimatedBundle* const animated = static_cast<IAnimatedBundle*>(
                child->queryInterface(IID_IAnimatedBundle));
            animated->clearTriggerMessage(1);
            animated->clearTriggerMessage(2);
            animated->clearTriggerMessage(4);
            animated->setTriggerMessage(m_triggerMessage);
        }
    }
}

// , 229 B. Turn the carried Soldier back into a world object,
// preserve the chute's velocity, enter ragdoll, and retarget the retained local free camera.
void Parachute::detachSoldier()
{
    typedef object_detail::LayoutAccess L;

    int* const occupyingBlock = m_occupyingObject;
    if (!weakTarget<IObject>(occupyingBlock))
        return;
    IObject* const soldierObject = weakTarget<IObject>(occupyingBlock);

    if (m_seatParentObject)
        m_seatParentObject->removeChild(soldierObject);

    reenablePhysicsOnSoldier(soldierObject);
    Soldier* const soldier = static_cast<Soldier*>(soldierObject);
    soldier->m_byte27F[0] = true;
    soldier->enableRagDoll();

    Vec3 positionalSpeed = Vec3::getZero();
    IPhysicsNode* const chuteNode = static_cast<IPhysicsNode*>(L::physicsNode(this));
    if (chuteNode)
        positionalSpeed = chuteNode->getPositionalSpeed();

    // Both shipped ports reload the weak target after re-enabling physics and entering ragdoll;
    // those callbacks are allowed to mutate the weak block. The reloaded target is intentionally
    // not null-guarded before its physics-node field is read.
    IObject* const currentSoldierObject = weakTarget<IObject>(m_occupyingObject);
    IPhysicsNode* const soldierNode =
        static_cast<IPhysicsNode*>(L::physicsNode(currentSoldierObject));
    if (soldierNode)
        soldierNode->setPositionalSpeed(positionalSpeed);

    // The camera weak target is likewise tested and loaded again for the use, rather than cached.
    if (weakTarget<IObject>(m_freeCameraWeak))
        static_cast<FreeCamera*>(weakTarget<IObject>(m_freeCameraWeak))->m_traceObject =
            soldierObject->getWeakPtr();
}

} // namespace world
} // namespace hfe
} // namespace dice
