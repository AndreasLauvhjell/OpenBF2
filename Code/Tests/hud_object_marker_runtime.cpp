// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-OBJECT-MARKER-FIXTURE]
// Tests object-marker layout, slots, and builder bindings with real registry and HUD state.
// Texture/event operations use observers; paintSelf is excluded.
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
#include "../BF2/Menu/Bf2ObjectMarkerNode.h"
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
// HudInformationLayer's constructor writes through this global.
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

class RecordingSystem : public System
{
public:
    explicit RecordingSystem(NameTreeNode& registry) : System(800, 600), nextTexture(100)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    int loadTexture(const char* name, bool menuPath) override
    {
        loaded.push_back(name ? name : "<null>");
        menuFlags.push_back(menuPath);
        return ++nextTexture;
    }
    std::vector<std::string> loaded;
    std::vector<bool> menuFlags;
    int nextTexture;
};
class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }

static HudInformationLayer* information;

// the class
static void markerClassCases()
{
    // Retail is 0x1CC; this port loses four bytes per retail 12-byte container.
    CHECK(sizeof(Bf2ObjectMarkerNode) == 444);
    CHECK(std::strcmp(Bf2ObjectMarkerNode::getClassType()->getName(),
                      "dice::meme::Bf2ObjectMarkerNode") == 0);
    CHECK(Bf2ObjectMarkerNode::getClassType()->getBaseType() == Node::getClassType());
    CHECK(Bf2ObjectMarkerNode::getClassType()->getInstanceSize() == 444);
    {   // The factory supplies two nulls and five.
        RuntimeObject* made = Bf2ObjectMarkerNode::getClassType()->create();
        Bf2ObjectMarkerNode* marker = static_cast<Bf2ObjectMarkerNode*>(made);
        CHECK(marker != nullptr);
        CHECK(marker && marker->getName() == nullptr);
        CHECK(marker && marker->m_objects == 5);
        CHECK(marker && marker->getType() == Bf2ObjectMarkerNode::getClassType());
        CHECK(marker && marker->getObjectSize() == 444);
        CHECK(marker && marker->slot_0x24());
        delete marker;
    }
    {
        Bf2ObjectMarkerNode marker(nullptr, "Probe", 3);
        CHECK(std::strcmp(marker.getName(), "Probe") == 0);
        CHECK(marker.m_objects == 3);
        // Every shipped initial value.
        for (int i = 0; i < 4; ++i) {
            CHECK(marker.m_textures[i] == 0);
            CHECK(marker.m_textureSizes[i].x == 32.0f && marker.m_textureSizes[i].y == 32.0f);
        }
        CHECK(marker.m_lockTextOffset.x == -3.0f && marker.m_lockTextOffset.y == 0.0f);
        CHECK(marker.m_field04c == -1 && marker.m_field050 == -1);
        CHECK(marker.m_lockOnType == 2);
        CHECK(marker.m_weapon == 0);
        CHECK(marker.m_field05c && marker.m_field060);
        CHECK(!marker.m_lockedShow && !marker.m_lockTextEnabled && !marker.m_field05f);
        for (int i = 0; i < Bf2ObjectMarkerNode::MarkerSlotCount; ++i) {
            CHECK(!marker.m_show[i]);
            CHECK(marker.m_names[i] == defaultMarkerName);
        }
        CHECK(marker.m_lockTextNodes.empty());
        // The constructor itself registers "<name>LockedShow".
        CHECK(information->getBoolByName(std::string("ProbeLockedShow")) == &marker.m_lockedShow);
        // setName replaces the clone; the registration keeps pointing at the node.
        CHECK(marker.setName("Renamed"));
        CHECK(std::strcmp(marker.getName(), "Renamed") == 0);
    }
}

static void markerSlotCases()
{
    Bf2ObjectMarkerNode marker(nullptr, "Slots", 5);
    CHECK(marker.setObjectCountForHud(9) && marker.m_objects == 9);
    CHECK(marker.setObjectCountForHud(-2) && marker.m_objects == -2); // the slot never checks
    CHECK(marker.setLockOnTypeForHud(7) && marker.m_lockOnType == 7);
    CHECK(marker.setWeaponForHud(4) && marker.m_weapon == 4);
    CHECK(marker.setLockTextOffsetForHud(-6, 11));
    CHECK(marker.m_lockTextOffset.x == -6.0f && marker.m_lockTextOffset.y == 11.0f);

    // Both indexed slots test only the upper bound, and both answer true when
    // the index is out of range and nothing was written.
    CHECK(marker.setTextureForHud(2, 55) && marker.m_textures[2] == 55);
    CHECK(marker.setTextureForHud(4, 66));
    CHECK(marker.m_textures[0] == 0 && marker.m_textures[3] == 0);
    CHECK(marker.setTextureSizeForHud(1, 8.0f, 9.0f));
    CHECK(marker.m_textureSizes[1].x == 8.0f && marker.m_textureSizes[1].y == 9.0f);
    CHECK(marker.setTextureSizeForHud(9, 1.0f, 2.0f));
    CHECK(marker.m_textureSizes[0].x == 32.0f);

    // A null lock-text node is the one refusal here.
    CHECK(!marker.addLockTextNodeForHud(nullptr));
    CHECK(marker.m_lockTextNodes.empty());
    Bf2TransformNode label("Label", 0, 0, 4, 4);
    CHECK(marker.addLockTextNodeForHud(&label));
    CHECK(marker.m_lockTextNodes.size() == 1);
    CHECK(marker.m_lockTextNodes.front() == &label);
    CHECK(marker.addLockTextNodeForHud(&label));   // the same node twice is fine
    CHECK(marker.m_lockTextNodes.size() == 2);

    // +C0 clears every show flag and the two -1 words, and ignores its arguments.
    marker.m_show[3] = true;
    marker.m_lockedShow = true;
    marker.m_field04c = 5;
    CHECK(marker.clearShowFlagsForHud());
    CHECK(!marker.m_show[3] && !marker.m_lockedShow);
    CHECK(marker.m_field04c == -1 && marker.m_field050 == -1 && marker.m_field064 == 0);
}

static void markerRegistrationCases()
{
    Bf2ObjectMarkerNode marker(nullptr, "Reg", 5);
    // Disabled still answers true and registers nothing.
    CHECK(marker.setLockTextForHud(false, "Off"));
    CHECK(!marker.m_lockTextEnabled);
    CHECK(information->getBoolByName(std::string("Off0Show")) == nullptr);
    // Enabled registers four entries per slot, twenty slots.
    CHECK(marker.setLockTextForHud(true, "Lk"));
    CHECK(marker.m_lockTextEnabled);
    CHECK(information->getStringByName(std::string("Lk0String")) == &marker.m_names[0]);
    CHECK(information->getBoolByName(std::string("Lk0Show")) == &marker.m_show[0]);
    CHECK(information->getIntByName(std::string("Lk0PosX")) == &marker.m_positions[0][0]);
    CHECK(information->getIntByName(std::string("Lk0PosY")) == &marker.m_positions[0][1]);
    CHECK(information->getStringByName(std::string("Lk19String")) == &marker.m_names[19]);
    CHECK(information->getIntByName(std::string("Lk19PosY")) == &marker.m_positions[19][1]);
    CHECK(information->getBoolByName(std::string("Lk20Show")) == nullptr); // exactly twenty
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
    {   // createObjectMarkerNode registers "<name>ObjectMarkerNode" plus the
        // transform named after the second argument, and selects the marker.
        Harness harness;
        CHECK(harness.builder.createObjectMarkerNode("IngameHud", "Igla", 1, 2, 30, 40));
        Bf2ObjectMarkerNode* marker = dice::hfe::hud::builderCast<Bf2ObjectMarkerNode>(
            harness.find("IglaObjectMarkerNode"));
        CHECK(marker != nullptr);
        CHECK(marker && marker->m_objects == 5);       // the row always passes five
        Bf2TransformNode* transform =
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Igla"));
        CHECK(transform && transform->getChild(1) == marker);
        CHECK(harness.builder.m_field014 == marker);
        // The duplicate search walks the parent's child-0 chain only, and the
        // first node under a branching parent lands on child 1, so a repeat is
        // NOT refused while that chain is empty - the same shipped hole the
        // picture family shows. Wiring the transform in is what makes it fire.
        CHECK(harness.builder.createObjectMarkerNode("IngameHud", "Igla", 0, 0, 1, 1));
        harness.root->setChild(0, transform);
        CHECK(!harness.builder.createObjectMarkerNode("IngameHud", "Igla", 0, 0, 1, 1));
        harness.root->setChild(0, nullptr);
        // The two extra markers left the selection on the last one built.
        marker = dice::hfe::hud::builderCast<Bf2ObjectMarkerNode>(harness.builder.m_field014);
        CHECK(marker != nullptr);

        // Every setter now reaches the selected marker.
        CHECK(harness.builder.setObjectMarkerNodeTexture(0, "Ingame/lock.tga"));
        CHECK(harness.system.loaded.size() == 1 && harness.system.loaded[0] == "Ingame/lock.tga");
        CHECK(harness.system.menuFlags.size() == 1 && harness.system.menuFlags[0]);
        CHECK(marker && marker->m_textures[0] == 101);
        CHECK(!harness.builder.setObjectMarkerNodeTexture(0, "")); // empty name refused
        CHECK(harness.system.loaded.size() == 1);                  // and never loaded

        CHECK(harness.builder.setObjectMarkerNodeTextureSize(1, 64, 16));
        CHECK(marker && marker->m_textureSizes[1].x == 64.0f);
        CHECK(marker && marker->m_textureSizes[1].y == 16.0f);

        CHECK(harness.builder.setObjectMarkerNodeObjects(12));
        CHECK(marker && marker->m_objects == 12);
        CHECK(!harness.builder.setObjectMarkerNodeObjects(-1)); // negative refused by the ROW
        CHECK(marker && marker->m_objects == 12);

        CHECK(harness.builder.setObjectMarkerNodeWeapon(3));
        CHECK(marker && marker->m_weapon == 3);
        CHECK(!harness.builder.setObjectMarkerNodeWeapon(-1));
        CHECK(marker && marker->m_weapon == 3);

        // The lock-on row has no range test at all, unlike the two above.
        CHECK(harness.builder.setObjectMarkerNodeLockOnType(-5));
        CHECK(marker && marker->m_lockOnType == -5);

        CHECK(harness.builder.setObjectMarkerNodeLockTextOffset(7, -8));
        CHECK(marker && marker->m_lockTextOffset.x == 7.0f);
        CHECK(marker && marker->m_lockTextOffset.y == -8.0f);

        CHECK(harness.builder.setObjectMarkerNodeLockText(true, "Iglal"));
        CHECK(marker && marker->m_lockTextEnabled);
        CHECK(information->getBoolByName(std::string("Iglal7Show")) == &marker->m_show[7]);

        // The lock-text node must be a registered Bf2TransformNode.
        CHECK(!harness.builder.addObjectMarkerNodeLockTextNode("Missing"));
        CHECK(harness.builder.addObjectMarkerNodeLockTextNode("Igla"));
        CHECK(marker && marker->m_lockTextNodes.size() == 1);
        CHECK(marker && marker->m_lockTextNodes.front() == transform);
        // The root exists but the marker itself is not a transform.
        CHECK(!harness.builder.addObjectMarkerNodeLockTextNode("IglaObjectMarkerNode"));
    }
    {   // setActiveObject (row 147) IS the builder's own node selection, at one
        // shared body. It records "<name>_notFound"/"<parent>_notFound" first and
        // leaves them in place when the lookup fails.
        Harness harness;
        CHECK(harness.builder.createObjectMarkerNode("IngameHud", "Igla", 0, 0, 1, 1));
        CHECK(!harness.builder.setActiveObject("IngameHud", "Nope"));
        CHECK(harness.builder.m_field018 == "Nope_notFound");
        CHECK(harness.builder.m_field034 == "IngameHud_notFound");
        CHECK(harness.builder.setActiveObject("IngameHud", "IglaObjectMarkerNode"));
        CHECK(harness.builder.m_field018 == "IglaObjectMarkerNode");
        CHECK(harness.builder.m_field034 == "IngameHud");
        CHECK(harness.builder.m_field050 == "IglaObjectMarkerNode");
        // The parent is never looked up - only recorded.
        CHECK(harness.builder.setActiveObject("Nonexistent", "Igla"));
        CHECK(harness.builder.m_field034 == "Nonexistent");
        // And the row reselects, so the marker setters follow it.
        CHECK(!harness.builder.setObjectMarkerNodeObjects(2)); // "Igla" is the transform
    }
    {   // With nothing selected every setter reports and returns false.
        Harness harness;
        CHECK(!harness.builder.setObjectMarkerNodeTexture(0, "Ingame/lock.tga"));
        CHECK(!harness.builder.setObjectMarkerNodeTextureSize(0, 1, 1));
        CHECK(!harness.builder.setObjectMarkerNodeObjects(1));
        CHECK(!harness.builder.setObjectMarkerNodeLockText(true, "None"));
        CHECK(!harness.builder.setObjectMarkerNodeLockTextOffset(1, 1));
        CHECK(!harness.builder.addObjectMarkerNodeLockTextNode("IngameHud"));
        CHECK(!harness.builder.setObjectMarkerNodeLockOnType(1));
        CHECK(!harness.builder.setObjectMarkerNodeWeapon(1));
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
    if (argc > 1 && !std::strcmp(argv[1], "duplicate-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.createObjectMarkerNode("IngameHud", "Igla", 0, 0, 1, 1);
        // The chain has to be wired before the duplicate search can see it.
        harness.root->setChild(0,
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Igla")));
        harness.builder.createObjectMarkerNode("IngameHud", "Igla", 0, 0, 1, 1);
        std::fprintf(stderr, "FIXTURE_CREATE_RETURNED\n");
        return 96;
    }
    if (argc > 1 && !std::strcmp(argv[1], "texture-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setObjectMarkerNodeTexture(0, "Ingame/lock.tga");
        std::fprintf(stderr, "FIXTURE_TEXTURE_RETURNED\n");
        return 97;
    }
    if (argc > 1 && !std::strcmp(argv[1], "size-death")) {
        // The only diagnostic in this family that names the index as well.
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setObjectMarkerNodeTextureSize(2, 1, 1);
        std::fprintf(stderr, "FIXTURE_SIZE_RETURNED\n");
        return 98;
    }
    if (argc > 1 && !std::strcmp(argv[1], "weapon-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setObjectMarkerNodeWeapon(-1);
        std::fprintf(stderr, "FIXTURE_WEAPON_RETURNED\n");
        return 99;
    }
    turnOffAllDebug(DebugTypeError);
    markerClassCases();
    markerSlotCases();
    markerRegistrationCases();
    commandCases();
    std::printf("HUD object marker %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
