// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-LIST-BOX-NODE-FIXTURE]
// Tests list-box layout, builder bindings, and no-data painting with real storage and registry.
// Only getListData and System use observers. Event/stream frontiers are excluded; updates have a separate fixture.
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <list>
#include <windows.h>
#include "../BF2/Core/Debug.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/HudInformationLayer.h"
#define private public
#define protected public
#include "../BF2/Menu/Bf2HudBuilder.h"
#include "../BF2/Menu/Bf2NewListBoxNode.h"
#include "../BF2/Menu/BfListBoxData.h"
#include "../BF2/Menu/Bf2ActionListAction.h"
#include "../BF2/Menu/Bf2TransformNode.h"
#include "../BF2/Menu/BF2HudManager.h"
#undef protected
#undef private
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Quad.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/Menu/Bf2HudBuilder.cpp"

namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
class ServerSettings;
ServerSettings* serverSettings = nullptr;
class IGameLogic;
IGameLogic* gameLogic = nullptr;
// ds:A10890. Only BF2Engine.cpp defines it in production, and this fixture does
// not link that unit - so without this definition /force resolves the symbol to
// address zero and the row that publishes the singleton writes through a null.
hud::BF2HudManager* hudManager = nullptr;
} }

using namespace dice::meme;
using namespace dice::hfe;
using dice::hfe::hud::Bf2HudBuilder;
using dice::hfe::hud::BF2HudManager;
using dice::hfe::hud::HudStringFunction;

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; \
    std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)

static std::vector<std::string> executed;
static bool __fastcall recordConsoleCommand(const std::string& command)
{
    executed.push_back(command);
    return true;
}

class PlainSystem : public System
{
public:
    explicit PlainSystem(NameTreeNode& registry) : System(800, 600)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    int loadFont(const char* name) override
    { fonts.push_back(name); return 300 + int(fonts.size()); }
    bool drawTexture(int texture, Quad* quad) override
    { textures.push_back(texture); quads.push_back(*quad); return false; }
    std::vector<std::string> fonts;
    std::vector<int> textures;
    std::vector<Quad> quads;
};
class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }
static HudInformationLayer* information;

// [TEMP] the row's data factory is the HUD singleton's +1CC slot; only that one
// slot is substituted, and the manager is torn down with the same storage
// teardown hud_vehicle_construct_runtime and hud_map_create_runtime use.
struct FixtureHud : BF2HudManager
{
    BfListBoxData* answer = nullptr;
    int lastSelector = 0;
    int queries = 0;
    BfListBoxData* getListData(int selector) override
    { ++queries; lastSelector = selector; return answer; }
};
static void destroyHudStorage(BF2HudManager* singleton)
{
    delete singleton->m_field2dc;
    using W = std::wstring; using S = std::string; using L = std::list<int>;
    using SL = std::list<std::string>; using V = decltype(singleton->m_field32c);
    singleton->m_field010.~W(); singleton->m_field038.~W(); singleton->m_field060.~W();
    singleton->m_field08c.~S(); singleton->m_field0c0.~S(); singleton->m_field304.~S();
    singleton->m_field0e8.~L(); singleton->m_field2e0.~SL(); singleton->m_field33c.~SL();
    singleton->m_field32c.~V();
    ::operator delete(singleton);
}

// [TEMP] a System whose pointer position and pointer owner the test controls, which is what
// onEvent1's two region arms are gated on.
class OwningSystem : public PlainSystem
{
public:
    explicit OwningSystem(NameTreeNode& registry) : PlainSystem(registry) {}
    Node* getPointerOwner() override { return owner; }
    bool playSound(int sound, float volume) override
    { sounds.push_back(sound); volumes.push_back(volume); return true; }
    void setPointer(float x, float y) { m_pointerX = x; m_pointerY = y; }
    Node* owner = nullptr;
    std::vector<int> sounds;
    std::vector<float> volumes;
};

struct RecordingAction : Action
{
    int runs = 0;
    bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent*) override
    { ++runs; return true; }
};

// onEvent1 (7C40A0)
// The node's input entry point. Every arm answers false, including the ones that acted.
static void onEventCases()
{
    NameTreeNode registry;
    OwningSystem system(registry);

    // the guard: no data, nothing happens
    {
        Bf2NewListBoxNode node("NoData");
        node.m_firstRow = 7;
        CHECK(node.onEvent1(&system, nullptr, nullptr, 1, 0, 0) == false);
        CHECK(node.m_firstRow == 7);
    }

    BfListBoxData data("Rows");
    Bf2NewListBoxNode node("List");
    node.setDataForHud(&data);
    node.bindDataToSelfForHud();
    data.addColumn(40, true, true);
    for (int i = 0; i < 8; ++i)
        data.appendCell(0, "row", 1.0f, 1.0f, 1.0f, 1.0f, 0);
    CHECK(data.getRowCount() == 8);

    node.m_visibleRows = 3;
    node.m_rowCount = 8;

    // the seven navigation codes, each answering false
    node.m_firstRow = 5;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 19, 0, 0) == false);   // view up one row
    CHECK(node.m_firstRow == 4);
    node.m_firstRow = 0;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 19, 0, 0) == false);
    CHECK(node.m_firstRow == 0);                                         // clamped, not negative

    node.m_firstRow = 0;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 20, 0, 0) == false);   // view down one row
    CHECK(node.m_firstRow == 1);
    node.m_firstRow = 99;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 20, 0, 0) == false);
    CHECK(node.m_firstRow == 5);                                         // clamped to count-visible

    node.m_firstRow = 7;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 11, 0, 0) == false);   // view up one page
    CHECK(node.m_firstRow == 4);
    node.m_firstRow = 1;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 11, 0, 0) == false);
    CHECK(node.m_firstRow == 0);

    node.m_firstRow = 0;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 12, 0, 0) == false);   // view down one page
    CHECK(node.m_firstRow == 3);

    data.selectRow(4, false);
    CHECK(node.onEvent1(&system, nullptr, nullptr, 6, 0, 0) == false);    // selection up
    CHECK(data.m_row == 3);

    const int beforeSwallow = node.m_firstRow;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 18, 0, 0) == false);   // swallowed
    CHECK(node.m_firstRow == beforeSwallow);
    CHECK(node.onEvent1(&system, nullptr, nullptr, 4242, 0, 0) == false); // unknown, untouched
    CHECK(node.m_firstRow == beforeSwallow);

    // the pointer arms
    const Rect bounds = {0.0f, 0.0f, 200.0f, 100.0f, 0.0f};
    node.m_left = node.m_top = node.m_bottom = node.m_right = 0;
    node.m_scrollWidth = 10.0f;
    node.m_scrollOffset = 0.0f;
    node.m_spacing = 0.0f;
    node.m_rowHeight = 10.0f;
    node.setSelectableForHud(true);
    node.m_firstRow = 0;
    node.m_field064 = -1;

    RecordingAction onRelease, onClickAway;
    node.setFieldE0ForHud(&onRelease);
    node.setFieldE8ForHud(&onClickAway);
    node.setSound06cForHud(61);
    node.setSound070ForHud(70);

    // A press nobody owns falls all the way through to the click-away action.
    system.owner = nullptr;
    system.setPointer(50.0f, 30.0f);
    CHECK(node.onEvent1(&system, &bounds, nullptr, 1, 0, 0) == false);
    CHECK(onClickAway.runs == 1 && onRelease.runs == 0);

    // The same press, owned: the row under the pointer is selected with notify true, a sound
    // plays either way, and the release action does NOT run.
    system.owner = &node;
    data.selectRow(0, false);
    CHECK(node.onEvent1(&system, &bounds, nullptr, 1, 0, 0) == false);
    CHECK(data.m_row == 3);                       // floor(|0 + 0 - 30| / (0 + 10))
    CHECK(node.m_field064 == 3);                  // latched once
    CHECK(onRelease.runs == 0 && onClickAway.runs == 1);
    CHECK(system.sounds.size() == 1 && system.volumes.back() == 1.0f);

    // The release runs the +4C action.
    data.selectRow(0, false);
    CHECK(node.onEvent1(&system, &bounds, nullptr, 21, 0, 0) == false);
    CHECK(data.m_row == 3);
    CHECK(onRelease.runs == 1);
    CHECK(node.m_field064 == 3);                  // still the first value, not re-latched

    // A pointer past the last row takes the column down with it.
    system.setPointer(50.0f, 95.0f);              // row 9, past the eight rows
    CHECK(node.onEvent1(&system, &bounds, nullptr, 1, 0, 0) == false);
    CHECK(data.m_row == -1 && data.m_column == -1);

    // The scrollbar strip: above the thumb pages up, below it pages down, on it starts a drag.
    node.m_field02c = 40.0f;                      // the thumb top
    node.m_field030 = 20.0f;                      // its height
    node.m_firstRow = 4;
    node.m_field074 = false;
    system.setPointer(195.0f, 50.0f);             // inside the thumb
    CHECK(node.onEvent1(&system, &bounds, nullptr, 1, 0, 0) == false);
    CHECK(node.m_field074 == true && node.m_field028 == 50.0f);
    CHECK(node.m_firstRow == 4);                  // a drag does not scroll by itself

    node.m_field074 = false;
    node.m_firstRow = 4;
    system.setPointer(195.0f, 80.0f);             // below the thumb
    CHECK(node.onEvent1(&system, &bounds, nullptr, 1, 0, 0) == false);
    CHECK(node.m_firstRow == 5 && node.m_field074 == false);

    node.m_firstRow = 4;
    system.setPointer(195.0f, 10.0f);             // above the thumb
    CHECK(node.onEvent1(&system, &bounds, nullptr, 1, 0, 0) == false);
    CHECK(node.m_firstRow == 1);

    node.setDataForHud(nullptr);
}

// the class
static void listBoxNodeCases()
{
    // Retail 0x130 = 304; native 300, because the base's retail 12-byte
    // std::list is 8 bytes on this toolchain. The class itself adds only a name.
    CHECK(sizeof(Bf2NewListBoxNode) == 300);
    CHECK(sizeof(BfNewListBoxNode) == 296);
    CHECK(sizeof(Bf2NewListBoxNode) == sizeof(BfNewListBoxNode) + 4);
    CHECK(std::strcmp(Bf2NewListBoxNode::getClassType()->getName(),
                      "dice::meme::Bf2NewListBoxNode") == 0);
    CHECK(std::strcmp(BfNewListBoxNode::getClassType()->getName(),
                      "dice::meme::BfNewListBoxNode") == 0);
    CHECK(Bf2NewListBoxNode::getClassType()->getBaseType() == BfNewListBoxNode::getClassType());
    CHECK(BfNewListBoxNode::getClassType()->getBaseType() == Node::getClassType());
    CHECK(Bf2NewListBoxNode::getClassType()->getInstanceSize() == 300);
    {
        Bf2NewListBoxNode node("Roster");
        CHECK(std::strcmp(node.getName(), "Roster") == 0);
        CHECK(node.getObjectSize() == 300);
        CHECK(node.slot_0x24());
        CHECK(node.getType() == Bf2NewListBoxNode::getClassType());
        // The base's shipped defaults survive the derived constructor.
        CHECK(node.m_rowHeight == 12.0f && node.m_spacing == 1.0f);
        CHECK(node.m_borderWidth == 1.0f && node.m_scrollWidth == 4.0f);
        CHECK(node.m_border && !node.m_selectable);
        CHECK(node.m_top == 2 && node.m_bottom == 2 && node.m_left == 2 && node.m_right == 2);
        CHECK(node.m_data == nullptr && node.m_field064 == -1);
        CHECK(node.m_selection.g == 0.1875f && node.m_selection.a == 0.8f);
        CHECK(node.m_scrollbar.r == 1.0f && node.m_scrollbar.a == 1.0f);
        // setName replaces the clone.
        CHECK(node.setName("Renamed") && std::strcmp(node.getName(), "Renamed") == 0);
        // A null name is accepted and answers null, exactly like the factory's.
        CHECK(node.setName(nullptr) && node.getName() == nullptr);
    }
    {
        NameTreeRoot registry;
        PlainSystem system(registry);
        Bf2NewListBoxNode node("Painter");
        const Rect bounds = {10.0f, 20.0f, 100.0f, 50.0f, 7.0f};
        const Rect clip = {0.0f, 0.0f, 800.0f, 600.0f, 0.0f};
        node.paintSelf(&system, nullptr, &bounds, &clip, nullptr);
        CHECK(system.textures.size() == 5);
        CHECK(system.textures.size() == 5 && system.textures[0] == 0);
        CHECK(system.quads.size() == 5 && system.quads[0].corners[0].x == 10.0f);
        CHECK(system.quads.size() == 5 && system.quads[0].corners[0].y == 20.0f);
        CHECK(system.quads.size() == 5 && system.quads[0].corners[2].x == 110.0f);
        CHECK(system.quads.size() == 5 && system.quads[0].corners[2].y == 70.0f);
        CHECK(system.quads.size() == 5 && system.quads[0].corners[0].z == 0.0f);
        CHECK(system.quads.size() == 5 && system.quads[1].corners[2].y == 22.0f);
        CHECK(system.quads.size() == 5 && system.quads[2].corners[0].x == 108.0f);
        CHECK(system.quads.size() == 5 && system.quads[3].corners[1].x == 108.0f);
        CHECK(system.quads.size() == 5 && system.quads[4].corners[1].x == 12.0f);
    }
}

static void listBoxSlotCases()
{
    Bf2NewListBoxNode node("Slots");
    BfListBoxData data("Data");
    CHECK(node.setDataForHud(&data) && node.m_data == &data);
    CHECK(node.getDataForHud() == &data);
    CHECK(data.m_node == nullptr);
    node.bindDataToSelfForHud();
    CHECK(data.m_node == &node);          // the back-pointer the row installs

    // The font slot answers with the index's low byte, so index 0 reports
    // false even though the write happened. Index 5 and above are refused.
    CHECK(!node.setFontForHud(77, 0));
    CHECK(node.m_fonts[0] == 77);
    CHECK(node.setFontForHud(78, 1) && node.m_fonts[1] == 78);
    CHECK(node.setFontForHud(79, 4) && node.m_fonts[4] == 79);
    CHECK(node.setFontForHud(80, 5) && node.m_fonts[4] == 79);   // refused, still true
    // A negative index passes the `< 5` test and would write before the array.
    // That hole is recorded, not exercised.

    RecordingAction select, focus;
    CHECK(node.setSelectActionForHud(&select) && node.m_selectAction == &select);
    CHECK(node.setFocusActionForHud(&focus) && node.m_focusAction == &focus);
    CHECK(node.getSelectActionForHud() == &select);
    CHECK(node.getFocusActionForHud() == &focus);
    // +4C and +54 are Action pointers, not words - onEvent1 runs both through Action::execute.
    RecordingAction onRelease, onClickAway;
    CHECK(node.setFieldE0ForHud(&onRelease) && node.m_field04c == &onRelease);
    CHECK(node.setFieldE8ForHud(&onClickAway) && node.m_field054 == &onClickAway);

    node.setSound068ForHud(1); node.setSound06cForHud(2); node.setSound070ForHud(3);
    CHECK(node.m_sound068 == 1 && node.m_sound06c == 2 && node.m_sound070 == 3);
    node.setWidthForHud(210.0f);      CHECK(node.m_width == 210.0f);
    node.setRowHeightForHud(19.0f);   CHECK(node.m_rowHeight == 19.0f);
    node.setRowSpacingForHud(3.0f);   CHECK(node.m_spacing == 3.0f);
    node.setSelectableForHud(true);   CHECK(node.m_selectable);
    node.setBorderEnabledForHud(false); CHECK(!node.m_border);
    node.setBorderTopForHud(1); node.setBorderBottomForHud(2);
    node.setBorderLeftForHud(3); node.setBorderRightForHud(4);
    CHECK(node.m_top == 1 && node.m_bottom == 2 && node.m_left == 3 && node.m_right == 4);
    node.setScrollWidthForHud(6.0f);  CHECK(node.m_scrollWidth == 6.0f);
    node.setScrollOffsetForHud(7.0f); CHECK(node.m_scrollOffset == 7.0f);
    node.setField109ForHud(true);     CHECK(node.m_field109);

    node.setBackgroundRedForHud(0.1f); node.setBackgroundGreenForHud(0.2f);
    node.setBackgroundBlueForHud(0.3f); node.setBackgroundAlphaForHud(0.4f);
    CHECK(node.m_background.r == 0.1f && node.m_background.a == 0.4f);
    node.setFrameRedForHud(0.5f); node.setFrameAlphaForHud(0.6f);
    CHECK(node.m_frame.r == 0.5f && node.m_frame.a == 0.6f);
    node.setSelectionBlueForHud(0.7f); CHECK(node.m_selection.b == 0.7f);
    // These two are the members the header used to call m_text and m_field0e8;
    // the console rows name them the scrollbar and its background.
    node.setScrollbarRedForHud(0.8f); node.setScrollbarAlphaForHud(0.9f);
    CHECK(node.m_scrollbar.r == 0.8f && node.m_scrollbar.a == 0.9f);
    node.setScrollbarBackRedForHud(0.11f); node.setScrollbarBackAlphaForHud(0.12f);
    CHECK(node.m_scrollbarBackground.r == 0.11f && node.m_scrollbarBackground.a == 0.12f);
    // The four colours are distinct storage.
    CHECK(node.m_background.r != node.m_frame.r);
    CHECK(node.m_scrollbar.r != node.m_scrollbarBackground.r);
}

struct Harness
{
    Harness() : system(registry)
    {
        void* allocation = ::operator new(sizeof(Bf2TransformNode));
        root = new(allocation) Bf2TransformNode("IngameHud", 0, 0, 800, 600);
        CHECK(registry.insert(nullptr, root, nullptr));
        builder.setSystem(&system);
    }
    Object* find(const char* name) { return registry.findObject(&system, name); }
    NameTreeRoot registry;
    PlainSystem system;
    Bf2TransformNode* root;
    Bf2HudBuilder builder;
};

// the builder rows
static void commandCases()
{
    {   // The stock line:
        //   createListNode SquadInvite SquadInviteList 0 0 210 445 19 1
        Harness harness;
        CHECK(harness.builder.createListNode("IngameHud", "SquadInviteList",
                                             0, 0, 210, 445, 19, true));
        Bf2NewListBoxNode* list = dice::hfe::hud::builderCast<Bf2NewListBoxNode>(
            harness.find("SquadInviteListListNode"));
        CHECK(list != nullptr);
        Bf2TransformNode* transform = dice::hfe::hud::builderCast<Bf2TransformNode>(
            harness.find("SquadInviteList"));
        CHECK(transform && transform->getChild(1) == list);
        CHECK(transform && transform->getSize().x == 210.0f);
        CHECK(transform && transform->getSize().y == 445.0f);
        CHECK(harness.builder.m_field014 == list);
        // Every shipped default the create row writes, in order.
        CHECK(harness.system.fonts.size() == 1);
        CHECK(harness.system.fonts.size() == 1 &&
              harness.system.fonts[0] == "Fonts/vehicleHudFont_6.dif");
        CHECK(list && list->m_fonts[0] == 301);
        CHECK(list && list->m_selectAction == nullptr && list->m_focusAction == nullptr);
        CHECK(list && list->m_sound068 == 0 && list->m_sound06c == 0 && list->m_sound070 == 0);
        CHECK(list && list->m_width == 200.0f);      // NOT the 210 the row was given
        CHECK(list && list->m_selectable);
        CHECK(list && list->m_rowHeight == 19.0f);
        CHECK(list && !list->m_border);
        CHECK(list && list->m_background.r == 1.0f && list->m_background.g == 0.0f);
        CHECK(list && list->m_background.b == 0.0f && list->m_background.a == 0.0f);
        CHECK(list && list->m_frame.r == 1.0f && list->m_frame.g == 1.0f);
        CHECK(list && list->m_frame.b == 0.0f && list->m_frame.a == 0.0f);
        CHECK(list && list->m_selection.r == 1.0f && list->m_selection.a == 0.5f);
        CHECK(list && list->m_scrollOffset == 1.0f);
        CHECK(list && !list->m_field109);
        // No duplicate test at all: the row never calls findNodePredecessor, so
        // a repeated name silently builds a second node and answers true.
        CHECK(harness.builder.createListNode("IngameHud", "SquadInviteList",
                                             0, 0, 1, 1, 1, false));

        // The colour rows: four separate one-float slots each.
        CHECK(harness.builder.setListNodeBackgroundColor(0.1f, 0.2f, 0.3f, 0.4f));
        CHECK(harness.builder.setListNodeBorderColor(0.5f, 0.6f, 0.7f, 0.8f));
        CHECK(harness.builder.setListNodeSelectColor(0.15f, 0.25f, 0.35f, 0.45f));
        CHECK(harness.builder.setListNodeScrollbarColor(0.55f, 0.65f, 0.75f, 0.85f));
        CHECK(harness.builder.setListNodeScrollbarBackgroundColor(0.16f, 0.26f, 0.36f, 0.46f));
        Bf2NewListBoxNode* second = dice::hfe::hud::builderCast<Bf2NewListBoxNode>(
            harness.builder.m_field014);
        CHECK(second && second->m_background.r == 0.1f && second->m_background.a == 0.4f);
        CHECK(second && second->m_frame.g == 0.6f);
        CHECK(second && second->m_selection.b == 0.35f);
        CHECK(second && second->m_scrollbar.r == 0.55f && second->m_scrollbar.a == 0.85f);
        CHECK(second && second->m_scrollbarBackground.g == 0.26f);

        // The border row turns the border ON before storing any width.
        CHECK(second && !second->m_border);
        CHECK(harness.builder.setListNodeBorder(3, 4, 5, 6));
        CHECK(second && second->m_border);
        CHECK(second && second->m_top == 3 && second->m_bottom == 4);
        CHECK(second && second->m_left == 5 && second->m_right == 6);

        CHECK(harness.builder.setListNodeScrollbar(9, 11));
        CHECK(second && second->m_scrollWidth == 9.0f && second->m_scrollOffset == 11.0f);
        CHECK(harness.builder.setListNodeRowSpacing(2));
        CHECK(second && second->m_spacing == 2.0f);
        // The ROW answers a hard true and discards the node's result, so the
        // node's index-0-returns-false quirk is invisible from the console.
        const std::size_t before = harness.system.fonts.size();
        CHECK(harness.builder.setListNodeFont("Fonts/list_6.dif", 0));
        CHECK(harness.system.fonts.size() == before + 1);
        CHECK(second && second->m_fonts[0] == int(harness.system.fonts.size()) + 300);
        CHECK(harness.builder.setListNodeFont("Fonts/list_6.dif", 2));
    }
    {   // Row 44 reaches the HUD singleton's +1CC data factory.
        Harness harness;
        CHECK(harness.builder.createListNode("IngameHud", "Roster", 0, 0, 10, 10, 8, false));
        Bf2NewListBoxNode* list =
            dice::hfe::hud::builderCast<Bf2NewListBoxNode>(harness.builder.m_field014);
        FixtureHud* const singleton = new FixtureHud;
        hudManager = singleton;
        BfListBoxData data("Rows");
        singleton->answer = &data;
        CHECK(harness.builder.setListNodeData(3));
        CHECK(singleton->queries == 1 && singleton->lastSelector == 3);
        CHECK(list && list->m_data == &data);
        CHECK(data.m_node == list);                 // the back-pointer, row 44's second call
        // The range test is dead - every selector reaches the factory, including
        // zero and negatives, which the shipped comparison pair cannot reject.
        singleton->answer = nullptr;
        CHECK(!harness.builder.setListNodeData(0));
        CHECK(singleton->queries == 2 && singleton->lastSelector == 0);
        CHECK(!harness.builder.setListNodeData(99));
        CHECK(singleton->queries == 3 && singleton->lastSelector == 99);

        // Row 55 reaches the data, not the node.
        singleton->answer = &data;
        CHECK(harness.builder.setListNodeData(3));
        CHECK(!data.m_outline);
        CHECK(harness.builder.setListNodeOutline(true) && data.m_outline);
        CHECK(harness.builder.setListNodeOutline(false) && !data.m_outline);
        // With no data attached it reports instead.
        CHECK(list && list->setDataForHud(nullptr));
        CHECK(!harness.builder.setListNodeOutline(true));

        hudManager = nullptr;
        destroyHudStorage(singleton);
    }
    {   // Row 45, the console-command row, against both action slots.
        Harness harness;
        CHECK(harness.builder.createListNode("IngameHud", "Cmd", 0, 0, 10, 10, 8, false));
        Bf2NewListBoxNode* list =
            dice::hfe::hud::builderCast<Bf2NewListBoxNode>(harness.builder.m_field014);
        // which == 1 picks the focus action; anything else the select action.
        CHECK(harness.builder.setListNodeConCmd(1, "hudManager.setDone 1"));
        CHECK(list && list->getFocusActionForHud() != nullptr);
        CHECK(list && list->getSelectActionForHud() == nullptr);
        CHECK(harness.builder.setListNodeConCmd(0, "sound.playSound rosePress"));
        CHECK(list && list->getSelectActionForHud() != nullptr);
        Bf2ActionListAction* focus =
            static_cast<Bf2ActionListAction*>(list->getFocusActionForHud());
        CHECK(focus && focus->getType() == Bf2ActionListAction::getClassType());
        CHECK(focus && focus->m_actions.size() == 1);
        // A second command on the same slot joins the list.
        CHECK(harness.builder.setListNodeConCmd(1, "hudItems.setBool SetSpawnPoint 0"));
        CHECK(focus && focus->m_actions.size() == 2);
        executed.clear();
        CHECK(focus && focus->execute(&harness.system, nullptr, nullptr, nullptr));
        // Reverse script order, the same insert-before-last defect the button
        // rows have.
        CHECK(executed.size() == 2);
        CHECK(executed.size() == 2 && executed[0] == "hudItems.setBool SetSpawnPoint 0\n");
        CHECK(executed.size() == 2 && executed[1] == "hudManager.setDone 1\n");
        // An empty command is refused before anything is allocated.
        CHECK(!harness.builder.setListNodeConCmd(1, ""));
        CHECK(focus && focus->m_actions.size() == 2);
    }
    {   // With nothing selected the nine undefended rows are a bare false, and
        // the four guarded ones report.
        Harness harness;
        CHECK(!harness.builder.setListNodeBackgroundColor(1, 1, 1, 1));
        CHECK(!harness.builder.setListNodeSelectColor(1, 1, 1, 1));
        CHECK(!harness.builder.setListNodeBorderColor(1, 1, 1, 1));
        CHECK(!harness.builder.setListNodeScrollbarColor(1, 1, 1, 1));
        CHECK(!harness.builder.setListNodeScrollbarBackgroundColor(1, 1, 1, 1));
        CHECK(!harness.builder.setListNodeBorder(1, 1, 1, 1));
        CHECK(!harness.builder.setListNodeScrollbar(1, 1));
        CHECK(!harness.builder.setListNodeRowSpacing(1));
        CHECK(!harness.builder.setListNodeFont("a.dif", 1));
        CHECK(!harness.builder.setListNodeData(1));
        CHECK(!harness.builder.setListNodeConCmd(1, "x"));
        CHECK(!harness.builder.setListNodeOutline(true));
    }
}

static void printDiagnostic(DebugType, const std::string&, const std::string&, int line,
    const std::string&, const std::string& message, const std::string&, void*)
{
    std::fprintf(stderr, "FIXTURE_DEBUG line=%d %s\n", line, message.c_str());
    std::fflush(stderr);
}

static RecordingEvents fixtureEvents;

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    eventManager = &fixtureEvents;
    void* memory = ::operator new(sizeof(HudInformationLayer));
    information = new(memory) HudInformationLayer;
    g_hudInformationLayer = information;
    information->registerStringFunction("executeConsoleCommand", &recordConsoleCommand);
    if (argc > 1 && !std::strcmp(argv[1], "data-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.setListNodeData(1);
        std::fprintf(stderr, "FIXTURE_DATA_RETURNED\n");
        return 96;
    }
    if (argc > 1 && !std::strcmp(argv[1], "concmd-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.setListNodeConCmd(1, "x");
        std::fprintf(stderr, "FIXTURE_CONCMD_RETURNED\n");
        return 97;
    }
    if (argc > 1 && !std::strcmp(argv[1], "outline-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setListNodeOutline(true);
        std::fprintf(stderr, "FIXTURE_OUTLINE_RETURNED\n");
        return 98;
    }
    turnOffAllDebug(DebugTypeError);
    listBoxNodeCases();
    onEventCases();
    listBoxSlotCases();
    commandCases();
    std::printf("HUD list box node %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
