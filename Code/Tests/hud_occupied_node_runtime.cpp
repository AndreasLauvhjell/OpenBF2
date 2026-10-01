// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-OCCUPIED-NODE-FIXTURE]
// Tests occupied-vehicle node layout, painting, and builder bindings with real data and quad helpers.
// Drawing, streams, HUD data selection, and events use observers.
#include <string>
#include <vector>
#include <cmath>
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
#include "../BF2/Menu/BfOccupiedVehicleNode.h"
#include "../BF2/Menu/BfOccupiedVehicleData.h"
#include "../BF2/Menu/Bf2TransformNode.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Quad.h"
#include "../BF2/External/BFMeme/Stream.h"
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
static bool closeTo(float a, float b) { return std::fabs(a - b) < 0.001f; }

struct DrawnQuad { int texture; float x, y, w, h; float red, alpha; };

class RecordingSystem : public System
{
public:
    explicit RecordingSystem(NameTreeNode& registry) : System(800, 600)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    bool drawTexture(int texture, Quad* quad) override
    {
        DrawnQuad d;
        d.texture = texture;
        d.x = quad->corners[0].x;
        d.y = quad->corners[0].y;
        d.w = quad->corners[1].x - quad->corners[0].x;
        d.h = quad->corners[2].y - quad->corners[0].y;
        d.red = quad->corners[0].color.r;
        d.alpha = quad->corners[0].color.a;
        drawn.push_back(d);
        return true;
    }
    int loadTexture(const char*, bool) override { return 200; }
    std::vector<DrawnQuad> drawn;
};

struct DataVisit { std::string label; };
class RecordingStream : public Stream
{
public:
    bool isPassive() override { return true; }
    bool streamInt(int*, const char* label) override
    { DataVisit v; v.label = label ? label : ""; visits.push_back(v); return true; }
    bool streamBool(bool*, const char* label) override
    { DataVisit v; v.label = label ? label : ""; visits.push_back(v); return true; }
    bool streamData(System*, Data*&, const char* label, TypeDescriptor* type) override
    { DataVisit v; v.label = label ? label : ""; visits.push_back(v); lastType = type; return true; }
    std::vector<DataVisit> visits;
    TypeDescriptor* lastType = nullptr;
};

class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }
static HudInformationLayer* information;
static int seatX = 11, seatY = 22;

// Leaves hudManager null to skip icon loading, then seeds icons and tests setDataForHud and refusal behavior.
// The full manager-to-data path is covered by hud_vehicle_construct_runtime.
static void occupiedNodeCases()
{
    CHECK(sizeof(BfOccupiedVehicleNode) == 44);
    CHECK(std::strcmp(BfOccupiedVehicleNode::getClassType()->getName(),
                      "dice::meme::BfOccupiedVehicleNode") == 0);
    CHECK(BfOccupiedVehicleNode::getClassType()->getBaseType() == Node::getClassType());
    CHECK(BfOccupiedVehicleNode::getClassType()->getInstanceSize() == 44);
    {   // The factory supplies a null name and three zeros.
        RuntimeObject* made = BfOccupiedVehicleNode::getClassType()->create();
        BfOccupiedVehicleNode* node = static_cast<BfOccupiedVehicleNode*>(made);
        CHECK(node && node->getName() == nullptr);
        CHECK(node && node->m_seat == 0);
        CHECK(node && node->getObjectSize() == 44);
        CHECK(node && node->slot_0x24());
        delete node;
    }
    {
        BfOccupiedVehicleNode node("Dot", 3, 8, 9);
        CHECK(std::strcmp(node.getName(), "Dot") == 0);
        CHECK(node.m_seat == 3);
        CHECK(node.m_width == 8.0f && node.m_height == 9.0f);
        CHECK(node.m_data == nullptr);
        CHECK(!node.m_drawDebugPicture);
        // Both bindings start at the node's OWN zero word, so an unbound node
        // still paints at the bounds origin instead of dereferencing null.
        CHECK(node.m_zero == 0);
        CHECK(node.m_positionX == &node.m_zero && node.m_positionY == &node.m_zero);
        CHECK(node.setName("Renamed") && std::strcmp(node.getName(), "Renamed") == 0);

        // +CC: index 0 binds x, every other index binds y, null is the refusal.
        CHECK(node.setPositionBindingForHud(0, &seatX));
        CHECK(node.m_positionX == &seatX && node.m_positionY == &node.m_zero);
        CHECK(node.setPositionBindingForHud(1, &seatY) && node.m_positionY == &seatY);
        CHECK(node.setPositionBindingForHud(7, &seatX) && node.m_positionY == &seatX);
        CHECK(!node.setPositionBindingForHud(0, nullptr));
        CHECK(node.m_positionX == &seatX);

        // +C8 is the folded one-store setter.
        BfOccupiedVehicleData data("Occupancy");
        CHECK(node.setDataForHud(&data) && node.m_data == &data);

        NameTreeRoot registry;
        RecordingSystem system(registry);
        RecordingStream stream;
        CHECK(node.streamProperties(&system, &stream));
        CHECK(stream.visits.size() == 3);
        if (stream.visits.size() == 3) {
            CHECK(stream.visits[0].label == "Position");
            CHECK(stream.visits[1].label == "Draw debug pic");
            CHECK(stream.visits[2].label == "BfOccupiedVehicleData");
        }
        CHECK(stream.lastType == BfOccupiedVehicleData::getClassType());
    }
}

static void occupiedPaintCases()
{
    NameTreeRoot registry;
    RecordingSystem system(registry);
    Rect bounds = { 100.0f, 200.0f, 50.0f, 50.0f, 0.0f };
    RenderContext context;
    context.parent = nullptr;
    context.effect = nullptr;

    {   // No data paints nothing and reports nothing.
        BfOccupiedVehicleNode node("Dot", 0, 8, 8);
        node.paintSelf(&system, nullptr, &bounds, nullptr, &context);
        CHECK(system.drawn.empty());
    }
    {   // The bound ints offset the icon from the element's own bounds.
        BfOccupiedVehicleData data("Occupancy");
        // With no HUD manager the constructor skipped its icon loads, so the
        // handles are filled here instead of through a Ref2System.
        data.m_textures[5] = 77;
        CHECK(data.setOccupantIcon(2, 5));            // seat 2 -> icon slot 5
        BfOccupiedVehicleNode node("Dot", 2, 8, 9);
        node.setDataForHud(&data);
        seatX = 11; seatY = 22;
        node.setPositionBindingForHud(0, &seatX);
        node.setPositionBindingForHud(1, &seatY);
        node.paintSelf(&system, nullptr, &bounds, nullptr, &context);
        CHECK(system.drawn.size() == 1);
        if (system.drawn.size() == 1) {
            CHECK(closeTo(system.drawn[0].x, 111.0f));   // 11 + 100
            CHECK(closeTo(system.drawn[0].y, 222.0f));   // 22 + 200
            CHECK(closeTo(system.drawn[0].w, 8.0f));
            CHECK(closeTo(system.drawn[0].h, 9.0f));
            CHECK(system.drawn[0].texture == 77);    // the seat's icon, untinted
            CHECK(closeTo(system.drawn[0].alpha, 1.0f));
        }
    }
    system.drawn.clear();
    {   // An unresolved seat icon is still drawn, tinted fully transparent.
        BfOccupiedVehicleData empty("Occupancy");
        BfOccupiedVehicleNode node("Dot", 0, 8, 8);
        node.setDataForHud(&empty);
        node.paintSelf(&system, nullptr, &bounds, nullptr, &context);
        CHECK(system.drawn.size() == 1);
        if (system.drawn.size() == 1) {
            CHECK(system.drawn[0].texture == 0);
            CHECK(closeTo(system.drawn[0].red, 0.0f));
            CHECK(closeTo(system.drawn[0].alpha, 0.0f));
            // Unbound, so the icon sits exactly on the bounds origin.
            CHECK(closeTo(system.drawn[0].x, 100.0f));
            CHECK(closeTo(system.drawn[0].y, 200.0f));
        }
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
    RecordingSystem system;
    Bf2TransformNode* root;
    Bf2HudBuilder builder;
};

// the builder rows
static void commandCases()
{
    {   // createOccupiedNode: "<name>OccupiedNode" + the transform, mode 0.
        // The third console argument is the seat, not a coordinate.
        Harness harness;
        CHECK(harness.builder.createOccupiedNode("IngameHud", "Dot1", 2, 149, 1, 8, 9));
        BfOccupiedVehicleNode* node = dice::hfe::hud::builderCast<BfOccupiedVehicleNode>(
            harness.find("Dot1OccupiedNode"));
        CHECK(node != nullptr);
        CHECK(node && node->m_seat == 2);
        CHECK(node && node->m_width == 8.0f && node->m_height == 9.0f);
        Bf2TransformNode* transform =
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Dot1"));
        CHECK(transform && transform->getChild(1) == node);
        // The transform takes arguments 4..7 as its geometry.
        CHECK(transform && transform->getPosition().x == 149.0f);
        CHECK(transform && transform->getPosition().y == 1.0f);
        CHECK(transform && transform->getSize().x == 8.0f);
        CHECK(harness.builder.m_field014 == node);

        // The pos-variable row binds the information layer's INTs.
        CHECK(harness.builder.setOccupiedNodePosVariable(0, "FixtureSeatX"));
        CHECK(node && node->m_positionX == &seatX);
        CHECK(harness.builder.setOccupiedNodePosVariable(1, "FixtureSeatY"));
        CHECK(node && node->m_positionY == &seatY);
        CHECK(!harness.builder.setOccupiedNodePosVariable(0, ""));        // empty refused
        CHECK(!harness.builder.setOccupiedNodePosVariable(0, "Missing")); // unresolved
        // A float-only name does not resolve: the lookup is the INT registry.
        CHECK(!harness.builder.setOccupiedNodePosVariable(0, "FixtureFloat"));
        CHECK(node && node->m_positionX == &seatX);

        // Row 74's payload, reached directly: the accessor the row calls.
        BfOccupiedVehicleData occupancy("Occupancy");
        CHECK(node && node->setDataForHud(&occupancy) && node->m_data == &occupancy);
    }
    {   // With nothing selected both setters report and return false.
        Harness harness;
        CHECK(!harness.builder.setOccupiedNodePosVariable(0, "FixtureSeatX"));
    }
}

static void printDiagnostic(DebugType, const std::string&, const std::string&, int line,
    const std::string&, const std::string& message, const std::string&, void*)
{
    std::fprintf(stderr, "FIXTURE_DEBUG line=%d %s\n", line, message.c_str());
    std::fflush(stderr);
}

static RecordingEvents fixtureEvents;
static float fixtureFloat = 1.0f;

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    eventManager = &fixtureEvents;
    void* memory = ::operator new(sizeof(HudInformationLayer));
    information = new(memory) HudInformationLayer;
    g_hudInformationLayer = information;
    information->registerInt("FixtureSeatX", &seatX);
    information->registerInt("FixtureSeatY", &seatY);
    information->registerFloat("FixtureFloat", &fixtureFloat);
    if (argc > 1 && !std::strcmp(argv[1], "create-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.createOccupiedNode("IngameHud", "Dot1", 0, 0, 0, 1, 1);
        harness.root->setChild(0,
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Dot1")));
        harness.builder.createOccupiedNode("IngameHud", "Dot1", 0, 0, 0, 1, 1);
        std::fprintf(stderr, "FIXTURE_CREATE_RETURNED\n");
        return 97;
    }
    if (argc > 1 && !std::strcmp(argv[1], "pos-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setOccupiedNodePosVariable(0, "Missing");
        std::fprintf(stderr, "FIXTURE_POS_RETURNED\n");
        return 98;
    }
    if (argc > 1 && !std::strcmp(argv[1], "data-death")) {
        // No System, so the row reports before it would reach the manager.
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.setSystem(nullptr);
        harness.builder.m_field018 = "Absent";
        harness.builder.setOccupiedNodeData(9);
        std::fprintf(stderr, "FIXTURE_DATA_RETURNED\n");
        return 99;
    }
    turnOffAllDebug(DebugTypeError);
    occupiedNodeCases();
    occupiedPaintCases();
    commandCases();
    std::printf("HUD occupied node %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
