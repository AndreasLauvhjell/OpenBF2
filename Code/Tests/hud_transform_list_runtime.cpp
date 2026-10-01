// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-TRANSFORM-LIST-FIXTURE]
// Tests transform-list layout and bindings with real registry and information storage.
// Update and paint inherit Node defaults.
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
#include "../BF2/Menu/Bf2TransformListNode.h"
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

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; \
    std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)

class PlainSystem : public System
{
public:
    explicit PlainSystem(NameTreeNode& registry) : System(800, 600)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    int loadTexture(const char*, bool) override { return 200; }
};
class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }
static HudInformationLayer* information;

// the class
static void listNodeCases()
{
    // 64 retail, 60 native: the retail 12-byte std::list is 8 here.
    CHECK(sizeof(Bf2TransformListNode) == 60);
    CHECK(std::strcmp(Bf2TransformListNode::getClassType()->getName(),
                      "dice::meme::Bf2TransformListNode") == 0);
    CHECK(Bf2TransformListNode::getClassType()->getBaseType() == Node::getClassType());
    CHECK(Bf2TransformListNode::getClassType()->getInstanceSize() == 60);
    {   // The factory pushes four zeros.
        RuntimeObject* made = Bf2TransformListNode::getClassType()->create();
        Bf2TransformListNode* node = static_cast<Bf2TransformListNode*>(made);
        CHECK(node && node->m_name == nullptr);
        CHECK(node && node->m_count == 0);
        CHECK(node && node->getObjectSize() == 60);
        delete node;
    }
    {
        Bf2TransformListNode node("Menu", 12.5f, -7.5f, 9);
        CHECK(std::strcmp(node.m_name, "Menu") == 0);
        CHECK(node.m_positionX == 12.5f && node.m_positionY == -7.5f);
        CHECK(node.m_count == 9);
        CHECK(node.m_field034 == 1.0f);          // the one non-zero default
        CHECK(node.m_field018 == 0 && node.m_field030 == 0);
        CHECK(node.m_positionBinding0 == nullptr && node.m_positionBinding1 == nullptr);
        CHECK(node.m_offsetX == 0 && node.m_offsetY == 0);
        CHECK(node.m_children.empty());
        CHECK(node.getType() == Bf2TransformListNode::getClassType());
    }
}

static void listSlotCases()
{
    Bf2TransformListNode node("Slots", 0.0f, 0.0f, 4);
    CHECK(node.setPositionForHud(3.5f, 4.5f));
    CHECK(node.m_positionX == 3.5f && node.m_positionY == 4.5f);
    CHECK(node.setOffsetForHud(-11, 22));
    CHECK(node.m_offsetX == -11 && node.m_offsetY == 22);   // never range-checked

    int first = 1, second = 2, third = 3;
    // NOT an array: index 0 lands in one slot, everything else in the other.
    CHECK(node.setPositionBindingForHud(0, &first));
    CHECK(node.m_positionBinding0 == &first && node.m_positionBinding1 == nullptr);
    CHECK(node.setPositionBindingForHud(1, &second));
    CHECK(node.m_positionBinding1 == &second && node.m_positionBinding0 == &first);
    CHECK(node.setPositionBindingForHud(7, &third));
    CHECK(node.m_positionBinding1 == &third);               // index 7 overwrites index 1
    CHECK(node.setPositionBindingForHud(-1, &second));
    CHECK(node.m_positionBinding1 == &second);              // so does a negative index
    // A null binding is the one thing it refuses, at either index.
    CHECK(!node.setPositionBindingForHud(0, nullptr));
    CHECK(!node.setPositionBindingForHud(3, nullptr));
    CHECK(node.m_positionBinding0 == &first && node.m_positionBinding1 == &second);

    void* storage = ::operator new(sizeof(Bf2TransformNode));
    Bf2TransformNode* childA = new(storage) Bf2TransformNode("A", 0, 0, 1, 1);
    storage = ::operator new(sizeof(Bf2TransformNode));
    Bf2TransformNode* childB = new(storage) Bf2TransformNode("B", 0, 0, 1, 1);
    CHECK(!node.addChildForHud(nullptr));
    CHECK(node.m_children.empty());
    CHECK(node.addChildForHud(childA) && node.m_children.size() == 1);
    CHECK(node.addChildForHud(childB) && node.m_children.size() == 2);
    // Appended in order - unlike Bf2ActionListAction, which reverses.
    CHECK(node.m_children.front() == childA && node.m_children.back() == childB);
    // The same child twice is accepted; there is no duplicate test.
    CHECK(node.addChildForHud(childA) && node.m_children.size() == 3);
    delete childA;
    delete childB;
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
        //   createTransformListNode SquadLeaderMenu OrderMenuButtonList 285 35 500 495 1
        Harness harness;
        CHECK(harness.builder.createTransformListNode("IngameHud", "OrderMenuButtonList",
                                                      285, 35, 500, 495, 1));
        Object* found = harness.find("OrderMenuButtonListTransformListNode");
        CHECK(found && found->getType() == Bf2TransformListNode::getClassType());
        Bf2TransformListNode* list = static_cast<Bf2TransformListNode*>(found);
        Bf2TransformNode* transform = dice::hfe::hud::builderCast<Bf2TransformNode>(
            harness.find("OrderMenuButtonList"));
        CHECK(transform && transform->getChild(1) == list);
        CHECK(transform && transform->getPosition().x == 285.0f);
        CHECK(transform && transform->getSize().x == 500.0f);
        CHECK(harness.builder.m_field014 == list);
        // The node gets x and y as floats and the count - but NOT the width or
        // height; only the transform carries those.
        CHECK(list && list->m_positionX == 285.0f && list->m_positionY == 35.0f);
        CHECK(list && list->m_count == 1);

        // Row 62: the named node must exist AND be a Bf2TransformNode.
        CHECK(harness.builder.addTransformListNode("OrderMenuButtonList"));
        CHECK(list && list->m_children.size() == 1);
        CHECK(list && list->m_children.front() == transform);
        CHECK(!harness.builder.addTransformListNode("NoSuchNode"));
        // A node of the wrong type is refused: the list node itself is not a
        // Bf2TransformNode, so naming it fails the second cast.
        CHECK(!harness.builder.addTransformListNode("OrderMenuButtonListTransformListNode"));
        CHECK(list && list->m_children.size() == 1);

        // Row 64.
        CHECK(harness.builder.setTranformListNodeOffset(6, 8));
        CHECK(list && list->m_offsetX == 6 && list->m_offsetY == 8);

        // Row 63: the binding comes from the information layer.
        static int slotValue = 0;
        CHECK(information->registerInt(std::string("OrderMenuIndex"), &slotValue) >= 0);
        CHECK(harness.builder.setTranformListNodePosVariable(0, "OrderMenuIndex"));
        CHECK(list && list->m_positionBinding0 == &slotValue);
        CHECK(harness.builder.setTranformListNodePosVariable(1, "OrderMenuIndex"));
        CHECK(list && list->m_positionBinding1 == &slotValue);
        // An unregistered variable reports and changes nothing.
        CHECK(!harness.builder.setTranformListNodePosVariable(0, "NeverRegistered"));

        // The duplicate search has the family's usual child-0-chain hole.
        CHECK(harness.builder.createTransformListNode("IngameHud", "OrderMenuButtonList",
                                                      0, 0, 1, 1, 1));
        harness.root->setChild(0, transform);
        CHECK(!harness.builder.createTransformListNode("IngameHud", "OrderMenuButtonList",
                                                       0, 0, 1, 1, 1));
        harness.root->setChild(0, nullptr);
    }
    {   // With nothing selected all three setters report and return false.
        Harness harness;
        CHECK(!harness.builder.addTransformListNode("IngameHud"));
        CHECK(!harness.builder.setTranformListNodeOffset(1, 1));
        CHECK(!harness.builder.setTranformListNodePosVariable(0, "OrderMenuIndex"));
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
    if (argc > 1 && !std::strcmp(argv[1], "create-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.createTransformListNode("IngameHud", "Warn", 0, 0, 1, 1, 1);
        Bf2TransformNode* transform = dice::hfe::hud::builderCast<Bf2TransformNode>(
            harness.find("Warn"));
        harness.root->setChild(0, transform);
        harness.builder.createTransformListNode("IngameHud", "Warn", 0, 0, 1, 1, 1);
        std::fprintf(stderr, "FIXTURE_CREATE_RETURNED\n");
        return 96;
    }
    if (argc > 1 && !std::strcmp(argv[1], "add-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.addTransformListNode("IngameHud");
        std::fprintf(stderr, "FIXTURE_ADD_RETURNED\n");
        return 97;
    }
    if (argc > 1 && !std::strcmp(argv[1], "posvar-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setTranformListNodePosVariable(0, "x");
        std::fprintf(stderr, "FIXTURE_POSVAR_RETURNED\n");
        return 98;
    }
    if (argc > 1 && !std::strcmp(argv[1], "offset-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setTranformListNodeOffset(1, 1);
        std::fprintf(stderr, "FIXTURE_OFFSET_RETURNED\n");
        return 99;
    }
    turnOffAllDebug(DebugTypeError);
    listNodeCases();
    listSlotCases();
    commandCases();
    std::printf("HUD transform list node %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
