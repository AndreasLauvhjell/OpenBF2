// [INFERRED] Source placement.

#include "AIObjectUnit.h"

#include <string>
#include <vector>

#include "AITemplateUnit.h"
#include "IWeapon.h"
#include "KitTemplate.h"
#include "KitTemplateManager.h"

#include "../SingletonServer.h"                              // BFSingletonServer::m_iInformationFactory
#include "../../AIDLL/AI/AIKnowledge/InformationFactory.h"        // IInformationFactory
#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../AIDLL/AI/Main/AISettingsInterfaces.h"             // IAISettings
#include "../../AIDLL/AI/Main/BFPathfindingCourier.h"             // IAIPathfindingCourier
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Game/Objects/Base/SimpleObjectTemplate.h"
#include "../../../Game/Objects/Game/Kit.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../Game/Objects/IPlayer.h"
#include "../../../Game/Objects/ISoldier.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

IAISettings* settings() { return static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings); }

// The walk the occupancy queries share: up the m_parent chain (+0x28) to the first object that
// answers IID_IPlayerControlObject.
world::IPlayerControlObject* findPlayerControlObject(world::IObject* object)
{
    for (world::IObject* o = object; o; o = o->getParent())
    {
        world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
            o->queryInterface(world::IID_IPlayerControlObject));
        if (pco)
            return pco;
    }
    return 0;
}

} // anonymous namespace

// Allocates battle and strategic arrays using one settings lookup. Only battle values
// are zeroed; strategic values remain uninitialized until init copies them.
AIObjectUnit::AIObjectUnit(IAIObject* object, AITemplateUnit* aiTemplate)
    : IAIObjectUnit(object), m_template(aiTemplate), m_lastPlayerPossesionTime(-1000.0),
      m_missileLockStatus(0)
{
    m_entryPositionOfNonMobile.x = 0.0f;
    m_entryPositionOfNonMobile.y = 0.0f;
    m_entryPositionOfNonMobile.z = 0.0f;

    IAISettings* const aiSettings = settings();
    const int battleCount = aiSettings->getNBattleStrengths();                      // +0x34
    m_battleStrengths = checkPointer(new int[battleCount]);
    m_strategicStrengths = checkPointer(new int[aiSettings->getNStrategicStrengths()]);   // +0x38
    for (int i = 0; i < battleCount; ++i)
        m_battleStrengths[i] = 0;
}

AIObjectUnit::~AIObjectUnit()
{
    delete[] m_battleStrengths;
    delete[] m_strategicStrengths;
}

int AIObjectUnit::getId() const
{
    return AIOFUnit;
}

void AIObjectUnit::createInformationPlugIn(unsigned int handle, bool real)
{
    static_cast<IInformationFactory*>(BFSingletonServer::m_iInformationFactory)
        ->requestInformationPlugin(handle, getId(), real);                         // +0x20
}

// Initializes ammunition and strengths from the object, kit and weapons.
// For soldiers, weapon strengths use the maximum in each category.
// An unsupported kit template leaves the existing strength and ammunition values unchanged.
void AIObjectUnit::init()
{
    if (!m_template->m_initialized)
        m_template->initFromObject(m_object->getObject());

    if (m_object->getObject()->getTemplate()->getClassID() != world::CID_SoldierTemplate)
    {
        const int strategicCount = settings()->getNStrategicStrengths();            // once
        for (int i = 0; i < strategicCount; ++i)
            m_strategicStrengths[i] = m_template->m_strategicStrengths[i];
        for (int i = 0; i < settings()->getNBattleStrengths(); ++i)
            m_battleStrengths[i] = m_template->m_battleStrengths[i];
        m_maxAmmo = m_template->m_maxAmmo;

        if (!m_object->isMobile() || m_template->m_useNoPathfindingToGetToObject)   // +0x70
        {
            const Vec3& position = m_object->getPosition();                         // +0xD4
            Vec3 target = position;
            const Mat4& transformation = m_object->getTransformation();             // +0xDC
            const float* const m = transformation.m;
            target.x = static_cast<float>(-static_cast<double>(m[8]) * 12.0 + target.x);
            target.y = static_cast<float>(-static_cast<double>(m[9]) * 12.0 + target.y);
            target.z = static_cast<float>(-static_cast<double>(m[10]) * 12.0 + target.z);
            static_cast<IAIPathfindingCourier*>(CommonSingletonServer::m_iaiPathfindingCourier)
                ->findClosestValidInfantryPolygon(m_entryPositionOfNonMobile, position, target, 10);   // +0x20
        }
        return;
    }

    world::ISoldier* const soldier = static_cast<world::ISoldier*>(
        m_object->getObject()->queryInterface(world::IID_ISoldier));
    world::Kit* const kit = soldier->getKit();                                      // +0x54
    KitTemplate* kitTemplate = 0;
    if (kit)
    {
        world::IObjectTemplate* const kitObjectTemplate =
            const_cast<world::IObjectTemplate*>(static_cast<world::IObject*>(kit)->getTemplate());
        if (!kitObjectTemplate->queryImplementation(world::CID_SimpleObjectTemplate))   // +0x50
            return;
        const world::SimpleObjectTemplate* const simpleTemplate =
            static_cast<const world::SimpleObjectTemplate*>(
                kitObjectTemplate->queryImplementation(world::CID_SimpleObjectTemplate));
        const std::string kitName(simpleTemplate->m_aiTemplate);
        kitTemplate = KitTemplateManager::m_instance->getTemplate(kitName);
        if (kitTemplate)
        {
            for (int i = 0; i < settings()->getNStrategicStrengths(); ++i)
                m_strategicStrengths[i] = kitTemplate->getStrategicStrength(i);     // +0x08
        }
        else
        {
            for (int i = 0; i < settings()->getNStrategicStrengths(); ++i)
                m_strategicStrengths[i] = m_template->m_strategicStrengths[i];
        }
    }
    else
    {
        for (int i = 0; i < settings()->getNStrategicStrengths(); ++i)
            m_strategicStrengths[i] = m_template->m_strategicStrengths[i];
    }

    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        static_cast<IBase*>(soldier)->queryInterface(world::IID_IPlayerControlObject));
    const std::vector<world::IObject*>* const weapons =
        static_cast<const std::vector<world::IObject*>*>(pco->getWeapons());       // ELF +0x6C
    m_maxAmmo = 0;
    if (weapons->begin() == weapons->end())
    {
        if (kitTemplate)
        {
            for (int i = 0; i < settings()->getNBattleStrengths(); ++i)
                m_battleStrengths[i] = kitTemplate->getBattleStrength(i);           // +0x14
        }
        else
        {
            for (int i = 0; i < settings()->getNBattleStrengths(); ++i)
                m_battleStrengths[i] = m_template->m_battleStrengths[i];
        }
        return;
    }

    for (int i = 0; i < settings()->getNBattleStrengths(); ++i)
        m_battleStrengths[i] = 0;
    for (std::vector<world::IObject*>::const_iterator it = weapons->begin(); it != weapons->end(); ++it)
    {
        IWeapon* const weapon =
            static_cast<IWeapon*>((*it)->queryComponent(IID_IWeapon, IID_IWeapon));  // +0x5C
        if (!weapon)
            continue;
        for (int j = 0; j < settings()->getNBattleStrengths(); ++j)
        {
            const int theirs = weapon->getStrength(j);                              // +0xA4
            const int mine = m_battleStrengths[j];
            m_battleStrengths[j] = (theirs >= mine) ? theirs : mine;
        }
        if (m_maxAmmo >= 0)
        {
            if (weapon->getMaxAmmo() < 0)                                           // +0xC4
                m_maxAmmo = -1;
            else
                m_maxAmmo += weapon->getMaxAmmo();
        }
    }
}

// an update is A full RE-init, through the slot.
void AIObjectUnit::update()
{
    init();                                                                         // +0x10
}

// Occupancy and team-lock queries require a reachable player-control object; lookup
// results are used without null checks.
bool AIObjectUnit::isOccupied() const
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object->getObject());
    if (pco->isOccupied())                                                          // +0x18
        return pco->getOccupingPlayer() != 0;                                       // +0x10
    return false;
}

IAIObject* AIObjectUnit::getOccupier() const
{
    world::IPlayerControlObject* const pco = findPlayerControlObject(m_object->getObject());
    if (pco->isOccupied())
    {
        world::IObject* const occupant = pco->getOccupingObject();                  // +0x14
        if (occupant)
            return static_cast<IAIObject*>(occupant->getAIComponent());
    }
    return 0;
}

int AIObjectUnit::getTeamLock() const
{
    return findPlayerControlObject(m_object->getObject())->getTeam();               // +0x98
}

int AIObjectUnit::getOccupyingPlayerId() const
{
    world::IPlayer* const player =
        findPlayerControlObject(m_object->getObject())->getOccupingPlayer();
    if (player)
        return player->getId();                                                     // +0x54
    return -1;
}

// The control object above this one (the walk starts at the parent), as that
// object's AI component. The engine object is asked for twice, the first answer discarded.
IAIObject* AIObjectUnit::getOccupiedObject() const
{
    m_object->getObject();
    world::IObject* const parent = m_object->getObject()->getParent();
    if (!parent)
        return 0;
    world::IPlayerControlObject* const pco = findPlayerControlObject(parent);
    if (!pco)
        return 0;
    world::IObject* const object =
        static_cast<world::IObject*>(pco->queryInterface(world::IID_IObject));
    if (object)
        return static_cast<IAIObject*>(object->getAIComponent());
    return 0;
}

// the walk is done and ignored - every unit's entry radius is 10.
float AIObjectUnit::getEntryRadius() const
{
    findPlayerControlObject(m_object->getObject());
    return 10.0f;
}

// A seat that has no cameras of its own borrows the vehicle's armament.
// No null test on the vehicle's control object.
bool AIObjectUnit::mustBorrowArmament(IAIObject* vehicle) const
{
    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        vehicle->getObject()->queryInterface(world::IID_IPlayerControlObject));
    return pco->getOwnCameras()->empty();                                           // +0x3C
}

// template reads (the equipment type is the template's +0x10).
int AIObjectUnit::getEquipmentType() const
{
    return m_template->m_equipmentType;
}

bool AIObjectUnit::useNoPathfindingToGetToObject() const
{
    return m_template->m_useNoPathfindingToGetToObject;
}

bool AIObjectUnit::hasExposedSoldier() const
{
    return m_template->m_hasExposedSoldier;
}

bool AIObjectUnit::getAlwaysTargetSoldier() const
{
    return m_template->m_alwaysTargetSoldier;
}

io::PlayerInputMap AIObjectUnit::getSelectKey() const
{
    return m_template->m_selectKey;
}

float AIObjectUnit::getCullDistance() const
{
    return m_template->m_cullDistance;
}

float AIObjectUnit::getCullDistanceSqr() const
{
    return m_template->m_cullDistanceSquared;
}

void AIObjectUnit::setMissileLockStatus(int status)
{
    m_missileLockStatus = status;
}

int AIObjectUnit::getMissileLockStatus() const
{
    return m_missileLockStatus;
}

// plain field access; the strength indices are not bounds-checked.
int AIObjectUnit::getMaxAmmo() const
{
    return m_maxAmmo;
}

const int* AIObjectUnit::getBattleStrengths() const
{
    return m_battleStrengths;
}

const int* AIObjectUnit::getStrategicStrengths() const
{
    return m_strategicStrengths;
}

int AIObjectUnit::getBattleStrength(int index) const
{
    return m_battleStrengths[index];
}

int AIObjectUnit::getStrategicStrength(int index) const
{
    return m_strategicStrengths[index];
}

void AIObjectUnit::setBattleStrength(int index, int value)
{
    m_battleStrengths[index] = value;
}

void AIObjectUnit::setStrategicStrength(int index, int value)
{
    m_strategicStrengths[index] = value;
}

double AIObjectUnit::getLastPlayerPossesionTime() const
{
    return m_lastPlayerPossesionTime;
}

void AIObjectUnit::setLastPlayerPossesionTime(double time)
{
    m_lastPlayerPossesionTime = time;
}

const Vec3& AIObjectUnit::getEntryPositionOfNonMobile() const
{
    return m_entryPositionOfNonMobile;
}

} // namespace ai
} // namespace hfe
} // namespace dice

// The attested part of the class - see AIObjectUnit.h.
#include "AIObjectUnit.inl"
