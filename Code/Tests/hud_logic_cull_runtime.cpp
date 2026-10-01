// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-LOGIC-CULL-FIXTURE]
// Tests logical culling with real nodes, data operations, registry, and information storage.
// Only selected-node naming and event registration use observers; split/map creation and alpha-show effects are excluded.
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include "../BF2/Core/Debug.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/HudInformationLayer.h"
#define private public
#include "../BF2/Menu/Bf2HudBuilder.h"
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/IntData.h"
#include "../BF2/External/BFMeme/EqualData.h"
#include "../BF2/External/BFMeme/OrData.h"
#include "../BF2/External/BFMeme/AndData.h"
#include "../BF2/External/BFMeme/NotData.h"
#include "../BF2/Menu/Bf2CullNode.h"
// Compile the real command in this translation unit so the test-only access
// exposure uses the same MSVC access-qualified symbols at every site.
#include "../BF2/Menu/Bf2HudBuilder.cpp"

namespace dice { namespace hfe {
// The only engine globals the exercised paths touch. Everything else the
// information layer references stays unresolved and is never reached here.
EventManager* eventManager = nullptr;
class ServerSettings;
ServerSettings* serverSettings = nullptr;
class IGameLogic;
IGameLogic* gameLogic = nullptr;
} }

using namespace dice::meme;
using namespace dice::hfe;
using dice::hfe::hud::Bf2HudBuilder;

static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; \
    std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)

// scaffolding
class FixtureNodeType : public TypeDescriptor
{
public:
    FixtureNodeType() : TypeDescriptor("fixture::SelectedNode") {}
    TypeDescriptor* getBaseType() override { return Node::getClassType(); }
};
static FixtureNodeType& fixtureNodeType() { static FixtureNodeType type; return type; }

class NamedNode : public Node
{
public:
    explicit NamedNode(const char* name) : m_name(name) { m_links[0] = m_links[1] = nullptr; }
    TypeDescriptor* getType() override { return &fixtureNodeType(); }
    const char* getName() override { return m_name.c_str(); }
    bool slot_0x24() override { return true; }
    int getChildCount() override { return 2; }
    Node*& getChild(int index) override { return m_links[index]; }
    void setChild(int index, Node* child) override { m_links[index] = child; }
    std::string m_name;
    Node* m_links[2];
};

class FixtureSystem : public System
{
public:
    explicit FixtureSystem(NameTreeNode& registry) : System(800, 600) { m_registry = &registry; }
    // Never invoked here; avoids publishing a fixture System descriptor.
    TypeDescriptor* getType() override { return Object::getClassType(); }
};

class RecordingEvents : public EventManager {};

// The command reaches the information layer through the client seam global.
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }

// data-class cases
static void intDataCases()
{
    CHECK(sizeof(IntData) == 20);
    CHECK(std::strcmp(IntData::getClassType()->getName(), "dice::meme::IntData") == 0);
    CHECK(IntData::getClassType()->getBaseType() == Data::getClassType());
    CHECK(IntData::getClassType()->getInstanceSize() == 20);

    IntData value(7, "Seven");
    CHECK(value.getType() == IntData::getClassType());
    CHECK(value.getObjectSize() == 20);
    CHECK(std::strcmp(value.getName(), "Seven") == 0);
    CHECK(value.getFloat(nullptr) == 7.0f);
    CHECK(value.getCharacter(nullptr, 0) == '7' && value.getCharacter(nullptr, 1) == 0);
    // Round half away from zero, then the truncating cast.
    CHECK(value.setFloat(nullptr, 2.4f) && value.getFloat(nullptr) == 2.0f);
    CHECK(value.setFloat(nullptr, 2.5f) && value.getFloat(nullptr) == 3.0f);
    CHECK(value.setFloat(nullptr, -2.4f) && value.getFloat(nullptr) == -2.0f);
    CHECK(value.setFloat(nullptr, -2.5f) && value.getFloat(nullptr) == -3.0f);
    // The cache reformats because the value changed since the first access.
    CHECK(value.getCharacter(nullptr, 0) == '-' && value.getCharacter(nullptr, 1) == '3');
    // atof prefix parsing, no validation, through the shared setString body.
    CHECK(value.setString(nullptr, "41abc") && value.getFloat(nullptr) == 41.0f);
    // Data's default element wrappers ignore the index and reach the scalar.
    CHECK(value.setFloatElement(nullptr, 9, 5.0f) && value.getFloat(nullptr) == 5.0f);

    // The descriptor's factory allocates20 bytes and zeroes all three members.
    RuntimeObject* created = IntData::getClassType()->create();
    CHECK(created != nullptr);
    IntData* factoryValue = static_cast<IntData*>(created);
    CHECK(factoryValue && factoryValue->getFloat(nullptr) == 0.0f);
    CHECK(factoryValue && factoryValue->getName() == nullptr);
    delete factoryValue;
}

static void intRefDataCases()
{
    CHECK(sizeof(IntRefData) == 20);
    int storage = -12;
    IntRefData reference(&storage, "Ref");
    // Inherits Data's descriptor and Data's size getter, although20 are used.
    CHECK(reference.getType() == Data::getClassType());
    CHECK(reference.getObjectSize() == sizeof(Data));
    CHECK(reference.getFloat(nullptr) == -12.0f);
    CHECK(reference.getCharacter(nullptr, 0) == '-' && reference.getCharacter(nullptr, 2) == '2');
    storage = 345;
    CHECK(reference.getFloat(nullptr) == 345.0f);
    CHECK(reference.getCharacter(nullptr, 0) == '3' && reference.getCharacter(nullptr, 2) == '5');
    CHECK(reference.setFloat(nullptr, 6.5f) && storage == 7);
    CHECK(reference.setFloat(nullptr, -6.5f) && storage == -7);
    CHECK(reference.setString(nullptr, "88") && storage == 88);

    // With no external value the pointer aliases the formatting cache, so the
    // "value changed" test can never fire again: the text stays at its first
    // rendering. This is the shipped quirk, not a fixture artifact.
    IntRefData owned(nullptr, nullptr);
    CHECK(owned.getFloat(nullptr) == 0.0f);
    CHECK(owned.getCharacter(nullptr, 0) == '0');
    CHECK(owned.setFloat(nullptr, 5.0f) && owned.getFloat(nullptr) == 5.0f);
    CHECK(owned.getCharacter(nullptr, 0) == '0');
}

static void logicalDataCases()
{
    int left = 3, right = 3;
    IntData constant(3, nullptr);
    IntRefData variable(&left, nullptr);
    EqualData equal(nullptr);
    // The added29th slot stores both operands and returns nothing.
    equal.setData(&constant, &variable);
    CHECK(equal.getType() == EqualData::getClassType());
    CHECK(equal.getFloat(nullptr) == 1.0f);
    left = 4;
    CHECK(equal.getFloat(nullptr) == 0.0f);

    bool flag = false;
    BoolRefData boolReference(&flag, nullptr);
    NotData negation(nullptr);
    negation.setData(&boolReference, nullptr);
    CHECK(negation.getFloat(nullptr) == 1.0f);
    flag = true;
    CHECK(negation.getFloat(nullptr) == 0.0f);

    OrData disjunction(nullptr);
    disjunction.setData(&equal, &boolReference);
    CHECK(disjunction.getFloat(nullptr) == 1.0f); // false OR true
    flag = false;
    CHECK(disjunction.getFloat(nullptr) == 0.0f); // false OR false
    left = right;
    CHECK(disjunction.getFloat(nullptr) == 1.0f); // true OR false

    AndData conjunction(nullptr);
    conjunction.setData(&equal, &boolReference);
    CHECK(conjunction.getFloat(nullptr) == 0.0f);
    flag = true;
    CHECK(conjunction.getFloat(nullptr) == 1.0f);

    // Both operands are borrowed; an unset operation answers zero, not a fault.
    EqualData empty(nullptr);
    CHECK(empty.getFloat(nullptr) == 0.0f);
}

// command harness
struct Harness
{
    Harness() : system(registry), selected("Sel")
    {
        CHECK(registry.insert(nullptr, &selected, nullptr));
        builder.setSystem(&system);
        builder.m_field050 = "Sel";
    }
    Bf2CullNode* cullNode()
    {
        // The command's own descriptor walk, so a wrong type reads as "absent".
        Object* object = registry.findObject(&system, "SelCullNode");
        return dice::hfe::hud::builderCast<Bf2CullNode>(object);
    }
    NameTreeRoot registry;
    FixtureSystem system;
    NamedNode selected;
    Bf2HudBuilder builder;
};

static HudInformationLayer* information;
static int guiIndex;
static bool showFlag;

static void commandCases()
{
    {   // equal on a registered int builds IntData == IntRefData under a cull node.
        Harness harness;
        guiIndex = 1;
        CHECK(harness.builder.setNodeLogicShowVariable("EQUAL", "FixtureGuiIndex", 1));
        Bf2CullNode* cull = harness.cullNode();
        CHECK(cull != nullptr);
        CHECK(harness.selected.m_links[1] == cull);
        CHECK(cull && std::strcmp(cull->getName(), "SelCullNode") == 0);
        Data* data = cull ? cull->getDataForHud() : nullptr;
        CHECK(data && data->getType() == EqualData::getClassType());
        CHECK(data && data->getFloat(nullptr) == 1.0f);
        guiIndex = 2;
        CHECK(data && data->getFloat(nullptr) == 0.0f);
    }
    {   // An unknown variable still creates and attaches a cull node, with no data.
        Harness harness;
        CHECK(harness.builder.setNodeLogicShowVariable("EQUAL", "FixtureAbsent", 1));
        Bf2CullNode* cull = harness.cullNode();
        CHECK(cull && cull->getDataForHud() == nullptr);
        CHECK(harness.selected.m_links[1] == cull);
    }
    {   // NOT on a registered bool negates a BoolRefData.
        Harness harness;
        showFlag = false;
        CHECK(harness.builder.setNodeLogicShowVariable("NOT", "FixtureShowFlag", 0));
        Data* data = harness.cullNode() ? harness.cullNode()->getDataForHud() : nullptr;
        CHECK(data && data->getType() == NotData::getClassType());
        CHECK(data && data->getFloat(nullptr) == 1.0f);
        showFlag = true;
        CHECK(data && data->getFloat(nullptr) == 0.0f);
    }
    {   // NOT on a registered int negates the equality instead.
        Harness harness;
        guiIndex = 5;
        CHECK(harness.builder.setNodeLogicShowVariable("NOT", "FixtureGuiIndex", 5));
        Data* data = harness.cullNode() ? harness.cullNode()->getDataForHud() : nullptr;
        CHECK(data && data->getType() == NotData::getClassType());
        CHECK(data && data->getFloat(nullptr) == 0.0f);
        guiIndex = 6;
        CHECK(data && data->getFloat(nullptr) == 1.0f);
    }
    {   // AND with no existing cull node reaches the creation tail and is refused
        // there, leaving the node untouched. The allocated AndData is abandoned.
        Harness harness;
        showFlag = true;
        CHECK(!harness.builder.setNodeLogicShowVariable("AND", "FixtureShowFlag", 1));
        CHECK(harness.selected.m_links[1] == nullptr);
        CHECK(harness.cullNode() == nullptr);
    }
    {   // OR combines with the data an existing cull node already carries.
        Harness harness;
        guiIndex = 1;
        showFlag = false;
        CHECK(harness.builder.setNodeLogicShowVariable("EQUAL", "FixtureGuiIndex", 1));
        Bf2CullNode* cull = harness.cullNode();
        CHECK(cull != nullptr);
        CHECK(harness.builder.setNodeLogicShowVariable("OR", "FixtureShowFlag", 1));
        CHECK(harness.cullNode() == cull); // no second node
        Data* data = cull ? cull->getDataForHud() : nullptr;
        CHECK(data && data->getType() == OrData::getClassType());
        CHECK(data && data->getFloat(nullptr) == 1.0f); // equal true OR flag false
        guiIndex = 2;
        CHECK(data && data->getFloat(nullptr) == 0.0f);
        showFlag = true;
        CHECK(data && data->getFloat(nullptr) == 1.0f);
    }
    {   // A zero console value negates the bool operand before combining.
        Harness harness;
        guiIndex = 1;
        showFlag = true;
        CHECK(harness.builder.setNodeLogicShowVariable("EQUAL", "FixtureGuiIndex", 1));
        CHECK(harness.builder.setNodeLogicShowVariable("AND", "FixtureShowFlag", 0));
        Data* data = harness.cullNode() ? harness.cullNode()->getDataForHud() : nullptr;
        CHECK(data && data->getType() == AndData::getClassType());
        CHECK(data && data->getFloat(nullptr) == 0.0f); // equal true AND NOT true
        showFlag = false;
        CHECK(data && data->getFloat(nullptr) == 1.0f);
    }
    {   // OR on an int variable equality, layered over the existing data.
        Harness harness;
        guiIndex = 1;
        showFlag = false;
        CHECK(harness.builder.setNodeLogicShowVariable("NOT", "FixtureShowFlag", 0));
        CHECK(harness.builder.setNodeLogicShowVariable("OR", "FixtureGuiIndex", 9));
        Data* data = harness.cullNode() ? harness.cullNode()->getDataForHud() : nullptr;
        CHECK(data && data->getType() == OrData::getClassType());
        CHECK(data && data->getFloat(nullptr) == 1.0f); // NOT false OR 1==9
        showFlag = true;
        CHECK(data && data->getFloat(nullptr) == 0.0f);
        guiIndex = 9;
        CHECK(data && data->getFloat(nullptr) == 1.0f);
    }
    {   // Unknown operation: line2034, no node change, no registration.
        Harness harness;
        CHECK(!harness.builder.setNodeLogicShowVariable("XOR", "FixtureShowFlag", 1));
        CHECK(harness.selected.m_links[1] == nullptr && harness.cullNode() == nullptr);
    }
    {   // No System and no selected node both reach the line-2048 arm.
        Bf2HudBuilder detached;
        detached.m_field050 = "Sel";
        CHECK(!detached.setNodeLogicShowVariable("EQUAL", "FixtureGuiIndex", 1));
        Harness harness;
        harness.builder.m_field050 = "Missing";
        CHECK(!harness.builder.setNodeLogicShowVariable("EQUAL", "FixtureGuiIndex", 1));
        CHECK(harness.cullNode() == nullptr);
    }
}

static void printDiagnostic(DebugType, const std::string&, const std::string&, int line,
    const std::string&, const std::string& message, const std::string&, void*)
{
    std::fprintf(stderr, "FIXTURE_DEBUG line=%d %s\n", line, message.c_str());
    std::fflush(stderr);
}

static RecordingEvents fixtureEvents;

static void createInformationLayer()
{
    eventManager = &fixtureEvents;
    void* memory = ::operator new(sizeof(HudInformationLayer));
    information = new(memory) HudInformationLayer;
    g_hudInformationLayer = information;
    information->registerInt("FixtureGuiIndex", &guiIndex);
    information->registerBool("FixtureShowFlag", &showFlag);
}

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    createInformationLayer();
    if (argc > 1 && !std::strcmp(argv[1], "operation-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.setNodeLogicShowVariable("XOR", "FixtureShowFlag", 1);
        std::fprintf(stderr, "FIXTURE_OPERATION_RETURNED\n");
        return 93; // The command script requires the named diagnostic, not failure.
    }
    if (argc > 1 && !std::strcmp(argv[1], "cull-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Bf2HudBuilder detached;
        detached.m_field050 = "Sel";
        detached.setNodeLogicShowVariable("EQUAL", "FixtureGuiIndex", 1);
        std::fprintf(stderr, "FIXTURE_CULL_RETURNED\n");
        return 94;
    }
    // Explicit fixture-only suppression exposes the shipped post-Debug returns.
    turnOffAllDebug(DebugTypeError);
    intDataCases();
    intRefDataCases();
    logicalDataCases();
    commandCases();
    std::printf("HUD logic cull %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
