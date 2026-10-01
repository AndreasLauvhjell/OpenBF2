// Acquires the nearest eligible projectile target.

#include "SeekClosestTargetComp.h"

#include <new>
#include <sstream>
#include <string>

#include "../ProjectileCompTemplates.h"
#include "../../Game/TargetObject.h"
#include "../../IObject.h"
#include "../../../IGame.h"
#include "../../../IGameLogic.h"
#include "../../../SingletonServer.h"
#include "../../../Utility/Vars.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

namespace {

// The TU's static initializer registers this live cvar handle. Both ports use the exact spelling
// and 1.0f default; updateComponent reads it after the tracking-delay gate.
float* const g_ignoreFriendlyTimer = Vars::getFloat("ignoreFriendlyTimer", 1.0f);

} // namespace

// This is a real namespace data symbol in the ELF (.data 0x8989C24, raw 0x40400000), not a Vars
// handle. makeScript is its sole observed reader. Notice that the constructor's field default is
// nevertheless zero, so a fresh template deliberately serializes `seek.reLockTime 0`.
float defaultReLockDelay = 3.0f;

SeekClosestTargetComp::SeekClosestTargetComp(SeekClosestTargetCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_targetObject()
    , m_lockedVehicle()
    , m_reLockPending(false)
    , m_forcedTarget(false)
    , m_pendingVehicle()
    , m_reLockStartTick(0)
    , m_template(objectTemplate)
    , m_ignoreFriendlyElapsed(0.0f)
{
    ProjectileComp::m_refCount = 1;
    m_object = 0;
    m_initObject = 0;
}

// Retail sends the unlock notification before releasing the remaining weak handles. Generated
// member destruction then drops pending, locked, and target handles in reverse declaration order.
SeekClosestTargetComp::~SeekClosestTargetComp()
{
    lockVehicle(0);
}

void SeekClosestTargetComp::addRef() { ++m_refCount; }
int SeekClosestTargetComp::getRef() { return m_refCount; }

int SeekClosestTargetComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* SeekClosestTargetComp::queryInterface(unsigned int iid) const
{
    SeekClosestTargetComp* const self = const_cast<SeekClosestTargetComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IProjectileComp || iid == IID_IProjectileSeekComp)
        return static_cast<IProjectileSeekComp*>(self);
    return 0;
}

unsigned int SeekClosestTargetComp::getInterfaceID() const
{
    return IID_IProjectileSeekComp;
}

bool SeekClosestTargetComp::hasUpdate() const
{
    return true;
}

// Both formal arguments are ABI-real, but flags is unused. The deadline addition and comparison
// are unsigned in both ports, including their 32-bit wraparound behavior. The friendly timer does
// not advance until after the tracking delay has elapsed.
void SeekClosestTargetComp::updateComponent(float seconds, unsigned int)
{
    const unsigned int launchTick = static_cast<unsigned int>(
        static_cast<GenericProjectile*>(m_initObject)->m_dword1BC);
    const unsigned int trackingDelay = static_cast<unsigned int>(
        m_template->getTrackingDelayTick());
    if (launchTick + trackingDelay <= static_cast<unsigned int>(WorldPref::getTickCount()))
    {
        checkTarget();
        seekForBestTarget();
        if (*g_ignoreFriendlyTimer > m_ignoreFriendlyElapsed)
            m_ignoreFriendlyElapsed += seconds;
    }
}

// The locked-vehicle handle and re-lock timestamp are deliberately left alone. In particular,
// activation does not emit an unlock notification; destruction and a later target change do.
void SeekClosestTargetComp::activate()
{
    m_targetObject = WeakPtr<IObject>();
    m_reLockPending = false;
    m_pendingVehicle = WeakPtr<IObject>();
    m_forcedTarget = false;
    m_ignoreFriendlyElapsed = 0.0f;
}

void SeekClosestTargetComp::setTargetObject(IObject* target, bool forced)
{
    m_targetObject = target ? target->getWeakPtr() : WeakPtr<IObject>();

    // getRootParent is intentionally called for null too; its shipped helper returns null.
    lockVehicle(getRootParent(target));

    // False does not clear a previous forced-target latch. Only activate and a completed delayed
    // re-lock clear it.
    if (forced)
        m_forcedTarget = true;
}

IObject* SeekClosestTargetComp::getTargetObject() const
{
    return m_targetObject.get();
}

int SeekClosestTargetComp::getTrackingDelayTick() const
{
    return m_template->getTrackingDelayTick();
}

float SeekClosestTargetComp::getMaxTargetDistance() const
{
    return m_template->getMaxTargetDistance();
}

float SeekClosestTargetComp::getMaxTargetAngle() const
{
    return m_template->getMaxTargetAngle();
}

TargetType SeekClosestTargetComp::getTargetType() const
{
    return m_template->getTargetType();
}

int SeekClosestTargetComp::getTargetObjectNetworkId() const
{
    TargetObject* const target = static_cast<TargetObject*>(m_targetObject.get());
    return target ? target->getTargetObjectId() : -1;
}

// The stack message is deliberately not value-initialized: the retail source assigns only these
// observed bytes/dwords before passing the 16-byte aggregate by value. The two unused high-byte
// regions therefore retain the shipped indeterminate behavior.
void SeekClosestTargetComp::lockVehicle(IObject* vehicle)
{
    IObject* const oldVehicle = m_lockedVehicle.get();
    if ((!vehicle && !oldVehicle) || vehicle == oldVehicle)
        return;

    MissileLockMessage message;
    unsigned char* const bytes = reinterpret_cast<unsigned char*>(&message);
    bytes[1] = 1;
    bytes[2] = 1;
    message.field08 = m_initObject->getObjectId();
    bytes[12] = static_cast<unsigned char>(getTargetType());

    if (oldVehicle && oldVehicle != vehicle)
    {
        bytes[0] = 1;
        message.field04 = -1;
        game->sendMissileMessageToOccupyingPlayers(oldVehicle, message);

        if (SingletonServer::m_iNametagManager)
        {
            SingletonServer::m_iNametagManager->removeItem(
                2, getRootParent(getObject()));
        }
        m_lockedVehicle = WeakPtr<IObject>();
    }

    if (vehicle)
    {
        bytes[0] = 1;
        message.field04 = getTargetObjectNetworkId();
        game->sendMissileMessageToOccupyingPlayers(vehicle, message);
        m_lockedVehicle = vehicle->getWeakPtr();

        if (SingletonServer::m_iNametagManager)
            SingletonServer::m_iNametagManager->addTouchItem(getRootParent(getObject()));
    }
}

void SeekClosestTargetComp::seekForBestTarget()
{
    const bool ordinaryTargetType =
        static_cast<int>(m_template->getTargetType()) != 3; // TTUnique

    GenericProjectile* const projectile = static_cast<GenericProjectile*>(m_initObject);

    // Preserve the observable virtual-call order in both binaries: weapon, vehicle, transform,
    // then position. The direction is the transformation's third basis row.
    IObject* const launcherWeapon = projectile->getLauncherWeapon();
    IObject* const launcherVehicle = projectile->getLauncherVehicle();
    const Mat4& transformation = m_initObject->getAbsoluteTransformation();
    const Vec3 forward = { transformation.m[8], transformation.m[9], transformation.m[10] };
    const Vec3& position = m_initObject->getAbsolutePosition();

    IObject* const candidate = gameLogic->findBestTargetObject(
        position,
        forward,
        m_template->getTargetType(),
        m_template->getMaxTargetDistance(),
        m_template->getMaxTargetAngle(),
        launcherVehicle,
        launcherWeapon,
        m_template->getDirectionBonus(),
        m_initObject,
        ordinaryTargetType,
        ordinaryTargetType,
        *g_ignoreFriendlyTimer > m_ignoreFriendlyElapsed);

    IObject* const candidateVehicle = candidate ? getRootParent(candidate) : 0;

    if (m_template->getReLockDelayTick() != 0 && candidateVehicle && m_forcedTarget)
    {
        if (m_targetObject.get())
        {
            setTargetObject(candidate, false);
            return;
        }

        // Shipped quirks: while a timer is pending, a different candidate does not restart it;
        // after success the pending vehicle is retained, so losing and reacquiring that same root
        // with the pending flag clear does not arm a new timer.
        if (!m_reLockPending && m_pendingVehicle.get() != candidateVehicle)
        {
            m_reLockPending = true;
            m_pendingVehicle = candidateVehicle->getWeakPtr();
            m_reLockStartTick = static_cast<unsigned int>(WorldPref::getTickCount());
        }

        if (m_reLockPending
            && m_reLockStartTick
                   + static_cast<unsigned int>(m_template->getReLockDelayTick())
               < static_cast<unsigned int>(WorldPref::getTickCount()))
        {
            // This delayed arm assigns the target directly: it deliberately does not call
            // lockVehicle and consequently emits no new lock notification here.
            m_targetObject = candidate->getWeakPtr();
            m_reLockPending = false;
            m_forcedTarget = false;
        }
        return;
    }

    setTargetObject(candidate, false);
}

// No template/class check is made: every non-null target is treated as TargetObject, as shipped.
void SeekClosestTargetComp::checkTarget()
{
    TargetObject* const target = static_cast<TargetObject*>(m_targetObject.get());
    if (target && !target->getTargetActive())
        m_targetObject = WeakPtr<IObject>();
}

// SeekClosestTargetCompTemplate. No virtual destructor belongs in its twelve-slot table; release
// performs direct operator delete, like FixedTargetCompTemplate and the other projectile leaves.
SeekClosestTargetCompTemplate::SeekClosestTargetCompTemplate()
    : m_refCount(1)
    , m_maxAngleLock(0.78539818525314331055f) // exact float 0x3F490FDB
    , m_maxTargetDistance(10000.0f)
    , m_targetType(static_cast<TargetType>(2)) // TTHeat
    , m_directionBonus(0.0f)
{
    setTrackingDelay(0.2f);
    setReLockDelay(0.0f);
}

void SeekClosestTargetCompTemplate::addRef() { ++m_refCount; }
int SeekClosestTargetCompTemplate::getRef() { return m_refCount; }

int SeekClosestTargetCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        ::operator delete(this);
    return remaining;
}

void* SeekClosestTargetCompTemplate::queryInterface(unsigned int iid) const
{
    SeekClosestTargetCompTemplate* const self =
        const_cast<SeekClosestTargetCompTemplate*>(this);
    if (iid == IID_IBase
        || iid == IID_IObjectTemplateComponent
        || iid == IID_IProjectileCompTemplate)
    {
        return static_cast<IObjectTemplateComponent*>(self);
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int SeekClosestTargetCompTemplate::getClassID() const
{
    return CID_SeekClosestTargetCompTemplate;
}

unsigned int SeekClosestTargetCompTemplate::getInterfaceID()
{
    return IID_IProjectileSeekCompTemplate;
}

IObjectComponent* SeekClosestTargetCompTemplate::createComponent()
{
    return new SeekClosestTargetComp(this);
}

void SeekClosestTargetCompTemplate::objectCreated(IObject*) {}
void SeekClosestTargetCompTemplate::objectDestroyed(IObject*) {}
void SeekClosestTargetCompTemplate::setTemplate(IObjectTemplate*) {}

void* SeekClosestTargetCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

void SeekClosestTargetCompTemplate::setTrackingDelay(float seconds)
{
    m_trackingDelay = seconds;
    m_trackingDelayTick = static_cast<int>(seconds * 30.0f);
}

void SeekClosestTargetCompTemplate::setReLockDelay(float seconds)
{
    m_reLockDelay = seconds;
    m_reLockDelayTick = static_cast<int>(seconds * 30.0f);
}

void SeekClosestTargetCompTemplate::setMaxAngleLock(float degrees)
{
    m_maxAngleLock = degrees * 0.01745329238474369049f;
}

bool SeekClosestTargetCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    script << "ObjectTemplate.seek.targetType " << m_targetType << std::endl;
    script << "ObjectTemplate.seek.trackingDelay " << m_trackingDelay << std::endl;
    script << "ObjectTemplate.seek.maxAngleLock " << getMaxAngleLock() << std::endl;
    script << "ObjectTemplate.seek.maxDistLock " << m_maxTargetDistance << std::endl;
    if (m_directionBonus != 0.0f)
        script << "ObjectTemplate.seek.directionBonus " << m_directionBonus << std::endl;
    if (m_reLockDelay != defaultReLockDelay)
        script << "ObjectTemplate.seek.reLockTime " << m_reLockDelay << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
