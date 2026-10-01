// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-BLEND-EFFECT-FIXTURE]
// Tests blend effects and transform bindings with real HUD nodes, registry, and information layer.
// System, stream, and event observers record external operations.
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
#include "../BF2/Menu/Bf2BlendFuncEffect.h"
#include "../BF2/Menu/Bf2ColorEffect.h"
#include "../BF2/Menu/Bf2EffectNode.h"
#include "../BF2/Menu/Bf2TransformNode.h"
#include "../BF2/External/BFMeme/BlendFuncEffect.h"
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
// HudInformationLayer's constructor writes through this global; a /force-nulled
// one kills the process before main's first check.
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

struct DrawnQuad { int handle; int source; int destination; };

class RecordingSystem : public System
{
public:
    explicit RecordingSystem(NameTreeNode& registry) : System(800, 600)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    bool drawCharacter(int character, int, Quad* quad) override
    {
        DrawnQuad record = { character, quad->state08, quad->state0c };
        characters.push_back(record);
        return true;
    }
    bool drawTexture(int texture, Quad* quad) override
    {
        DrawnQuad record = { texture, quad->state08, quad->state0c };
        textures.push_back(record);
        return true;
    }
    int loadTexture(const char*, bool) override { return 200; }
    std::vector<DrawnQuad> characters, textures;
};

// Only the one visitor slot the blend effect uses. Everything else keeps the
// base Stream behaviour, which answers isPassive().
struct EnumVisit { int* slot; std::string label; EnumName name; EnumNext next; };
class RecordingStream : public Stream
{
public:
    RecordingStream() : refuseAt(-1) {}
    bool isPassive() override { return true; }
    bool streamEnum(int* value, EnumNext next, EnumName name, const char* label) override
    {
        EnumVisit visit;
        visit.slot = value;
        visit.label = label ? label : "<null>";
        visit.name = name;
        visit.next = next;
        visits.push_back(visit);
        return static_cast<int>(visits.size()) != refuseAt;
    }
    std::vector<EnumVisit> visits;
    int refuseAt; // 1-based index of the visit that answers false
};

class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }

static HudInformationLayer* information;
static float alphaSlot = 0.25f, redSlot = 0.5f, greenSlot = 0.6f, blueSlot = 0.7f;
static int columnSlot = 11, rowSlot = 22;

// the enum table
static void blendFuncCases()
{
    CHECK(std::strcmp(blendFuncName(0), "ERROR") == 0);
    CHECK(std::strcmp(blendFuncName(1), "Zero") == 0);
    CHECK(std::strcmp(blendFuncName(2), "One") == 0);
    CHECK(std::strcmp(blendFuncName(7), "Source alpha") == 0);
    CHECK(std::strcmp(blendFuncName(8), "One minus source alpha") == 0);
    CHECK(std::strcmp(blendFuncName(11), "Source alpha saturate") == 0);
    CHECK(blendFuncName(12) == nullptr);
    CHECK(blendFuncName(-1) == nullptr);
    // The walk is terminated by the first unnamed value, not by a count, and
    // the terminator it publishes is 0 - the same value the table names "ERROR".
    int value = 0, steps = 0;
    while ((value = blendFuncNext(value)) != 0 && steps < 32) ++steps;
    CHECK(steps == 12); // 0 steps through 1..12; 12 is unnamed and ends the walk
    CHECK(blendFuncNext(11) == 12);
    CHECK(blendFuncNext(12) == 0);
    CHECK(blendFuncNext(-5) == 0);
}

// the two classes
static void effectCases()
{
    CHECK(sizeof(BlendFuncEffect) == 12);
    CHECK(sizeof(Bf2BlendFuncEffect) == 12);
    CHECK(std::strcmp(BlendFuncEffect::getClassType()->getName(),
                      "dice::meme::BlendFuncEffect") == 0);
    CHECK(std::strcmp(Bf2BlendFuncEffect::getClassType()->getName(),
                      "dice::meme::Bf2BlendFuncEffect") == 0);
    CHECK(Bf2BlendFuncEffect::getClassType()->getBaseType() == BlendFuncEffect::getClassType());
    CHECK(BlendFuncEffect::getClassType()->getBaseType() == Effect::getClassType());
    CHECK(BlendFuncEffect::getClassType()->getInstanceSize() == 12);
    CHECK(Bf2BlendFuncEffect::getClassType()->getInstanceSize() == 12);

    {   // Both factories supply the ordinary alpha blend, 7 and 8.
        RuntimeObject* made = BlendFuncEffect::getClassType()->create();
        BlendFuncEffect* effect = static_cast<BlendFuncEffect*>(made);
        CHECK(effect && effect->m_source == 7 && effect->m_destination == 8);
        CHECK(effect && effect->getType() == BlendFuncEffect::getClassType());
        CHECK(effect && effect->getObjectSize() == 12);
        CHECK(effect && effect->getColor() == nullptr); // inherited, still nothing
        delete effect;
    }
    {
        RuntimeObject* made = Bf2BlendFuncEffect::getClassType()->create();
        Bf2BlendFuncEffect* effect = static_cast<Bf2BlendFuncEffect*>(made);
        CHECK(effect && effect->m_source == 7 && effect->m_destination == 8);
        CHECK(effect && effect->getType() == Bf2BlendFuncEffect::getClassType());
        CHECK(effect && effect->getObjectSize() == 12);
        // The two added slots are plain stores with no validation and no answer.
        effect->setSource(1);
        effect->setDestination(11);
        CHECK(effect && effect->m_source == 1 && effect->m_destination == 11);
        effect->setSource(-4);                      // out of the named range
        CHECK(effect && effect->m_source == -4);
        delete effect;
    }
    {   // Both draw bodies stamp the quad's two state words and then hand the
        // quad on. With a null parent context the chain reaches the System.
        NameTreeRoot registry;
        RecordingSystem system(registry);
        Bf2BlendFuncEffect effect(3, 9);
        Rect bounds = { 0.0f, 0.0f, 10.0f, 10.0f };
        Quad quad(&bounds, &bounds);
        RenderContext context;
        context.parent = nullptr;
        context.effect = &effect;
        // The Quad constructor already publishes the ordinary alpha blend, so
        // the effect exists to replace 7/8, not to fill in an empty pair.
        CHECK(quad.state08 == 7 && quad.state0c == 8);
        CHECK(effect.drawCharacter(&system, 'A', 5, 0, &quad, &context));
        CHECK(quad.state08 == 3 && quad.state0c == 9);
        CHECK(system.characters.size() == 1);
        CHECK(system.characters.size() == 1 && system.characters[0].handle == 'A');
        CHECK(system.characters.size() == 1 && system.characters[0].source == 3);
        CHECK(system.characters.size() == 1 && system.characters[0].destination == 9);
        CHECK(effect.drawTexture(&system, 77, &quad, &context));
        CHECK(system.textures.size() == 1 && system.textures[0].handle == 77);
        CHECK(system.textures.size() == 1 && system.textures[0].destination == 9);
    }
    {   // streamProperties visits source then destination, each with the same
        // enum pair, and stops on the first refusal.
        NameTreeRoot registry;
        RecordingSystem system(registry);
        Bf2BlendFuncEffect effect(2, 6);
        RecordingStream stream;
        CHECK(effect.streamProperties(&system, &stream));
        CHECK(stream.visits.size() == 2);
        if (stream.visits.size() == 2) {
            CHECK(stream.visits[0].slot == &effect.m_source);
            CHECK(stream.visits[0].label == "Source blend func");
            CHECK(stream.visits[1].slot == &effect.m_destination);
            CHECK(stream.visits[1].label == "Destination blend func");
            CHECK(stream.visits[0].name == stream.visits[1].name);
            CHECK(stream.visits[0].next == stream.visits[1].next);
            CHECK(std::strcmp(stream.visits[0].name(8), "One minus source alpha") == 0);
        }
        RecordingStream refused;
        refused.refuseAt = 1;
        CHECK(!effect.streamProperties(&system, &refused));
        CHECK(refused.visits.size() == 1); // the destination is never visited
    }
}

// the four transform accessors
static void transformAccessorCases()
{
    Bf2TransformNode node("Panel", 1, 2, 3, 4);
    CHECK(node.getPosition().x == 1.0f && node.getPosition().y == 2.0f);
    CHECK(node.getSize().x == 3.0f && node.getSize().y == 4.0f);
    CHECK(node.setPositionForHud(11.5f, 12.5f));
    CHECK(node.getPosition().x == 11.5f && node.getPosition().y == 12.5f);
    CHECK(node.setSizeForHud(20.0f, 30.0f));
    CHECK(node.getSize().x == 20.0f && node.getSize().y == 30.0f);
    CHECK(node.setOffsetForHud(-7, 9));
    CHECK(node.m_offsetX == -7 && node.m_offsetY == 9);
    // Index 0 binds x; every other index - not just 1 - binds y.
    CHECK(node.setPositionBindingForHud(0, &columnSlot));
    CHECK(node.m_positionX == &columnSlot && node.m_positionY == nullptr);
    CHECK(node.setPositionBindingForHud(1, &rowSlot));
    CHECK(node.m_positionY == &rowSlot);
    CHECK(node.setPositionBindingForHud(7, &columnSlot));
    CHECK(node.m_positionY == &columnSlot);
    // A null pointer is the one refusal, and it leaves both slots alone.
    CHECK(!node.setPositionBindingForHud(0, nullptr));
    CHECK(node.m_positionX == &columnSlot);
}

struct Harness
{
    Harness() : system(registry)
    {
        void* rootAllocation = ::operator new(sizeof(Bf2TransformNode));
        root = new(rootAllocation) Bf2TransformNode("IngameHud", 0, 0, 800, 600);
        CHECK(registry.insert(nullptr, root, nullptr));
        void* frameAllocation = ::operator new(sizeof(Bf2TransformNode));
        frame = new(frameAllocation) Bf2TransformNode("Frame", 0, 0, 64, 64);
        CHECK(registry.insert(&system, frame, nullptr));
        root->setChild(0, frame);
        builder.setSystem(&system);
        builder.m_field050 = "Frame";  // the node name every row below acts on
    }
    Object* find(const char* name) { return registry.findObject(&system, name); }
    NameTreeRoot registry;
    RecordingSystem system;
    Bf2TransformNode* root;
    Bf2TransformNode* frame;
    Bf2HudBuilder builder;
};

// the builder rows
static void commandCases()
{
    {   // With no cull node the command falls back to the bare node and splices
        // an EffectNode carrying a Bf2BlendFuncEffect in front of its child 0.
        Harness harness;
        Node* const originalChild = harness.frame->getChild(0);
        CHECK(harness.builder.addNodeBlendEffect(1, 11));
        EffectNode* carrier =
            dice::hfe::hud::builderCast<EffectNode>(harness.frame->getChild(0));
        CHECK(carrier != nullptr);
        CHECK(carrier && carrier->getChild(0) == originalChild);
        Bf2BlendFuncEffect* effect = carrier
            ? dice::hfe::hud::builderCast<Bf2BlendFuncEffect>(carrier->m_effect) : nullptr;
        CHECK(effect != nullptr);
        CHECK(effect && effect->m_source == 1 && effect->m_destination == 11);
        // The effect node is never named and never registered, so the shipped
        // "%sBlendEffect" duplicate check can never see it. A second command
        // therefore succeeds again and chains a second effect node.
        CHECK(harness.find("FrameBlendEffect") == nullptr);
        CHECK(harness.builder.addNodeBlendEffect(2, 3));
        EffectNode* second =
            dice::hfe::hud::builderCast<EffectNode>(harness.frame->getChild(0));
        CHECK(second != nullptr && second != carrier);
        CHECK(second && second->getChild(0) == carrier);
    }
    {   // A registered "<name>CullNode" wins over the bare node.
        Harness harness;
        void* allocation = ::operator new(sizeof(Bf2TransformNode));
        Bf2TransformNode* cull = new(allocation) Bf2TransformNode("FrameCullNode", 0, 0, 1, 1);
        CHECK(harness.registry.insert(&harness.system, cull, nullptr));
        CHECK(harness.builder.addNodeBlendEffect(5, 6));
        CHECK(dice::hfe::hud::builderCast<EffectNode>(cull->getChild(0)) != nullptr);
        CHECK(dice::hfe::hud::builderCast<EffectNode>(harness.frame->getChild(0)) == nullptr);
    }
    {   // An object already registered under the duplicate name refuses, and
        // an unknown node name refuses too.
        Harness harness;
        void* allocation = ::operator new(sizeof(Bf2TransformNode));
        Bf2TransformNode* existing =
            new(allocation) Bf2TransformNode("FrameBlendEffect", 0, 0, 1, 1);
        CHECK(harness.registry.insert(&harness.system, existing, nullptr));
        CHECK(!harness.builder.addNodeBlendEffect(1, 2));
        harness.builder.m_field050 = "Absent";
        CHECK(!harness.builder.addNodeBlendEffect(1, 2));
    }
    {   // setNodeAlphaVariable creates the colour effect on its first call and
        // retries itself, so one command both builds and binds.
        Harness harness;
        CHECK(harness.find("FrameColorEffect") == nullptr);
        CHECK(harness.builder.setNodeAlphaVariable("FixtureAlpha"));
        Bf2EffectNode* carrier =
            dice::hfe::hud::builderCast<Bf2EffectNode>(harness.find("FrameColorEffect"));
        CHECK(carrier != nullptr);
        Bf2ColorEffect* effect = carrier
            ? dice::hfe::hud::builderCast<Bf2ColorEffect>(carrier->getEffectForHud()) : nullptr;
        CHECK(effect != nullptr);
        CHECK(effect && effect->m_alphaBinding == &alphaSlot);
        // The created effect is white; only the alpha is bound.
        CHECK(effect && effect->getColor()->r == 1.0f && effect->getColor()->a == 1.0f);
        CHECK(effect && effect->m_redBinding == &effect->m_disabledBinding);
        // A second call reuses the same effect rather than building another.
        CHECK(harness.builder.setNodeAlphaVariable("FixtureAlpha"));
        CHECK(dice::hfe::hud::builderCast<Bf2EffectNode>(harness.find("FrameColorEffect")) == carrier);
        // An unresolved name leaves the binding as it was.
        CHECK(!harness.builder.setNodeAlphaVariable("FixtureMissing"));
        CHECK(effect && effect->m_alphaBinding == &alphaSlot);
    }
    {   // setNodeRGBVariables binds all three channels through the same effect.
        Harness harness;
        CHECK(harness.builder.setNodeRGBVariables("FixtureRed", "FixtureGreen", "FixtureBlue"));
        Bf2EffectNode* carrier =
            dice::hfe::hud::builderCast<Bf2EffectNode>(harness.find("FrameColorEffect"));
        Bf2ColorEffect* effect = carrier
            ? dice::hfe::hud::builderCast<Bf2ColorEffect>(carrier->getEffectForHud()) : nullptr;
        CHECK(effect != nullptr);
        CHECK(effect && effect->m_redBinding == &redSlot);
        CHECK(effect && effect->m_greenBinding == &greenSlot);
        CHECK(effect && effect->m_blueBinding == &blueSlot);
        CHECK(effect && effect->m_alphaBinding == &effect->m_disabledBinding);
        // One missing name refuses the whole row and changes nothing, even
        // though the shipped body still looks the other two up first.
        CHECK(!harness.builder.setNodeRGBVariables("FixtureRed", "FixtureMissing", "FixtureBlue"));
        CHECK(effect && effect->m_greenBinding == &greenSlot);
        // Both variable rows share the one effect the first of them created.
        CHECK(harness.builder.setNodeAlphaVariable("FixtureAlpha"));
        CHECK(effect && effect->m_alphaBinding == &alphaSlot);
        CHECK(dice::hfe::hud::builderCast<Bf2EffectNode>(harness.find("FrameColorEffect")) == carrier);
    }
    {   // setNodePosVariable binds an INT through the transform itself. There is
        // no cull-node fallback and no effect is built.
        Harness harness;
        CHECK(harness.builder.setNodePosVariable(0, "FixtureColumn"));
        CHECK(harness.frame->m_positionX == &columnSlot);
        CHECK(harness.frame->m_positionY == nullptr);
        CHECK(harness.builder.setNodePosVariable(1, "FixtureRow"));
        CHECK(harness.frame->m_positionY == &rowSlot);
        CHECK(harness.find("FrameColorEffect") == nullptr);
        // A float-only name does not resolve: the lookup is the INT registry.
        CHECK(!harness.builder.setNodePosVariable(0, "FixtureAlpha"));
        CHECK(!harness.builder.setNodePosVariable(0, "FixtureMissing"));
        CHECK(harness.frame->m_positionX == &columnSlot);
        // A name that is not a Bf2TransformNode refuses as well.
        harness.builder.m_field050 = "IngameHud";
        CHECK(harness.builder.setNodePosVariable(0, "FixtureColumn"));
        harness.builder.m_field050 = "Absent";
        CHECK(!harness.builder.setNodePosVariable(0, "FixtureColumn"));
    }
    {   // The three plain geometry rows (139, 140, 144) on the selected
        // transform. Position and size fild their console integers into
        // the float position/size; offset passes them through as the signed
        // ints the transform adds each frame.
        Harness harness;
        CHECK(harness.builder.setNodePos(7, -8));
        CHECK(harness.frame->getPosition().x == 7.0f);
        CHECK(harness.frame->getPosition().y == -8.0f);
        CHECK(harness.builder.setNodeSize(120, 34));
        CHECK(harness.frame->getSize().x == 120.0f);
        CHECK(harness.frame->getSize().y == 34.0f);
        CHECK(harness.builder.setNodeOffset(-3, 9));
        CHECK(harness.frame->m_offsetX == -3 && harness.frame->m_offsetY == 9);
        // A name that resolves to nothing refuses all three.
        harness.builder.m_field050 = "Absent";
        CHECK(!harness.builder.setNodePos(1, 1));
        CHECK(!harness.builder.setNodeOffset(1, 1));
        CHECK(!harness.builder.setNodeSize(1, 1));
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
    information->registerFloat("FixtureAlpha", &alphaSlot);
    information->registerFloat("FixtureRed", &redSlot);
    information->registerFloat("FixtureGreen", &greenSlot);
    information->registerFloat("FixtureBlue", &blueSlot);
    information->registerInt("FixtureColumn", &columnSlot);
    information->registerInt("FixtureRow", &rowSlot);
    if (argc > 1 && !std::strcmp(argv[1], "duplicate-death")) {
        // The duplicate arm falls through to a second diagnostic. With Debug errors enabled, the first abort ends the test;
        // with errors disabled, commandCases verifies the false return from the fall-through path.
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        void* allocation = ::operator new(sizeof(Bf2TransformNode));
        Bf2TransformNode* existing =
            new(allocation) Bf2TransformNode("FrameBlendEffect", 0, 0, 1, 1);
        harness.registry.insert(&harness.system, existing, nullptr);
        harness.builder.addNodeBlendEffect(1, 2);
        std::fprintf(stderr, "FIXTURE_DUPLICATE_RETURNED\n");
        return 95; // the command script requires the named diagnostic, not failure
    }
    if (argc > 1 && !std::strcmp(argv[1], "blend-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field050 = "Absent";
        harness.builder.addNodeBlendEffect(1, 2);
        std::fprintf(stderr, "FIXTURE_BLEND_RETURNED\n");
        return 96;
    }
    if (argc > 1 && !std::strcmp(argv[1], "alpha-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.setNodeAlphaVariable("FixtureMissing");
        std::fprintf(stderr, "FIXTURE_ALPHA_RETURNED\n");
        return 97;
    }
    if (argc > 1 && !std::strcmp(argv[1], "rgb-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.setNodeRGBVariables("FixtureRed", "FixtureMissing", "FixtureBlue");
        std::fprintf(stderr, "FIXTURE_RGB_RETURNED\n");
        return 98;
    }
    if (argc > 1 && !std::strcmp(argv[1], "size-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field050 = "Absent";
        harness.builder.setNodeSize(1, 1);
        std::fprintf(stderr, "FIXTURE_SIZE_RETURNED\n");
        return 94;
    }
    if (argc > 1 && !std::strcmp(argv[1], "pos-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field050 = "Absent";
        harness.builder.setNodePosVariable(0, "FixtureColumn");
        std::fprintf(stderr, "FIXTURE_POS_RETURNED\n");
        return 99;
    }
    turnOffAllDebug(DebugTypeError);
    blendFuncCases();
    effectCases();
    transformAccessorCases();
    commandCases();
    std::printf("HUD blend effect %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
