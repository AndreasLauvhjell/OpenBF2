// [TEMP] [CHOSEN, not recovered] x86 call-boundary fixture, never a production image.
// Executes the real AnimatedBundle::handleVisualUpdate. Constructor-free storage models only
// its already-created receiver/skeleton; no construction, animation evaluation or rendering
// acceptance is implied. Skeleton::transform and geometry callbacks below are observers.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <string>
#include <vector>
#define protected public
#include "../BF2/Game/Objects/Game/AnimatedBundle.h"
#undef protected
#include "../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../BF2/Animation/BoneAnimation/AnimationSystem.h"
#include "../BF2/Animation/BoneAnimation/Skeleton.h"

using namespace dice::hfe;
using namespace dice::hfe::world;
using namespace dice::anim;
static_assert(sizeof(void*) == 4, "This regression requires MSVC x86");

namespace {
int checks = 0;
int failures = 0;
char calls[16] = {};
unsigned int callCount = 0;
void note(char c) { calls[callCount++] = c; calls[callCount] = 0; }
void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
}
[[noreturn]] void unexpected()
{
    std::fprintf(stderr, "FAIL unexpected animation fixture call\n");
    std::exit(87);
}

struct GeometrySpy { void** vptr; unsigned int deliveries; };
Skeleton* expectedSkeleton;
Mat4* expectedPalette;
GeometrySpy* expectedGeometry;
void __fastcall acceptPose(void* receiver, void*, void* palette, int count)
{
    note('P');
    GeometrySpy* spy = static_cast<GeometrySpy*>(receiver);
    ++spy->deliveries;
    check(spy == expectedGeometry, "pose keeps geometry captured before callbacks");
    check(palette == expectedPalette && count == 3, "PE +0x54 receives palette and bone count");
    check(expectedPalette->m[0] == 17.0f, "pose is delivered after skeleton transform");
}
void __fastcall rejectGather(void*, void*, void*)
{
    // PE +0x58 takes one vector pointer. Stop before the old two-argument call can corrupt it.
    std::fprintf(stderr, "FAIL pose reached PE +0x58 gatherVisibleParts instead of +0x54\n");
    std::fflush(stderr);
    std::exit(86);
}

class AnimationSpy : public IAnimationSystem
{
public:
    IObject* expectedObject = 0;
    GeometrySpy* replacement = 0;
    AnimationSystemTemplate* getTemplate() override { unexpected(); }
    void reset() override { unexpected(); }
    void update(IObject* object, float dt) override
    {
        note('U');
        check(object == expectedObject && dt == 0.125f, "animation receives owner and frame delta");
        if (replacement) object_detail::LayoutAccess::geometry(object) = replacement;
    }
    void applyOnSkeleton(Skeleton* skeleton, bool postTrigger) override
    {
        note('A');
        check(skeleton == expectedSkeleton && !postTrigger, "animation applies before transform");
    }
    void disable(bool) override { unexpected(); }
    bool playTrigger(const std::string&, bool) override { unexpected(); }
    bool playPostTrigger(const std::string&, bool) override { unexpected(); }
    void setActiveTrigger(const std::string&, bool) override { unexpected(); }
    void copyStateToSystem(IAnimationSystem*, bool) override { unexpected(); }
    void setTriggerPose(unsigned int) override { unexpected(); }
    void setTriggerMessage(unsigned int) override { unexpected(); }
    void setTriggerOnceMessage(unsigned int) override { unexpected(); }
    void clearTriggerMessage(unsigned int) override { unexpected(); }
    void setTriggerMovement(float, const Vec3&, float, const Vec3&) override { unexpected(); }
    void getPlayingAnimationLength(float&, float&, bool) override { unexpected(); }
    void syncMovementAnimations(const IAnimationSystem*) override { unexpected(); }
    void setForceTime(float) override { unexpected(); }
    float getForceTime() override { unexpected(); }
};
}

// [TEMP] Observer for the sole direct dependency of the exercised production method.
void Skeleton::transform(const Mat4* parent, int maxDepth)
{
    note('T');
    check(this == expectedSkeleton && !parent && maxDepth == -1,
          "skeleton transform receives retail null-parent/unlimited-depth arguments");
    m_transforms->m[0] = 17.0f;
}

int main()
{
    alignas(AnimatedBundle) unsigned char ownerBytes[sizeof(AnimatedBundle)] = {};
    alignas(Skeleton) unsigned char skeletonBytes[sizeof(Skeleton)] = {};
    alignas(Mat4) unsigned char paletteBytes[3 * sizeof(Mat4)] = {};
    AnimatedBundle* owner = reinterpret_cast<AnimatedBundle*>(ownerBytes);
    expectedSkeleton = reinterpret_cast<Skeleton*>(skeletonBytes);
    expectedPalette = reinterpret_cast<Mat4*>(paletteBytes);
    expectedSkeleton->m_transforms = expectedPalette;
    expectedSkeleton->m_boneCount = 3;
    void* geometryTable[0x5C / 4] = {};
    geometryTable[0x54 / 4] = reinterpret_cast<void*>(&acceptPose);
    geometryTable[0x58 / 4] = reinterpret_cast<void*>(&rejectGather);
    GeometrySpy original = {geometryTable, 0};
    GeometrySpy replacement = {geometryTable, 0};
    expectedGeometry = &original;
    AnimationSpy animation;
    animation.expectedObject = owner;

    owner->m_skeleton = 0;
    object_detail::LayoutAccess::geometry(owner) = &original;
    owner->AnimatedBundle::handleVisualUpdate(0.125f, 9.0f);
    check(callCount == 0, "null skeleton returns without animation or geometry calls");
    owner->m_skeleton = expectedSkeleton;
    object_detail::LayoutAccess::geometry(owner) = 0;
    owner->AnimatedBundle::handleVisualUpdate(0.125f, 9.0f);
    check(callCount == 0, "null geometry returns without animation or transform calls");

    object_detail::LayoutAccess::geometry(owner) = &original;
    owner->AnimatedBundle::handleVisualUpdate(0.125f, 9.0f);
    check(std::strcmp(calls, "TP") == 0, "no animation system still transforms and delivers pose");

    callCount = 0; calls[0] = 0;
    expectedPalette->m[0] = 0;
    animation.replacement = &replacement;
    owner->m_animationSystems[0] = &animation;
    owner->AnimatedBundle::handleVisualUpdate(0.125f, 9.0f);
    check(std::strcmp(calls, "UATP") == 0, "animation, apply, transform, pose order preserved");
    check(original.deliveries == 2 && replacement.deliveries == 0,
          "callback geometry replacement does not redirect this frame's pose");
    check(object_detail::LayoutAccess::geometry(owner) == &replacement,
          "callback geometry replacement itself remains observable");
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
