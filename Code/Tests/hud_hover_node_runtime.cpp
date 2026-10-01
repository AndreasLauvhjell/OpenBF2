// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-HOVER-NODE-FIXTURE]
// Tests hover-node layout, constructor values, slots, and builder bindings with real HUD storage.
// System/events use observers; the updateSelf frontier is excluded.
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
#include "../BF2/Menu/Bf2HoverNode.h"
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
static void hoverNodeCases()
{
    CHECK(sizeof(Bf2HoverNode) == 84);
    CHECK(std::strcmp(Bf2HoverNode::getClassType()->getName(),
                      "dice::meme::Bf2HoverNode") == 0);
    CHECK(Bf2HoverNode::getClassType()->getBaseType() == Node::getClassType());
    CHECK(Bf2HoverNode::getClassType()->getInstanceSize() == 84);
    {   // The factory supplies two nulls - including a null name, which makes
        // the constructor register three entries called "(null)XPos" etc. only
        // because formatString prints a null pointer; it still registers.
        RuntimeObject* made = Bf2HoverNode::getClassType()->create();
        Bf2HoverNode* node = static_cast<Bf2HoverNode*>(made);
        CHECK(node && node->getName() == nullptr);
        CHECK(node && node->getObjectSize() == 84);
        CHECK(node && node->slot_0x24());
        delete node;
    }
    {
        Bf2HoverNode node(nullptr, "Hover");
        CHECK(std::strcmp(node.getName(), "Hover") == 0);
        // Every shipped initial value.
        CHECK(node.m_maxValue == 0.5f);
        CHECK(node.m_width == 0.0f && node.m_length == 0.0f);
        CHECK(node.m_xPos == 0 && node.m_yPos == 0);
        CHECK(node.m_middleX == 0 && node.m_middleY == 0);
        CHECK(!node.m_show);
        CHECK(node.m_boundsX == 0.0f && node.m_boundsHeight == 0.0f);
        // The node publishes three values named after itself, registered by
        // the constructor itself - two ints and a bool.
        CHECK(information->getIntByName(std::string("HoverXPos")) == &node.m_xPos);
        CHECK(information->getIntByName(std::string("HoverYPos")) == &node.m_yPos);
        CHECK(information->getBoolByName(std::string("HoverShow")) == &node.m_show);
        // setName replaces the clone; the registrations keep pointing at the node.
        CHECK(node.setName("Renamed") && std::strcmp(node.getName(), "Renamed") == 0);
        CHECK(information->getIntByName(std::string("HoverXPos")) == &node.m_xPos);
    }
}

static void hoverSlotCases()
{
    Bf2HoverNode node(nullptr, "Slots");
    CHECK(node.setMaxValueForHud(2.5f) && node.m_maxValue == 2.5f);
    CHECK(node.setMaxValueForHud(-1.0f) && node.m_maxValue == -1.0f); // never checked
    CHECK(node.setMiddlePositionForHud(11, -22));
    CHECK(node.m_middleX == 11 && node.m_middleY == -22);
    // crossed: the first argument lands at +18 (width), the second at +14.
    CHECK(node.setWidthLengthForHud(3.0f, 4.0f));
    CHECK(node.m_width == 3.0f && node.m_length == 4.0f);
    // The create row's four integers: x, y, then w, h, all FILD'd.
    CHECK(node.setBoundsForHud(5, 6, 70, 80));
    CHECK(node.m_boundsX == 5.0f && node.m_boundsY == 6.0f);
    CHECK(node.m_boundsWidth == 70.0f && node.m_boundsHeight == 80.0f);
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
    {   // createHoverNode registers "<name>HoverNode" plus the transform, and
        // hands the node the same four integers a second time through +D4.
        Harness harness;
        CHECK(harness.builder.createHoverNode("IngameHud", "Warn", 12, 34, 56, 78));
        Bf2HoverNode* hover =
            dice::hfe::hud::builderCast<Bf2HoverNode>(harness.find("WarnHoverNode"));
        CHECK(hover != nullptr);
        Bf2TransformNode* transform =
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Warn"));
        CHECK(transform && transform->getChild(1) == hover);
        CHECK(transform && transform->getPosition().x == 12.0f);
        CHECK(transform && transform->getSize().y == 78.0f);
        // The node keeps its own float copy of the same geometry.
        CHECK(hover && hover->m_boundsX == 12.0f && hover->m_boundsY == 34.0f);
        CHECK(hover && hover->m_boundsWidth == 56.0f && hover->m_boundsHeight == 78.0f);
        CHECK(harness.builder.m_field014 == hover);
        // And its three published values exist under the node's own name.
        CHECK(information->getBoolByName(std::string("WarnHoverNodeShow")) == &hover->m_show);

        CHECK(harness.builder.setHoverInMiddlePos(7, 9));
        CHECK(hover && hover->m_middleX == 7 && hover->m_middleY == 9);
        CHECK(harness.builder.setHoverMaxValue(0.25f));
        CHECK(hover && hover->m_maxValue == 0.25f);
        CHECK(harness.builder.setHoverWidthLength(2.0f, 16.0f));
        CHECK(hover && hover->m_width == 2.0f && hover->m_length == 16.0f);

        // The duplicate search has the family's usual child-0-chain hole.
        CHECK(harness.builder.createHoverNode("IngameHud", "Warn", 0, 0, 1, 1));
        harness.root->setChild(0, transform);
        CHECK(!harness.builder.createHoverNode("IngameHud", "Warn", 0, 0, 1, 1));
        harness.root->setChild(0, nullptr);
    }
    {   // newLayer (row 148) is the smallest row in the builder: it clears
        // both insertion cursors and answers a hard true, so the next create
        // row starts a fresh chain. It has no System test and no failure path.
        Harness harness;
        CHECK(harness.builder.createHoverNode("IngameHud", "Warn", 0, 0, 1, 1));
        harness.builder.m_field00c = harness.root;
        harness.builder.m_field010 = harness.root;
        CHECK(harness.builder.newLayer());
        CHECK(harness.builder.m_field00c == nullptr);
        CHECK(harness.builder.m_field010 == nullptr);
        // It touches nothing else - not even the selection.
        CHECK(harness.builder.m_field014 != nullptr);
        harness.builder.setSystem(nullptr);
        CHECK(harness.builder.newLayer());   // still true with no System
    }
    {   // With nothing selected every setter reports and returns false.
        Harness harness;
        CHECK(!harness.builder.setHoverInMiddlePos(1, 1));
        CHECK(!harness.builder.setHoverMaxValue(1.0f));
        CHECK(!harness.builder.setHoverWidthLength(1.0f, 1.0f));
    }
    {   // With no System even the create row refuses before allocating.
        Harness harness;
        harness.builder.setSystem(nullptr);
        CHECK(!harness.builder.createHoverNode("IngameHud", "None", 0, 0, 1, 1));
        CHECK(!harness.builder.setHoverMaxValue(1.0f));
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
        harness.builder.setSystem(nullptr);
        harness.builder.createHoverNode("IngameHud", "Warn", 0, 0, 1, 1);
        std::fprintf(stderr, "FIXTURE_CREATE_RETURNED\n");
        return 96;
    }
    if (argc > 1 && !std::strcmp(argv[1], "middle-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setHoverInMiddlePos(1, 1);
        std::fprintf(stderr, "FIXTURE_MIDDLE_RETURNED\n");
        return 97;
    }
    if (argc > 1 && !std::strcmp(argv[1], "max-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setHoverMaxValue(1.0f);
        std::fprintf(stderr, "FIXTURE_MAX_RETURNED\n");
        return 98;
    }
    if (argc > 1 && !std::strcmp(argv[1], "scale-death")) {
        // The diagnostic calls it "scale", which is neither of its two names.
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setHoverWidthLength(1.0f, 1.0f);
        std::fprintf(stderr, "FIXTURE_SCALE_RETURNED\n");
        return 99;
    }
    turnOffAllDebug(DebugTypeError);
    hoverNodeCases();
    hoverSlotCases();
    commandCases();
    std::printf("HUD hover node %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
