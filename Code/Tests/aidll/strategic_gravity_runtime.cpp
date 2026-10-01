// [TEMP] Chosen native lifetimes and typed virtual observers around exact production methods.
// No production constructor/destructor, retail STL layout or full AI update is claimed here.
#include "strategic_gravity_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>

__declspec(noreturn) static void unexpected(const char* method)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", method);
    std::fflush(stderr);
    std::abort();
}

#include "strategic_gravity_mocks.inl"

namespace dice { namespace hfe { namespace ai {
AIStrategicObject::AIStrategicObject(float, float)
{
    m_position[0] = { 0.0f, 0.0f };
    m_position[1] = { 0.0f, 0.0f };
    m_size[0] = 1234.0f;
    m_size[1] = 25.0f;
    m_size[2] = 50.0f;
}
AIStrategicObject::~AIStrategicObject() {}
AIStrategicArea::AIStrategicArea(const std::string& name, unsigned int, float radius)
    : AIStrategicObject(radius, 0.0f), m_name(name) {}
AIStrategicArea::~AIStrategicArea() {}
} } }

using namespace dice::hfe;
using namespace dice::hfe::ai;
static int checks;
static void check(bool condition, const char* expression)
{
    ++checks;
    if (!condition) {
        std::fprintf(stderr, "FAIL %s\n", expression);
        std::exit(2);
    }
}
static unsigned int bits(float value)
{
    unsigned int result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
static float fromBits(unsigned int value)
{
    float result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
static void equal(float actual, float expected, const char* label)
{
    check(bits(actual) == bits(expected), label);
}

class Observer : public AIStrategicArea
{
public:
    Observer() : AIStrategicArea("observer", 0, 1.0f) {}
    using AIStrategicArea::gravity;
    bool returnAtVec2 = false;
    bool returnAtScalar = false;
    bool changePosition = false;
    bool changeSize = false;
    Vec2 observedPosition = { 0.0f, 0.0f };
    float observedDistance = 0.0f;
    int observedSide = 0;
    std::vector<int> sequence;
    float gravity(const Vec2& position, int side) override
    {
        sequence.push_back(2);
        observedPosition = position;
        observedSide = side;
        if (returnAtVec2) return 91.0f;
        if (changePosition) m_position[side - 1] = { 6.0f, 8.0f };
        return AIStrategicArea::gravity(position, side);
    }
    float gravity(float distance, int side) override
    {
        sequence.push_back(1);
        observedDistance = distance;
        observedSide = side;
        if (returnAtScalar) return 92.0f;
        if (changeSize) m_size[side] = 100.0f;
        return AIStrategicArea::gravity(distance, side);
    }
};

int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "Fixture requires MSVC x86");
    if (argc != 2) return 3;
    const char* mode = argv[1];
    const float infinity = std::numeric_limits<float>::infinity();
    const float nan = fromBits(0x7fc12345u);
    AIStrategicArea area("native", 0, 1.0f);

    if (!std::strcmp(mode, "delegation")) {
        Observer observer;
        observer.returnAtVec2 = true;
        AIStrategicObject& view = observer;
        Vec3 point = { -7.0f, nan, 11.0f };
        equal(view.gravity(point, 2), 91.0f, "Vec3 preserves virtual Vec2 return");
        check(observer.sequence == std::vector<int>{2}, "Vec3 dispatches only Vec2 override");
        equal(observer.observedPosition.x, -7.0f, "flatten X");
        equal(observer.observedPosition.y, 11.0f, "flatten Z ignores NaN height");
        check(observer.observedSide == 2, "delegates side unchanged");
        observer.sequence.clear();
        observer.returnAtScalar = true;
        Vec2 flat = { 3.0f, 4.0f };
        equal(observer.AIStrategicArea::gravity(flat, 1), 92.0f, "Vec2 preserves virtual scalar return");
        check(observer.sequence == std::vector<int>{1}, "Vec2 calls scalar exactly once");
        equal(observer.observedDistance, 5.0f, "scalar receives distance");
        check(observer.observedSide == 1, "scalar receives side");
    } else if (!std::strcmp(mode, "chain")) {
        Observer observer;
        observer.changePosition = true;
        observer.changeSize = true;
        AIStrategicObject& view = observer;
        Vec3 point = { 9.0f, -9999.0f, 12.0f };
        equal(view.gravity(point, 2), 4.0f, "nested chain uses fresh position and size");
        check(observer.sequence == std::vector<int>({2, 1}), "Vec2 then scalar callback order");
        equal(observer.observedDistance, 5.0f, "fresh side 2 position before distance");
        equal(observer.m_size[1], 25.0f, "other side size unchanged");
        equal(observer.m_size[2], 100.0f, "scalar callback updates selected size");
    } else if (!std::strcmp(mode, "thresholds")) {
        const float inputs[] = { -infinity, -9.0f, -0.0f, 0.0f, fromBits(0x3f7fffffu), 1.0f };
        for (float input : inputs) equal(area.gravity(input, 1), 25.0f, "ordered <=1 is unchanged size");
        equal(area.gravity(2.0f, 1), 6.25f, "distance square divides size");
        area.m_size[1] = 1.0f;
        // Exact next-float threshold oracle; 1/(1+2^-23)^2 rounds to 1-2^-22.
        equal(area.gravity(fromBits(0x3f800001u), 1), fromBits(0x3f7ffffcu), "next float above threshold divides");
        area.m_size[1] = -0.0f;
        equal(area.gravity(1.0f, 1), -0.0f, "threshold preserves negative zero size");
        equal(area.gravity(2.0f, 1), -0.0f, "division preserves negative zero size");
    } else if (!std::strcmp(mode, "side_index")) {
        area.m_position[0] = { 1.0f, 2.0f };
        area.m_position[1] = { 4.0f, 6.0f };
        Vec2 point = { 4.0f, 6.0f };
        equal(area.gravity(point, 1), 1.0f, "side 1 position index zero and size index one");
        equal(area.gravity(point, 2), 50.0f, "side 2 position index one and size index two");
        area.m_size[2] = -37.0f;
        equal(area.gravity(point, 2), -37.0f, "fresh size read");
        area.m_position[1] = { 1.0f, 2.0f };
        equal(area.gravity(point, 2), -1.48f, "fresh position read and negative size retained");
        equal(area.m_size[0], 1234.0f, "radius slot is separate");
    } else if (!std::strcmp(mode, "wide")) {
        Observer observer;
        observer.returnAtScalar = true;
        const float big = fromBits(0x67800000u); // 2^80: square exceeds float range.
        const float small = fromBits(0x17800000u); // 2^-80: square underflows float.
        Vec2 point = { big, 0.0f };
        equal(observer.AIStrategicArea::gravity(point, 1), 92.0f, "large vector delegates");
        equal(observer.observedDistance, big, "square did not prematurely overflow float");
        point.x = small;
        observer.AIStrategicArea::gravity(point, 1);
        equal(observer.observedDistance, small, "square did not prematurely underflow float");
        area.m_size[1] = big;
        equal(area.gravity(big, 1), small, "scalar denominator stays wide");
        point.x = std::numeric_limits<float>::max();
        point.y = point.x;
        observer.AIStrategicArea::gravity(point, 1);
        check(std::isinf(observer.observedDistance), "distance is narrowed to float at virtual boundary");
    } else if (!std::strcmp(mode, "nonfinite")) {
        check(std::isnan(area.gravity(nan, 1)), "NaN distance takes division");
        equal(area.gravity(infinity, 1), 0.0f, "positive infinity divides to zero");
        equal(area.gravity(-infinity, 2), 50.0f, "negative infinity takes threshold arm");
        area.m_size[1] = nan;
        check(std::isnan(area.gravity(0.0f, 1)), "NaN size at threshold");
        Observer observer;
        observer.returnAtScalar = true;
        Vec2 point = { nan, 1.0f };
        equal(observer.AIStrategicArea::gravity(point, 2), 92.0f, "NaN vector still dispatches scalar");
        check(std::isnan(observer.observedDistance), "NaN vector reaches scalar as NaN");
        check(observer.sequence == std::vector<int>{1}, "NaN scalar called once");
    } else return 3;
    std::printf("PASS %s checks=%d\n", mode, checks);
    return 0;
}
