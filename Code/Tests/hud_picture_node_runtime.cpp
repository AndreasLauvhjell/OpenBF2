// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-PICTURE-NODE-FIXTURE]
// Tests picture nodes and builder bindings with real registry, quads, drawing context, and information storage.
// Only texture loading/drawing and events use observers.
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
#define protected public
#include "../BF2/Menu/Bf2HudBuilder.h"
#include "../BF2/Menu/Bf2PictureNode.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Quad.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/Menu/Bf2TransformNode.h"
// Compile the real command bodies in this TU so the test-only access exposure
// uses the same MSVC access-qualified symbols at every site.
#include "../BF2/Menu/Bf2HudBuilder.cpp"

namespace dice { namespace hfe {
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

struct DrawnQuad { int texture; float x[4], y[4]; Color color; };

class RecordingSystem : public System
{
public:
    explicit RecordingSystem(NameTreeNode& registry) : System(800, 600), nextTexture(100)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    int loadTexture(const char* name, bool menuPath) override
    {
        loaded.push_back(name ? name : "<null>");
        menuPaths.push_back(menuPath);
        return nextTexture++;
    }
    bool drawTexture(int texture, Quad* quad) override
    {
        DrawnQuad drawn;
        drawn.texture = texture;
        for (int i = 0; i != 4; ++i) {
            drawn.x[i] = quad->corners[i].x;
            drawn.y[i] = quad->corners[i].y;
        }
        drawn.color = quad->corners[0].color;
        drawnQuads.push_back(drawn);
        return true;
    }
    std::vector<std::string> loaded;
    std::vector<bool> menuPaths;
    std::vector<DrawnQuad> drawnQuads;
    int nextTexture;
};

class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }

static HudInformationLayer* information;
static const char* iconPath = "Ingame/icon.tga";
static float dial;

// class cases
static void identityCases()
{
    CHECK(sizeof(Bf2PictureNode) == 96);
    CHECK(std::strcmp(Bf2PictureNode::getClassType()->getName(),
                      "dice::meme::Bf2PictureNode") == 0);
    CHECK(Bf2PictureNode::getClassType()->getBaseType() == PictureNode::getClassType());
    CHECK(Bf2PictureNode::getClassType()->getInstanceSize() == 96);

    Bf2PictureNode node("Frame");
    CHECK(node.getType() == Bf2PictureNode::getClassType());
    CHECK(node.getObjectSize() == 96);
    CHECK(std::strcmp(node.getName(), "Frame") == 0);
    CHECK(node.slot_0x24());
    CHECK(node.m_texture == 0 && node.m_secondaryTexture == 0);
    CHECK(!node.m_centerSpecified && !node.m_borderEnabled);
    CHECK(node.m_rotationBinding == &node.m_rotation);
    CHECK(node.m_textureNameBinding == &node.m_textureName);
    CHECK(node.m_requestedTextureName == node.m_previousTextureName);
    CHECK(node.setName("Other") && std::strcmp(node.getName(), "Other") == 0);

    RuntimeObject* created = Bf2PictureNode::getClassType()->create();
    CHECK(created != nullptr);
    Bf2PictureNode* factoryNode = static_cast<Bf2PictureNode*>(created);
    CHECK(factoryNode && factoryNode->getName() == nullptr);
    delete factoryNode;
}

static void accessorCases()
{
    Bf2PictureNode node("Frame");
    const char* texture = "Ingame/a.tga";
    CHECK(node.setTextureNameForHud(texture));
    CHECK(std::strcmp(node.m_requestedTextureName, texture) == 0);
    CHECK(node.m_requestedTextureName != texture); // an owned clone, not the caller's text
    // The identity refusal is against the bound name, not the requested one.
    CHECK(!node.setTextureNameForHud(node.m_textureName));
    CHECK(node.setTextureNameForHud("Ingame/b.tga"));
    CHECK(std::strcmp(node.m_requestedTextureName, "Ingame/b.tga") == 0);

    CHECK(node.setTextureForHud(7) && node.m_texture == 7);
    CHECK(node.setSecondaryTextureForHud(9) && node.m_secondaryTexture == 9);
    const char* variable = "Ingame/c.tga";
    CHECK(node.setTextureNameBindingForHud(&variable));
    CHECK(node.m_textureNameBinding == &variable);
    const char* empty = nullptr;
    CHECK(!node.setTextureNameBindingForHud(&empty)); // the pointee is checked
    CHECK(node.m_textureNameBinding == &variable);
    float angle = 0.5f;
    CHECK(node.setRotationBindingForHud(&angle) && node.m_rotationBinding == &angle);
    CHECK(node.setAdditionalRotationForHud(90.0f) && node.m_additionalRotation == 90.0f);
    CHECK(node.setCenterPointForHud(3, 4));
    CHECK(node.m_centerSpecified && node.m_centerX == 3.0f && node.m_centerY == 4.0f);
    CHECK(node.setBorderForHud(1, 2, 3, 4));
    CHECK(node.m_borderEnabled && node.m_borderLeft == 1 && node.m_borderRight == 2);
    CHECK(node.m_borderTop == 3 && node.m_borderBottom == 4);
    CHECK(node.setBorderColorForHud(0.1f, 0.2f, 0.3f, 0.4f));
    CHECK(node.m_borderRed == 0.1f && node.m_borderAlpha == 0.4f);
    CHECK(dispatchPictureTexture(&node, 11) && node.m_texture == 11);
}

static void updateCases()
{
    NameTreeRoot registry;
    RecordingSystem system(registry);
    const Rect bounds = {0.0f, 0.0f, 10.0f, 10.0f, 0.0f};
    {   // Unbound: the requested name drives one load, then nothing.
        Bf2PictureNode node("Frame");
        CHECK(node.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.empty() && node.m_texture == 0); // unset marker, no load
        node.setTextureNameForHud("Ingame/a.tga");
        CHECK(node.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.size() == 1 && system.loaded[0] == "Ingame/a.tga");
        CHECK(system.menuPaths[0] && node.m_texture == 100);
        CHECK(node.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.size() == 1); // unchanged name reloads nothing
        node.setTextureNameForHud("Ingame/b.tga");
        CHECK(node.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.size() == 2 && node.m_texture == 101);
        // An empty requested name clears the handle without loading.
        node.setTextureNameForHud("");
        CHECK(node.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.size() == 2 && node.m_texture == 0);
    }
    {   // Bound: the variable drives the load and a changed variable reloads.
        Bf2PictureNode node("Frame");
        const char* variable = "Ingame/x.tga";
        node.setTextureNameBindingForHud(&variable);
        const size_t before = system.loaded.size();
        CHECK(node.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.size() == before + 1);
        CHECK(system.loaded.back() == "Ingame/x.tga");
        CHECK(node.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.size() == before + 1);
        variable = "Ingame/y.tga";
        CHECK(node.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.size() == before + 2);
        CHECK(system.loaded.back() == "Ingame/y.tga");
    }
}

static void paintCases()
{
    NameTreeRoot registry;
    RecordingSystem system(registry);
    RenderContext context = {nullptr, nullptr, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    const Rect bounds = {10.0f, 20.0f, 100.0f, 40.0f, 0.0f};
    const Rect clip = {0.0f, 0.0f, 800.0f, 600.0f, 0.0f};
    {   // No border, no rotation: exactly one quad at the bounds.
        Bf2PictureNode node("Frame");
        node.setTextureForHud(42);
        system.drawnQuads.clear();
        node.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 1);
        if (system.drawnQuads.size() == 1) {
            const DrawnQuad& quad = system.drawnQuads[0];
            CHECK(quad.texture == 42);
            CHECK(quad.x[0] == 10.0f && quad.y[0] == 20.0f);
            CHECK(quad.x[2] == 110.0f && quad.y[2] == 60.0f);
        }
    }
    {   // A border adds the four strips after the picture, in top/right/bottom/left order.
        Bf2PictureNode node("Frame");
        node.setTextureForHud(42);
        node.setBorderForHud(2, 3, 4, 5);          // left2 right3 top4 bottom5
        node.setBorderColorForHud(0.25f, 0.5f, 0.75f, 1.0f);
        system.drawnQuads.clear();
        node.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 5);
        if (system.drawnQuads.size() == 5) {
            CHECK(system.drawnQuads[1].texture == 0);
            CHECK(system.drawnQuads[1].color.r == 0.25f && system.drawnQuads[1].color.a == 1.0f);
            // top strip: full width, y 20..24
            CHECK(system.drawnQuads[1].x[0] == 10.0f && system.drawnQuads[1].x[1] == 110.0f);
            CHECK(system.drawnQuads[1].y[0] == 20.0f && system.drawnQuads[1].y[2] == 24.0f);
            // right strip: x 107..110, y 24..60
            CHECK(system.drawnQuads[2].x[0] == 107.0f && system.drawnQuads[2].x[1] == 110.0f);
            CHECK(system.drawnQuads[2].y[0] == 24.0f && system.drawnQuads[2].y[2] == 60.0f);
            // bottom strip: x 10..107, y 55..60
            CHECK(system.drawnQuads[3].x[0] == 10.0f && system.drawnQuads[3].x[1] == 107.0f);
            CHECK(system.drawnQuads[3].y[0] == 55.0f && system.drawnQuads[3].y[2] == 60.0f);
            // left strip: x 10..12, y 24..60
            CHECK(system.drawnQuads[4].x[0] == 10.0f && system.drawnQuads[4].x[1] == 12.0f);
            CHECK(system.drawnQuads[4].y[0] == 24.0f && system.drawnQuads[4].y[2] == 60.0f);
        }
    }
    {   // A 180 degree rotation about the bounds centre mirrors the corners.
        Bf2PictureNode node("Frame");
        node.setAdditionalRotationForHud(180.0f);
        system.drawnQuads.clear();
        node.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 1);
        if (system.drawnQuads.size() == 1) {
            const DrawnQuad& quad = system.drawnQuads[0];
            // The centre is (60, 40); corner0 (10,20) maps to (110,60).
            CHECK(quad.x[0] > 109.9f && quad.x[0] < 110.1f);
            CHECK(quad.y[0] > 59.9f && quad.y[0] < 60.1f);
        }
        // A bound rotation variable is used only when there is no additional one.
        Bf2PictureNode bound("Frame");
        dial = 0.0f;
        bound.setRotationBindingForHud(&dial);
        system.drawnQuads.clear();
        bound.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 1 && system.drawnQuads[0].x[0] == 10.0f);
    }
}

// command cases
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
    Bf2PictureNode* picture(const char* name)
    { return dice::hfe::hud::builderCast<Bf2PictureNode>(find(name)); }
    NameTreeRoot registry;
    RecordingSystem system;
    Bf2TransformNode* root;
    Bf2HudBuilder builder;
};

static void commandCases()
{
    {   // The row registers both the transform and the suffixed picture, and
        // selects the picture so the following setters resolve to it.
        Harness harness;
        CHECK(harness.builder.createPictureNode("IngameHud", "Frame", 1, 2, 3, 4));
        Bf2PictureNode* picture = harness.picture("FramePictureNode");
        CHECK(picture != nullptr);
        Bf2TransformNode* transform =
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Frame"));
        CHECK(transform != nullptr);
        CHECK(transform && transform->getChild(1) == picture);
        CHECK(harness.builder.m_field014 == picture);
        CHECK(harness.builder.m_field050 == "Frame");
        CHECK(harness.builder.m_field018 == "FramePictureNode");
        // The duplicate search walks the parent's child-0 chain only, and the
        // first node under a branching parent lands on child 1, so a repeat is
        // NOT refused while that chain is empty. Wiring the transform into the
        // chain is what makes the search see it.
        CHECK(harness.builder.createPictureNode("IngameHud", "Frame", 1, 2, 3, 4));
        harness.root->setChild(0, transform);
        CHECK(!harness.builder.createPictureNode("IngameHud", "Frame", 1, 2, 3, 4));
        harness.root->setChild(0, nullptr);

        CHECK(harness.builder.setPictureNodeTexture("Ingame/a.tga"));
        CHECK(picture && std::strcmp(picture->m_requestedTextureName, "Ingame/a.tga") == 0);
        CHECK(harness.builder.setPictureNodeVariableTexture("FixtureIconPath"));
        CHECK(picture && picture->m_textureNameBinding == &iconPath);
        CHECK(!harness.builder.setPictureNodeVariableTexture("FixtureMissing"));
        CHECK(harness.builder.setPictureNodeRotateVariable("FixtureDial"));
        CHECK(picture && picture->m_rotationBinding == &dial);
        CHECK(!harness.builder.setPictureNodeRotateVariable("FixtureMissing"));
        CHECK(harness.builder.setPictureNodeRotation(45));
        CHECK(picture && picture->m_additionalRotation == 45.0f);
        CHECK(harness.builder.setPictureNodeCenterPoint(6, 7));
        CHECK(picture && picture->m_centerX == 6.0f && picture->m_centerY == 7.0f);
        // Console order is top/bottom/left/right; the node stores l/r/t/b.
        CHECK(harness.builder.setPictureNodeBorder(1, 2, 3, 4));
        CHECK(picture && picture->m_borderTop == 1 && picture->m_borderBottom == 2);
        CHECK(picture && picture->m_borderLeft == 3 && picture->m_borderRight == 4);
        CHECK(harness.builder.setPictureNodeBorderColor(0.1f, 0.2f, 0.3f, 0.4f));
        CHECK(picture && picture->m_borderGreen == 0.2f);
    }
    {   // With nothing selected every setter reports and returns false.
        Harness harness;
        CHECK(!harness.builder.setPictureNodeTexture("Ingame/a.tga"));
        CHECK(!harness.builder.setPictureNodeVariableTexture("FixtureIconPath"));
        CHECK(!harness.builder.setPictureNodeRotateVariable("FixtureDial"));
        CHECK(!harness.builder.setPictureNodeRotation(45));
        CHECK(!harness.builder.setPictureNodeCenterPoint(6, 7));
        CHECK(!harness.builder.setPictureNodeBorder(1, 2, 3, 4));
        CHECK(!harness.builder.setPictureNodeBorderColor(0.1f, 0.2f, 0.3f, 0.4f));
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
    information->registerString("FixtureIconPath", &iconPath);
    information->registerFloat("FixtureDial", &dial);
    if (argc > 1 && !std::strcmp(argv[1], "texture-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setPictureNodeTexture("Ingame/a.tga");
        std::fprintf(stderr, "FIXTURE_TEXTURE_RETURNED\n");
        return 95; // the command script requires the named diagnostic, not failure
    }
    // Explicit fixture-only suppression exposes the shipped post-Debug returns.
    turnOffAllDebug(DebugTypeError);
    identityCases();
    accessorCases();
    updateCases();
    paintCases();
    commandCases();
    std::printf("HUD picture node %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
