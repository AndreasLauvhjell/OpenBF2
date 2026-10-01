// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HUD-TEXT-NODE-FIXTURE]
// Tests text nodes, styles, string data, and builder bindings with real HUD storage and quad helpers.
// Observers supply glyph metrics, font loading, drawing, and event registration.
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
#include "../BF2/Menu/Bf2TextNode.h"
#include "../BF2/Menu/Bf2Style.h"
#include "../BF2/External/BFMeme/TextNode.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Quad.h"
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/Menu/Bf2TransformNode.h"
#include "../BF2/Menu/Font.h"
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

struct DrawnGlyph { int character; int font; float x, y; };

// Every glyph is a fixed 10-wide box so the arithmetic in the tests is exact.
class GlyphSystem : public System
{
public:
    explicit GlyphSystem(NameTreeNode& registry) : System(800, 600), nextFont(50)
    { m_registry = &registry; }
    TypeDescriptor* getType() override { return Object::getClassType(); }
    int loadFont(const char* name) override
    {
        // Style::prepareFont dereferences the handle as a menu::Font, so the
        // fixture hands back a real one. Its name has no ".dif" suffix, which
        // keeps the unscaled arm of positionGlyph.
        loadedFonts.push_back(name ? name : "<null>");
        ++nextFont;
        return static_cast<int>(reinterpret_cast<std::size_t>(&fixtureFont()));
    }
    static dice::hfe::menu::Font& fixtureFont()
    {
        static dice::hfe::menu::Font font("fixture");
        return font;
    }
    static int fixtureFont0()
    { return static_cast<int>(reinterpret_cast<std::size_t>(&fixtureFont())); }
    float getFontHeight(int) override { return 12.0f; }
    bool getGlyphMetrics(int, int, GlyphMetrics* out) override
    {
        out->leftBearing = 1.0f;
        out->width = 8.0f;
        out->rightBearing = 1.0f;
        out->height = 10.0f;
        out->verticalOffset = 0.0f;
        return true;
    }
    bool drawCharacter(int character, int font, Quad* quad) override
    {
        DrawnGlyph glyph;
        glyph.character = character;
        glyph.font = font;
        glyph.x = quad->corners[0].x;
        glyph.y = quad->corners[0].y;
        drawn.push_back(glyph);
        return true;
    }
    std::vector<std::string> loadedFonts;
    std::vector<DrawnGlyph> drawn;
    int nextFont;
};
class RecordingEvents : public EventManager {};
namespace dice { namespace hfe { IBase* g_hudInformationLayer = nullptr; } }

static HudInformationLayer* information;
static const char* narrowSlot = "narrow";
static const wchar_t* wideSlot = L"wide";

// data class cases
static void stringDataCases()
{
    CHECK(sizeof(StringRefData) == 16);
    CHECK(sizeof(WstringRefData) == 16);
    CHECK(sizeof(WstringData) == 12);
    CHECK(std::strcmp(StringRefData::getClassType()->getName(),
                      "dice::meme::StringRefData") == 0);
    CHECK(std::strcmp(WstringRefData::getClassType()->getName(),
                      "dice::meme::WstringRefData") == 0);
    CHECK(std::strcmp(WstringData::getClassType()->getName(),
                      "dice::meme::WstringData") == 0);
    {   // The narrow reference reads and writes through the information slot.
        const char* slot = nullptr;
        StringRefData data(&slot, "");
        CHECK(data.getObjectSize() == 16);
        CHECK(data.getFloat(nullptr) == 0.0f);     // a null slot answers zero
        CHECK(data.getCharacter(nullptr, 0) == 0);
        CHECK(data.setString(nullptr, "42abc"));
        CHECK(slot && std::strcmp(slot, "42abc") == 0);
        CHECK(data.getFloat(nullptr) == 42.0f);
        CHECK(data.getCharacter(nullptr, 1) == '2');
        // With no slot the pointer aims at the object's own fallback.
        StringRefData owned(nullptr, nullptr);
        CHECK(owned.setString(nullptr, "x") && owned.getCharacter(nullptr, 0) == 'x');
    }
    {   // The wide reference widens on a narrow write.
        const wchar_t* slot = nullptr;
        WstringRefData data(&slot, "");
        CHECK(data.getType() == WstringRefData::getClassType());
        CHECK(data.setString(nullptr, "hi"));
        CHECK(slot && slot[0] == L'h' && slot[1] == L'i' && slot[2] == 0);
        CHECK(data.getCharacter(nullptr, 1) == 'i');
        CHECK(data.setWideString(nullptr, L"7"));
        CHECK(data.getFloat(nullptr) == 7.0f);
    }
    {   // The owning wide class clones and releases its own text.
        WstringData data(L"abc", "");
        CHECK(data.getObjectSize() == 12);
        CHECK(data.getCharacter(nullptr, 2) == 'c');
        CHECK(data.setWideString(nullptr, L"12") && data.getFloat(nullptr) == 12.0f);
        CHECK(data.setString(nullptr, "z") && data.getCharacter(nullptr, 0) == 'z');
        CHECK(data.getCharacter(nullptr, 1) == 0);
    }
}

// text node cases
static void textNodeCases()
{
    CHECK(sizeof(BfTextNode) == 24);
    CHECK(sizeof(Bf2TextNode) == 28);
    CHECK(std::strcmp(BfTextNode::getClassType()->getName(),
                      "dice::meme::BfTextNode") == 0);
    CHECK(Bf2TextNode::getClassType()->getBaseType() == BfTextNode::getClassType());
    CHECK(BfTextNode::getClassType()->getBaseType() == Node::getClassType());

    Bf2TextNode node("Label");
    CHECK(node.getObjectSize() == 28);
    CHECK(std::strcmp(node.getName(), "Label") == 0);
    CHECK(node.slot_0x24());
    CHECK(node.m_shadow != nullptr);          // the base allocates a black effect
    CHECK(node.m_shadow->getColor()->r == 0.0f && node.m_shadow->getColor()->a == 1.0f);
    CHECK(node.getStringDataForHud() == nullptr);
    CHECK(node.getStyleForHud() == nullptr);
    StringData text("hello", "");
    IntData type(0, "");
    Bf2Style style("", GlyphSystem::fixtureFont0());
    CHECK(node.setStringDataForHud(&text) && node.getStringDataForHud() == &text);
    CHECK(node.setStyleForHud(&style) && node.getStyleForHud() == &style);
    CHECK(node.setTypeDataForHud(&type) && node.m_type == &type);

    RuntimeObject* created = Bf2TextNode::getClassType()->create();
    CHECK(created != nullptr);
    delete static_cast<Bf2TextNode*>(created);

    // paintSelf needs both a string and a type, and the type must truncate to 0.
    NameTreeRoot registry;
    GlyphSystem system(registry);
    RenderContext context = {nullptr, nullptr, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    Rect bounds = {10.0f, 20.0f, 100.0f, 40.0f, 0.0f};
    const Rect clip = {0.0f, 0.0f, 800.0f, 600.0f, 0.0f};
    {
        Bf2TextNode bare("Bare");
        system.drawn.clear();
        bare.paintSelf(&system, nullptr, &bounds, &clip, &context);
        CHECK(system.drawn.empty());          // no string, no type
    }
    node.paintSelf(&system, nullptr, &bounds, &clip, &context);
    CHECK(system.drawn.size() == 5);          // "hello"
    CHECK(bounds.y == 21.0f);                 // the bounds rectangle is mutated
    type.setFloat(nullptr, 1.0f);
    system.drawn.clear();
    node.paintSelf(&system, nullptr, &bounds, &clip, &context);
    CHECK(system.drawn.empty());              // a non-zero type gates the paint
}

// style cases
static void styleCases()
{
    CHECK(sizeof(Bf2Style) == 44);
    CHECK(sizeof(Bf2RightAlignedStyle) == 44);
    CHECK(sizeof(Bf2CenterAlignedStyle) == 44);
    CHECK(std::strcmp(Bf2Style::getClassType()->getName(), "dice::meme::Bf2Style") == 0);
    CHECK(Bf2Style::getClassType()->getBaseType() == Style::getClassType());
    CHECK(Bf2RightAlignedStyle::getClassType()->getBaseType() == Bf2Style::getClassType());
    CHECK(Bf2CenterAlignedStyle::getClassType()->getBaseType() == Bf2Style::getClassType());

    Bf2Style style("Font", GlyphSystem::fixtureFont0());
    CHECK(style.getObjectSize() == 44);
    CHECK(style.m_wordWrap);                  // the constructor enables wrap
    CHECK(style.m_shadowFont == 0 && style.m_texture == 0);
    CHECK(style.m_shadowOffsetX == 0.0f && style.m_shadowOffsetY == 0.0f);
    CHECK(!style.setShadowFont(0));           // zero is refused
    CHECK(style.setShadowFont(9) && style.m_shadowFont == 9);
    CHECK(style.setShadowOffset(1.5f, -2.5f));
    CHECK(style.m_shadowOffsetX == 1.5f && style.m_shadowOffsetY == -2.5f);
    CHECK(style.setTexture(4) && style.m_texture == 4);
    RuntimeObject* created = Bf2Style::getClassType()->create();
    CHECK(created != nullptr);
    delete static_cast<Bf2Style*>(created);
}

static void styleDrawCases()
{
    NameTreeRoot registry;
    GlyphSystem system(registry);
    RenderContext context = {nullptr, nullptr, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    const Rect bounds = {0.0f, 0.0f, 100.0f, 40.0f, 0.0f};
    const Rect clip = {0.0f, 0.0f, 800.0f, 600.0f, 0.0f};
    StringData text("abc", "");
    {   // Left aligned: the pen starts at the left edge and steps 10 per glyph.
        Bf2Style style("", GlyphSystem::fixtureFont0());
        system.drawn.clear();
        style.drawText(&system, &text, 0, &bounds, &clip, &context);
        CHECK(system.drawn.size() == 3);
        if (system.drawn.size() == 3) {
            CHECK(system.drawn[0].character == 'a');
            CHECK(system.drawn[1].character == 'b');
            CHECK(system.drawn[2].character == 'c');
        }
    }
    {   // A shadow offset adds a second, earlier draw per glyph.
        Bf2Style style("", GlyphSystem::fixtureFont0());
        style.setShadowOffset(2.0f, 2.0f);
        style.setShadowFont(GlyphSystem::fixtureFont0() + 0);
        system.drawn.clear();
        style.drawText(&system, &text, 0, &bounds, &clip, &context);
        CHECK(system.drawn.size() == 6);
        if (system.drawn.size() == 6) {
            CHECK(system.drawn[0].font == GlyphSystem::fixtureFont0());
            CHECK(system.drawn[1].font == GlyphSystem::fixtureFont0());
            CHECK(system.drawn[0].x == system.drawn[1].x + 2.0f);
        }
    }
    {   // Right aligned walks the string backwards from its last character.
        Bf2RightAlignedStyle style("", GlyphSystem::fixtureFont0());
        system.drawn.clear();
        style.drawText(&system, &text, 0, &bounds, &clip, &context);
        CHECK(system.drawn.size() == 3);
        if (system.drawn.size() == 3) {
            CHECK(system.drawn[0].character == 'c');
            CHECK(system.drawn[2].character == 'a');
            CHECK(system.drawn[0].x > system.drawn[2].x);
        }
    }
    {   // Centre aligned measures the line first: 3 glyphs of 10 in 100 wide
        // start at ceil((100 - 30) * 0.5) = 35, plus the first left bearing.
        Bf2CenterAlignedStyle style("", GlyphSystem::fixtureFont0());
        system.drawn.clear();
        style.drawText(&system, &text, 0, &bounds, &clip, &context);
        CHECK(system.drawn.size() == 3);
        if (system.drawn.size() == 3) {
            CHECK(system.drawn[0].character == 'a');
            CHECK(system.drawn[0].x == 36.0f);
        }
    }
    {   // Newlines start a new line in every variant; the left one stops once
        // the pen leaves the bottom of the bounds.
        StringData lines("a\nb\nc", "");
        const Rect shallow = {0.0f, 0.0f, 100.0f, 20.0f, 0.0f};
        Bf2Style style("", GlyphSystem::fixtureFont0());
        system.drawn.clear();
        style.drawText(&system, &lines, 0, &shallow, &clip, &context);
        CHECK(system.drawn.size() == 2); // 'a' and 'b'; the third line is below
    }
    {   // The section-sign escape consumes five characters and draws none.
        StringData escaped("\xA7" "C1234x", "");
        Bf2Style style("", GlyphSystem::fixtureFont0());
        system.drawn.clear();
        style.drawText(&system, &escaped, 0, &bounds, &clip, &context);
        CHECK(system.drawn.size() == 1 && system.drawn[0].character == 'x');
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
    NameTreeRoot registry;
    GlyphSystem system;
    Bf2TransformNode* root;
    Bf2HudBuilder builder;
};

static void commandCases()
{
    {   // createTextNode registers "<name>TextNode" plus the transform, gives
        // the node a zero IntData type and selects it.
        Harness harness;
        CHECK(harness.builder.createTextNode("IngameHud", "Label", 1, 2, 3, 4));
        Bf2TextNode* node =
            dice::hfe::hud::builderCast<Bf2TextNode>(harness.find("LabelTextNode"));
        CHECK(node != nullptr);
        CHECK(node && node->m_type != nullptr);
        CHECK(node && node->m_type->getFloat(&harness.system) == 0.0f);
        Bf2TransformNode* transform =
            dice::hfe::hud::builderCast<Bf2TransformNode>(harness.find("Label"));
        CHECK(transform && transform->getChild(1) == node);
        CHECK(harness.builder.m_field014 == node);

        // A style row builds one of the three classes by alignment.
        CHECK(harness.builder.setTextNodeStyle("Fonts/a.dif", 2));
        CHECK(node && node->getStyleForHud() != nullptr);
        CHECK(node && node->getStyleForHud()->getType() == Bf2Style::getClassType());
        CHECK(harness.system.loadedFonts.size() == 1);
        CHECK(harness.builder.setTextNodeStyle("Fonts/a.dif", 1));
        CHECK(node && node->getStyleForHud()->getType() ==
              Bf2RightAlignedStyle::getClassType());
        CHECK(harness.builder.setTextNodeStyle("Fonts/a.dif", 0));
        CHECK(node && node->getStyleForHud()->getType() ==
              Bf2CenterAlignedStyle::getClassType());
        CHECK(!harness.builder.setTextNodeStyle("Fonts/a.dif", 9)); // invalid style
        CHECK(!harness.builder.setTextNodeStyle("", 2));            // empty path

        // The outline offset reaches the style, not the node.
        CHECK(harness.builder.setTextNodeOutLineOffset(3.0f, 4.0f));
        Bf2Style* style = node
            ? dice::hfe::hud::builderCast<Bf2Style>(node->getStyleForHud()) : nullptr;
        CHECK(style && style->m_shadowOffsetX == 3.0f && style->m_shadowOffsetY == 4.0f);

        // A named narrow variable becomes a StringRefData.
        CHECK(harness.builder.setTextNodeStringVariable("FixtureNarrow"));
        CHECK(node && node->getStringDataForHud() != nullptr);
        CHECK(node && node->getStringDataForHud()->getType() ==
              StringRefData::getClassType());
        // A wide-only variable becomes a WstringRefData.
        CHECK(harness.builder.setTextNodeStringVariable("FixtureWide"));
        CHECK(node && node->getStringDataForHud()->getType() ==
              WstringRefData::getClassType());
        CHECK(!harness.builder.setTextNodeStringVariable("FixtureMissing"));
    }
    {   // With nothing selected every row reports and returns false.
        Harness harness;
        CHECK(!harness.builder.setTextNodeStyle("Fonts/a.dif", 2));
        CHECK(!harness.builder.setTextNodeOutLineOffset(1.0f, 1.0f));
        CHECK(!harness.builder.setTextNodeStringVariable("FixtureNarrow"));
        CHECK(!harness.builder.setTextNodeString("hello"));
        CHECK(!harness.builder.setTextNodeString("")); // an empty string is refused
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
    information->registerString("FixtureNarrow", &narrowSlot);
    information->registerWideString("FixtureWide", &wideSlot);
    if (argc > 1 && !std::strcmp(argv[1], "style-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setTextNodeStyle("Fonts/a.dif", 9);
        std::fprintf(stderr, "FIXTURE_STYLE_RETURNED\n");
        return 98; // the command script requires the named diagnostic, not failure
    }
    if (argc > 1 && !std::strcmp(argv[1], "variable-death")) {
        setDebugCallback(DebugTypeError, printDiagnostic, nullptr);
        Harness harness;
        harness.builder.m_field018 = "Absent";
        harness.builder.setTextNodeStringVariable("FixtureMissing");
        std::fprintf(stderr, "FIXTURE_VARIABLE_RETURNED\n");
        return 99;
    }
    turnOffAllDebug(DebugTypeError);
    stringDataCases();
    textNodeCases();
    styleCases();
    styleDrawCases();
    commandCases();
    std::printf("HUD text node %d/%d checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
