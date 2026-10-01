// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-BAR-COMPASS-FIXTURE]
// Tests bar, compass, and effect nodes with real registry, node, quad, and information-layer code.
// Only texture operations and event registration use recording observers.
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
#include "../BF2/Menu/Bf2BarNode.h"
#include "../BF2/Menu/Bf2CompassNode.h"
#include "../BF2/Menu/Bf2ColorEffect.h"
#include "../BF2/Menu/Bf2EffectNode.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Quad.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/Menu/Bf2TransformNode.h"
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

struct DrawnQuad { int texture; float x[4], y[4]; const Rect* clip; Color color; };

class RecordingSystem : public System
{
public:
    explicit RecordingSystem(NameTreeNode& registry) : System(800, 600), nextTexture(200)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    int loadTexture(const char* name, bool) override
    {
        loaded.push_back(name ? name : "<null>");
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
        drawn.clip = quad->clip;
        drawn.color = quad->corners[0].color;
        drawnQuads.push_back(drawn);
        return true;
    }
    std::vector<std::string> loaded;
    std::vector<DrawnQuad> drawnQuads;
    int nextTexture;
};
class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }

static HudInformationLayer* information;
static float health;
static const char* skinPath = "Ingame/skin.tga";

// bar cases
static void barCases()
{
    CHECK(sizeof(Bf2BarNode) == 144);
    CHECK(std::strcmp(Bf2BarNode::getClassType()->getName(), "dice::meme::Bf2BarNode") == 0);
    CHECK(Bf2BarNode::getClassType()->getBaseType() == Bf2PictureNode::getClassType());
    CHECK(Bf2BarNode::getClassType()->getInstanceSize() == 144);

    Bf2BarNode bar("Health", 1);
    CHECK(bar.getObjectSize() == 144);
    CHECK(std::strcmp(bar.getName(), "Health") == 0);
    CHECK(bar.m_direction == 1 && bar.m_snap == 1 && !bar.m_snapTowardsEnd);
    CHECK(bar.m_fillTextureBinding == &bar.m_textureName);
    CHECK(*bar.m_value == 0.0f); // the shared default float, never null
    RuntimeObject* created = Bf2BarNode::getClassType()->create();
    CHECK(created != nullptr);
    delete static_cast<Bf2BarNode*>(created);

    // Index 1 is the picture texture, anything else the fill texture.
    CHECK(bar.setIndexedTextureNameForHud(1, "Ingame/frame.tga"));
    CHECK(std::strcmp(bar.m_requestedTextureName, "Ingame/frame.tga") == 0);
    CHECK(bar.setIndexedTextureNameForHud(2, "Ingame/fill.tga"));
    CHECK(std::strcmp(bar.m_fillRequestedTextureName, "Ingame/fill.tga") == 0);
    CHECK(!bar.setIndexedTextureNameForHud(1, emptyHudTextureName));

    CHECK(bar.setFillBorderForHud(1, 2, 3, 4));
    CHECK(bar.m_fillTop == 1 && bar.m_fillBottom == 2 && bar.m_fillLeft == 3 && bar.m_fillRight == 4);
    CHECK(bar.setSnapForHud(5) && bar.m_snap == 5);
    CHECK(bar.setSnapDirectionForHud(true) && bar.m_snapTowardsEnd);
    bar.setDirectionForHud(2);
    CHECK(bar.m_direction == 2);
    // The binding is stored even when the range check fails and zeroes it.
    float outside = 4.0f;
    CHECK(!bar.setValueBindingForHud(&outside));
    CHECK(bar.m_value == &outside && outside == 0.0f);
    health = 0.5f;
    CHECK(bar.setValueBindingForHud(&health) && bar.m_value == &health);

    NameTreeRoot registry;
    RecordingSystem system(registry);
    const Rect bounds = {0.0f, 0.0f, 10.0f, 10.0f, 0.0f};
    {   // The first pair short-circuits: an unchanged picture name returns
        // true before the fill pair is even looked at.
        Bf2BarNode quiet("Quiet", 0);
        CHECK(quiet.setIndexedTextureNameForHud(2, "Ingame/fill.tga"));
        CHECK(quiet.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.empty());
        // Once the picture name changes both pairs load, in that order.
        CHECK(quiet.setIndexedTextureNameForHud(1, "Ingame/frame.tga"));
        CHECK(quiet.updateSelf(&system, nullptr, &bounds, 0.1f));
        CHECK(system.loaded.size() == 2);
        CHECK(system.loaded[0] == "Ingame/frame.tga" && system.loaded[1] == "Ingame/fill.tga");
        CHECK(quiet.m_texture == 200 && quiet.m_fillTexture == 201);
    }
}

static void barPaintCases()
{
    NameTreeRoot registry;
    RecordingSystem system(registry);
    RenderContext context = {nullptr, nullptr, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    const Rect bounds = {0.0f, 0.0f, 100.0f, 20.0f, 0.0f};
    const Rect clip = {0.0f, 0.0f, 800.0f, 600.0f, 0.0f};
    {   // Direction 3 with no border: the background, then the bar quad whose
        // clip is the filled sub-rectangle.
        Bf2BarNode bar("Health", 3);
        bar.setTextureForHud(11);
        health = 0.25f;
        bar.setValueBindingForHud(&health);
        system.drawnQuads.clear();
        bar.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 2);
        if (system.drawnQuads.size() == 2) {
            CHECK(system.drawnQuads[0].texture == 0);  // no fill texture yet
            CHECK(system.drawnQuads[1].texture == 11);
            // 100 wide, value 0.25, snap 1 -> 75 removed from the right.
            CHECK(system.drawnQuads[1].clip->width == 25.0f);
            CHECK(system.drawnQuads[1].clip->x == 0.0f);
        }
        CHECK(bar.m_texture == 11); // restored after the background paint
    }
    {   // Direction 0 grows from the bottom: the clip moves down and shrinks.
        Bf2BarNode bar("Health", 0);
        health = 0.25f;
        bar.setValueBindingForHud(&health);
        system.drawnQuads.clear();
        bar.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 2);
        if (system.drawnQuads.size() == 2) {
            CHECK(system.drawnQuads[1].clip->y == 15.0f);
            CHECK(system.drawnQuads[1].clip->height == 5.0f);
        }
    }
    {   // The snap quantises the removed length; direction 0 can ceil.
        Bf2BarNode bar("Health", 0);
        health = 0.5f;
        bar.setValueBindingForHud(&health);
        bar.setSnapForHud(3);
        system.drawnQuads.clear();
        bar.paintSelf(&system, nullptr, &bounds, &clip, &context);
        // 20 * 0.5 = 10 unfilled; floor(10/3) * 3 = 9.
        CHECK(system.drawnQuads.size() == 2 && system.drawnQuads[1].clip->y == 9.0f);
        bar.setSnapDirectionForHud(true);
        system.drawnQuads.clear();
        bar.paintSelf(&system, nullptr, &bounds, &clip, &context);
        // ceil(10/3) * 3 = 12.
        CHECK(system.drawnQuads.size() == 2 && system.drawnQuads[1].clip->y == 12.0f);
    }
    {   // The fill border insets the fill rect on all four sides.
        Bf2BarNode bar("Health", 3);
        health = 1.0f;
        bar.setValueBindingForHud(&health);
        bar.setFillBorderForHud(1, 2, 3, 4); // top bottom left right
        system.drawnQuads.clear();
        bar.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 2);
        if (system.drawnQuads.size() == 2) {
            CHECK(system.drawnQuads[1].clip->x == 3.0f);
            CHECK(system.drawnQuads[1].clip->y == 1.0f);
            CHECK(system.drawnQuads[1].clip->width == 93.0f);
            CHECK(system.drawnQuads[1].clip->height == 17.0f);
        }
    }
    {   // Out-of-range values clamp; the background uses the fill texture.
        Bf2BarNode bar("Health", 3);
        bar.m_fillTexture = 77;
        health = 2.0f;
        bar.m_value = &health;
        system.drawnQuads.clear();
        bar.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 2 && system.drawnQuads[0].texture == 77);
        CHECK(system.drawnQuads[1].clip->width == 100.0f);
    }
}

// compass cases
static void compassCases()
{
    CHECK(sizeof(Bf2CompassNode) == 196);
    CHECK(std::strcmp(Bf2CompassNode::getClassType()->getName(),
                      "dice::meme::Bf2CompassNode") == 0);
    CHECK(Bf2CompassNode::getClassType()->getBaseType() == Bf2BarNode::getClassType());
    Bf2CompassNode compass("Compass", 3, true, false);
    CHECK(compass.getObjectSize() == 196);
    CHECK(compass.m_direction == 3 && compass.m_snapEnabled && !compass.m_wrapEnabled);
    CHECK(std::strcmp(compass.getName(), "Compass") == 0);
    CHECK(compass.setTextureSizeForHud(1024.0f, 32.0f));
    CHECK(compass.m_textureRect.width == 1024.0f && compass.m_textureRect.height == 32.0f);
    CHECK(compass.m_textureRect.x == 0.0f && compass.m_textureRect.y == 0.0f);
    CHECK(compass.setOffsetForHud(375) && compass.m_offset == 375);
    CHECK(compass.setSnapOffsetForHud(1, 2, 3, 4));
    CHECK(compass.m_snapOffset[0] == 1 && compass.m_snapOffset[3] == 4);
    CHECK(compass.setSnapTextureForHud(true, 9) && compass.m_snapTexturePrimary == 9);
    CHECK(compass.setSnapTextureForHud(false, 8) && compass.m_snapTextureSecondary == 8);
    // updateSelf is the inherited bar body, at the same PE address.
    RuntimeObject* created = Bf2CompassNode::getClassType()->create();
    CHECK(created != nullptr);
    delete static_cast<Bf2CompassNode*>(created);
}

static void compassPaintCases()
{
    NameTreeRoot registry;
    RecordingSystem system(registry);
    RenderContext context = {nullptr, nullptr, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    const Rect bounds = {10.0f, 20.0f, 186.0f, 32.0f, 0.0f};
    const Rect clip = {0.0f, 0.0f, 800.0f, 600.0f, 0.0f};
    {   // Horizontal, snapping off: the strip scrolls by the bound angle.
        Bf2CompassNode compass("Compass", 3, false, false);
        compass.setTextureSizeForHud(1024.0f, 32.0f);
        compass.setTextureForHud(5);
        health = 0.0f;
        compass.setValueBindingForHud(&health);
        compass.setOffsetForHud(0);
        system.drawnQuads.clear();
        compass.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 1);
        if (system.drawnQuads.size() == 1) {
            CHECK(system.drawnQuads[0].texture == 5);
            CHECK(system.drawnQuads[0].x[0] == 10.0f);
            CHECK(system.drawnQuads[0].x[1] == 10.0f + 1024.0f);
            CHECK(system.drawnQuads[0].clip == &bounds); // the bounds are the clip
        }
        // A quarter turn scrolls by (1024-0) * 0.25 rad in degrees / 360.
        health = 1.0f;
        system.drawnQuads.clear();
        compass.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 1);
        if (system.drawnQuads.size() == 1) {
            const int expected = static_cast<int>(1024.0f * 57.295776f * 0.0027777778f);
            CHECK(system.drawnQuads[0].x[0] == 10.0f - static_cast<float>(expected));
        }
    }
    {   // With wrap on, a sufficiently negative shift draws a second copy.
        Bf2CompassNode compass("Compass", 3, false, true);
        compass.setTextureSizeForHud(1024.0f, 32.0f);
        compass.setTextureForHud(5);
        health = 0.0f;
        compass.setValueBindingForHud(&health);
        compass.setOffsetForHud(-600);
        system.drawnQuads.clear();
        compass.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 2);
        if (system.drawnQuads.size() == 2)
            CHECK(system.drawnQuads[1].x[0] == system.drawnQuads[0].x[0] - 1024.0f);
    }
    {   // Snapping clamps the scroll and swaps in the snap texture.
        Bf2CompassNode compass("Compass", 3, true, false);
        compass.setTextureSizeForHud(1024.0f, 32.0f);
        compass.setTextureForHud(5);
        compass.setSnapTextureForHud(true, 91);
        compass.setSnapOffsetForHud(0, 0, 0, 0);
        health = 1.0f;
        compass.setValueBindingForHud(&health);
        compass.setOffsetForHud(0);
        system.drawnQuads.clear();
        compass.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 1);
        if (system.drawnQuads.size() == 1) {
            CHECK(system.drawnQuads[0].texture == 91);
            CHECK(system.drawnQuads[0].x[0] == 10.0f - 93.0f); // bounds.width * 0.5
        }
    }
    {   // A direction other than 0 or 3 never scrolls.
        Bf2CompassNode compass("Compass", 1, false, false);
        compass.setTextureSizeForHud(1024.0f, 32.0f);
        health = 1.0f;
        compass.setValueBindingForHud(&health);
        system.drawnQuads.clear();
        compass.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawnQuads.size() == 1 && system.drawnQuads[0].x[0] == 10.0f);
    }
}

// colour effect cases
static void colorEffectCases()
{
    CHECK(sizeof(Bf2ColorEffect) == 48);
    CHECK(sizeof(Bf2EffectNode) == 16);
    CHECK(std::strcmp(Bf2ColorEffect::getClassType()->getName(),
                      "dice::meme::Bf2ColorEffect") == 0);
    CHECK(Bf2ColorEffect::getClassType()->getBaseType() == ColorEffect::getClassType());
    CHECK(std::strcmp(Bf2EffectNode::getClassType()->getName(),
                      "dice::meme::Bf2EffectNode") == 0);
    CHECK(Bf2EffectNode::getClassType()->getBaseType() == EffectNode::getClassType());

    Bf2ColorEffect effect(0.4f, 0.5f, 0.6f, 0.7f);
    CHECK(effect.getObjectSize() == 48);
    CHECK(effect.getColor()->r == 0.4f && effect.getColor()->a == 0.7f);
    CHECK(effect.m_disabledBinding == -1.0f);
    CHECK(effect.m_alphaBinding == &effect.m_disabledBinding);

    const Rect bounds = {0.0f, 0.0f, 4.0f, 4.0f, 0.0f};
    RenderContext context = {nullptr, nullptr, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    NameTreeRoot registry;
    RecordingSystem system(registry);
    {   // Every default binding is the shared -1, so the base colour survives.
        Quad quad(&bounds, &bounds);
        CHECK(effect.drawTexture(&system, 3, &quad, &context));
        CHECK(quad.corners[0].color.r == 0.4f && quad.corners[3].color.a == 0.7f);
    }
    {   // Alpha multiplies, the three colour channels replace.
        float alpha = 0.5f, red = 1.0f;
        CHECK(effect.setAlphaBinding(&alpha) && effect.setRgbBindings(&red, nullptr, nullptr));
        Quad quad(&bounds, &bounds);
        CHECK(effect.drawTexture(&system, 3, &quad, &context));
        CHECK(quad.corners[0].color.a == 0.35f && quad.corners[0].color.r == 1.0f);
        CHECK(quad.corners[0].color.g == 0.5f);
        // A negative sample leaves the channel alone.
        red = -1.0f;
        Quad again(&bounds, &bounds);
        CHECK(effect.drawTexture(&system, 3, &again, &context));
        CHECK(again.corners[0].color.r == 0.4f);
    }
    CHECK(effect.setColor(1.0f, 1.0f, 1.0f, 1.0f) && effect.getColor()->r == 1.0f);

    Bf2EffectNode node("MyColorEffect", &effect);
    CHECK(std::strcmp(node.getName(), "MyColorEffect") == 0);
    CHECK(node.getEffectForHud() == &effect);
    CHECK(node.setName("Other") && std::strcmp(node.getName(), "Other") == 0);
    RuntimeObject* created = Bf2EffectNode::getClassType()->create();
    CHECK(created != nullptr);
    delete static_cast<Bf2EffectNode*>(created);
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
    NameTreeRoot registry;
    RecordingSystem system;
    Bf2TransformNode* root;
    Bf2HudBuilder builder;
};

static void commandCases()
{
    {   // createBarNode registers "<name>BarNode" and the transform "<name>".
        Harness harness;
        CHECK(harness.builder.createBarNode("IngameHud", "Health", 1, 2, 3, 4, 5));
        Bf2BarNode* bar = dice::hfe::hud::builderCast<Bf2BarNode>(harness.find("HealthBarNode"));
        CHECK(bar != nullptr);
        CHECK(bar && bar->m_direction == 1);
        CHECK(harness.builder.m_field014 == bar);
        CHECK(harness.builder.setBarNodeTexture(1, "Ingame/frame.tga"));
        CHECK(bar && std::strcmp(bar->m_requestedTextureName, "Ingame/frame.tga") == 0);
        CHECK(harness.builder.setBarNodeTexture(2, "Ingame/fill.tga"));
        CHECK(bar && std::strcmp(bar->m_fillRequestedTextureName, "Ingame/fill.tga") == 0);
        CHECK(harness.builder.setBarNodeVariableTexture(2, "FixtureSkinPath"));
        CHECK(bar && bar->m_fillTextureBinding == &skinPath);
        CHECK(!harness.builder.setBarNodeVariableTexture(2, "")); // empty name refused
        CHECK(harness.builder.setBarNodeBorder(1, 2, 3, 4));
        CHECK(bar && bar->m_fillTop == 1 && bar->m_fillRight == 4);
        CHECK(harness.builder.setBarNodeSnap(7) && bar && bar->m_snap == 7);
        CHECK(harness.builder.setBarNodeSnapDir(true) && bar && bar->m_snapTowardsEnd);
        health = 0.5f;
        CHECK(harness.builder.setBarNodeValueVariable("FixtureHealth"));
        CHECK(bar && bar->m_value == &health);
        CHECK(!harness.builder.setBarNodeValueVariable("FixtureMissing"));
    }
    {   // createCompassNode also names its node "<name>BarNode", and the three
        // forwarding rows work on it because a compass IS a bar.
        Harness harness;
        CHECK(harness.builder.createCompassNode("IngameHud", "Comp", 3, 309, 165, 186, 32, true, false));
        Bf2CompassNode* compass =
            dice::hfe::hud::builderCast<Bf2CompassNode>(harness.find("CompBarNode"));
        CHECK(compass != nullptr);
        CHECK(compass && compass->m_direction == 3 && compass->m_snapEnabled);
        Bf2TransformNode* transform =
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Comp"));
        CHECK(transform && transform->getChild(1) == compass);
        CHECK(harness.builder.setCompassNodeTexture(1, "Ingame/compass.tga"));
        CHECK(compass && std::strcmp(compass->m_requestedTextureName, "Ingame/compass.tga") == 0);
        CHECK(harness.builder.setCompassNodeTextureSize(1024, 32));
        CHECK(compass && compass->m_textureRect.width == 1024.0f);
        CHECK(harness.builder.setCompassNodeBorder(0, 0, 0, 164));
        CHECK(compass && compass->m_fillRight == 164);
        health = 0.25f;
        CHECK(harness.builder.setCompassNodeValueVariable("FixtureHealth"));
        CHECK(compass && compass->m_value == &health);
        CHECK(harness.builder.setCompassNodeOffset(375) && compass && compass->m_offset == 375);
        CHECK(harness.builder.setCompassNodeSnapOffset(1, 2, 3, 4));
        CHECK(compass && compass->m_snapOffset[2] == 3);
        CHECK(harness.builder.setCompassNodeSnapTexture(true, "Ingame/snap.tga"));
        CHECK(compass && compass->m_snapTexturePrimary == 200);
    }
    {   // setNodeColor builds a Bf2EffectNode carrying a Bf2ColorEffect, then
        // retints the same effect on a second call.
        Harness harness;
        CHECK(harness.builder.createPictureNode("IngameHud", "Frame", 0, 0, 8, 8));
        CHECK(harness.builder.setNodeColor(0.4f, 0.4f, 0.4f, 1.0f));
        Bf2EffectNode* carrier =
            dice::hfe::hud::builderCast<Bf2EffectNode>(harness.find("FrameColorEffect"));
        CHECK(carrier != nullptr);
        CHECK(harness.root->getChild(1) != nullptr);
        Effect* effect = carrier ? carrier->getEffectForHud() : nullptr;
        CHECK(effect && effect->getColor()->r == 0.4f && effect->getColor()->a == 1.0f);
        CHECK(harness.builder.setNodeColor(0.1f, 0.2f, 0.3f, 0.4f));
        CHECK(effect && effect->getColor()->r == 0.1f && effect->getColor()->b == 0.3f);
        // Still exactly one carrier: the second call reused it.
        CHECK(dice::hfe::hud::builderCast<Bf2EffectNode>(harness.find("FrameColorEffect")) == carrier);
    }
    {   // With nothing selected every row reports and returns false.
        Harness harness;
        CHECK(!harness.builder.setBarNodeTexture(1, "Ingame/a.tga"));
        CHECK(!harness.builder.setBarNodeBorder(1, 2, 3, 4));
        CHECK(!harness.builder.setBarNodeSnap(1));
        CHECK(!harness.builder.setBarNodeSnapDir(true));
        CHECK(!harness.builder.setBarNodeValueVariable("FixtureHealth"));
        CHECK(!harness.builder.setCompassNodeOffset(1));
        CHECK(!harness.builder.setCompassNodeSnapOffset(1, 2, 3, 4));
        CHECK(!harness.builder.setCompassNodeTextureSize(1, 2));
        CHECK(!harness.builder.setCompassNodeSnapTexture(true, "Ingame/snap.tga"));
        // setNodeColor only needs the selected name to resolve to some Node, so
        // an unselected builder fails while a plain transform succeeds.
        CHECK(!harness.builder.setNodeColor(1.0f, 1.0f, 1.0f, 1.0f));
        harness.builder.m_field050 = "IngameHud";
        CHECK(harness.builder.setNodeColor(1.0f, 1.0f, 1.0f, 1.0f));
        CHECK(harness.find("IngameHudColorEffect") != nullptr);
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
    information->registerFloat("FixtureHealth", &health);
    information->registerString("FixtureSkinPath", &skinPath);
    if (argc > 1 && !std::strcmp(argv[1], "snap-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setBarNodeSnap(3);
        std::fprintf(stderr, "FIXTURE_SNAP_RETURNED\n");
        return 96; // the command script requires the named diagnostic, not failure
    }
    if (argc > 1 && !std::strcmp(argv[1], "color-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Bf2HudBuilder detached;
        detached.m_field050 = "Absent";
        detached.setNodeColor(1.0f, 1.0f, 1.0f, 1.0f);
        std::fprintf(stderr, "FIXTURE_COLOR_RETURNED\n");
        return 97;
    }
    turnOffAllDebug(DebugTypeError);
    barCases();
    barPaintCases();
    compassCases();
    compassPaintCases();
    colorEffectCases();
    commandCases();
    std::printf("HUD bar/compass/colour %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
