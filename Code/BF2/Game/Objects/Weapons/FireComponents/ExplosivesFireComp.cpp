// Remote-explosive firing and template properties.

#include "ExplosivesFireComp.h"

#include <list>
#include <sstream>
#include <string>

#include "../../ExplosivesFireCompTemplate.h"
#include "../GenericFireArm.h"
#include "../../IObject.h"
#include "../../IObjectTemplate.h"
#include "../../IPlayer.h"
#include "../../Projectiles/GenericProjectile.h"
#include "../../../IGameLogic.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/StringUtil.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

// ExplosivesFireComp runtime

// ELF C2/C1 0x82605C0/0x8260760; PE 0x5B07E0.  The apparently untouched ELF dword at +0x148 is
// the old libstdc++ set object's third word, not an extra source member: PE's three-word set begins
// at +0x14C and is followed immediately by m_toggleCompleteAtTick at +0x158.
ExplosivesFireComp::ExplosivesFireComp(ExplosivesFireCompTemplate* objectTemplate)
    : FireComp(objectTemplate)
    , m_refCount(1)
    , m_fireCompleteAtTick(0)
    , m_fireLaunchDelayStartedAtTick(0)
    , m_waitingForFireLaunchDelay(false)
    , m_weaponState(static_cast<WeaponState>(1))
    , m_pendingWeaponState(static_cast<WeaponState>(0))
    , m_plantedProjectiles()
    , m_toggleCompleteAtTick(0)
    , m_switchToDetonatorAfterFire(false)
    , m_lastBusyAtTick(0)
    , m_triggerChargesAtTick(0)
    , m_triggerChargesPending(false)
    , m_explosivesReloadAtTick(0)
    , m_reloading(false)
    , m_alternateObject(0)
    , m_destroyingPlantedCharges(false)
    , m_explosivesTemplate(objectTemplate)
{
}

ExplosivesFireComp::~ExplosivesFireComp()
{
}

void ExplosivesFireComp::addRef()
{
    ++m_refCount;
}

int ExplosivesFireComp::getRef()
{
    return m_refCount;
}

int ExplosivesFireComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// .  Both shipped x86 images place the adjusted IWeaponFireComp face at +0x10.
void* ExplosivesFireComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return const_cast<ExplosivesFireComp*>(this);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponFireComp)
    {
        return static_cast<IWeaponFireComp*>(
            const_cast<ExplosivesFireComp*>(this));
    }
    return 0;
}

void ExplosivesFireComp::initComponent(WeaponInitEvent& event)
{
    FireComp::initComponent(event);

    const std::string& detonatorName = m_explosivesTemplate->getDetonatorObject();
    if (!detonatorName.empty())
    {
        for (IObject* child = firearm()->getChild(); child; child = child->getSibling())
        {
            if (!compareIgnoreCase(child->getTemplate()->getName(), detonatorName))
                m_alternateObject = child; // no break: retail retains the last matching child
        }
    }
    showDetonator(false);
}

void ExplosivesFireComp::enable()
{
    if (m_fireCompleteAtTick
        && m_fireCompleteAtTick <= static_cast<unsigned int>(WorldPref::getTickCount()))
    {
        fireCompleteUpdateStates();
    }
    reset();
    m_switchToDetonatorAfterFire = false;
    showDetonator(static_cast<int>(m_weaponState) == 2);
}

void ExplosivesFireComp::disable()
{
    reset();
}

void ExplosivesFireComp::reset()
{
    setFireState(static_cast<FireState>(0));
    m_triggerChargesPending = false;
    m_pendingWeaponState = static_cast<WeaponState>(0);
    m_reloading = false;
    m_waitingForFireLaunchDelay = false;
    m_destroyingPlantedCharges = false;
}

IObject* ExplosivesFireComp::getAlternateObject() const
{
    return m_alternateObject;
}

bool ExplosivesFireComp::preUpdate()
{
    if (FireComp::preUpdate())
        return true;

    if (m_waitingForFireLaunchDelay)
    {
        const unsigned int launchAt =
            m_fireLaunchDelayStartedAtTick
            + static_cast<unsigned int>(m_explosivesTemplate->getFireLaunchDelayInTicks());
        if (launchAt < static_cast<unsigned int>(WorldPref::getTickCount()))
        {
            m_fireLaunchDelayPending = false;
            m_waitingForFireLaunchDelay = false;
            setFireState(static_cast<FireState>(4));
            return true;
        }
    }
    return false;
}

// .  Each scheduled arm reads WorldPref independently, matching the retail ordering.
void ExplosivesFireComp::updateComponent(float, unsigned int)
{
    if (static_cast<int>(getFireState()) > 3
        && m_fireCompleteAtTick <= static_cast<unsigned int>(WorldPref::getTickCount())
        && !m_triggerChargesPending)
    {
        fireComplete(m_fireCompleteAtTick);
    }

    if (static_cast<int>(m_pendingWeaponState) == 0)
    {
        if (m_triggerChargesPending
            && m_triggerChargesAtTick <= static_cast<unsigned int>(WorldPref::getTickCount()))
        {
            m_triggerChargesPending = false;
            destroyPlantedCharges(true);
            toggleWeaponState(static_cast<WeaponState>(1));
        }
    }
    else if (m_toggleCompleteAtTick <= static_cast<unsigned int>(WorldPref::getTickCount()))
    {
        if (!m_explosivesTemplate->getDetonatorObject().empty() && !m_alternateObject)
        {
            WeaponInitEvent initEvent = {0, 0};
            initComponent(initEvent);
        }

        if (m_weaponState != m_pendingWeaponState)
        {
            m_weaponState = m_pendingWeaponState;
            showDetonator(static_cast<int>(m_pendingWeaponState) == 2);
            firearm()->updateFlags(2, 0);
        }
        m_pendingWeaponState = static_cast<WeaponState>(0);
    }

    if (static_cast<int>(m_weaponState) == 1 && m_reloading)
    {
        const unsigned int showAt =
            m_explosivesReloadAtTick
            + static_cast<unsigned int>(
                m_explosivesTemplate->getShowWeaponAfterReloadDelayInTicks());
        if (showAt < static_cast<unsigned int>(WorldPref::getTickCount()))
        {
            firearm()->updateFlags(2, 0);
            m_reloading = false;
        }
    }
}

void ExplosivesFireComp::showDetonator(bool show)
{
    if (m_alternateObject)
    {
        firearm()->setAlternateDisplayObject(
            show ? m_alternateObject : static_cast<IObject*>(firearm()),
            static_cast<AlternateDisplayMode>(0));
    }
}

void ExplosivesFireComp::pickup()
{
    IPlayer* const player = firearm()->getPlayerByForce();
    m_fireStartDelay = false;

    for (std::set<IObject*>::iterator it = m_plantedProjectiles.begin();
         it != m_plantedProjectiles.end(); ++it)
    {
        IObject* const object = *it;
        void* const projectileInterface = object
            ? object->queryInterface(IID_IProjectileObject)
            : 0;
        if (projectileInterface)
        {
            // GenericProjectile is the sole IProjectileObject implementor in both shipped images.
            GenericProjectile* const projectile = static_cast<GenericProjectile*>(
                static_cast<IProjectileObject*>(projectileInterface));

            std::list<WeakPtr<IPlayer> > assists;
            IPlayer* const previousPlayer = projectile->getPlayer();
            if (previousPlayer)
                assists.push_back(previousPlayer->getWeakPtr());
            projectile->setAssistPlayers(assists);
            projectile->setPlayer(player);
            projectile->setTeam(player ? player->getTeam() : 0);
        }
    }
}

bool ExplosivesFireComp::requestFire(bool first, bool second)
{
    bool result = FireComp::requestFire(first, second);
    if (m_lastBusyAtTick == static_cast<unsigned int>(WorldPref::getTickCount())
        && static_cast<int>(getFireState()) != 0
        && !second)
    {
        result = false;
    }

    if (result && second)
    {
        if (static_cast<int>(m_weaponState) == 1 && !m_plantedProjectiles.empty())
        {
            if (m_waitingForFireLaunchDelay)
            {
                m_waitingForFireLaunchDelay = false;
                result = false;
                setFireState(static_cast<FireState>(0));
            }
            toggleWeaponState(static_cast<WeaponState>(2));
        }
        else if (static_cast<int>(m_weaponState) == 2
                 && m_maxShots + *reinterpret_cast<const int*>(m_fireData) > 1)
        {
            // m_fireData's source type remains unresolved, but both ports read its first dword as
            // the current magazine/charge count at this exact decision.
            toggleWeaponState(static_cast<WeaponState>(1));
        }
        result = false;
    }
    return result;
}

bool ExplosivesFireComp::launchesProjectiles(int index) const
{
    if (index == 1)
        return false;
    if (index == 0)
        return static_cast<int>(m_weaponState) != 2;
    return static_cast<int>(m_weaponState) == 1;
}

int ExplosivesFireComp::fire(bool, int)
{
    m_signalFire = false;
    m_signalComplete = false;
    m_firedProjectiles.clear();

    if (static_cast<int>(m_weaponState) == 2)
    {
        setFireState(static_cast<FireState>(4));
        triggerPlantedCharges();
        return false;
    }

    if (m_fireLaunchDelayPending)
    {
        if (m_fireStartDelay)
            return false;

        if (m_explosivesTemplate->getFireStartDelay().value != 0.0f)
        {
            m_fireStartDelay = true;
            m_fireAtTick = static_cast<unsigned int>(WorldPref::getTickCount());
            m_fireStartDelayTicks = getFireStartDelayInTicks();
            setFireState(static_cast<FireState>(1));
            return false;
        }
    }

    if (m_fireStartDelay)
        return false;
    m_fireStartDelay = false;

    if (static_cast<int>(getFireState()) <= 2
        && !m_waitingForFireLaunchDelay
        && m_explosivesTemplate->getFireLaunchDelayInTicks() != 0)
    {
        m_fireLaunchDelayStartedAtTick =
            static_cast<unsigned int>(WorldPref::getTickCount());
        m_waitingForFireLaunchDelay = true;
        setFireState(static_cast<FireState>(3));

        for (IObject* child = firearm()->getChild(); child; child = child->getSibling())
        {
            child->handleMessage(static_cast<TemplateMessage>(0),
                                 firearm()->getPlayer(),
                                 false);
        }
        return true;
    }

    m_fireAtTick = static_cast<unsigned int>(WorldPref::getTickCount());
    m_fireCompleteAtTick =
        m_fireAtTick + static_cast<unsigned int>(m_explosivesTemplate->getTimePerRound());
    setFireState(static_cast<FireState>(4));
    if (static_cast<int>(m_weaponState) != 2)
        firearm()->updateFlags(0, 2);
    launchProjectile(-1);

    if (m_maxShots == 0 && *reinterpret_cast<const int*>(m_fireData) == 1)
        m_switchToDetonatorAfterFire = true;
    return m_explosivesTemplate->getFireLaunchDelayInTicks() == 0;
}

void ExplosivesFireComp::fireComplete(unsigned int tick)
{
    FireComp::fireComplete(tick);
    fireCompleteUpdateStates();
}

void ExplosivesFireComp::fireCompleteUpdateStates()
{
    m_waitingForFireLaunchDelay = false;
    m_fireCompleteAtTick = 0;
    if (m_switchToDetonatorAfterFire)
    {
        m_switchToDetonatorAfterFire = false;
        toggleWeaponState(static_cast<WeaponState>(2));
    }
}

bool ExplosivesFireComp::launchProjectile(int index)
{
    return FireComp::launchProjectile(index);
}

// .  The local-predict call is made through the projectile face in
// retail; GenericProjectile is its sole implementor, so the typed downcast avoids ABI-specific
// raw slots (+0x110 ELF versus +0x10C PE).
IObject* ExplosivesFireComp::createProjectile(const IObjectTemplate* objectTemplate,
                                               const Mat4& transform)
{
    IObject* projectile = 0;
    if (!ignoreNetworkedProjectileOnClient(objectTemplate))
        projectile = FireComp::createProjectile(objectTemplate, transform);

    if (projectile)
    {
        void* const projectileInterface = projectile->queryInterface(IID_IProjectileObject);
        if (!projectileInterface
            || !static_cast<GenericProjectile*>(
                    static_cast<IProjectileObject*>(projectileInterface))->getLocalPredict())
        {
            m_plantedProjectiles.insert(projectile);
        }
    }
    return projectile;
}

void ExplosivesFireComp::setWeaponState(unsigned int state)
{
    const WeaponState next = static_cast<WeaponState>((state == 1) + 1);
    if (next != m_weaponState)
    {
        m_weaponState = next;
        m_toggleCompleteAtTick = 0;
        showDetonator(static_cast<int>(next) == 2);
    }
}

unsigned int ExplosivesFireComp::getWeaponState() const
{
    return static_cast<int>(m_weaponState) == 2;
}

void ExplosivesFireComp::toggleWeaponState(WeaponState state)
{
    m_pendingWeaponState = state;
    m_toggleCompleteAtTick =
        static_cast<unsigned int>(WorldPref::getTickCount())
        + static_cast<unsigned int>(m_explosivesTemplate->getToggleWeaponTimeInTicks());
}

void ExplosivesFireComp::destroyPlantedCharges(bool trigger)
{
    if (gameLogic->isServer())
    {
        m_destroyingPlantedCharges = true;
        for (std::set<IObject*>::iterator it = m_plantedProjectiles.begin();
             it != m_plantedProjectiles.end(); ++it)
        {
            (*it)->handleMessage(
                static_cast<TemplateMessage>((trigger ? 0 : 1) + 31),
                0,
                false);
        }
        m_destroyingPlantedCharges = false;
    }
    m_plantedProjectiles.clear();
}

bool ExplosivesFireComp::ownsProjectile(IObject* projectile) const
{
    return m_plantedProjectiles.find(projectile) != m_plantedProjectiles.end();
}

bool ExplosivesFireComp::releaseProjectile(IObject* projectile)
{
    std::set<IObject*>::iterator it = m_plantedProjectiles.find(projectile);
    if (it == m_plantedProjectiles.end())
        return false;
    m_plantedProjectiles.erase(it);
    return true;
}

void ExplosivesFireComp::destroy()
{
    destroyPlantedCharges(false);
}

void ExplosivesFireComp::triggerPlantedCharges()
{
    m_triggerChargesPending = true;
    m_triggerChargesAtTick =
        static_cast<unsigned int>(WorldPref::getTickCount())
        + static_cast<unsigned int>(m_explosivesTemplate->getTriggerChargesTimeInTicks());
}

void ExplosivesFireComp::reload(ReloadState)
{
    m_reloading = true;
    m_explosivesReloadAtTick = static_cast<unsigned int>(WorldPref::getTickCount());
}

void ExplosivesFireComp::postReload()
{
    FireComp::postReload();
}

void ExplosivesFireComp::addProjectileReference(IObject* projectile)
{
    m_plantedProjectiles.insert(projectile);
}

void ExplosivesFireComp::projectileWasReset(IObject* projectile)
{
    if (!m_destroyingPlantedCharges)
        releaseProjectile(projectile);
}

bool ExplosivesFireComp::busy()
{
    if (static_cast<int>(m_pendingWeaponState) != 0 || m_triggerChargesPending)
        return true;

    if (static_cast<int>(getFireState()) <= 0
        || static_cast<int>(getFireState()) > 3)
    {
        if (static_cast<int>(getFireState()) > 3)
            m_lastBusyAtTick = static_cast<unsigned int>(WorldPref::getTickCount());
        return false;
    }
    return true;
}

bool ExplosivesFireComp::requestReload(bool)
{
    return static_cast<int>(getFireState()) == 0;
}

// this concrete override is an exact five-byte empty retail body.
void ExplosivesFireComp::switchFireRate()
{
}

// ExplosivesFireCompTemplate

ExplosivesFireCompTemplate::ExplosivesFireCompTemplate()
    : FireCompTemplate()
    , m_refCount(1)
    , m_toggleWeaponTime(0.0f)
    , m_toggleWeaponTimeTimes30(0)
    , m_triggerChargesTime(0.0f)
    , m_triggerChargesTimeTimes30(0)
    , m_showWeaponAfterReloadDelay(0.0f)
    , m_showWeaponAfterReloadDelayTimes30(0)
    , m_detonatorObject()
{
}

ExplosivesFireCompTemplate::~ExplosivesFireCompTemplate()
{
}

void ExplosivesFireCompTemplate::addRef()
{
    ++m_refCount;
}

int ExplosivesFireCompTemplate::getRef()
{
    return m_refCount;
}

int ExplosivesFireCompTemplate::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* ExplosivesFireCompTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase
        || iid == IID_IObjectTemplateComponent
        || iid == IID_IWeaponCompTemplate)
    {
        return const_cast<ExplosivesFireCompTemplate*>(this);
    }
    if (iid == io::IID_IConsoleSaveable)
    {
        return static_cast<io::IConsoleSaveable*>(
            const_cast<ExplosivesFireCompTemplate*>(this));
    }
    return 0;
}

unsigned int ExplosivesFireCompTemplate::getClassID() const
{
    return CID_ExplosivesFireCompTemplate;
}

void* ExplosivesFireCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ExplosivesFireCompTemplate || classId == CID_FireCompTemplate)
        return this;
    return 0;
}

IObjectComponent* ExplosivesFireCompTemplate::createComponent()
{
    return new ExplosivesFireComp(this);
}

void ExplosivesFireCompTemplate::setToggleWeaponTime(float value)
{
    m_toggleWeaponTime = value;
    m_toggleWeaponTimeTimes30 = static_cast<int>(value * 30.0f);
}

void ExplosivesFireCompTemplate::setTriggerChargesTime(float value)
{
    m_triggerChargesTime = value;
    m_triggerChargesTimeTimes30 = static_cast<int>(value * 30.0f);
}

void ExplosivesFireCompTemplate::setShowWeaponAfterReloadDelay(float value)
{
    m_showWeaponAfterReloadDelay = value;
    m_showWeaponAfterReloadDelayTimes30 = static_cast<int>(value * 30.0f);
}

// (1,287 B), PE 0x5AFF10 (360 B).  showWeaponAfterReloadDelay is intentionally not
// serialized by this leaf even though it is a live console property and runtime timing field.
bool ExplosivesFireCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!FireCompTemplate::makeScript(stream))
        return false;

    if (m_toggleWeaponTime != 0.0f)
        script << "ObjectTemplate.fire.toggleWeaponTime "
               << m_toggleWeaponTime << std::endl;
    if (m_triggerChargesTime != 0.0f)
        script << "ObjectTemplate.fire.triggerChargesTime "
               << m_triggerChargesTime << std::endl;
    if (!m_detonatorObject.empty())
        script << "ObjectTemplate.fire.detonatorObject "
               << m_detonatorObject << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
