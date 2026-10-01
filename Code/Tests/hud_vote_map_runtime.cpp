// [TEMP] [CHOSEN, not recovered] Actual VoteMap and list-data methods with
// typed callback observers. No production method is replaced. Uncalled wider
// UI/game bodies remain /FORCE frontiers; this is not client-init acceptance.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <new>
#define private public
#define protected public
#include "../BF2/Menu/HudVoteMap.h"
#include "../BF2/Menu/BfListBoxData.h"
#undef protected
#undef private
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
IBase* g_hudInformationLayer = nullptr;
namespace world {
PlayerManager* playerManager = nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
}
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
using namespace dice::meme;
static int checks, failures;
static void check(bool value, const char* label) {
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr,"FAIL %s\n",label); }
}
struct Players : world::PlayerManager {
    int queries = 0;
    world::IPlayer* getLocalHumanPlayer() override { ++queries; return nullptr; }
};
struct Events : EventManager {
    HudVoteMap* constructing = nullptr;
    IEventListener* registered = nullptr;
    bool registerEventHandler(EventCategory category, IEventListener* listener, int priority) override {
        if (constructing) {
            check(category == ECHud && priority == 0, "ctor registers ECHud at priority0");
            registered = listener;
            check(static_cast<HudVoteMap*>(listener) == constructing, "secondary listener points to owner");
            check(reinterpret_cast<char*>(listener) - reinterpret_cast<char*>(constructing) == 4, "listener adjustment is4");
            check(constructing->m_field028 == static_cast<int>(0xa5a5a5a5), "field028 write is after registration");
            check(constructing->m_columnWidth == static_cast<int>(0xa5a5a5a5), "column width write is after registration");
            check(constructing->m_field034.empty(), "string is constructed before registration");
        }
        return EventManager::registerEventHandler(category, listener, priority);
    }
};
struct Information : HudInformationLayer {
    int bindings = 0;
    bool* bound = nullptr;
    int registerBool(const std::string& name, bool* value) override {
        ++bindings; bound = value;
        check(name == "VoteMapSelected", "exact bound name");
        return -1; // Only this observed registration return; retail discards it.
    }
};
static void cleanupData(BfListBoxData* data) {
    // Retail leaks columns/cells. Explicit fixture cleanup owns only test data.
    for (auto* column : data->m_columns) if (column) {
        for (auto* cell : column->cells) delete cell;
        delete column;
    }
    delete data;
}
int main() {
    static_assert(sizeof(void*) == 4, "x86 fixture required");
    Events events; eventManager = &events;
    Players players; world::playerManager = &players;
    Information information; g_hudInformationLayer = &information;
    events.unregisterEventHandler(ECHud, &information); // Its wider listener is not this fixture's subject.
    alignas(HudVoteMap) unsigned char memory[sizeof(HudVoteMap)];
    std::memset(memory, 0xa5, sizeof(memory));
    events.constructing = reinterpret_cast<HudVoteMap*>(memory);
    HudVoteMap* map = new(memory) HudVoteMap(nullptr);
    events.constructing = nullptr;
    check(map->m_system == nullptr && map->m_field00c == 0, "ctor system and zero field");
    check(map->m_field010 == 0xa5a5a5a5 && map->m_field014 == 0xa5a5a5a5, "ctor preserves unknown fields");
    check(reinterpret_cast<std::uintptr_t>(map->m_data) == 0xa5a5a5a5, "ctor preserves list pointer");
    check(map->m_visibleRows == 15 && map->m_rowCount == 0, "ctor row settings");
    check(!map->m_visible && !map->m_selected && map->m_rowOffset == 2, "ctor flags and offset");
    check(map->m_field028 == 12 && map->m_columnWidth == 160, "ctor late defaults");
    IHudVoteMap* primary = map;
    auto* system = reinterpret_cast<System*>(0x1234);
    primary->setSystem(system);
    check(map->m_system == system, "slot0 writes borrowed system");
    check(primary->registerFunctions(), "slot8 retail unconditional true");
    map->handleEvent(ECGame, 99, nullptr, nullptr);
    check(players.queries == 0, "non-HUD event does not query player");
    map->handleEvent(ECHud, 99, nullptr, nullptr);
    check(players.queries == 1, "HUD event always queries once independent of id");
    events.postEvent(ECHud, 123, nullptr, 0);
    check(players.queries == 2, "real synchronous event reaches adjusted listener");
    check(primary->registerData(), "data registration ignores false binding result");
    check(information.bindings == 1 && information.bound == &map->m_selected, "binding keeps actual selection address");
    BfListBoxData* data = map->m_data;
    check(std::string(data->getName()) == "LevelsList", "list name");
    check(data->m_columns.size() == 4 && data->m_columnCount == 4, "default plus two visible plus hidden columns");
    for (int i=0;i<4;++i) {
        auto* column = data->m_columns[i];
        check(column && column->index == i && column->cells.empty(), "column index and empty native vector");
        check(column->width == ((i == 1 || i == 2) ? 160 : 0), "column width");
        check(column->enabled == (i != 3) && column->field019 && !column->field01a, "hidden column flags");
    }
    check(data->m_visibleRows == 15 && data->m_field021 && !data->m_field023 && !data->m_field022, "list flags and row limit");
    check(map->getSelectedMap().empty(), "initial negative selected row returns empty");
    auto* cell = new ListBoxCell(L"unrelated wide text",1,1,1,1,0);
    cell->narrow = std::string("dalian\0suffix",13);
    data->m_columns[3]->cells.push_back(cell);
    data->setRow(0);
    check(data->getSelectedString(3).size() == 13, "data returns full narrow string including embedded NUL");
    check(map->getSelectedMap() == "dalian", "widget reconstructs through c_str and truncates embedded NUL");
    check(data->getSelectedString(-1).empty() && data->getSelectedString(4).empty(), "unsigned column bounds");
    data->setRow(1);
    check(data->getSelectedString(3).empty(), "selected row upper bound");
    data->setRow(-2);
    check(data->getSelectedString(3).empty(), "selected row unsigned lower bound");
    auto* savedColumn = data->m_columns[3]; data->m_columns[3] = nullptr;
    check(data->getSelectedString(3).empty(), "null column guard");
    data->m_columns[3] = savedColumn;
    data->setRow(0); savedColumn->cells[0] = nullptr;
    check(data->getSelectedString(3).empty(), "null cell guard");
    savedColumn->cells[0] = cell;
    map->~HudVoteMap();
    events.postEvent(ECHud, 5, nullptr, 0);
    check(players.queries == 2, "dtor unregisters listener");
    check(data->m_columns[3]->cells[0] == cell, "dtor leaves list-data ownership untouched");
    cleanupData(data);
    std::printf("HUD vote map: %d/%d checks passed\n",checks-failures,checks);
    return failures ? 1 : 0;
}
