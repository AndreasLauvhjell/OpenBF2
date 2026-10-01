
#include "AIObjectArmament.h"

#include <algorithm>

#include "AITemplateArmament.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)
#include "BFTimer.h"                                         // IBFTimer::getGameTime
#include "IAIObject.h"
#include "IWeapon.h"

#include "../SingletonServer.h"                              // BFSingletonServer::m_iInformationFactory
#include "../../AIDLL/AI/AIKnowledge/InformationFactory.h"   // IInformationFactory
#include "../../AIDLL/AI/Main/AISettingsInterfaces.h"        // IAISettings
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Game/Objects/Game/Soldier.h"
#include "../../../Game/Objects/IItemObject.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../Game/Objects/ObjectLayoutAccess.h"
#include "../../../Physics/IPhysicsNode.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace ai {

// Clears the eight weapon slots and handles and selects slot zero.
// lastFiredWeapon remains uninitialized until its setter is called.
AIObjectArmament::AIObjectArmament(IAIObject* object, AITemplateArmament* aiTemplate)
    : IAIObjectArmament(object), m_template(aiTemplate), m_activeWeapon(0)
{
    m_weapons.resize(8);
    for (unsigned int i = 0; i < m_weapons.size(); ++i)
        m_weapons[i] = 0;
}

// the set, then the vector. The weapons are components of their engine objects and
// are not this table's to delete.
AIObjectArmament::~AIObjectArmament()
{
}

int AIObjectArmament::getId() const
{
    return AIOFArmament;
}

// The handle is remembered (update prunes the set), then the
// factory builds the plug-in information.
void AIObjectArmament::createInformationPlugIn(unsigned int handle, bool real)
{
    m_informationHandles.insert(handle);
    static_cast<IInformationFactory*>(BFSingletonServer::m_iInformationFactory)
        ->requestInformationPlugin(handle, getId(), real);                         // +0x20
}

// Outside precaching, initializes the template and discovers weapons. Unit strength uses
// the largest ammunition capacity for each type; type-zero weapons also contribute.
// The settings pointer is cached, but the strength-type count is queried each iteration.
void AIObjectArmament::init()
{
    if (!static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getIsPrecaching())    // +0x68
        traverseObject(m_object->getObject());
    m_template->initFromObject(m_object->getObject());

    if (!m_object->isUnit())                                                        // +0x74
        return;

    IAIObjectUnit* const unit = m_object->getUnit();                                // +0x98
    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    for (int i = 0; i < settings->getNBattleStrengths(); ++i)                       // +0x34
        unit->setBattleStrength(i, 0);

    for (std::vector<IWeapon*>::iterator it = m_weapons.begin(); it != m_weapons.end(); ++it)
    {
        IWeapon* const weapon = *it;
        if (weapon && weapon->getAmmo() && !weapon->getWeaponType())
        {
            for (int i = 0; i < settings->getNBattleStrengths(); ++i)
                unit->setBattleStrength(i, std::max(unit->getBattleStrength(i), weapon->getStrength(i)));
        }
    }
}

// Initializes on every update, then releases handles the information factory no longer updates.
void AIObjectArmament::update()
{
    init();

    std::set<unsigned int>::iterator it = m_informationHandles.begin();
    while (it != m_informationHandles.end())
    {
        if (static_cast<IInformationFactory*>(BFSingletonServer::m_iInformationFactory)
                ->updateInformationPlugin(*it, getId()))
            ++it;
        else
            m_informationHandles.erase(it++);
    }
}

// the template's int, 1 or 2 in stock content.
int AIObjectArmament::isAntiAircraft() const
{
    return m_template->m_isAntiAircraft;
}

// Returns the active weapon slot for the object, or the applicable no-weapon sentinel.
int AIObjectArmament::getActiveWeapon() const
{
    world::IObjectTemplate* const objectTemplate =
        const_cast<world::IObjectTemplate*>(m_object->getObject()->getTemplate());
    if (objectTemplate->queryImplementation(world::CID_PlayerControlObjectTemplate))     // +0x50
    {
        if (m_weapons[m_activeWeapon])
            return static_cast<int>(m_activeWeapon);
        for (unsigned int i = 0; i < m_weapons.size(); ++i)
        {
            if (m_weapons[i])
                return static_cast<int>(i);
        }
        return -1;
    }
    if (objectTemplate->queryImplementation(world::CID_SoldierTemplate))
    {
        world::IObject* const weapon =
            static_cast<world::Soldier*>(m_object->getObject())->getActiveWeapon(0);
        if (!weapon)
            return -1;
        return static_cast<world::IItemObject*>(weapon->queryInterface(world::IID_IItemObject))
                   ->getItemIndex() - 1;                                             // +0x30
    }
    Debug(DebugTypeError, "AI/AIInterface/BFKnowledge/AIObjectArmament.cpp", 410, "")
        << "Unknown implementation of playercontrolobject";
    return -1;
}

// Only for the AI object's own side; an index past the table
// selects slot 0.
void AIObjectArmament::setActiveWeapon(unsigned int index, int side)
{
    if (side == m_object->getSide())                                                // +0x12C
        m_activeWeapon = index < m_weapons.size() ? index : 0;
}

// bounds-checked, unlike the in-hand reads.
IWeapon* AIObjectArmament::getWeapon(unsigned int index) const
{
    if (index < m_weapons.size())
        return m_weapons[index];
    return 0;
}

// The weapon in hand (getActiveWeapon) is remembered and stamped
// with the game time.
void AIObjectArmament::setLastFiredWeapon()
{
    m_lastFiredWeapon = getActiveWeapon();
    if (m_lastFiredWeapon != -1 && m_weapons[m_lastFiredWeapon])
    {
        m_weapons[m_lastFiredWeapon]->setTimeOfFiring(
            static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime());
    }
}

// The filled slot with the latest firing time (strictly later than
// 1.0 and than every earlier slot), or null. The time is asked twice for the winner, as shipped.
IWeapon* AIObjectArmament::getLastFiredWeapon() const
{
    IWeapon* lastFired = 0;
    double lastTime = -1.0;
    for (std::vector<IWeapon*>::const_iterator it = m_weapons.begin(); it != m_weapons.end(); ++it)
    {
        IWeapon* const weapon = *it;
        if (weapon && weapon->getTimeOfFiring() > lastTime)
        {
            lastFired = weapon;
            lastTime = weapon->getTimeOfFiring();
        }
    }
    return lastFired;
}

const std::vector<IWeapon*>& AIObjectArmament::getWeapons() const
{
    return m_weapons;
}

// Maps Select1..Select9 to slots 0..8, adding one for alternate fire. No selection maps
// to 0 for fire and 1 otherwise; unknown actions return -1. Callers must keep the result
// within the eight-slot weapon array; this mapping does not enforce that bound.
int AIObjectArmament::getWeaponIdFromActivationId(int activation, int fire) const
{
    const int altFire = fire == io::PIAltFire ? 1 : 0;
    switch (activation)
    {
    case io::PIWeaponSelect1: return altFire;
    case io::PIWeaponSelect2: return altFire + 1;
    case io::PIWeaponSelect3: return altFire + 2;
    case io::PIWeaponSelect4: return altFire + 3;
    case io::PIWeaponSelect5: return altFire + 4;
    case io::PIWeaponSelect6: return altFire + 5;
    case io::PIWeaponSelect7: return altFire + 6;
    case io::PIWeaponSelect8: return altFire + 7;
    case io::PIWeaponSelect9: return altFire + 8;
    case io::PINone:          return fire != io::PIFire ? 1 : 0;
    default:                  return -1;
    }
}

// Only for the AI object's own side.
bool AIObjectArmament::rearm(IWeapon* weapon, float amount, int side)
{
    if (m_object->getSide() == side)
        return weapon->refillAmmo(amount);                                          // +0xD8
    return false;
}

// The in-hand weapon's live projectile (getProjectile), placed absolutely.
// The in-hand slot is indexed unchecked.
bool AIObjectArmament::getProjectilePosition(Vec3& out) const
{
    IWeapon* const weapon = m_weapons[m_activeWeapon];
    if (!weapon)
        return false;
    world::IObject* const projectile = weapon->getProjectile();                     // +0x11C
    if (!projectile)
        return false;
    out = projectile->getAbsolutePosition();                                        // +0x6C
    return true;
}

// The same projectile's positional speed, from its physics node (+0x44
// read directly). The physics node is not null-tested.
bool AIObjectArmament::getProjectileSpeed(Vec3& out) const
{
    IWeapon* const weapon = m_weapons[m_activeWeapon];
    if (!weapon)
        return false;
    world::IObject* const projectile = weapon->getProjectile();
    if (!projectile)
        return false;
    out = static_cast<world::IPhysicsNode*>(world::object_detail::LayoutAccess::physicsNode(projectile))
              ->getPositionalSpeed();                                               // +0x60
    return true;
}

// Rebuilds the weapon table from the object and its children.
void AIObjectArmament::traverseObject(world::IObject* object)
{
    if (!object->queryInterface(world::IID_IObject))
        return;

    std::vector<IWeapon*> previous(m_weapons);
    m_weapons.resize(0);
    m_weapons.resize(8);

    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        object->queryInterface(world::IID_IPlayerControlObject));
    // Both answer a std::vector<IObject*> (IPlayerControlObject.h, [RETIRE-WHEN: pco-getweapons-vector]).
    const std::vector<world::IObject*>& allWeapons =
        *static_cast<const std::vector<world::IObject*>*>(pco->getWeapons());
    const std::vector<world::IObject*>& firstSlotWeapons =
        *static_cast<const std::vector<world::IObject*>*>(pco->getWeapons(0));
    const std::vector<world::IObject*>& weapons = allWeapons.size() ? allWeapons : firstSlotWeapons;
    for (std::vector<world::IObject*>::const_iterator it = weapons.begin(); it != weapons.end(); ++it)
    {
        IWeapon* const weapon = static_cast<IWeapon*>((*it)->queryComponent(IID_IWeapon, IID_IWeapon));
        if (weapon)
            m_weapons[getWeaponIdFromActivationId(weapon->getWeaponActivate(), weapon->getWeaponFire())] =
                weapon;
    }

    const std::vector<IWeapon*>& table = getWeapons();
    for (int i = 0; i < static_cast<int>(table.size()); ++i)
    {
        if (table[i])
            table[i]->update();                                                     // +0x40
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice
