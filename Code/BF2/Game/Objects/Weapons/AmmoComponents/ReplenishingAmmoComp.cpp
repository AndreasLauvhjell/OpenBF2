// Path:     attested - Game/Objects/Weapons/AmmoComponents/ReplenishingAmmoComp.cpp occurs in
// both BF2 1.50 retail source-path maps.
// Contents: complete ReplenishingAmmoComp runtime/template family recovered from bf2.i64
// (0x8259DA0..0x825C2AF) and cross-checked against bf2_w32ded.exe.

#include "ReplenishingAmmoComp.h"

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

#include "../GenericFireArm.h"
#include "../../Base/Camera.h"
#include "../../Game/Kit.h"
#include "../../Game/Soldier.h"
#include "../../GameLogicComp/IArmorGLComp.h"
#include "../../IObject.h"
#include "../../IObjectComponent.h"
#include "../../IObjectTemplate.h"
#include "../../IPlayer.h"
#include "../../ISoldier.h"
#include "../../IWeaponObject.h"
#include "../../../IGameLogic.h"
#include "../../../Utility/Vars.h"
#include "../../../../AI/AIInterface/AIInstance.h"
#include "../../../../AI/AIInterface/IAIPlayerCourier.h"
#include "../../../../AI/AIInterface/SingletonServer.h"
#include "../../../../Core/Debug.h"
#include "../../../../Core/EventManager.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../Scene/Object/ObjectManager.h"
#include "../../../../Scene/Object/ObjectPredicator.h"
#include "../../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace world {

namespace {

int* const replenishIgnoreTeam = Vars::getInt("replenish-ignoreteam", 0);

// ELF static initializer 0x825BE81..0x825BEAA builds these as a float plus a pointer to it, in this
// order. updateComponent dereferences the selected pointer at use time.
float noTargetRefreshFrequencyValue = 1.0f;
float* noTargetRefreshFrequency = &noTargetRefreshFrequencyValue;
float targetRefreshFrequencyValue = 0.5f;
float* targetRefreshFrequency = &targetRefreshFrequencyValue;

IPlayerControlObject* findPlayerControlObject(IObject* object)
{
    for (; object; object = object->getParent())
    {
        IPlayerControlObject* const pco = static_cast<IPlayerControlObject*>(
            object->queryInterface(IID_IPlayerControlObject));
        if (pco)
            return pco;
    }
    return 0;
}

ai::IAIPlayerCourier* aiPlayerCourier()
{
    return static_cast<ai::IAIPlayerCourier*>(
        ai::BFSingletonServer::m_iaiPlayerCourier);
}

} // anonymous namespace


ReplenishingAmmoComp::ReplenishingAmmoComp(ReplenishingAmmoCompTemplate* objectTemplate)
    : AmmoComp(objectTemplate)
    , m_refCount(1)
    , m_unusedValues()
    , m_abilityStopped(false)
    , m_releaseRequired(false)
    , m_replenishingTemplate(objectTemplate)
    , m_targetRefreshTimer(0.0f)
    , m_abilityObjects()
{
}

ReplenishingAmmoComp::~ReplenishingAmmoComp() {}

void ReplenishingAmmoComp::addRef() { ++m_refCount; }
int ReplenishingAmmoComp::getRef() { return m_refCount; }

int ReplenishingAmmoComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* ReplenishingAmmoComp::queryInterface(unsigned int iid) const
{
    ReplenishingAmmoComp* const self = const_cast<ReplenishingAmmoComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponAmmoComp)
        return static_cast<IWeaponAmmoComp*>(self);
    return 0;
}

unsigned int ReplenishingAmmoComp::getInterfaceID() const
{
    return IID_IWeaponAmmoComp;
}

void ReplenishingAmmoComp::enable()
{
    AmmoComp::enable();
    firearm()->updateFlags(2, 0);
    if (m_replenishingTemplate->getOnlyActiveWhileFiring())
        m_abilityStopped = true;
}

// Free helper at ELF 0x825A0E0. The PCO team getter is intentionally repeated on the neutral-team
// arm, and absence of either a PCO or player accepts the target.
bool checkObjectTeam(IObject* object, IPlayer* player)
{
    if (*replenishIgnoreTeam > 0)
        return true;

    IPlayerControlObject* const pco = findPlayerControlObject(getRootParent(object));
    if (pco && player)
    {
        if (pco->getTeam() == player->getTeam())
            return true;
        return pco->getTeam() == 0;
    }
    return true;
}

// Free helper at ELF 0x825A1A0. It is emitted despite having no call sites in the retail image.
IWeaponObject* getPrimWeapon(IPlayer* player)
{
    IObject* const vehicle = player->getVehicle();
    if (!vehicle)
        return 0;

    IPlayerControlObject* const pco = findPlayerControlObject(vehicle);
    if (!pco)
        return 0;

    IObject* const weapon = pco->getActiveWeapon(0);
    return weapon
        ? static_cast<IWeaponObject*>(weapon->queryInterface(IID_IWeaponObject))
        : 0;
}

bool ReplenishingAmmoComp::canReload() const
{
    if (getBulletInChamber())
        return false;
    return m_replenishingTemplate->getAbilityCost() <= getAbility();
}

// . The overhead-camera path only accumulates the timer and never
// refreshes targets. Target processing remains server-only, but target acquisition itself does not.
void ReplenishingAmmoComp::updateComponent(float seconds, unsigned int flags)
{
    AmmoComp::updateComponent(seconds, flags);

    IPlayer* const player = firearm()->getPlayer();
    if (!player || m_replenishingTemplate->getAbilityMaterial() == -1)
        return;

    const bool enoughAbility =
        getAbility() >= m_replenishingTemplate->getAbilityDrainPerTick();
    const float refreshFrequency = *(m_abilityObjects.empty()
        ? noTargetRefreshFrequency
        : targetRefreshFrequency);

    if (player->getOverheadCameraEnabled())
    {
        m_targetRefreshTimer += seconds;
    }
    else if (m_targetRefreshTimer > 1.0f / refreshFrequency)
    {
        if (m_replenishingTemplate->getOnlyActiveWhileFiring())
            updateAbilityObjectAim();
        else
            updateAbilityObjectRadius();
        m_targetRefreshTimer = 0.0f;
    }
    else
    {
        m_targetRefreshTimer += seconds;
    }

    if (!gameLogic->isServer() || !enoughAbility || m_abilityStopped || m_releaseRequired)
        return;

    bool reloadedProjectile = false;
    for (std::size_t i = 0; i < m_abilityObjects.size(); ++i)
    {
        IObject* const target = m_abilityObjects[i].get();
        if (!target || target->isObjectDestroyed())
            continue;

        IArmorGLComp* const armor = static_cast<IArmorGLComp*>(target->getArmorComponent());
        const int targetMaterial = armor ? armor->getDefaultMaterial() : 0;

        bool worked;
        // This arm is a raw zero/non-zero test in both retail images. Invalid non-zero values
        // therefore follow the ammo path here even though the test-only helper diagnoses them.
        if (m_replenishingTemplate->getReplenishingType() != RTHeal)
        {
            worked = gameLogic->replenishAmmo(
                target,
                m_replenishingTemplate->getAbilityStrengthPerTick(),
                player,
                m_replenishingTemplate->getAbilityMaterial(),
                targetMaterial,
                0,
                false);
        }
        else
        {
            worked = gameLogic->heal(
                target,
                m_replenishingTemplate->getAbilityStrengthPerTick(),
                player,
                m_replenishingTemplate->getAbilityMaterial(),
                targetMaterial,
                0,
                false,
                true);
        }

        if (worked)
            taxAbility(m_replenishingTemplate->getAbilityDrainPerTick());

        if (armor && target->queryInterface(IID_IProjectileObject)
            && armor->getHitPoints() == armor->getMaxHitPoints())
        {
            IObject* const vehicle = player->getVehicle();
            if (vehicle
                && vehicle->getTemplate()->getClassID() == CID_SoldierTemplate)
            {
                std::vector<IObject*>* const weapons =
                    reinterpret_cast<std::vector<IObject*>*>(
                        static_cast<Soldier*>(vehicle)->getWeapons());
                for (std::size_t weaponIndex = 0; weaponIndex < weapons->size(); ++weaponIndex)
                {
                    IWeaponObject* const weapon = static_cast<IWeaponObject*>(
                        (*weapons)[weaponIndex]->queryInterface(IID_IWeaponObject));
                    if (weapon && weapon->getProjectileTemplate() == target->getTemplate())
                    {
                        weapon->reloadAmmo(1, false);
                        reloadedProjectile = true;
                    }
                }
            }

            if (ai::g_aiInstance)
            {
                unsigned int objectId = target->getObjectId();
                eventManager->postEvent(ECAI, 21, &objectId, 0.0f);
            }
            objectManager->destroyObject(target);
        }
    }

    if (reloadedProjectile)
        m_targetRefreshTimer = 1.0f / *targetRefreshFrequency + 1.0f;
}

void ReplenishingAmmoComp::updateAbilityObjectAim()
{
    m_abilityObjects.clear();

    IObject* const root = getRootParent(firearm());
    AbilityPredicator predicator(
        0x400u,
        root,
        static_cast<unsigned int>(m_replenishingTemplate->getAbilityMaterial()),
        false);
    IPlayer* const player = firearm()->getPlayer();
    IPlayerControlObject* const pco = findPlayerControlObject(firearm());

    IObject* cameraObject = 0;
    if (pco)
    {
        pco->queryInterface(IID_IObject); // retained discarded query
        ICameraObject* const cameraFace = pco->getActiveCamera();
        // The typed cross-cast reproduces the shipped active-camera adjustment: PE -0x1C8,
        // ELF -0x1B0. Keeping the face typed avoids freezing either displacement into source.
        Camera* const camera = static_cast<Camera*>(cameraFace);
        if (!cameraFace)
            return;
        cameraObject = static_cast<IObject*>(camera);
    }

    if (!cameraObject)
        return;

    IObject* hitObject = 0;
    Vec3 hitPoint = {0.0f, 0.0f, 0.0f};
    Vec3 hitNormal = {0.0f, 0.0f, 0.0f};
    int hitPart;
    const Vec3 from = cameraObject->getAbsolutePosition();
    const Mat4& transformation = cameraObject->getAbsoluteTransformation();
    const float radius = m_replenishingTemplate->getAbilityRadius();
    const Vec3 direction = {
        transformation.m[8] * radius,
        transformation.m[9] * radius,
        transformation.m[10] * radius
    };

    if (objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                                     from, direction, &predicator,
                                     true, false, false, true, 0))
    {
        hitObject = getRootParent(hitObject);
        if (isAbilityWorkPossible(hitObject))
            m_abilityObjects.push_back(hitObject->getWeakPtr());
    }

    if (m_abilityObjects.empty() && ai::g_aiInstance && player->getIsAIPlayer())
    {
        ai::IAIPlayerCourier* const courier = aiPlayerCourier();
        IObject* abilityObject = courier->getFirstAbilityTarget(player->getId());
        bool abilityWorkPossible = isAbilityWorkPossible(abilityObject);
        if (!abilityWorkPossible)
        {
            abilityObject = objectManager->getObjectFromGrid(
                courier->getSecondAbilityTargetGridId(player->getId()));
            abilityWorkPossible = isAbilityWorkPossible(abilityObject);
        }
        if (abilityWorkPossible)
            m_abilityObjects.push_back(abilityObject->getWeakPtr());
    }
}

void ReplenishingAmmoComp::updateAbilityObjectRadius()
{
    m_abilityObjects.clear();
    firearm()->getPlayer(); // retained discarded lookup

    IObject* const root = getRootParent(firearm());
    AbilityPredicator predicator(
        0x1000400u,
        0,
        static_cast<unsigned int>(m_replenishingTemplate->getAbilityMaterial()),
        true);

    static std::vector<IObject*> candidates;
    objectManager->getObjectsFromGrid(
        root->getAbsolutePosition(),
        m_replenishingTemplate->getAbilityRadius(),
        candidates,
        &predicator);

    for (std::size_t i = 0; i < candidates.size(); ++i)
    {
        IObject* candidate = candidates[i];
        if (candidate->getTemplate()->getClassID() == CID_ParachuteTemplate)
        {
            IPlayerControlObject* const pco = static_cast<IPlayerControlObject*>(
                candidate->queryInterface(IID_IPlayerControlObject));
            candidate = pco->getOccupingObject(); // no PCO guard in either retail image
        }

        if (candidate && isAbilityWorkPossible(candidate))
            m_abilityObjects.push_back(candidate->getWeakPtr());
    }
}

bool ReplenishingAmmoComp::isAbilityWorkPossible(IObject* object)
{
    IPlayer* const player = firearm()->getPlayer();
    if (!object || !checkObjectTeam(object, player))
        return false;

    IArmorGLComp* const armor = static_cast<IArmorGLComp*>(object->getArmorComponent());
    const int targetMaterial = armor ? armor->getDefaultMaterial() : 0;
    if (!player || !armor)
        return false;

    switch (m_replenishingTemplate->getReplenishingType())
    {
    case RTHeal:
        return gameLogic->heal(
            object,
            m_replenishingTemplate->getAbilityStrengthPerTick(),
            player,
            m_replenishingTemplate->getAbilityMaterial(),
            targetMaterial,
            0,
            true,
            true);

    case RTAmmo:
        return gameLogic->replenishAmmo(
            object,
            m_replenishingTemplate->getAbilityStrengthPerTick(),
            player,
            m_replenishingTemplate->getAbilityMaterial(),
            targetMaterial,
            0,
            true);

    default:
        Debug(DEBUGTYPE_6,
              "Game/Objects/Weapons/AmmoComponents/ReplenishingAmmoComp.cpp", 327, "")
            << "Unknown replenishtype "
            << static_cast<int>(m_replenishingTemplate->getReplenishingType());
        return false;
    }
}

void* ReplenishingAmmoComp::getAbilityObjectList()
{
    return &m_abilityObjects;
}

bool ReplenishingAmmoComp::getAbilityWorking()
{
    return !m_abilityObjects.empty() && !m_abilityStopped;
}

bool ReplenishingAmmoComp::requestFire(bool released, bool)
{
    if (m_replenishingTemplate->getOnlyActiveWhileFiring() && m_abilityObjects.empty())
        return false;

    if (m_releaseRequired && m_replenishingTemplate->getOnlyActiveWhileFiring())
    {
        if (released)
            return false;
        m_releaseRequired = false;
    }

    if (gameLogic->getDebugReplenish())
        getAbility(); // retail reads and discards the value while debug-replenish is enabled
    return !needsReload();
}

void ReplenishingAmmoComp::postFire(WeaponFireEvent& event)
{
    if (event.fireCount > 0 && m_magazines[0] != -1)
    {
        taxAbility(static_cast<float>(event.fireCount)
                   * m_replenishingTemplate->getAbilityCost());
        m_magazines[0] -= event.fireCount;
    }

    if (m_replenishingTemplate->getMinimumTimeToReloadTimes30() != 0)
        m_lastFireTick = static_cast<unsigned int>(WorldPref::getTickCount());

    m_abilityStopped = false;
    m_autoReloadPending = m_replenishingTemplate->getAutoReload()
        && m_magazines[0] == 0;
}

void ReplenishingAmmoComp::fireStop(bool)
{
    if (m_replenishingTemplate->getOnlyActiveWhileFiring())
        m_abilityStopped = true;
}

bool ReplenishingAmmoComp::requestReload(bool automatic)
{
    if (automatic && m_replenishingTemplate->getAutoReload())
        return false;
    if (!getIsReloading())
        return canReload();
    return false;
}

void ReplenishingAmmoComp::queueReload() {}
bool ReplenishingAmmoComp::canBeRefilled() { return false; }
float ReplenishingAmmoComp::refillPercent(float percent, bool) { return percent; }
unsigned int ReplenishingAmmoComp::refillAbsolute(unsigned int amount, bool) { return amount; }

float ReplenishingAmmoComp::getReloadTimeLeftInPercent() const
{
    return getAbility();
}

int ReplenishingAmmoComp::getMaxNrOfMags() const
{
    if (m_magazines[0] == -1)
        return 1;

    const int fromAbility = static_cast<int>(
        1.0f / m_replenishingTemplate->getAbilityCost()) + 1;
    return m_replenishingTemplate->getNrOfMags() <= fromAbility
        ? m_replenishingTemplate->getNrOfMags()
        : fromAbility;
}

int ReplenishingAmmoComp::getMagVectorSize() const
{
    if (m_magazines[0] == -1 || m_replenishingTemplate->getNrOfMags() == -1)
        return 1;

    int desired;
    if (m_replenishingTemplate->getAbilityCost() == 0.0f)
    {
        desired = m_replenishingTemplate->getNrOfMags();
    }
    else
    {
        desired = static_cast<int>(
            getAbility() / m_replenishingTemplate->getAbilityCost()
            + 1.0f - static_cast<float>(m_magazines[0]));
    }

    if (m_replenishingTemplate->getNrOfMags() <= desired)
        desired = m_replenishingTemplate->getNrOfMags();
    return desired > 0 ? desired : 1;
}

float ReplenishingAmmoComp::getAbility() const
{
    ISoldier* const soldier = getSoldier();
    if (soldier)
        return soldier->getCurrentAbility();

    IObject* const root = getRootParent(firearm());
    if (root->getTemplate()->getClassID() == CID_KitTemplate)
        return static_cast<Kit*>(root)->getCurrentAbility();
    return 0.0f;
}

void ReplenishingAmmoComp::taxAbility(float amount)
{
    ISoldier* const soldier = getSoldier();
    if (soldier)
        soldier->taxAbility(amount);
}

ISoldier* ReplenishingAmmoComp::getSoldier() const
{
    IPlayer* player = firearm()->getPlayer();
    if (!player)
        player = firearm()->getPlayerByForce();
    if (!player)
        return 0;

    IObject* vehicle = player->getVehicle();
    if (vehicle->getTemplate()->getClassID() != CID_SoldierTemplate)
    {
        IPlayerControlObject* const pco = static_cast<IPlayerControlObject*>(
            vehicle->queryInterface(IID_IPlayerControlObject));
        if (pco && pco->isChildSoldierActive())
            vehicle = pco->getOccupingObject();
    }

    if (vehicle
        && vehicle->getTemplate()->getClassID() == CID_SoldierTemplate)
    {
        return static_cast<ISoldier*>(vehicle->queryInterface(IID_ISoldier));
    }
    return 0;
}

std::vector<int>* ReplenishingAmmoComp::getAmmo()
{
    if (m_magazines[0] != -1)
    {
        int difference = getMagVectorSize() - static_cast<int>(m_magazines.size());
        if (difference < 0)
        {
            for (int i = 0; i < -difference; ++i)
                m_magazines.pop_back();
        }
        else
        {
            while (difference > 0)
            {
                m_magazines.push_back(m_replenishingTemplate->getMagSize());
                --difference;
            }
        }
    }
    return &m_magazines;
}

// Template

ReplenishingAmmoCompTemplate::ReplenishingAmmoCompTemplate()
    : AmmoCompTemplate()
    , m_refCount(1)
    , m_abilityDrain(-1.0f)
    // m_abilityDrainPerTick is intentionally untouched by the retail constructor.
    , m_abilityStrength(1.0f)
    // Retail does not run the setter here: its cached default is zero, not 1/30.
    , m_abilityStrengthPerTick(0.0f)
    , m_abilityCost(0.0f)
    , m_abilityMaterial(-1)
    , m_abilityRadius(1.0f)
    , m_replenishingType(RTHeal)
    , m_onlyActiveWhileFiring(false)
{
}

ReplenishingAmmoCompTemplate::~ReplenishingAmmoCompTemplate() {}

void ReplenishingAmmoCompTemplate::addRef() { ++m_refCount; }
int ReplenishingAmmoCompTemplate::getRef() { return m_refCount; }

int ReplenishingAmmoCompTemplate::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* ReplenishingAmmoCompTemplate::queryInterface(unsigned int iid) const
{
    ReplenishingAmmoCompTemplate* const self =
        const_cast<ReplenishingAmmoCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IWeaponCompTemplate)
        return static_cast<IWeaponCompTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int ReplenishingAmmoCompTemplate::getClassID() const
{
    return CID_ReplenishingAmmoCompTemplate;
}

IObjectComponent* ReplenishingAmmoCompTemplate::createComponent()
{
    return new ReplenishingAmmoComp(this);
}

void* ReplenishingAmmoCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ReplenishingAmmoCompTemplate || classId == CID_AmmoCompTemplate)
        return this;
    return 0;
}

void ReplenishingAmmoCompTemplate::setAbilityDrain(float value)
{
    m_abilityDrain = value;
    m_abilityDrainPerTick = value / 30.0f;
}

void ReplenishingAmmoCompTemplate::setAbilityStrength(float value)
{
    m_abilityStrength = value;
    m_abilityStrengthPerTick = value / 30.0f;
}

// . replenishingType is emitted unconditionally; the other fields use
// the exact retail defaults as their serialization gates.
bool ReplenishingAmmoCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!AmmoCompTemplate::makeScript(stream))
        return false;

    if (m_abilityDrain != -1.0f)
        script << "ObjectTemplate.ammo.abilityDrain " << m_abilityDrain << std::endl;
    if (m_abilityRadius != 1.0f)
        script << "ObjectTemplate.ammo.abilityRadius " << m_abilityRadius << std::endl;
    if (m_abilityStrength != 1.0f)
        script << "ObjectTemplate.ammo.abilityStrength " << m_abilityStrength << std::endl;
    if (m_abilityCost != 0.0f)
        script << "ObjectTemplate.ammo.abilityCost " << m_abilityCost << std::endl;
    if (m_abilityMaterial != -1)
        script << "ObjectTemplate.ammo.abilityMaterial " << m_abilityMaterial << std::endl;
    script << "ObjectTemplate.ammo.replenishingType " << m_replenishingType << std::endl;
    if (m_onlyActiveWhileFiring)
        script << "ObjectTemplate.ammo.onlyActiveWhileFiring "
               << m_onlyActiveWhileFiring << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
