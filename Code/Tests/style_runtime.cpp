// [TEMP] Scoped x86 client fixture, not a renderer or production font provider.
// Uses real Style/Data/Object/System construction and real recovered methods.
// Only the requested metric responses, character source and one virtual-forwarding
// observer are fabricated. drawText/prepareFont/registration are never invoked.
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/External/BFMeme/System.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <vector>

using namespace dice::meme;
namespace {
int checks = 0, failures = 0;
void check(bool value, const char* label)
{
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr, "FAIL: %s\n", label); }
}

struct MetricRequest
{
    int character, font;
    GlyphMetrics metrics;
    bool result;
};

// [TEMP] Only prerecorded calls are accepted. Even a false result writes the
// recorded outputs so the ignored-bool case is tested without uninitialized data.
class RecordedFontSystem : public System
{
public:
    RecordedFontSystem() : System(800.0f, 600.0f), next(0) {}
    void expect(std::initializer_list<MetricRequest> requests)
    {
        check(next == expected.size(), "previous metric request sequence consumed");
        expected.assign(requests.begin(), requests.end());
        next = 0;
    }
    bool getGlyphMetrics(int character, int font, GlyphMetrics* output) override
    {
        if (next >= expected.size())
        {
            std::fprintf(stderr, "[TEMP] Unexpected fixture glyph request %d/%d\n", character, font);
            std::fflush(stderr);
            std::abort();
        }
        const MetricRequest& request = expected[next++];
        check(character == request.character && font == request.font,
              "metric call order and complete character/font values");
        *output = request.metrics;
        return request.result;
    }
    bool consumed() const { return next == expected.size(); }
private:
    std::vector<MetricRequest> expected;
    unsigned int next;
};

class FixtureStyle : public Style
{
public:
    FixtureStyle() : Style("metric fixture", 73)
    { m_shadowFont = 0; m_scale = 1.0f; m_reciprocalScale = 1.0f; }
    // [TEMP] Supply the recorded post-prepare state without implementing or
    // invoking the unresolved production font preparation method.
    void scale(float value, float reciprocal)
    { m_scale = value; m_reciprocalScale = reciprocal; }
};

// [TEMP] Preserves the real indexed Data wrapper; only supplies its scalar input.
class IntegerCharacterData : public Data
{
public:
    IntegerCharacterData(std::initializer_list<int> characters)
        : Data("integer characters"), values(characters), calls(0) {}
    int getCharacter(System* system, int index) override
    {
        check(system != 0 && index == calls, "real Data wrapper forwards scalar index in order");
        ++calls;
        return index < static_cast<int>(values.size()) ? values[index] : 0;
    }
    std::vector<int> values;
    int calls;
};

// [TEMP] An observer replaces the already-recovered positioning operation only
// to prove that the real integer wrapper dispatches virtually and truncates.
class ForwardingStyle : public FixtureStyle
{
public:
    ForwardingStyle() : calls(0), inputX(0), inputY(0), seenOutput(0), seenMetrics(0) {}
    void positionGlyph(Rect* output, const GlyphMetrics* metrics, float* x, float* y) override
    {
        ++calls; inputX = *x; inputY = *y;
        seenOutput = output; seenMetrics = metrics;
        *x = -3.75f; *y = 4.75f;
        output->x = 101.0f; output->y = 102.0f;
    }
    int calls;
    float inputX, inputY;
    Rect* seenOutput;
    const GlyphMetrics* seenMetrics;
};

void widths()
{
    RecordedFontSystem font;
    FixtureStyle object;
    Style* style = &object;
    const GlyphMetrics ordinary{2.0f, 7.0f, 3.0f, 99.0f, -42.0f};
    font.expect({{-128, 73, ordinary, false}, {0x10041, 73, ordinary, true}});
    check(style->getCharacterWidth(&font, -128) == 12.0f,
          "scalar width preserves signed character and ignores false metric result");
    check(style->getCharacterWidth(&font, 0x10041) == 12.0f,
          "scalar width forwards character bits above sixteen");

    const GlyphMetrics half{0.0f, 0.5f, 0.0f, 0.0f, 0.0f};
    const GlyphMetrics oneQuarter{0.0f, 1.25f, 0.0f, 0.0f, 0.0f};
    const GlyphMetrics twoQuarter{0.0f, 2.25f, 0.0f, 0.0f, 0.0f};
    font.expect({{-128, 73, half, true}, {-1, 73, oneQuarter, true}, {'A', 73, twoQuarter, true}});
    const char narrow[] = {static_cast<char>(0x80), static_cast<char>(0xff), 'A', 0};
    check(style->getStringWidth(&font, narrow) == 4.0f,
          "narrow text sign-extends each character and visits in sequence");

    font.expect({{-128, 73, ordinary, true}, {0x10041, 73, ordinary, true}});
    IntegerCharacterData integers{-128, 0x10041};
    check(style->getDataWidth(&font, &integers, 999) == 24.0f && integers.calls == 3,
          "real indexed Data wrapper retains full signed character values and terminator");

    const GlyphMetrics six{1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    BoolRefData boolean(0, "real Boolean data");
    font.expect({{'f', 73, six, true}, {'a', 73, six, true}, {'l', 73, six, true},
                 {'s', 73, six, true}, {'e', 73, six, true}});
    check(style->getDataWidth(&font, &boolean, -42) == 30.0f,
          "real BoolRefData false string through real Data indexed wrapper");
    boolean.setFloat(&font, 1.0f);
    font.expect({{'t', 73, six, true}, {'r', 73, six, true}, {'u', 73, six, true}, {'e', 73, six, true}});
    check(style->getDataWidth(&font, &boolean, 123) == 24.0f,
          "real BoolRefData true string ignores arbitrary element index");

    const float huge = std::ldexp(1.0f, 80);
    const GlyphMetrics cancellation{huge, 1.0f, -huge, 0.0f, 0.0f};
    font.expect({{'X', 73, cancellation, true}, {'X', 73, cancellation, true}, {'X', 73, cancellation, true}});
    check(style->getCharacterWidth(&font, 'X') == 1.0f,
          "scalar addition order is left plus right then width");
    check(style->getStringWidth(&font, "X") == 0.0f,
          "narrow loop addition order is right plus width then left");
    IntegerCharacterData one{'X'};
    check(style->getDataWidth(&font, &one, 0) == 0.0f,
          "Data loop retains the different addition order");

    const GlyphMetrics large{0.0f, 16777216.0f, 0.0f, 0.0f, 0.0f};
    const GlyphMetrics unit{0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    const GlyphMetrics negativeLarge{0.0f, -16777216.0f, 0.0f, 0.0f, 0.0f};
    font.expect({{'A', 73, large, true}, {'B', 73, unit, true}, {'C', 73, negativeLarge, true}});
    check(style->getStringWidth(&font, "ABC") == 0.0f,
          "text accumulation rounds to float after every character");
    font.expect({{'A', 73, large, true}, {'B', 73, unit, true}, {'C', 73, negativeLarge, true}});
    IntegerCharacterData rounded{'A', 'B', 'C'};
    check(style->getDataWidth(&font, &rounded, 2) == 0.0f,
          "Data accumulation also rounds after every character");

    font.expect({});
    Data empty;
    check(style->getStringWidth(&font, "") == 0.0f &&
          style->getDataWidth(&font, &empty, 77) == 0.0f &&
          style->getDataWidth(&font, 0, 77) == 0.0f && font.consumed(),
          "empty and null Data paths perform no metric calls");
}

void positioning()
{
    FixtureStyle object;
    Style* style = &object;
    const GlyphMetrics metrics{2.0f, 7.0f, 3.0f, 99.0f, 3.0f};
    Rect output{0.0f, 0.0f, 17.0f, 18.0f, 19.0f};
    float x = 1.25f, y = 2.25f;
    object.scale(1.0f, 99.0f);
    style->positionGlyph(&output, &metrics, &x, &y);
    check(output.x == 1.25f && output.y == 5.25f && x == 1.25f && y == 2.25f,
          "unscaled position adds offset and preserves both caller coordinates");
    check(output.width == 17 && output.height == 18 && output.z == 19,
          "positioning leaves non-position output fields untouched");

    object.scale(2.0f, 0.5f);
    style->positionGlyph(&output, &metrics, &x, &y);
    check(output.x == 1.5f && output.y == -0.5f && x == 1.5f && y == 2.25f,
          "scaled half rounds upward and subtracts offset while leaving input y untouched");
    x = -1.125f; y = -2.125f;
    style->positionGlyph(&output, &metrics, &x, &y);
    check(output.x == -1.5f && output.y == -5.5f && x == -1.5f && y == -2.125f,
          "negative fmod follows floor instead of symmetric nearest rounding");
    x = std::nextafter(1.25f, 0.0f); y = 2.25f;
    style->positionGlyph(&output, &metrics, &x, &y);
    check(output.x == 1.0f && x == 1.0f,
          "float immediately below positive half stays in lower branch");
    check(output.width == 17 && output.height == 18 && output.z == 19,
          "scaled path still leaves width height and depth unchanged");

    object.scale(0.4f, 2.5f);
    int integerX = 3, integerY = -3;
    style->positionGlyphInt(&output, &metrics, &integerX, &integerY);
    check(output.x == 2.5f && output.y == -8.0f && integerX == 2 && integerY == -3,
          "real integer wrapper truncates scaled x and preserves unmodified y");

    ForwardingStyle forwarding;
    Style* forwardingApi = &forwarding;
    integerX = 16777217; integerY = -17;
    forwardingApi->positionGlyphInt(&output, &metrics, &integerX, &integerY);
    check(forwarding.calls == 1 && forwarding.inputX == 16777216.0f && forwarding.inputY == -17.0f &&
          forwarding.seenOutput == &output && forwarding.seenMetrics == &metrics,
          "integer wrapper converts to float then calls actual virtual override once");
    check(integerX == -3 && integerY == 4 && output.x == 101.0f && output.y == 102.0f,
          "integer wrapper truncates both overridden float results toward zero");
}
}

int main()
{
    static_assert(sizeof(void*) == 4, "client fixture is x86");
    static_assert(sizeof(Style) == 24, "retail Style scalar layout");
    static_assert(sizeof(Data) == 8, "retail Data layout");
    widths();
    positioning();
    std::printf("Style metric/positioning: %d checks, %d failures; draw/prepare/registration uncalled\n",
                checks, failures);
    return failures ? 1 : 0;
}
