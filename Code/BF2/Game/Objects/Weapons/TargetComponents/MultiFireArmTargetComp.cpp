// Path:     [INFERRED] beside the attested LPTargetComp.cpp.
// Contents: heat/laser lock-on target runtime and template recovered from the BF2 1.50
// symbolized Linux ELF and cross-checked against the Win32 dedicated-server PE.

#include "MultiFireArmTargetComp.h"

#include <sstream>

#include "../GenericFireArm.h"
#include "../../Game/TargetObject.h"
#include "../../ICameraObject.h"
#include "../../IObject.h"
#include "../../IObjectTemplate.h"
#include "../../IPlayer.h"
#include "../../Projectiles/GenericProjectile.h"
#include "../../../IGame.h"
#include "../../../IGameLogic.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../Scene/Object/PlayerManager.h"
#include "../../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace world {

namespace {

IPlayerControlObject* findPco(IObject* object)
{
    for (; object; object = object->getParent())
    {
        IPlayerControlObject* const result = static_cast<IPlayerControlObject*>(
            object->queryInterface(IID_IPlayerControlObject));
        if (result)
            return result;
    }
    return 0;
}

GenericProjectile* projectileFromObject(IObject* object)
{
    if (!object || !object->queryInterface(IID_IProjectileObject))
        return 0;
    return static_cast<GenericProjectile*>(object);
}

} // namespace

MultiFireArmTargetComp::MultiFireArmTargetComp(MultiFireArmTargetCompTemplate* objectTemplate)
    : TargetComp(objectTemplate)
    , m_leafRefCount(1)
    , m_acquiredFeedbackPending(false)
    , m_losingLock(false)
    , m_transitionTick(0)
    , m_targetChangeTick(0)
    , m_multiTemplate(objectTemplate)
{
}

MultiFireArmTargetComp::~MultiFireArmTargetComp()
{
}

void MultiFireArmTargetComp::addRef() { ++m_leafRefCount; }
int MultiFireArmTargetComp::getRef() { return m_leafRefCount; }

int MultiFireArmTargetComp::release()
{
    const int remaining = --m_leafRefCount;
    if (remaining == 0 && this)
        delete this;
    return remaining;
}

void* MultiFireArmTargetComp::queryInterface(unsigned int iid) const
{
    MultiFireArmTargetComp* const self = const_cast<MultiFireArmTargetComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return self;
    if (iid == IID_IWeaponComp || iid == IID_IWeaponTargetComp)
        return static_cast<IWeaponTargetComp*>(self);
    return 0;
}

unsigned int MultiFireArmTargetComp::getInterfaceID() const
{
    return IID_IWeaponTargetComp;
}

bool MultiFireArmTargetComp::busy()
{
    return false;
}

void MultiFireArmTargetComp::updateComponent(float, unsigned int)
{
    IPlayer* const player = firearm()->getPlayer();

    if (m_multiTemplate->getTargetSystem() == TSLaserGuided)
    {
        TargetObject* const target = static_cast<TargetObject*>(getTargetObject());
        if (target)
        {
            if (target->getTargetActive())
            {
                if (m_acquiredFeedbackPending
                    && m_targetChangeTick + 35 < WorldPref::getTickCount())
                {
                    if (player == playerManager->getLocalHumanPlayer()
                        && !player->getIsAIPlayer())
                    {
                        game->localRadioMessage(TCPlayer, "target_aquired");
                    }
                    m_acquiredFeedbackPending = false;
                }
            }
            else if (player == playerManager->getLocalHumanPlayer()
                     && !player->getIsAIPlayer())
            {
                game->localRadioMessage(TCPlayer, "target_lost");
            }
        }
    }

    const TargetSystem system = m_multiTemplate->getTargetSystem();
    if (system == TSHeatSeeking || system == TSLaserGuided)
    {
        Vec3 direction = { 0.0f, 0.0f, 0.0f };
        if (m_multiTemplate->getUseCameraDirectionToTarget()
            && player && player->getCamera())
        {
            ICameraObject* const camera = player->getCamera();
            IObject* const cameraObject = static_cast<IObject*>(
                camera->queryInterface(IID_IObject));
            const Mat4& transform = cameraObject->getAbsoluteTransformation();
            direction.x = transform.m[8];
            direction.y = transform.m[9];
            direction.z = transform.m[10];
        }
        else
        {
            const Mat4& transform = firearm()->getAbsoluteTransformation();
            direction.x = transform.m[8];
            direction.y = transform.m[9];
            direction.z = transform.m[10];
        }

        IObject* candidate = 0;
        IPlayerControlObject* const pco = findPco(firearm());
        bool isActiveWeapon = false;
        for (int index = 0; index <= 2; ++index)
        {
            if (pco->getActiveWeapon(index) == firearm())
            {
                isActiveWeapon = true;
                break;
            }
        }

        if (isActiveWeapon && m_allowTargetSeek)
        {
            candidate = gameLogic->findBestTargetObject(
                firearm()->getAbsolutePosition(), direction,
                static_cast<TargetType>(TargetComp::getTargetType()),
                getMaxTargetDistance(),
                m_multiTemplate->getLockAngle() * 0.017453292f,
                getRootParent(firearm()), firearm(), 0.0f, firearm(), true, true, true);
        }

        IObject* const current = getTargetObject();
        if (candidate == current)
        {
            if (m_losingLock)
                m_losingLock = false;
        }
        else if (!hasTargetLock())
        {
            setTargetObject(candidate, false, getRootParent(candidate));
        }
        else
        {
            if (!m_losingLock)
            {
                m_losingLock = true;
                m_transitionTick = static_cast<unsigned int>(WorldPref::getTickCount());
            }

            if (m_transitionTick
                    + static_cast<unsigned int>(m_multiTemplate->getLoseLockDelayTicks())
                < static_cast<unsigned int>(WorldPref::getTickCount()))
            {
                setTargetObject(candidate, false, 0);
                m_losingLock = false;
            }
        }
    }

    IObject* const current = getTargetObject();
    if (current && !hasTargetLock()
        && m_targetChangeTick + m_multiTemplate->getLockDelayTicks()
            < WorldPref::getTickCount())
    {
        setTargetObject(current, true, getRootParent(current));
        m_acquiredFeedbackPending = true;
    }
}

void MultiFireArmTargetComp::setTargetObject(IObject* target, bool locked, IObject* vehicle)
{
    const bool changed = target != getTargetObject();
    TargetComp::setTargetObject(target, locked, vehicle);

    if ((!target || target->getTemplate()->getClassID() == CID_TargetObjectTemplate) && changed)
    {
        if (target)
            m_targetChangeTick = WorldPref::getTickCount();
        m_acquiredFeedbackPending = false;
        m_signalChangeTarget = true;
    }
}

bool MultiFireArmTargetComp::requestFire(bool released, bool alternate)
{
    if (!m_multiTemplate->getOnLockChangeToWeaponName().empty() && !m_onLockWeapon)
    {
        WeaponInitEvent event;
        event.firearmTemplate = 0;
        initComponent(event);
    }
    return TargetComp::requestFire(released, alternate);
}

void MultiFireArmTargetComp::postFire(WeaponFireEvent& event)
{
    TargetComp::postFire(event);
    if (!event.projectile)
        return;

    GenericProjectile* const projectile = projectileFromObject(event.projectile);
    IPlayer* const player = firearm()->getPlayer();
    IObject* const target = getTargetObject();
    if (target && player && player == playerManager->getLocalHumanPlayer())
    {
        game->radioMessage(TCPlayer, "missile_away",
                           playerManager->getLocalHumanPlayer(),
                           static_cast<TargetObject*>(target)->getTargetPlayerId(),
                           true, false);
    }
    if (target)
        m_targetChangeTick = -70;

    projectile->setTargetObject(hasTargetLock() ? getTargetObject() : 0);
}

void MultiFireArmTargetComp::disable()
{
    setTargetObject(0, false, 0);
}

MultiFireArmTargetCompTemplate::MultiFireArmTargetCompTemplate()
    : TargetCompTemplate()
    , m_leafRefCount(1)
    , m_lockAngle(45.0f)
{
    setLockDelay(0.0f);
    setLoseLockDelay(0.2f);
}

void MultiFireArmTargetCompTemplate::addRef() { ++m_leafRefCount; }
int MultiFireArmTargetCompTemplate::getRef() { return m_leafRefCount; }

int MultiFireArmTargetCompTemplate::release()
{
    const int remaining = --m_leafRefCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* MultiFireArmTargetCompTemplate::queryInterface(unsigned int iid) const
{
    MultiFireArmTargetCompTemplate* const self =
        const_cast<MultiFireArmTargetCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IWeaponCompTemplate)
        return static_cast<IObjectTemplateComponent*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int MultiFireArmTargetCompTemplate::getClassID() const
{
    return CID_MultiFireArmTargetCompTemplate;
}

IObjectComponent* MultiFireArmTargetCompTemplate::createComponent()
{
    return new MultiFireArmTargetComp(this);
}

void* MultiFireArmTargetCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_MultiFireArmTargetCompTemplate || classId == CID_TargetCompTemplate)
        return this;
    return 0;
}

void MultiFireArmTargetCompTemplate::setLockDelay(float seconds)
{
    m_lockDelay = seconds;
    m_lockDelayTicks = static_cast<int>(seconds * 30.0f);
}

void MultiFireArmTargetCompTemplate::setLoseLockDelay(float seconds)
{
    m_loseLockDelay = seconds;
    m_loseLockDelayTicks = static_cast<int>(seconds * 30.0f);
}

bool MultiFireArmTargetCompTemplate::makeScript(io::IStream* stream)
{
    if (!TargetCompTemplate::makeScript(stream))
        return false;

    std::ostringstream script;
    if (m_lockDelay != 0.0f)
        script << "ObjectTemplate.target.lockDelay " << m_lockDelay << std::endl;
    if (m_loseLockDelay != 0.2f)
        script << "ObjectTemplate.target.loseLockDelay " << m_loseLockDelay << std::endl;
    if (m_lockAngle != 0.0f)
        script << "ObjectTemplate.target.lockAngle " << m_lockAngle << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
