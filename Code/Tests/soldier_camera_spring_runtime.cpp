// [TEMP] Arithmetic regression for actual CameraSpring::merge and extracted applySpringFunction.
// No Soldier construction, renderer, network or playable-client acceptance is implied.
#include "../BF2/Animation/BoneAnimation/CameraSpring.h"
#include "../BF2/Game/Utility/Vars.h"
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace dice { namespace hfe { namespace world {
void applySpringFunction(float, float&, float);
} } }
using dice::anim::CameraSpring;
using dice::hfe::Vec3;
using dice::hfe::world::applySpringFunction;
static_assert(sizeof(void*) == 4, "MSVC x86 required");
static_assert(sizeof(CameraSpring) == 80, "CameraSpring has twenty floats");
namespace {
float liveClamp = 0.001f;
int checks = 0, failures = 0;
void check(bool value, const char* label)
{
    ++checks;
    if (!value) ++failures;
    std::printf("%s %s\n", value ? "PASS" : "FAIL", label);
}
bool near(float a, float b) { return std::fabs(a - b) < 0.000001f; }
bool vectorIs(const Vec3& v, float x, float y, float z)
{ return v.x == x && v.y == y && v.z == z; }
}
// [TEMP] Only the cvar handle required by the extracted production helper is supplied.
float* dice::hfe::Vars::getFloat(const char* name, float value)
{
    if (std::strcmp(name, "anim-soldier-spring-clamp") != 0 || value != 0.001f)
        std::abort();
    return &liveClamp;
}
int main()
{
    CameraSpring current, empty;
    current.amplitude = {1,2,3}; current.zoomDamping = {4,5,6};
    current.stiffness = {7,8,9}; current.move = {10,11,12};
    current.hit = {13,14,15}; current.fire = {16,17,18}; current.look = {19,20};
    CameraSpring before = current;
    current.merge(&empty);
    check(std::memcmp(&before, &current, sizeof(current)) == 0, "zero override preserves all fields");
    CameraSpring overrideSpring;
    overrideSpring.amplitude = {0,2,0}; overrideSpring.zoomDamping = {0,3,0};
    overrideSpring.stiffness = {0,4,0}; overrideSpring.move = {0,5,0};
    overrideSpring.hit = {0,6,0}; overrideSpring.fire = {0,7,0}; overrideSpring.look = {0,8};
    current.merge(&overrideSpring);
    check(vectorIs(current.amplitude,0,2,0), "amplitude replaces full vector including zero axes");
    check(vectorIs(current.zoomDamping,0,3,0), "zoom damping replacement");
    check(vectorIs(current.stiffness,0,4,0), "stiffness replacement");
    check(vectorIs(current.move,0,5,0), "movement replacement");
    check(vectorIs(current.hit,0,6,0), "hit replacement");
    check(vectorIs(current.fire,0,7,0), "fire replacement");
    check(current.look.x == 0 && current.look.y == 8, "look replacement");
    overrideSpring = empty;
    overrideSpring.amplitude = {FLT_EPSILON, -FLT_EPSILON, FLT_EPSILON};
    overrideSpring.look = {FLT_EPSILON,0};
    current.merge(&overrideSpring);
    check(vectorIs(current.amplitude,0,2,0), "Vec3 equality includes epsilon boundary");
    check(current.look.x == 0 && current.look.y == 8, "look sum equal to epsilon preserves previous");
    overrideSpring.amplitude.x = FLT_EPSILON * 2;
    overrideSpring.look = {FLT_EPSILON * .75f, FLT_EPSILON * .75f};
    current.merge(&overrideSpring);
    check(current.amplitude.x == FLT_EPSILON * 2, "Vec3 above epsilon replaces");
    check(current.look.x == FLT_EPSILON * .75f, "look tests sum instead of separate axes");
    overrideSpring = empty;
    overrideSpring.amplitude.y = std::numeric_limits<float>::quiet_NaN();
    overrideSpring.look.x = std::numeric_limits<float>::quiet_NaN();
    current.merge(&overrideSpring);
    check(std::isnan(current.amplitude.y), "unordered Vec3 does not compare equal to zero");
    check(std::isnan(current.look.x), "unordered look sum replaces per retail branch");

    float x = .5f; applySpringFunction(10,x,.02f);
    check(near(x,.45f), "positive quadratic restoring step");
    x = -.5f; applySpringFunction(10,x,.02f);
    check(near(x,-.45f), "negative quadratic restoring step");
    x = .002f; applySpringFunction(0,x,0);
    check(near(x,.001f), "minimum restoring step still runs with zero dt and stiffness");
    x = .00105f; applySpringFunction(0,x,0);
    check(x == 0, "positive threshold snaps without sign inversion");
    x = -.00105f; applySpringFunction(0,x,0);
    check(x == 0, "negative threshold snaps without sign inversion");
    x = .5f; applySpringFunction(100,x,1);
    check(x == 0, "overshoot snaps to zero");
    x = 0; applySpringFunction(10,x,1);
    check(x == 0, "zero remains stationary");
    liveClamp = .05f; x = .1f; applySpringFunction(0,x,0);
    check(near(x,.05f), "restoring floor reads live cvar storage");
    x = std::numeric_limits<float>::quiet_NaN(); applySpringFunction(10,x,1);
    check(std::isnan(x), "unordered displacement remains unordered");
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
