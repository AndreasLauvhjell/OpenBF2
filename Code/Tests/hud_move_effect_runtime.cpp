// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-MOVE-EFFECT-FIXTURE]
// Tests move effects and builder bindings with real nodes, data, and quad helpers.
// System, stream, and event observers record drawing and serialization boundaries.
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
#include "../BF2/Menu/Bf2MoveEffect.h"
#include "../BF2/Menu/Bf2TransformNode.h"
#include "../BF2/External/BFMeme/MoveEffect.h"
#include "../BF2/External/BFMeme/EffectNode.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Quad.h"
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/FloatData.h"
#include "../BF2/External/BFMeme/IntData.h"
#include "../BF2/External/BFMeme/ShowEffectNode.h"
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

struct DrawnQuad { int handle; float x, y; float clipX, clipY; bool hadClip; };

class RecordingSystem : public System
{
public:
    explicit RecordingSystem(NameTreeNode& registry) : System(800, 600)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    bool drawCharacter(int character, int, Quad* quad) override
    { record(character, quad); return true; }
    bool drawTexture(int texture, Quad* quad) override
    { record(texture, quad); return true; }
    int loadTexture(const char*, bool) override { return 200; }
    void record(int handle, Quad* quad)
    {
        DrawnQuad d;
        d.handle = handle;
        d.x = quad->corners[0].x;
        d.y = quad->corners[0].y;
        d.hadClip = quad->clip != nullptr;
        d.clipX = d.hadClip ? quad->clip->x : 0.0f;
        d.clipY = d.hadClip ? quad->clip->y : 0.0f;
        drawn.push_back(d);
    }
    std::vector<DrawnQuad> drawn;
};

struct DataVisit { Data** slot; std::string label; };
class RecordingStream : public Stream
{
public:
    RecordingStream() : refuseAt(-1) {}
    bool isPassive() override { return true; }
    bool streamData(System*, Data*& value, const char* label, TypeDescriptor*) override
    {
        DataVisit visit;
        visit.slot = &value;
        visit.label = label ? label : "<null>";
        visits.push_back(visit);
        return static_cast<int>(visits.size()) != refuseAt;
    }
    std::vector<DataVisit> visits;
    int refuseAt;
};

class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }
static HudInformationLayer* information;
static bool frameShow = true;

// the two classes
static void moveEffectCases()
{
    CHECK(sizeof(MoveEffect) == 12);
    CHECK(sizeof(Bf2MoveEffect) == 12);
    CHECK(std::strcmp(MoveEffect::getClassType()->getName(),
                      "dice::meme::MoveEffect") == 0);
    CHECK(std::strcmp(Bf2MoveEffect::getClassType()->getName(),
                      "dice::meme::Bf2MoveEffect") == 0);
    CHECK(Bf2MoveEffect::getClassType()->getBaseType() == MoveEffect::getClassType());
    CHECK(MoveEffect::getClassType()->getBaseType() == Effect::getClassType());
    CHECK(MoveEffect::getClassType()->getInstanceSize() == 12);
    {   // Both factories leave the two operands null.
        RuntimeObject* made = MoveEffect::getClassType()->create();
        MoveEffect* effect = static_cast<MoveEffect*>(made);
        CHECK(effect && effect->m_direction == nullptr && effect->m_length == nullptr);
        CHECK(effect && effect->getObjectSize() == 12);
        delete effect;
        made = Bf2MoveEffect::getClassType()->create();
        Bf2MoveEffect* derived = static_cast<Bf2MoveEffect*>(made);
        CHECK(derived && derived->m_direction == nullptr && derived->m_length == nullptr);
        CHECK(derived && derived->getType() == Bf2MoveEffect::getClassType());
        // The constructor takes direction first even though it lives at +8.
        FloatData angle(1.0f, nullptr);
        IntData pixels(4, nullptr);
        CHECK(derived && derived->setDirection(&angle) && derived->m_direction == &angle);
        CHECK(derived && derived->setLength(&pixels) && derived->m_length == &pixels);
        delete derived;
    }
    {
        FloatData angle(0.5f, nullptr);
        IntData pixels(7, nullptr);
        Bf2MoveEffect effect(&angle, &pixels);
        CHECK(effect.m_direction == &angle);   // first argument -> +8
        CHECK(effect.m_length == &pixels);     // second argument -> +4
        NameTreeRoot registry;
        RecordingSystem system(registry);
        RecordingStream stream;
        CHECK(effect.streamProperties(&system, &stream));
        CHECK(stream.visits.size() == 2);
        if (stream.visits.size() == 2) {
            // Length is visited first, matching the member order, not the ctor's.
            CHECK(stream.visits[0].label == "Move length");
            CHECK(stream.visits[1].label == "Move direction");
        }
        RecordingStream refused;
        refused.refuseAt = 1;
        CHECK(!effect.streamProperties(&system, &refused));
        CHECK(refused.visits.size() == 1);
    }
}

static void moveDrawCases()
{
    NameTreeRoot registry;
    RecordingSystem system(registry);
    Rect bounds = { 100.0f, 200.0f, 10.0f, 10.0f, 0.0f };
    RenderContext context;
    context.parent = nullptr;
    context.effect = nullptr;
    context.field18 = 0.0f;                    // fully hidden: the whole distance

    {   // direction 0 moves the pen a full length in -x and nothing in y.
        FloatData angle(0.0f, nullptr);
        IntData pixels(10, nullptr);
        Bf2MoveEffect effect(&angle, &pixels);
        Quad quad(&bounds, nullptr);
        CHECK(effect.drawCharacter(&system, 'A', 1, 0, &quad, &context));
        CHECK(system.drawn.size() == 1);
        CHECK(system.drawn.size() == 1 && closeTo(system.drawn[0].x, 90.0f));
        CHECK(system.drawn.size() == 1 && closeTo(system.drawn[0].y, 200.0f));
    }
    system.drawn.clear();
    {   // A quarter turn moves it in +y instead.
        FloatData angle(1.5707963f, nullptr);
        IntData pixels(10, nullptr);
        Bf2MoveEffect effect(&angle, &pixels);
        Quad quad(&bounds, nullptr);
        CHECK(effect.drawTexture(&system, 5, &quad, &context));
        CHECK(system.drawn.size() == 1 && closeTo(system.drawn[0].x, 100.0f));
        CHECK(system.drawn.size() == 1 && closeTo(system.drawn[0].y, 210.0f));
    }
    system.drawn.clear();
    {   // Fully shown: the remaining distance is zero, so nothing moves.
        FloatData angle(0.0f, nullptr);
        IntData pixels(10, nullptr);
        Bf2MoveEffect effect(&angle, &pixels);
        RenderContext shown;
        shown.parent = nullptr;
        shown.effect = nullptr;
        shown.field18 = 1.0f;
        Quad quad(&bounds, nullptr);
        CHECK(effect.drawCharacter(&system, 'B', 1, 0, &quad, &shown));
        CHECK(system.drawn.size() == 1 && closeTo(system.drawn[0].x, 100.0f));
    }
    system.drawn.clear();
    {   // A null length keeps the shipped zero distance, and the direction is
        // still sampled first.
        FloatData angle(0.0f, nullptr);
        Bf2MoveEffect effect(&angle, nullptr);
        Quad quad(&bounds, nullptr);
        CHECK(effect.drawCharacter(&system, 'C', 1, 0, &quad, &context));
        CHECK(system.drawn.size() == 1 && closeTo(system.drawn[0].x, 100.0f));
    }
    system.drawn.clear();
    {   // The clip travels with the quad on a stack copy, and the quad's own
        // clip pointer is put back the moment the chain returns.
        FloatData angle(0.0f, nullptr);
        IntData pixels(10, nullptr);
        Bf2MoveEffect effect(&angle, &pixels);
        Rect clip = { 50.0f, 60.0f, 400.0f, 300.0f, 0.0f };
        Quad quad(&bounds, &clip);
        CHECK(quad.clip == &clip);
        CHECK(effect.drawCharacter(&system, 'D', 1, 0, &quad, &context));
        CHECK(system.drawn.size() == 1 && system.drawn[0].hadClip);
        CHECK(system.drawn.size() == 1 && closeTo(system.drawn[0].clipX, 40.0f));
        CHECK(system.drawn.size() == 1 && closeTo(system.drawn[0].clipY, 60.0f));
        CHECK(quad.clip == &clip);             // restored, and never written through
        CHECK(clip.x == 50.0f && clip.y == 60.0f);
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
    {   // createTransformNode: one node, no data object, mode 2.
        Harness harness;
        CHECK(harness.builder.createTransformNode("IngameHud", "Panel", 3, 4, 50, 20));
        Bf2TransformNode* panel =
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Panel"));
        CHECK(panel != nullptr);
        CHECK(panel && panel->getPosition().x == 3.0f && panel->getPosition().y == 4.0f);
        CHECK(panel && panel->getSize().x == 50.0f && panel->getSize().y == 20.0f);
        // Mode 2 selects the node itself, so m_field050 follows it.
        CHECK(harness.builder.m_field050 == "Panel");
        CHECK(harness.builder.m_field014 == panel);
        // The same child-0-chain hole the other create rows have.
        CHECK(harness.builder.createTransformNode("IngameHud", "Panel", 0, 0, 1, 1));
        harness.root->setChild(0, panel);
        CHECK(!harness.builder.createTransformNode("IngameHud", "Panel", 0, 0, 1, 1));
        harness.root->setChild(0, nullptr);
    }
    {   // addNodeMoveShowEffect needs the cull node the show-variable row builds.
        Harness harness;
        CHECK(harness.builder.createTransformNode("IngameHud", "Panel", 0, 0, 10, 10));
        CHECK(!harness.builder.addNodeMoveShowEffect(1.57f, 14)); // no "<name>CullNode"
        CHECK(harness.builder.setNodeShowVariable("FixtureShow"));
        Node* cull = dice::hfe::hud::builderCast<Node>(harness.find("PanelCullNode"));
        CHECK(cull != nullptr);
        Node* const original = cull ? cull->getChild(0) : nullptr;
        CHECK(harness.builder.addNodeMoveShowEffect(1.57f, 14));
        ShowEffectNode* show = cull
            ? dice::hfe::hud::builderCast<ShowEffectNode>(cull->getChild(0)) : nullptr;
        CHECK(show != nullptr);
        CHECK(show && show->getChild(0) == original);
        Bf2MoveEffect* effect = show
            ? dice::hfe::hud::builderCast<Bf2MoveEffect>(show->m_effect) : nullptr;
        CHECK(effect != nullptr);
        CHECK(effect && effect->m_direction != nullptr);
        CHECK(effect && effect->m_direction->getType() == FloatData::getClassType());
        CHECK(effect && closeTo(effect->m_direction->getFloat(&harness.system), 1.57f));
        CHECK(effect && effect->m_length != nullptr);
        CHECK(effect && effect->m_length->getType() == IntData::getClassType());
        CHECK(effect && closeTo(effect->m_length->getFloat(&harness.system), 14.0f));
        // The effect node is never named or registered, so the "%sMoveEffect"
        // duplicate check can never see it: a repeat succeeds and chains again.
        CHECK(harness.find("PanelMoveEffect") == nullptr);
        CHECK(harness.builder.addNodeMoveShowEffect(0.0f, 1));
        CHECK(cull && dice::hfe::hud::builderCast<ShowEffectNode>(cull->getChild(0)) != show);
    }
    {   // An object already registered under the duplicate name refuses.
        Harness harness;
        CHECK(harness.builder.createTransformNode("IngameHud", "Panel", 0, 0, 10, 10));
        CHECK(harness.builder.setNodeShowVariable("FixtureShow"));
        void* allocation = ::operator new(sizeof(Bf2TransformNode));
        Bf2TransformNode* existing =
            new(allocation) Bf2TransformNode("PanelMoveEffect", 0, 0, 1, 1);
        CHECK(harness.registry.insert(&harness.system, existing, nullptr));
        CHECK(!harness.builder.addNodeMoveShowEffect(1.0f, 2));
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
    information->registerBool("FixtureShow", &frameShow);
    if (argc > 1 && !std::strcmp(argv[1], "create-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.createTransformNode("IngameHud", "Panel", 0, 0, 1, 1);
        harness.root->setChild(0,
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Panel")));
        harness.builder.createTransformNode("IngameHud", "Panel", 0, 0, 1, 1);
        std::fprintf(stderr, "FIXTURE_CREATE_RETURNED\n");
        return 98;
    }
    if (argc > 1 && !std::strcmp(argv[1], "move-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field050 = "Absent";
        harness.builder.addNodeMoveShowEffect(1.0f, 2);
        std::fprintf(stderr, "FIXTURE_MOVE_RETURNED\n");
        return 99;
    }
    turnOffAllDebug(DebugTypeError);
    moveEffectCases();
    moveDrawCases();
    commandCases();
    std::printf("HUD move effect %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
