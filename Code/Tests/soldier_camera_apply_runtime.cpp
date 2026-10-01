// [TEMP] [CHOSEN, not recovered] constructor-free storage for the real camera-animation method.
// No Soldier lifecycle, skeleton evaluation, rendering or live camera acceptance is implied.
#include <cstdio>
#include <cstring>
#include <cmath>
#include <list>
#include <map>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../BF2/Game/Objects/Game/Soldier.h"
#include "../BF2/Game/Objects/Game/SoldierTemplate.h"
#undef protected
#undef private
#include "../BF2/Animation/BoneAnimation/Skeleton.h"

using namespace dice::hfe;
using namespace dice::hfe::world;
using dice::anim::Skeleton;
static_assert(sizeof(void*) == 4, "This fixture requires MSVC x86");
int cameraAnimationsFixture = 1;
namespace {
int checks = 0, failures = 0;
void check(bool result, const char* name)
{
    ++checks;
    if (!result) ++failures;
    std::printf("%s %s\n", result ? "PASS" : "FAIL", name);
}
bool same(const Mat4& a, const Mat4& b)
{
    for (int i = 0; i != 16; ++i)
        if (!(std::fabs(a.m[i] - b.m[i]) < 0.0001f)) return false;
    return true;
}
}

int main()
{
    alignas(Soldier) unsigned char soldierBytes[sizeof(Soldier)] = {};
    alignas(SoldierTemplate) unsigned char templateBytes[sizeof(SoldierTemplate)] = {};
    alignas(Skeleton) unsigned char skeletonBytes[sizeof(Skeleton)] = {};
    alignas(Skeleton::Bone) unsigned char boneBytes[sizeof(Skeleton::Bone)] = {};
    Soldier* const soldier = reinterpret_cast<Soldier*>(soldierBytes);
    SoldierTemplate* const soldierTemplate = reinterpret_cast<SoldierTemplate*>(templateBytes);
    Skeleton* const skeleton = reinterpret_cast<Skeleton*>(skeletonBytes);
    Skeleton::Bone* const bone = reinterpret_cast<Skeleton::Bone*>(boneBytes);
    const Mat4 sentinel = {{2,0,0,7, 0,3,0,8, 0,0,4,9, 40,50,60,10}};
    const Mat4 input = {{2,0,0,0, 0,3,0,0, 0,0,4,0, 10,20,30,1}};
    Mat4 output = sentinel;
    soldier->m_mat33C = sentinel;
    cameraAnimationsFixture = 0;
    check(!soldier->Soldier::applyCameraAnimation(input, output), "disabled animation declines before skeleton access");
    check(same(output, sentinel) && same(soldier->m_mat33C, sentinel), "disabled animation preserves output and cache");
    cameraAnimationsFixture = 1;
    soldier->m_characterMode = 1;
    check(!soldier->Soldier::applyCameraAnimation(input, output), "living third-person animation declines");
    check(same(output, sentinel) && same(soldier->m_mat33C, sentinel), "third-person decline preserves output and cache");

    soldier->m_characterMode = 0;
    soldier->m_soldierTemplate = soldierTemplate;
    soldier->m_animSkeleton = skeleton;
    skeleton->m_bones = bone;
    bone->m_transform = {{0,-1,0,0, 1,0,0,0, 0,0,1,0, 1,2,3,1}};
    soldierTemplate->m_bone0InverseTransform = Mat4::getIdentity();
    soldierTemplate->m_bone0InverseTransform.m[12] = 4;
    const Mat4 expected = {{0,-3,0,0, 2,0,0,0, 0,0,4,0, 20,26,42,1}};
    check(soldier->Soldier::applyCameraAnimation(input, output), "first-person animation accepts");
    check(same(output, expected), "root then bind inverse then input retain noncommuting order");
    check(same(soldier->m_mat33C, input), "first-person animation caches the input camera");
    check(same(soldierTemplate->m_bone0InverseTransform, Mat4{{1,0,0,0, 0,1,0,0, 0,0,1,0, 4,0,0,1}}),
          "template bind-pose inverse stays unchanged");

    soldier->m_characterMode = 1;
    soldier->m_ragDoll = reinterpret_cast<dice::anim::RagDoll*>(1);
    soldier->m_animSkeleton = 0;
    soldier->m_float318 = -2;
    soldier->m_float31C[0] = -3;
    soldier->m_float31C[1] = -4;
    soldier->m_float328[1] = 90;
    output = sentinel;
    const Mat4 ragdollZ = {{0,2,0,7, -3,0,0,8, 0,0,4,9, -2,-3,-4,10}};
    check(soldier->Soldier::applyCameraAnimation(input, output), "ragdoll accepts in third-person mode without skeleton");
    check(same(output, ragdollZ), "ragdoll negates stored Z angle and rotates the previous output");
    check(same(soldier->m_mat33C, input), "ragdoll leaves the cached first-person camera unchanged");
    soldier->m_float328[1] = 0;
    soldier->m_float328[0] = 90;
    output = sentinel;
    const Mat4 ragdollX = {{2,0,0,7, 0,0,3,8, 0,-4,0,9, -2,-3,-4,10}};
    soldier->Soldier::applyCameraAnimation(input, output);
    check(same(output, ragdollX), "ragdoll maps stored X angle and preserves fourth column");
    soldier->m_float328[0] = 0;
    soldier->m_float324 = 90;
    output = sentinel;
    const Mat4 ragdollY = {{0,0,-2,7, 0,3,0,8, 4,0,0,9, -2,-3,-4,10}};
    soldier->Soldier::applyCameraAnimation(input, output);
    check(same(output, ragdollY), "ragdoll maps stored Y angle and installs saved translation");
    cameraAnimationsFixture = 0;
    output = sentinel;
    check(!soldier->Soldier::applyCameraAnimation(input, output) && same(output, sentinel),
          "disabled cvar also rejects ragdoll without touching output");
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
