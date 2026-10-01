// HUD message display and selection.
#include "HudMessages.h"
#if !DEDICATED
#include "../Core/EventManager.h"
#include "../Core/StringUtil.h"
#include "../Core/Ids.h"
#include "../Game/HudInformationLayer.h"
#include "../Game/Main/BF2ClientSeams.h"
#include "../Game/Objects/GenericFireArmTemplate.h"
#include "../Game/Objects/Weapons/GenericFireArm.h"
#include "../Game/Objects/IPlayer.h"
#include "../Game/Objects/IWeaponHudComp.h"
#include "../Game/Objects/IVehicleHudComp.h"
#include "../Game/Objects/WeaponHudCompTemplate.h"
#include "../Scene/Object/PlayerManager.h"
#include "../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../Sound/BfSound.h"
#include <cstring>
#include <list>
namespace dice { namespace hfe {
namespace world { extern PlayerManager* playerManager; }
namespace hud {
namespace {
// PE440060: no ownership transfer, first matching interface on the parent chain.
world::IPlayerControlObject* controlAncestor(world::IObject* object)
{
    while (object) {
        auto* control = static_cast<world::IPlayerControlObject*>(
            object->queryInterface(world::IID_IPlayerControlObject));
        if (control) return control;
        object = object->getParent();
    }
    return nullptr;
}
// The interface's chosen void* return already denotes this native vector; see
// IPlayerControlObject.h / pco-getweapons-vector. PE6A7D40 is vector assignment,
// not append; 4F1C40 is the copy constructor used by populateItems's value argument.
const std::vector<world::IObject*>& weaponList(world::IPlayerControlObject* control, int slot)
{
    return *static_cast<const std::vector<world::IObject*>*>(control->getWeapons(slot));
}
}
char HudMessages::s_emptyIcon = 0;
HudMessages::HudMessages()
    : m_enabled(true), m_visible(false), m_selectedItem(-1), m_field010(0),
      m_itemCount(0), m_duration(10), m_remainingTime(10), m_field020(0.3), m_field028(0)
{
    eventManager->registerEventHandler(ECGame, this, 0);
    eventManager->registerEventHandler(ECHud, this, 0);
    for (Item& item : m_items) {
        item.icon = &s_emptyIcon;
        item.id = 0;
        item.active = false;
    }
}
HudMessages::~HudMessages()
{
    eventManager->unregisterEventHandler(ECGame, this);
    eventManager->unregisterEventHandler(ECHud, this);
    for (Item& item : m_items) if (item.icon != &s_emptyIcon) delete[] item.icon;
}
void HudMessages::handleEvent(EventCategory, unsigned int, EventNode*, void*) {} // RET16
void HudMessages::init() {} // RET
bool HudMessages::slot_0x0c() { return false; } // XOR AL,AL; RET
bool HudMessages::registerData()
{
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerBool("ShowItemSelect", &m_visible);
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerInt("ItemSelectActive", &m_selectedItem);
    for (int i = 0; i < 10; ++i) {
        {
            const std::string name = formatString("ItemSlot%iActive", i);
            static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerBool(name, &m_items[i].active);
        }
        {
            const std::string name = formatString("Item%iIcon", i);
            static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerString(name, &m_items[i].icon);
        }
        {
            const std::string name = formatString("ItemSlot%iBackActiveAlpha", i);
            static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerFloat(name, &m_items[i].backgroundAlpha);
        }
        {
            const std::string name = formatString("ItemSlot%iActiveAlpha", i);
            static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerFloat(name, &m_items[i].activeAlpha);
        }
    }
    return true;
}
bool HudMessages::update(float delta)
{
    if (m_visible && m_remainingTime >= 0) m_remainingTime -= delta;
    else m_visible = false;
    return m_visible;
}
int HudMessages::getSelectedItem() { return m_items[m_selectedItem].id; }
// 79E610 (66 B). The mouse wheel's next step: the following slot when it exists and is active,
// otherwise a wrap to slot [0] - but only when [0] is active. Both select with the sound.
// No lower bound on m_selectedItem, as shipped (-1, the constructor's value, steps to [0]).
void HudMessages::selectNext(int)
{
    if (!m_itemCount)
        return;
    if (m_selectedItem + 1 < m_itemCount && m_items[m_selectedItem + 1].active) {
        selectItem(m_items[m_selectedItem + 1].id, true);
        return;
    }
    if (m_items[0].active)
        selectItem(m_items[0].id, true);
}
// 79E660 (68 B). The previous step: the slot before when there is one and it is active, otherwise
// the last slot when it is active, otherwise slot [0].
// Unlike selectNext's wrap, that final fallback does not test [0]'s active byte - it selects
// [0] regardless, as shipped.
void HudMessages::selectPrevious(int)
{
    if (!m_itemCount)
        return;
    if (m_selectedItem > 0 && m_items[m_selectedItem - 1].active) {
        selectItem(m_items[m_selectedItem - 1].id, true);
        return;
    }
    if (m_items[m_itemCount - 1].active)
        selectItem(m_items[m_itemCount - 1].id, true);
    else
        selectItem(m_items[0].id, true);
}
int HudMessages::findItem(int item)
{
    for (int i = 0; i < m_itemCount; ++i) if (m_items[i].id == item) return i;
    return 0;
}
int HudMessages::confirmSelection()
{
    const int selected = m_selectedItem;
    m_visible = false;
    return m_items[selected].id;
}
void HudMessages::assignIcon(const char*& destination, const char* source)
{
    if (!source) return;
    if (std::strcmp(source, destination) != 0) {
        if (destination && destination != &s_emptyIcon) {
            delete[] destination;
            destination = nullptr;
        }
        const std::size_t length = std::strlen(source) + 1;
        char* copy = new char[length];
        std::memcpy(copy, source, length);
        destination = copy;
    }
}
void HudMessages::compactItems()
{
    for (int index = 0; index < 10; ++index) {
        if (m_items[index].active) continue;
        int next = index + 1;
        while (next < 10 && !m_items[next].active) ++next;
        if (next >= 10) return;
        if (next != index) {
            assignIcon(m_items[index].icon, m_items[next].icon);
            m_items[index].id = m_items[next].id;
            m_items[index].active = m_items[next].active;
            m_items[next].id = -1;
            m_items[next].active = false;
        }
    }
}
void HudMessages::populateItems(std::vector<world::IObject*> objects)
{
    for (world::IObject* object : objects) {
        if (!object) continue;
        const auto* definition = static_cast<const world::GenericFireArmTemplate*>(object->getTemplate());
        if (definition->m_isGasMask || definition->m_isNightVision) continue;
        auto* item = static_cast<world::IItemObject*>(object->queryInterface(world::IID_IItemObject));
        if (!item) continue;
        int index = item->getItemIndex();
        if (index < 0 || index >= 9) continue;
        if (m_enabled) index = 10 - index;
        // Retail admits item0 and, in reversed order, writes past the ten slots.
        // Preserve that observed defect; no fabricated count/index clamp.
        if (object->m_weaponHudComponent) {
            auto* hud = static_cast<world::IWeaponHudComp*>(object->m_weaponHudComponent);
            assignIcon(m_items[index].icon, hud->getHudIcons()->selectIcon.c_str());
        }
        m_items[index].id = item->getItemIndex();
        m_items[index].active = true;
        ++m_itemCount;
    }
}
void HudMessages::selectItem(int item, bool playSound)
{
    m_selectedItem = findItem(item);
    m_remainingTime = m_duration;
    if (playSound) g_bfSound->playSound(std::string("weaponOver"));
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player) return;
    world::IObject* vehicle = player->getVehicle();
    if (!vehicle) return;
    // The unused string list precedes the vector in PE79E337 and is destroyed
    // last. 4055F0 allocates its36-byte sentinel;4072F0 destroys string nodes.
    std::list<std::string> names;
    std::vector<world::IObject*> objects;
    if (vehicle == player->getVehicle()) {
        auto* control = controlAncestor(vehicle);
        if (control) objects = weaponList(control, 0);
    }
    for (world::IObject* object : objects) {
        if (!object) continue;
        auto* selected = static_cast<world::IItemObject*>(object->queryInterface(world::IID_IItemObject));
        if (!selected) continue;
        const int index = selected->getItemIndex();
        // PE uses the immediately following IWeaponObject subobject, with no
        // extra query. ELF82565D0 proves IItem+154 / IWeapon+158 on GenericFireArm.
        if (m_selectedItem + 1 == index
            && !static_cast<world::IWeaponObject*>(static_cast<world::GenericFireArm*>(selected))->getWorksInWater()
            && player->getIsUnderwater()) {
            m_items[m_selectedItem].backgroundAlpha = 0.4f;
            m_items[m_selectedItem].activeAlpha = 0.7f;
        } else {
            m_items[index - 1].backgroundAlpha = 1.0f;
            m_items[index - 1].activeAlpha = 1.0f;
        }
    }
}
// Refreshes the item list, selects the held item and updates visibility from the item count.
// A failed prerequisite leaves the previous selection intact. An unknown item id selects the first entry.
bool HudMessages::showItems()
{
    if (world::IPlayer* const player = world::playerManager->getLocalHumanPlayer())
    {
        // PE +0xA0 = ELF +0xA4 getDefaultVehicle; PE +0x44 = ELF +0x48 getVehicle.
        world::IObject* const source = refreshItems() ? player->getDefaultVehicle()
                                                      : player->getVehicle();
        if (world::IPlayerControlObject* const pco = world::findOwningPlayerControlObject(source))
        {
            // Returns the active weapon for the current item display.
            if (world::IObject* const held = pco->getActiveWeapon(m_field010))   // PE +0x74
            {
                if (world::IItemObject* const item = static_cast<world::IItemObject*>(
                        held->queryInterface(world::IID_IItemObject)))          // 50332
                    m_selectedItem = findItem(item->getItemIndex());            // +0x30
            }
        }
    }
    m_visible = m_itemCount > 0;             // +09 from +14, a signed test (setnle)
    m_remainingTime = m_duration;            // +1C from +18
    return m_visible;
}

bool HudMessages::refreshItems()
{
    m_itemCount = 0;
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    bool fallback = false;
    if (!player) return false;
    world::IObject* vehicle = player->getVehicle();
    if (!vehicle) return false;
    m_field010 = 0;
    std::list<std::string> names;
    std::vector<world::IObject*> objects;
    auto* control = controlAncestor(vehicle);
    if (control) objects = weaponList(control, 0);
    if (objects.empty() && vehicle != player->getDefaultVehicle()) {
        world::IObject* soldier = player->getDefaultVehicle();
        if (soldier) {
            auto* fallbackControl = controlAncestor(soldier);
            if (fallbackControl) {
                objects = weaponList(fallbackControl, 0);
                fallback = true;
            }
        }
    }
    for (Item& item : m_items) item.active = false;
    auto* vehicleHud = static_cast<world::IVehicleHudComp*>(vehicle->m_vehicleHudComponent);
    if (vehicleHud && control && player->getDefaultVehicle() && vehicle != player->getDefaultVehicle()
        && vehicleHud->getHudInfo()->useSelectionIcons) {
        const std::size_t oldCount = objects.size();
        if (oldCount < weaponList(control, 1).size()) {
            m_field010 = 1;
            objects = weaponList(control, 1);
        }
    }
    populateItems(objects);
    compactItems();
    return fallback;
}
} } }
#endif
