// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-BUILDER-COMMAND-FIXTURE]
// Native x86 HUD builder fixture using real registry, node lifetimes, descriptors, and Debug.
// Observers control links, names, ancestry, and registry failures; event 10 is checked in a death process.
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include "../BF2/Core/Debug.h"
#define private public
#include "../BF2/Menu/Bf2HudBuilder.h"
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
// Compile the real source in this fixture TU so the test-only access exposure
// uses the same MSVC access-qualified symbols at definition and call sites.
#include "../BF2/Menu/Bf2HudBuilder.cpp"

using namespace dice::meme;
using namespace dice::hfe;
using dice::hfe::hud::Bf2HudBuilder;
static int checks, failures;
static std::vector<std::string> events;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)

class ObservedType : public TypeDescriptor
{
public:
    ObservedType(const char* name, TypeDescriptor* base)
        : TypeDescriptor(name), base(base), baseCalls(0), registrations(0) {}
    TypeDescriptor* getBaseType() override
    { ++baseCalls; events.push_back(std::string(getName()) + ":base"); return base; }
    void addInstance() override { ++registrations; }
    TypeDescriptor* base;
    int baseCalls, registrations;
};
static ObservedType& derivedNodeType()
{ static ObservedType type("fixture::Derived", Node::getClassType()); return type; }
static ObservedType& unrelatedType()
{ static ObservedType type("fixture::Unrelated", nullptr); return type; }

class ObservedNode : public Node
{
public:
    explicit ObservedNode(const char* name, int children = 1)
        : name(name), type(&derivedNodeType()), nextType(nullptr), named(true), nullName(false), permitCreate(true),
          childCount(children), nameCalls(0), flagCalls(0), typeCalls(0),
          countCalls(0), createCalls(0)
    { links[0] = links[1] = nullptr; reads[0] = reads[1] = writes[0] = writes[1] = 0; }
    TypeDescriptor* getType() override
    {
        ++typeCalls; events.push_back(name + ":type");
        TypeDescriptor* result = type;
        if (nextType) { type = nextType; nextType = nullptr; }
        return result;
    }
    const char* getName() override { ++nameCalls; return nullName ? nullptr : name.c_str(); }
    bool slot_0x24() override { ++flagCalls; return named; }
    bool onCreate(System*) override { ++createCalls; return permitCreate; }
    int getChildCount() override { ++countCalls; return childCount; }
    Node*& getChild(int index) override { ++reads[index]; return links[index]; }
    void setChild(int index, Node* node) override
    { ++writes[index]; links[index] = node; }
    void clearCalls()
    {
        nameCalls = flagCalls = typeCalls = countCalls = createCalls = 0;
        reads[0] = reads[1] = writes[0] = writes[1] = 0;
    }
    std::string name;
    TypeDescriptor* type;
    TypeDescriptor* nextType;
    bool named, nullName, permitCreate;
    int childCount, nameCalls, flagCalls, typeCalls, countCalls, createCalls;
    int reads[2], writes[2];
    Node* links[2];
};

class ObservedRegistry : public NameTreeRoot
{
public:
    ObservedRegistry() : rejectInsert(nullptr) {}
    Object* findObject(System* system, const char* path) override
    {
        lookups.push_back(path);
        events.push_back(std::string("find:") + path);
        if (hiddenName == path) return nullptr;
        return NameTreeRoot::findObject(system, path);
    }
    bool insert(System* system, Object* object, const char* container) override
    {
        inserts.push_back(object);
        containers.push_back(container);
        if (object == rejectInsert) return false;
        return NameTreeRoot::insert(system, object, container);
    }
    void seed(Object* object) { CHECK(NameTreeRoot::insert(nullptr, object, nullptr)); }
    std::vector<std::string> lookups;
    std::vector<Object*> inserts;
    std::vector<const char*> containers;
    Object* rejectInsert;
    std::string hiddenName;
};
class FixtureSystem : public System
{
public:
    explicit FixtureSystem(NameTreeNode& registry) : System(800, 600) { m_registry = &registry; }
    // Never invoked by this test; avoids publishing a fixture System descriptor.
    TypeDescriptor* getType() override { return Object::getClassType(); }
};

static void selectionCases()
{
    ObservedNode old("Old"), selected("Selected");
    Bf2HudBuilder builder;
    builder.m_field014 = &old;
    builder.m_field050 = "preserved";
    CHECK(!builder.selectNode("Parent", "Selected"));
    CHECK(builder.m_field018 == "Selected_notFound");
    CHECK(builder.m_field034 == "Parent_notFound");
    CHECK(builder.m_field014 == &old && builder.m_field050 == "preserved");
    ObservedRegistry registry; registry.seed(&selected);
    FixtureSystem system(registry); builder.setSystem(&system);
    CHECK(!builder.selectNode("Parent", "Absent"));
    CHECK(!builder.m_field014 && builder.m_field050 == "preserved");
    CHECK(builder.m_field018 == "Absent_notFound" && builder.m_field034 == "Parent_notFound");
    CHECK(builder.selectNode("NonexistentParent", "Selected"));
    CHECK(builder.m_field014 == &selected);
    CHECK(builder.m_field018 == "Selected" && builder.m_field050 == "Selected");
    CHECK(builder.m_field034 == "NonexistentParent");
    CHECK(registry.lookups.size() == 2 && registry.lookups[0] == "Absent" && registry.lookups[1] == "Selected");
}

static void lookupCases()
{
    ObservedRegistry registry;
    FixtureSystem system(registry); Bf2HudBuilder builder; builder.setSystem(&system);
    events.clear();
    CHECK(!builder.findLastNode("Absent"));
    CHECK(events.size() == 1 && events[0] == "find:Absent");
    ObservedNode parent("Parent"), first("First"), second("Second"); registry.seed(&parent);
    events.clear(); parent.clearCalls(); derivedNodeType().baseCalls = 0;
    CHECK(builder.findLastNode("Parent") == &parent);
    CHECK(events.size() == 3 && events[0] == "find:Parent"
        && events[1] == "Parent:type" && events[2] == "fixture::Derived:base");
    CHECK(derivedNodeType().baseCalls == 1 && parent.typeCalls == 1);
    CHECK(parent.countCalls == 1 && parent.reads[0] == 1 && parent.reads[1] == 0);
    parent.type = &unrelatedType(); parent.clearCalls();
    CHECK(!builder.findLastNode("Parent"));
    CHECK(parent.typeCalls == 1 && parent.countCalls == 0 && parent.reads[0] == 0);
    parent.type = &derivedNodeType(); parent.childCount = 2; parent.clearCalls();
    CHECK(!builder.findLastNode("Parent"));
    CHECK(parent.reads[1] == 1 && parent.reads[0] == 0);
    parent.links[1] = &first; parent.clearCalls(); first.clearCalls();
    CHECK(builder.findLastNode("Parent") == &first);
    CHECK(parent.reads[1] == 2 && first.reads[0] == 1);
    parent.childCount = 1; parent.links[0] = &first; first.links[0] = &second;
    second.named = false; parent.clearCalls(); first.clearCalls(); second.clearCalls();
    CHECK(builder.findLastNode("Parent") == &second);
    CHECK(parent.reads[0] == 5 && first.reads[0] == 5 && second.reads[0] == 1);
    CHECK(first.flagCalls == 1 && second.flagCalls == 1);
    // Matching returns the predecessor and repeats the initial child access.
    parent.clearCalls(); first.clearCalls();
    CHECK(builder.findNodePredecessor("Parent", "First") == &parent);
    CHECK(parent.reads[0] == 4 && first.nameCalls == 1 && first.flagCalls == 1);
    second.named = true;
    CHECK(builder.findNodePredecessor("Parent", "Second") == &first);
    CHECK(!builder.findNodePredecessor("Parent", "second"));
    CHECK(!builder.findNodePredecessor("Parent", ""));
    parent.clearCalls(); first.clearCalls(); second.clearCalls();
    CHECK(builder.findNodePredecessor("Parent", "*suffix") == &second);
    CHECK(parent.reads[0] == 6 && first.reads[0] == 5 && second.reads[0] == 1);
    CHECK(first.nameCalls == 2 && second.nameCalls == 2);
    first.named = second.named = false;
    CHECK(!builder.findNodePredecessor("Parent", "*"));
    parent.links[0] = nullptr; parent.clearCalls();
    CHECK(!builder.findNodePredecessor("Parent", "First"));
    CHECK(parent.reads[0] == 1);
    CHECK(!builder.findNodePredecessor("Absent", "First"));
    parent.type = &unrelatedType();
    CHECK(!builder.findNodePredecessor("Parent", "First"));
}

static void insertionCases()
{
    ObservedRegistry registry; ObservedNode parent("Parent"), a("A"), b("B"), c("C"), d("D"), e("E");
    registry.seed(&parent); FixtureSystem system(registry); Bf2HudBuilder builder; builder.setSystem(&system);
    CHECK(builder.insertNode("Parent", &a, 0));
    CHECK(parent.links[0] == &a && !a.links[0]);
    CHECK(builder.m_field00c == &a && builder.m_field010 == &a && builder.m_field088);
    registry.lookups.clear();
    CHECK(builder.insertNode("Parent", &b, 0));
    CHECK(registry.lookups.empty() && a.links[0] == &b);
    CHECK(builder.m_field00c == &b && builder.m_field010 == &b && builder.m_field088);
    CHECK(builder.insertNode("Parent", &c, 1));
    CHECK(b.links[0] == &c && builder.m_field00c == &b && builder.m_field010 == &c && !builder.m_field088);
    CHECK(builder.insertNode("Parent", &d, 0));
    CHECK(b.links[0] == &d && d.links[0] == &c);
    CHECK(builder.m_field00c == &d && builder.m_field010 == &c && !builder.m_field088);
    CHECK(builder.insertNode("Parent", &e, 2));
    CHECK(c.links[0] == &e && !e.links[0]);
    CHECK(builder.m_field00c == &d && builder.m_field010 == &c && !builder.m_field088);
    // A post-mutation descriptor rejection must neither roll back links nor update cursors.
    ObservedNode rejected("Rejected"); rejected.type = &unrelatedType();
    CHECK(!builder.insertNode("Parent", &rejected, 0));
    CHECK(d.links[0] == &rejected && rejected.links[0] == &c);
    CHECK(builder.m_field00c == &d && builder.m_field010 == &c && !builder.m_field088);
    // Unknown modes still select/reset a new parent's cursor pair.
    ObservedNode other("Other"); registry.seed(&other);
    CHECK(!builder.insertNode("Other", &a, -1));
    CHECK(builder.m_field06c == "Other" && builder.m_field00c == &other && builder.m_field010 == &other);
    CHECK(builder.m_field088 && !other.links[0]);
    CHECK(!builder.insertNode("Missing", &a, 0));
    CHECK(builder.m_field06c == "Missing" && !builder.m_field00c && !builder.m_field010 && builder.m_field088);
    // Typed callback changes its reported type between the Node and Object casts;
    // this exercises rejection without altering the real Node descriptor.
    other.nextType = &unrelatedType();
    other.clearCalls();
    builder.m_field088 = false;
    CHECK(!builder.insertNode("Other", &a, 3));
    CHECK(!builder.m_field00c && !builder.m_field010 && !builder.m_field088);
    CHECK(other.typeCalls == 2);
    other.type = &derivedNodeType();
    for (int mode = 0; mode != 3; ++mode) {
        ObservedRegistry branchRegistry; ObservedNode branch("Branch", 2), child("Child");
        branchRegistry.seed(&branch); FixtureSystem branchSystem(branchRegistry);
        Bf2HudBuilder insertion; insertion.setSystem(&branchSystem);
        CHECK(insertion.insertNode("Branch", &child, mode));
        CHECK(branch.links[1] == &child && !branch.links[0] && !child.links[0]);
        CHECK(branch.writes[1] == 1 && child.writes[0] == 1);
        CHECK(insertion.m_field00c == (mode == 0 ? &child : nullptr));
        CHECK(insertion.m_field010 == (mode == 2 ? nullptr : &child));
        CHECK(insertion.m_field088 == (mode != 1));
    }
    ObservedRegistry appendRegistry; ObservedNode appendParent("Append"), appended("Appended");
    appendRegistry.seed(&appendParent); FixtureSystem appendSystem(appendRegistry);
    Bf2HudBuilder append; append.setSystem(&appendSystem);
    CHECK(append.insertNode("Append", &appended, 2));
    CHECK(appendRegistry.lookups.size() == 2); // Initial lookup plus explicit mode2 lookup.
    CHECK(append.m_field00c == &appendParent && append.m_field010 == &appendParent);
}

static void registrationCases()
{
    {
        Bf2HudBuilder builder; ObservedNode old("Old"); builder.m_field014 = &old; builder.m_field050 = "old";
        CHECK(!builder.registerNode("Parent", nullptr, nullptr, 3));
        CHECK(builder.m_field050 == "NULL" && builder.m_field014 == &old);
    }
    // Scenarios cover named/unnamed nodes, data fallback, and registration/selection failures with fresh registries.
    // Deeper insertion/onCreate rejection can fault while formatting a null container, so those paths are excluded.
    for (int scenario = 0; scenario != 8; ++scenario) {
        ObservedRegistry registry;
        ObservedNode parent("Parent", 2), node("Node"), data("Data"), old("Old");
        registry.seed(&parent); FixtureSystem system(registry); Bf2HudBuilder builder; builder.setSystem(&system);
        builder.m_field014 = &old; builder.m_field018 = "oldName"; builder.m_field034 = "oldParent"; builder.m_field050 = "oldNode";
        if (scenario == 0) node.nullName = true;
        if (scenario == 2 || scenario == 5) node.named = false;
        if (scenario == 3) data.nullName = true;
        if (scenario == 6) registry.hiddenName = "Data";
        if (scenario == 7) registry.hiddenName = "Node";
        const bool hasData = scenario >= 3 && scenario <= 6;
        const bool result = builder.registerNode("Parent", &node, hasData ? &data : nullptr, 2);
        CHECK(result == (scenario == 1 || scenario == 3 || scenario == 4 || scenario == 5));
        CHECK(parent.links[1] == &node && node.writes[0] == 1 && parent.writes[1] == 1);
        CHECK(node.createCalls == (scenario == 0 ? 0 : 1));
        for (const char* container : registry.containers) CHECK(container == nullptr);
        if (scenario == 0) {
            CHECK(builder.m_field014 == &old && builder.m_field018 == "oldName" && builder.m_field034 == "oldParent");
            CHECK(builder.m_field050 == "NULL");
            CHECK(!registry.NameTreeRoot::findObject(&system, "Node"));
        } else {
            CHECK(registry.NameTreeRoot::findObject(&system, "Node") == &node);
            if (scenario == 2) {
                CHECK(builder.m_field014 == &old && builder.m_field050 == "NULL");
            } else if (scenario == 6 || scenario == 7) {
                CHECK(!builder.m_field014 && builder.m_field050 == "NULL");
                CHECK(builder.m_field018 == (scenario == 6 ? "Data_notFound" : "Node_notFound"));
                CHECK(builder.m_field034 == "Parent_notFound");
            } else {
                CHECK(builder.m_field014 == (scenario == 4 || scenario == 5 ? &data : &node));
                CHECK(builder.m_field018 == (scenario == 4 || scenario == 5 ? "Data" : "Node"));
                CHECK(builder.m_field050 == (scenario == 5 ? "Data" : "Node"));
                CHECK(builder.m_field034 == "Parent");
            }
            if (hasData) {
                CHECK(data.createCalls == (scenario == 3 ? 0 : 1));
                CHECK(registry.NameTreeRoot::findObject(&system, "Data") == (scenario == 3 ? nullptr : &data));
            }
        }
    }
}

static void printDiagnostic(DebugType, const std::string&, const std::string&, int line,
    const std::string&, const std::string& message, const std::string&, void*)
{
    std::fprintf(stderr, "FIXTURE_DEBUG line=%d %s\n", line, message.c_str());
    std::fflush(stderr);
}

static void nodeDefaultEventCases()
{
    Node* node = static_cast<Node*>(Node::getClassType()->create());
    CHECK(node != nullptr);
    if (!node) return;
    Node successor;
    node->setChild(0, &successor);
    for (int eventType : {17, 13, 14, 7, 9}) {
        NodeInputEvent event = {eventType, 23, 1.25f, -2.5f, 0x12345678};
        const NodeInputEvent original = event;
        CHECK(!node->dispatchEvent(nullptr, nullptr, nullptr, &event));
        CHECK(node->getChild(0) == &successor);
        CHECK(!std::memcmp(&event, &original, sizeof(event)));
    }
    delete node;
}

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (argc > 1 && !std::strcmp(argv[1], "node-event10-death")) {
        Node* node = static_cast<Node*>(Node::getClassType()->create());
        NodeInputEvent event = {}; event.type = 10;
        node->dispatchEvent(nullptr, nullptr, nullptr, &event);
        std::fprintf(stderr, "FIXTURE_NODE_EVENT10_RETURNED\n");
        return 91; // The command script requires the named stop, not merely failure.
    }
    if (argc > 1 && !std::strcmp(argv[1], "debug-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        ObservedRegistry registry; FixtureSystem system(registry); Bf2HudBuilder builder; builder.setSystem(&system);
        builder.findNodePredecessor("Absent", "Node");
        std::fprintf(stderr, "FIXTURE_DEBUG_RETURNED\n");
        return 92;
    }
    // Explicit fixture-only suppression exposes shipped post-Debug return arms.
    turnOffAllDebug(DebugTypeError);
    CHECK(sizeof(Bf2HudBuilder) == 140 && sizeof(Node) == 8 && sizeof(System) == 88);
    nodeDefaultEventCases();
    selectionCases(); lookupCases(); insertionCases(); registrationCases();
    std::printf("BF2 HUD builder commands %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
