// [TEMP] Strict x86 test of whole production Math/Vec3.cpp and its real header.
// No tested-body extraction, collaborators, chosen lifetimes or production hooks.
#include "../../BF2/Math/Vec3.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <float.h>
#include <string>
#include <type_traits>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
static_assert(sizeof(void*) == 4, "MSVC x86 only");
static_assert(std::is_same<decltype(&getBoxMaxCorner), Vec2&(*)(const Vec2&, const Vec2&, Vec2&)>::value,
              "two const inputs and borrowed output reference");
static unsigned checks;
[[noreturn]] static void fail(const char* message)
{
    std::fprintf(stderr, "FAIL %s\n", message); std::fflush(stderr); std::abort();
}
static void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) fail(message);
}
static float fromBits(unsigned value) { float out; std::memcpy(&out, &value, sizeof(out)); return out; }
static unsigned bits(float value) { unsigned out; std::memcpy(&out, &value, sizeof(out)); return out; }
static bool same(const Vec2& first, const Vec2& second) { return std::memcmp(&first, &second, sizeof(first)) == 0; }

// [TEMP] Independent one-component oracle transcribed from the paired PE sequence:
// fld centre; fsub minimum; fadd centre; fstp output. No C++ arithmetic oracle.
__declspec(noinline) static float x87Component(float minimum, float centre)
{
    float output;
    __asm {
        fld centre
        fsub minimum
        fadd centre
        fstp output
    }
    return output;
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (argc != 2) fail("mode required");
    const std::string mode(argv[1]);
    unsigned oldControl;
    if (_controlfp_s(&oldControl, 0, 0) != 0) fail("read floating control");
    unsigned control;
    if (_controlfp_s(&control, _PC_64 | _RC_NEAR, _MCW_PC | _MCW_RC) != 0) fail("set wide x87 round-nearest");

    if (mode == "basic")
    {
        const Vec2 minimum = {-3.0f, 19.0f}, centre = {7.0f, -11.0f};
        struct Guarded { unsigned before; Vec2 output; unsigned after; } value = {0x12345678, {}, 0x87654321};
        require(&getBoxMaxCorner(minimum, centre, value.output) == &value.output, "returns exact supplied output address");
        require(value.output.x == 17.0f && value.output.y == -41.0f, "component formula and input order");
        require(value.before == 0x12345678 && value.after == 0x87654321, "only two output floats written");
        require(minimum.x == -3.0f && minimum.y == 19.0f && centre.x == 7.0f && centre.y == -11.0f,
                "separate inputs unchanged");
    }
    else if (mode == "aliases")
    {
        const Vec2 originalMinimum = {-7.0f, 3.0f}, originalCentre = {11.0f, -5.0f};
        const Vec2 expected = {29.0f, -13.0f};
        Vec2 minimum = originalMinimum, centre = originalCentre;
        require(&getBoxMaxCorner(minimum, centre, minimum) == &minimum, "output aliases minimum return identity");
        require(same(minimum, expected) && same(centre, originalCentre), "minimum alias result and other input");
        minimum = originalMinimum; centre = originalCentre;
        require(&getBoxMaxCorner(minimum, centre, centre) == &centre, "output aliases centre return identity");
        require(same(centre, expected) && same(minimum, originalMinimum), "centre alias result and other input");
        Vec2 all = originalCentre;
        require(&getBoxMaxCorner(all, all, all) == &all && same(all, originalCentre), "all arguments alias safely");
        Vec2 output = {};
        require(&getBoxMaxCorner(originalCentre, originalCentre, output) == &output && same(output, originalCentre),
                "identical inputs and separate output");
    }
    else if (mode == "rounding")
    {
        // 2^-24 + 2^-47: the wide result is just below the midpoint below 2.
        const Vec2 minimum = {fromBits(0x33800001), -fromBits(0x33800001)};
        const Vec2 centre = {1.0f, -1.0f};
        Vec2 output;
        getBoxMaxCorner(minimum, centre, output);
        require(bits(output.x) == 0x3FFFFFFF && bits(output.y) == 0xBFFFFFFF,
                "subtraction is not narrowed before addition");
        volatile float roundedDelta = centre.x - minimum.x;
        const float prematurelyRounded = roundedDelta + centre.x;
        require(bits(prematurelyRounded) == 0x40000000 && bits(output.x) != bits(prematurelyRounded),
                "fixture distinguishes early float materialization");
        require(bits(output.x) == bits(x87Component(minimum.x, centre.x)) &&
                bits(output.y) == bits(x87Component(minimum.y, centre.y)), "paired x87 oracle agrees");
    }
    else if (mode == "extremes")
    {
        Vec2 minimum = {FLT_MAX, -FLT_MAX}, centre = minimum, output;
        getBoxMaxCorner(minimum, centre, output);
        require(same(output, centre), "same maximal input does not overflow through doubled-centre rewrite");
        minimum = {-FLT_MAX, FLT_MAX};
        getBoxMaxCorner(minimum, centre, output);
        require(bits(output.x) == 0x7F800000 && bits(output.y) == 0xFF800000, "final stores overflow with correct signs");
        minimum = {fromBits(1), fromBits(3)}; centre = {fromBits(2), fromBits(1)};
        getBoxMaxCorner(minimum, centre, output);
        require(bits(output.x) == 3 && bits(output.y) == 0x80000001, "subnormal finite arithmetic survives");
        minimum = {0.0f, -0.0f}; centre = {-0.0f, 0.0f};
        getBoxMaxCorner(minimum, centre, output);
        require(bits(output.x) == 0x80000000 && bits(output.y) == 0, "signed zeros follow subtraction then addition");
    }
    else if (mode == "nonfinite")
    {
        Vec2 minimum = {fromBits(0x7F800000), fromBits(0xFF800000)};
        Vec2 centre = {3.0f, -4.0f}, output;
        getBoxMaxCorner(minimum, centre, output);
        require(bits(output.x) == 0xFF800000 && bits(output.y) == 0x7F800000, "infinite minimum signs");
        minimum = {3.0f, -4.0f}; centre = {fromBits(0x7F800000), fromBits(0xFF800000)};
        getBoxMaxCorner(minimum, centre, output);
        require(bits(output.x) == 0x7F800000 && bits(output.y) == 0xFF800000, "infinite centre signs");
        minimum = centre;
        getBoxMaxCorner(minimum, centre, output);
        require(std::isnan(output.x) && std::isnan(output.y), "same infinity subtraction remains NaN");
        minimum = {fromBits(0x7FC12345), 1.0f}; centre = {1.0f, fromBits(0xFFC45678)};
        getBoxMaxCorner(minimum, centre, output);
        require(std::isnan(output.x) && std::isnan(output.y), "quiet NaN inputs propagate without guard");
    }
    else if (mode == "x87_matrix")
    {
        const unsigned values[] = {0,0x80000000,1,0x80000001,0x007FFFFF,0x00800000,
            0x33800001,0xB3800001,0x3F7FFFFF,0x3F800000,0x3F800001,0xBF800001,
            0x4B000001,0xCB000001,0x6F123456,0xEF654321,0x7F7FFFFF,0xFF7FFFFF};
        for (unsigned minimumBits : values)
        {
            for (unsigned centreBits : values)
            {
                const Vec2 minimum = {fromBits(minimumBits), fromBits(centreBits)};
                const Vec2 centre = {fromBits(centreBits), fromBits(minimumBits)};
                Vec2 output;
                getBoxMaxCorner(minimum, centre, output);
                require(bits(output.x) == bits(x87Component(minimum.x, centre.x)), "x87 finite matrix X agreement");
                require(bits(output.y) == bits(x87Component(minimum.y, centre.y)), "x87 finite matrix Y agreement");
            }
        }
    }
    else fail("unknown mode");
    if (_controlfp_s(&control, oldControl, _MCW_PC | _MCW_RC) != 0) fail("restore floating control");
    std::printf("PASS %s %u checks\n", mode.c_str(), checks);
    return 0;
}
