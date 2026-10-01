// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-BUTTON-NODE-FIXTURE]
// Tests button layout, slots, events, builder bindings, and action objects with real registry and HUD state.
// Only pointer ownership and event registration use observers; updateSelf and paintSelf frontiers are excluded.
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <windows.h>
#include "../BF2/Core/Debug.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/HudInformationLayer.h"
#define private public
#define protected public
#include "../BF2/Menu/Bf2HudBuilder.h"
#include "../BF2/Menu/Bf2ButtonNode.h"
#include "../BF2/Menu/Bf2CallFunctionAction.h"
#include "../BF2/Menu/Bf2ActionListAction.h"
#include "../BF2/Menu/Bf2TransformNode.h"
#undef protected
#undef private
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
hud::BF2HudManager* hudManager = nullptr;
} }

using namespace dice::meme;
using namespace dice::hfe;
using dice::hfe::hud::Bf2HudBuilder;
using dice::hfe::hud::HudStringFunction;

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; \
    std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)

// The commands the fixture's registered "executeConsoleCommand" receives.
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
    int loadTexture(const char* name, bool menuPath) override
    { textures.push_back(name); menuFlags += menuPath ? 1 : 0; return 200 + int(textures.size()); }
    // All three event overrides ask who owns the pointer before doing anything.
    Node* getPointerOwner() override { return owner; }
    std::vector<std::string> textures;
    int menuFlags = 0;
    Node* owner = nullptr;
};
class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }
static HudInformationLayer* information;

// [TEMP] an Action that only records that it ran, for the event overrides.
struct RecordingAction : Action
{
    int runs = 0;
    const NodeInputEvent* lastEvent = nullptr;
    bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent* event) override
    { ++runs; lastEvent = event; return true; }
};

// the class
static void buttonNodeCases()
{
    CHECK(sizeof(Bf2ButtonNode) == 192);
    CHECK(std::strcmp(Bf2ButtonNode::getClassType()->getName(),
                      "dice::meme::Bf2ButtonNode") == 0);
    CHECK(Bf2ButtonNode::getClassType()->getBaseType() == Bf2PictureNode::getClassType());
    CHECK(Bf2ButtonNode::getClassType()->getInstanceSize() == 192);
    {   // The factory (7D0CB0) supplies a null name and 20 for all four integers,
        // so the mouse-area size starts (20,20) and its position stays (0,0).
        RuntimeObject* made = Bf2ButtonNode::getClassType()->create();
        Bf2ButtonNode* node = static_cast<Bf2ButtonNode*>(made);
        CHECK(node && node->getName() == nullptr);
        CHECK(node && node->getObjectSize() == 192);
        CHECK(node && node->m_mouseAreaSize.x == 20.0f && node->m_mouseAreaSize.y == 20.0f);
        CHECK(node && node->m_mouseAreaPosition.x == 0.0f);
        delete node;
    }
    {
        Bf2ButtonNode node("Button", 11, 22, 33, 44);
        CHECK(std::strcmp(node.getName(), "Button") == 0);
        // x and y are accepted and ignored; only w/h reach storage.
        CHECK(node.m_mouseAreaSize.x == 33.0f && node.m_mouseAreaSize.y == 44.0f);
        CHECK(node.m_mouseAreaPosition.x == 0.0f && node.m_mouseAreaPosition.y == 0.0f);
        CHECK(node.m_normalTexture == 0 && node.m_pressedTexture == 0);
        CHECK(node.m_highlightTexture == 0 && node.m_currentTexture == 0);
        CHECK(node.m_texture == 0);                     // the PictureNode handle
        CHECK(!node.m_mouseOverColorSet && !node.m_debug);
        CHECK(!node.m_pointerHeld && node.m_pointerReleased);   // +B3 starts true
        CHECK(!node.m_fieldbc);
        bool empty = true;
        for (int i = 0; i != 4; ++i)
            empty = empty && !node.m_actions[i] && !node.m_altActions[i];
        CHECK(empty);
        CHECK(node.m_mouseOverColor.r == 0.0f && node.m_mouseOverColor.a == 0.0f);
    }
}

static void buttonSlotCases()
{
    Bf2ButtonNode node("Slots", 0, 0, 10, 20);
    // A zero texture is refused before the index is even looked at.
    CHECK(!node.setTextureForHud(1, 0));
    CHECK(!node.setTextureForHud(0, 7));               // index 0 is refused
    CHECK(!node.setTextureForHud(4, 7));               // and so is anything above 3
    CHECK(!node.setTextureForHud(-1, 7));
    // Index 1 lands in TWO places: the button's own handle and PictureNode's.
    CHECK(node.setTextureForHud(1, 91));
    CHECK(node.m_normalTexture == 91 && node.m_texture == 91);
    CHECK(node.setTextureForHud(2, 92) && node.m_pressedTexture == 92);
    CHECK(node.setTextureForHud(3, 93) && node.m_highlightTexture == 93);
    CHECK(node.m_currentTexture == 0);                 // none of them displays anything

    RecordingAction a0, a3, b0, b3;
    CHECK(node.setActionForHud(&a0, 0) && node.m_actions[0] == &a0);
    CHECK(node.setActionForHud(&a3, 3) && node.m_actions[3] == &a3);
    CHECK(node.setAltActionForHud(&b0, 0) && node.m_altActions[0] == &b0);
    CHECK(node.setAltActionForHud(&b3, 3) && node.m_altActions[3] == &b3);
    CHECK(node.getActionForHud(0) == &a0 && node.getActionForHud(3) == &a3);
    CHECK(node.getAltActionForHud(0) == &b0 && node.getAltActionForHud(3) == &b3);
    CHECK(node.getActionForHud(1) == nullptr);
    // The two arrays are independent.
    CHECK(node.m_actions[0] != node.m_altActions[0]);

    // crossed: the size pair is stored before the position pair.
    CHECK(node.setMouseAreaForHud(5, 6, 70, 80));
    CHECK(node.m_mouseAreaPosition.x == 5.0f && node.m_mouseAreaPosition.y == 6.0f);
    CHECK(node.m_mouseAreaSize.x == 70.0f && node.m_mouseAreaSize.y == 80.0f);
    CHECK(node.getMouseAreaPositionForHud().x == 5.0f);
    CHECK(node.getMouseAreaPositionForHud().y == 6.0f);
    CHECK(node.getMouseAreaSizeForHud().x == 70.0f);
    CHECK(node.getMouseAreaSizeForHud().y == 80.0f);

    Color colour = { 1.0f, 0.5f, 0.25f, 0.125f };
    CHECK(node.setMouseOverColorForHud(colour));
    CHECK(node.m_mouseOverColor.r == 1.0f && node.m_mouseOverColor.g == 0.5f);
    CHECK(node.m_mouseOverColor.b == 0.25f && node.m_mouseOverColor.a == 0.125f);
    CHECK(node.m_mouseOverColorSet);                    // the flag goes up last

    CHECK(node.setDebugForHud(true) && node.m_debug);
    CHECK(node.setDebugForHud(false) && !node.m_debug);

    // The +C0 slot: the two pointer flags, nothing else.
    node.m_pointerHeld = true;
    node.m_pointerReleased = false;
    CHECK(node.releasePointerForHud());
    CHECK(!node.m_pointerHeld && node.m_pointerReleased);
}

static void buttonEventCases()
{
    NameTreeRoot registry;
    PlainSystem system(registry);
    Bf2ButtonNode node("Events", 0, 0, 10, 20);
    RecordingAction press, alt, drag, altDrag;
    node.setActionForHud(&press, 0);
    node.setAltActionForHud(&alt, 0);
    node.setActionForHud(&drag, 3);
    node.setAltActionForHud(&altDrag, 3);
    node.setTextureForHud(1, 91);
    node.setTextureForHud(2, 92);
    node.setTextureForHud(3, 93);

    // Every one of the three overrides asks the System who owns the pointer.
    system.owner = nullptr;
    CHECK(!node.onEvent1(&system, nullptr, nullptr, 1, 0, 0));
    CHECK(press.runs == 0);
    CHECK(!node.onEvent2(&system, nullptr, nullptr, 1, 0));
    CHECK(drag.runs == 0);

    system.owner = &node;
    CHECK(node.onEvent1(&system, nullptr, nullptr, 1, 0, 0));
    CHECK(press.runs == 1 && press.lastEvent != nullptr);
    // The texture change happens after the action has already run.
    CHECK(node.m_currentTexture == 93);
    CHECK(node.onEvent1(&system, nullptr, nullptr, 0x15, 0, 0));
    CHECK(alt.runs == 1 && node.m_currentTexture == 93);   // the alt arm leaves it
    CHECK(!node.onEvent1(&system, nullptr, nullptr, 2, 0, 0));  // any other id: false
    CHECK(press.runs == 1 && alt.runs == 1);

    // onEvent2 always forwards to Node, which answers false for a childless node.
    node.m_currentTexture = 0;
    CHECK(!node.onEvent2(&system, nullptr, nullptr, 1, 0));
    CHECK(drag.runs == 1 && node.m_currentTexture == 92);
    CHECK(!node.onEvent2(&system, nullptr, nullptr, 0x15, 0));
    CHECK(altDrag.runs == 1);
    CHECK(!node.onEvent2(&system, nullptr, nullptr, 7, 0));
    CHECK(drag.runs == 1 && altDrag.runs == 1);

    // onEvent3 has NO pointer-owner test and no action of its own.
    system.owner = nullptr;
    node.m_currentTexture = 0;
    CHECK(!node.onEvent3(&system, nullptr, nullptr, 1, 0));
    CHECK(node.m_currentTexture == 91);                 // back to the normal texture
    node.m_currentTexture = 0;
    CHECK(!node.onEvent3(&system, nullptr, nullptr, 2, 0));
    CHECK(node.m_currentTexture == 0);
}

// the actions
static void actionCases()
{
    CHECK(sizeof(CallFunctionAction) == 8);
    CHECK(sizeof(Bf2CallFunctionAction) == 8);
    CHECK(sizeof(Bf2ActionListAction) == 24);
    CHECK(std::strcmp(CallFunctionAction::getClassType()->getName(),
                      "dice::meme::CallFunctionAction") == 0);
    CHECK(std::strcmp(Bf2CallFunctionAction::getClassType()->getName(),
                      "dice::meme::Bf2CallFunctionAction") == 0);
    CHECK(std::strcmp(Bf2ActionListAction::getClassType()->getName(),
                      "dice::meme::Bf2ActionListAction") == 0);
    CHECK(Bf2CallFunctionAction::getClassType()->getBaseType()
          == CallFunctionAction::getClassType());
    CHECK(Bf2ActionListAction::getClassType()->getBaseType()
          == ActionListAction::getClassType());

    NameTreeRoot registry;
    PlainSystem system(registry);
    {   // An action with no Function reports false, with no diagnostic.
        Bf2CallFunctionAction action;
        CHECK(action.m_function == nullptr);
        CHECK(!action.execute(&system, nullptr, nullptr, nullptr));
        // The setter accepts a null Function and still answers true.
        CHECK(action.setFunction(nullptr) && action.m_function == nullptr);
        HudStringFunction function(&recordConsoleCommand, "", std::string("say hi\n"));
        CHECK(action.setFunction(&function) && action.m_function == &function);
        executed.clear();
        CHECK(action.execute(&system, nullptr, nullptr, nullptr));
        CHECK(executed.size() == 1 && executed[0] == "say hi\n");
        // The Function is borrowed - the shared empty destructor frees nothing.
        CHECK(std::strcmp(function.getName(), "") == 0);
    }
    {   // Bf2ActionListAction runs every action and reports the last one's
        // result, where the base ActionListAction stops at the first true.
        Bf2ActionListAction list;
        CHECK(!list.execute(&system, nullptr, nullptr, nullptr)); // empty: false
        RecordingAction first, second;
        CHECK(list.addAction(&system, &first));
        CHECK(list.addAction(&system, &second));
        CHECK(list.m_actions.size() == 2);
        CHECK(list.execute(&system, nullptr, nullptr, nullptr));
        CHECK(first.runs == 1 && second.runs == 1);      // both ran
        // NOT appended in order. ObjectList::insert puts the new link before
        // the position it is given, and addAction gives it last() - so every
        // action after the first lands in front of the original one.
        CHECK(list.m_actions.getObjectAt(0) == &second);
        CHECK(list.m_actions.getObjectAt(1) == &first);
        // And insert never updates m_last, so a third action goes in front of
        // the same original link rather than in front of the second.
        RecordingAction third;
        CHECK(list.addAction(&system, &third));
        CHECK(list.m_actions.getObjectAt(0) == &second);
        CHECK(list.m_actions.getObjectAt(1) == &third);
        CHECK(list.m_actions.getObjectAt(2) == &first);
    }
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
    {   // The stock line: createButtonNode IngameHud MinimizeSpawnButton 770 31 16 16
        Harness harness;
        CHECK(harness.builder.createButtonNode("IngameHud", "MinimizeSpawnButton",
                                               770, 31, 16, 16));
        Bf2ButtonNode* button = dice::hfe::hud::builderCast<Bf2ButtonNode>(
            harness.find("MinimizeSpawnButtonButtonNode"));
        CHECK(button != nullptr);
        Bf2TransformNode* transform = dice::hfe::hud::builderCast<Bf2TransformNode>(
            harness.find("MinimizeSpawnButton"));
        CHECK(transform && transform->getChild(1) == button);
        CHECK(transform && transform->getPosition().x == 770.0f);
        CHECK(transform && transform->getPosition().y == 31.0f);
        CHECK(transform && transform->getSize().x == 16.0f);
        // The node keeps only the size half; x and y never reach it.
        CHECK(button && button->m_mouseAreaSize.x == 16.0f);
        CHECK(button && button->m_mouseAreaSize.y == 16.0f);
        CHECK(button && button->m_mouseAreaPosition.x == 0.0f);
        CHECK(harness.builder.m_field014 == button);

        // Row 117: the handle is parked on the builder on its way to the node.
        CHECK(harness.builder.setButtonNodeTexture(2, "Ingame/GeneralIcons/empty.tga"));
        CHECK(harness.system.textures.size() == 1 && harness.system.menuFlags == 1);
        CHECK(harness.system.textures[0] == "Ingame/GeneralIcons/empty.tga");
        CHECK(harness.builder.m_buttonTexture == 201);
        CHECK(button && button->m_pressedTexture == 201);
        CHECK(harness.builder.setButtonNodeTexture(1, "Ingame/GeneralIcons/empty.tga"));
        CHECK(button && button->m_normalTexture == 202 && button->m_texture == 202);
        // An index the node refuses still costs a load, and the builder keeps it.
        CHECK(!harness.builder.setButtonNodeTexture(0, "Ingame/GeneralIcons/full.tga"));
        CHECK(harness.system.textures.size() == 3);
        CHECK(harness.builder.m_buttonTexture == 203);

        // Row 118.
        CHECK(harness.builder.setButtonNodeMouseArea(1, 2, 3, 4));
        CHECK(button && button->m_mouseAreaPosition.x == 1.0f);
        CHECK(button && button->m_mouseAreaSize.y == 4.0f);
        // Row 123.
        CHECK(harness.builder.setButtonNodeDebug(true) && button && button->m_debug);
        CHECK(harness.builder.setButtonNodeDebug(false) && button && !button->m_debug);

        // The duplicate search has the family's usual child-0-chain hole.
        CHECK(harness.builder.createButtonNode("IngameHud", "MinimizeSpawnButton", 0, 0, 1, 1));
        harness.root->setChild(0, transform);
        CHECK(!harness.builder.createButtonNode("IngameHud", "MinimizeSpawnButton", 0, 0, 1, 1));
        harness.root->setChild(0, nullptr);
    }
    {   // Rows 120/121, the console-command rows and their self-recursion.
        Harness harness;
        CHECK(harness.builder.createButtonNode("IngameHud", "Cmd", 0, 0, 8, 8));
        Bf2ButtonNode* button =
            dice::hfe::hud::builderCast<Bf2ButtonNode>(harness.find("CmdButtonNode"));
        CHECK(button != nullptr);
        // First call: the slot is empty, so a Bf2ActionListAction is installed and
        // the row calls itself; the list ends up holding ONE action.
        CHECK(harness.builder.setButtonNodeConCmd("hudItems.setBool SetSpawnPoint 0", 3));
        Action* installed = button ? button->getActionForHud(3) : nullptr;
        CHECK(installed != nullptr);
        Bf2ActionListAction* list = static_cast<Bf2ActionListAction*>(installed);
        CHECK(list && list->getType() == Bf2ActionListAction::getClassType());
        CHECK(list && list->m_actions.size() == 1);
        // Second call on the same slot joins the list instead of replacing it.
        CHECK(harness.builder.setButtonNodeConCmd("hudManager.setDone 1", 3));
        CHECK(list && list->m_actions.size() == 2);
        CHECK(button && button->getActionForHud(3) == installed);
        // Both commands run, each with a trailing newline - but in reverse
        // script order, because addAction inserts before the list's last link.
        executed.clear();
        CHECK(list && list->execute(&harness.system, nullptr, nullptr, nullptr));
        CHECK(executed.size() == 2);
        CHECK(executed.size() == 2 && executed[0] == "hudManager.setDone 1\n");
        CHECK(executed.size() == 2 && executed[1] == "hudItems.setBool SetSpawnPoint 0\n");
        // A different index is a different list.
        CHECK(harness.builder.setButtonNodeConCmd("sound.playSound rosePress", 0));
        CHECK(button && button->getActionForHud(0) != installed);
        CHECK(button && button->getActionForHud(0) != nullptr);
        // An empty command is refused before anything is allocated, and so is
        // an index of 4 or more - but a negative index passes the `< 4` test.
        CHECK(!harness.builder.setButtonNodeConCmd("", 0));
        CHECK(!harness.builder.setButtonNodeConCmd("x", 4));
        // The alternate row drives the second array and leaves the first alone.
        CHECK(harness.builder.setButtonNodeAltConCmd("hudManager.setDone 1", 3));
        CHECK(button && button->getAltActionForHud(3) != nullptr);
        CHECK(button && button->getAltActionForHud(3) != button->getActionForHud(3));
        CHECK(!harness.builder.setButtonNodeAltConCmd("", 3));
        executed.clear();
        CHECK(button && button->getAltActionForHud(3)
              ->execute(&harness.system, nullptr, nullptr, nullptr));
        CHECK(executed.size() == 1 && executed[0] == "hudManager.setDone 1\n");
    }
    {   // With nothing selected every setter reports and returns false.
        Harness harness;
        CHECK(!harness.builder.setButtonNodeTexture(1, "a.tga"));
        CHECK(!harness.builder.setButtonNodeMouseArea(1, 1, 1, 1));
        CHECK(!harness.builder.setButtonNodeDebug(true));
        CHECK(!harness.builder.setButtonNodeConCmd("x", 0));
        CHECK(!harness.builder.setButtonNodeAltConCmd("x", 0));
    }
    {   // With no System the four setter rows refuse. createButtonNode does NOT
        // test it - findNodePredecessor dereferences it - so that row is not
        // exercised here; the create-death run below covers its diagnostic.
        Harness harness;
        harness.builder.setSystem(nullptr);
        CHECK(!harness.builder.setButtonNodeTexture(1, "a.tga"));
        CHECK(!harness.builder.setButtonNodeDebug(true));
        CHECK(!harness.builder.setButtonNodeConCmd("x", 0));
    }
    {   // A layer with no "executeConsoleCommand" answers a silent false - no
        // diagnostic at all, unlike every other refusal in these two rows.
        Harness harness;
        CHECK(harness.builder.createButtonNode("IngameHud", "Quiet", 0, 0, 8, 8));
        IBase* const saved = g_hudInformationLayer;
        void* memory = ::operator new(sizeof(HudInformationLayer));
        HudInformationLayer* bare = new(memory) HudInformationLayer;
        g_hudInformationLayer = bare;
        CHECK(!harness.builder.setButtonNodeConCmd("x", 0));
        Bf2ButtonNode* button =
            dice::hfe::hud::builderCast<Bf2ButtonNode>(harness.find("QuietButtonNode"));
        CHECK(button && button->getActionForHud(0) == nullptr);
        g_hudInformationLayer = saved;
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
    if (argc > 1 && !std::strcmp(argv[1], "create-death")) {
        // The duplicate message uses the comma spelling, not the space
        // createHoverNode and createObjectSelectionNode write.
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.createButtonNode("IngameHud", "Warn", 0, 0, 1, 1);
        Bf2TransformNode* transform = dice::hfe::hud::builderCast<Bf2TransformNode>(
            harness.find("Warn"));
        harness.root->setChild(0, transform);
        harness.builder.createButtonNode("IngameHud", "Warn", 0, 0, 1, 1);
        std::fprintf(stderr, "FIXTURE_CREATE_RETURNED\n");
        return 96;
    }
    if (argc > 1 && !std::strcmp(argv[1], "texture-death")) {
        // The diagnostic is the picture row's text, borrowed unchanged.
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setButtonNodeTexture(1, "a.tga");
        std::fprintf(stderr, "FIXTURE_TEXTURE_RETURNED\n");
        return 97;
    }
    if (argc > 1 && !std::strcmp(argv[1], "mousearea-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setButtonNodeMouseArea(1, 1, 1, 1);
        std::fprintf(stderr, "FIXTURE_MOUSEAREA_RETURNED\n");
        return 98;
    }
    if (argc > 1 && !std::strcmp(argv[1], "debug-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setButtonNodeDebug(true);
        std::fprintf(stderr, "FIXTURE_DEBUG_RETURNED\n");
        return 99;
    }
    if (argc > 1 && !std::strcmp(argv[1], "concmd-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setButtonNodeConCmd("x", 0);
        std::fprintf(stderr, "FIXTURE_CONCMD_RETURNED\n");
        return 94;
    }
    if (argc > 1 && !std::strcmp(argv[1], "altconcmd-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setButtonNodeAltConCmd("x", 0);
        std::fprintf(stderr, "FIXTURE_ALTCONCMD_RETURNED\n");
        return 93;
    }
    if (argc > 1 && !std::strcmp(argv[1], "updateself-death")) {
        Bf2ButtonNode node("Frontier", 0, 0, 1, 1);
        node.updateSelf(nullptr, nullptr, nullptr, 0.0f);
        std::fprintf(stderr, "FIXTURE_UPDATESELF_RETURNED\n");
        return 92;
    }
    if (argc > 1 && !std::strcmp(argv[1], "paintself-death")) {
        Bf2ButtonNode node("Frontier", 0, 0, 1, 1);
        node.paintSelf(nullptr, nullptr, nullptr, nullptr, nullptr);
        std::fprintf(stderr, "FIXTURE_PAINTSELF_RETURNED\n");
        return 91;
    }
    turnOffAllDebug(DebugTypeError);
    buttonNodeCases();
    buttonSlotCases();
    buttonEventCases();
    actionCases();
    commandCases();
    std::printf("HUD button node %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
