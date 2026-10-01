// [TEMP] [CHOSEN, not recovered] Camera inside-view regression. No game or network.
// Real Camera/base constructors and production camera/math bodies. Test-only parent and
// heightmap overrides model animation acceptance and two authored flat surfaces.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef near
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../BF2/Game/Objects/Base/Camera.h"
#include "../BF2/Game/Objects/Game/PlayerControlObject.h"
#include "../BF2/Game/Objects/PlayerControlObjectTemplate.h"
#include "../BF2/Physics/HeightmapCluster.h"
#include "../BF2/Core/Ids.h"

namespace dice { namespace hfe {
// The supporting engine archive is dedicated. Model only the client's runtime switch.
bool cameraTestDedicated = false;
bool isDedicated() { return cameraTestDedicated; }
namespace world { extern IBase* heightmapCluster; }
} }
using namespace dice::hfe;
using namespace dice::hfe::world;
static int checks, failures;
static void check(bool ok, const char* label)
{
    ++checks; if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label); std::fflush(stdout);
}
static bool near(float a, float b) { return std::fabs(a - b) < 0.00001f; }
static LONG WINAPI fault(EXCEPTION_POINTERS* e)
{
    std::fprintf(stderr, "FAIL exception %08lx at %p\n", e->ExceptionRecord->ExceptionCode,
                 e->ExceptionRecord->ExceptionAddress); std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

#define UNUSED(ret, name, args, qualifiers) ret name args qualifiers override { std::abort(); }
struct Surface : IHeightmapCluster
{
    mutable int groundCalls = 0, waterCalls = 0;
    mutable float lastX = 0, lastZ = 0;
    float ground = -100, water = -100;
    UNUSED(void, addRef, (), )
    UNUSED(int, getRef, (), )
    UNUSED(int, release, (), )
    UNUSED(void*, queryInterface, (unsigned int), const)
    UNUSED(void, init, (), ) UNUSED(void, cleanup, (), ) UNUSED(void, reset, (), )
    UNUSED(void, setClusterSize, (int), ) UNUSED(int, getClusterSize, (), const)
    UNUSED(void, setHeightmapSize, (int), ) UNUSED(int, getHeightmapSize, (), const)
    UNUSED(int, getWorldSize, (), const)
    UNUSED(void, addHeightmap, (const std::string&, int, int), )
    UNUSED(void, setActiveHeightmap, (IHeightmap*), )
    UNUSED(IHeightmap*, getActiveHeightmap, (), const)
    UNUSED(IHeightmap*, getPrimaryHeightmap, (), const)
    UNUSED(IHeightmap*, getHeightmapInSector, (int, int), const)
    UNUSED(bool, getSectorOfHeightmap, (const IHeightmap*, int&, int&), const)
    UNUSED(IHeightmap*, getHeightmapInWorldCoords, (float, float), const)
    UNUSED(void, getSectorOfWorldCoords, (float, float, int*, int*), const)
    UNUSED(const Vec3&, getPrimaryScale, (), const)
    UNUSED(const Vec3&, getSecondaryScale, (), const)
    UNUSED(void, setSeaWaterLevel, (float), ) UNUSED(float, getSeaWaterLevel, (), )
    UNUSED(int, getHeightFromGrid, (int, int), const)
    float getHeightInWorldCoords(float x, float z) const override
    { ++groundCalls; lastX = x; lastZ = z; return ground; }
    UNUSED(float, getHeightInWorldCoords, (const Vec3&), const)
    UNUSED(float, getWorldScaledHeightInWorldCoords, (float, float), const)
    UNUSED(float, getWorldScaledHeightInWorldCoords, (const Vec3&), const)
    UNUSED(void, getNormalInWorldCoords, (float, float, Vec3&), const)
    UNUSED(void, getNormalInWorldCoords, (const Vec3&, Vec3&), const)
    UNUSED(float, getHeightAndNormalInWorldCoords, (float, float, Vec3&, unsigned char&), const)
    UNUSED(bool, getTriangleAndNormalAndMaterialInWorldCoords,
           (float, float, Vec3&, Vec3&, Vec3&, Vec3&, unsigned char&), const)
    UNUSED(bool, getSquareInWorldCoords, (float, float, Vec3*, bool&), const)
    UNUSED(bool, intersectRayInWorldCoords,
           (const Vec3&, const Vec3&, Vec3&, float&, Vec3&, unsigned char&, bool), const)
    UNUSED(unsigned char, getMaterialInWorldCoords, (float, float), const)
    float getWaterLevelInWorldCoords(float x, float z) const override
    { ++waterCalls; check(x == lastX && z == lastZ, "water follows terrain at same x/z"); return water; }
    UNUSED(float, getWaterLevelInWorldCoords, (const Vec3&), const)
    UNUSED(bool, isInsideHeightmapCluster, (float, float), const)
    UNUSED(void, intersectCameraRay, (), )
    UNUSED(void, compileWaterMeshesIntoHeightmap, (), )
    UNUSED(void, setUsePhongNormalInterpolation, (bool), )
    UNUSED(bool, getUsePhongNormalInterpolation, (), const)
};
#undef UNUSED

struct Parent : PlayerControlObject
{
    explicit Parent(const PlayerControlObjectTemplate* t) : PlayerControlObject(t) {}
    mutable int queries = 0;
    int calls = 0;
    bool offers = true, accepts = true, identityOutput = false;
    Mat4 incoming = Mat4::getIdentity(), result = Mat4::getIdentity();
    void* queryInterface(unsigned int iid) const override
    {
        if (iid != IID_IPlayerControlObject) std::abort();
        ++queries;
        return offers ? static_cast<IPlayerControlObject*>(const_cast<Parent*>(this)) : 0;
    }
    bool applyCameraAnimation(const Mat4& in, Mat4& out) override
    {
        ++calls; incoming = in;
        identityOutput = std::memcmp(&out, &Mat4::getIdentity(), sizeof out) == 0;
        out = result;
        return accepts;
    }
};
struct CameraFixture : Camera
{
    explicit CameraFixture(const CameraTemplate* t) : Camera(t) {}
    mutable int absoluteCalls = 0;
    Mat4 absolute = Mat4::getIdentity();
    const Mat4& getAbsoluteTransformation() const override { ++absoluteCalls; return absolute; }
    void parent(IObject* object) { m_parent = object; }
};

int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(fault);
    static_assert(sizeof(void*) == 4, "x86 fixture only");
    // These uninitialised-to-game objects intentionally live until process exit; exercising
    // unrelated destroy/manager paths would change this bounded camera regression's scope.
    CameraTemplate* templ = new CameraTemplate;
    CameraFixture* camera = new CameraFixture(templ);
    Parent* parent = new Parent(new PlayerControlObjectTemplate);
    Surface surface;
    IBase* saved = heightmapCluster; heightmapCluster = &surface;
    camera->absolute.m[12] = 3; camera->absolute.m[13] = 7; camera->absolute.m[14] = 11;
    check(camera->m_explosionShakeRoll == 0 && camera->m_soldierShakeRoll == 0 &&
          camera->m_cameraRecoilY == 0 && camera->m_speedShakeRoll == 0, "real constructor clears four roll sources");
    const Mat4& none = camera->getTransformationCVMInside(1);
    check(&none == &camera->m_defaultTransformation && none.m[13] == 7, "no parent retains absolute view and member return");
    check(camera->absoluteCalls == 1 && surface.groundCalls == 1 && surface.waterCalls == 1,
          "one absolute fetch and both client surface calls");
    camera->parent(parent);
    parent->offers = false;
    camera->getTransformationCVMInside(0);
    check(parent->queries == 1 && parent->calls == 0, "parent without PCO is queried once");
    parent->offers = true; parent->accepts = false; parent->queries = 0;
    parent->result.m[12] = 20; parent->result.m[13] = 30; parent->result.m[14] = 40;
    const Mat4& refused = camera->getTransformationCVMInside(0);
    check(parent->queries == 2 && parent->calls == 1 && parent->identityOutput, "PCO is queried twice with identity output");
    check(refused.m[13] == 7 && parent->incoming.m[12] == 3, "refused animation output is ignored");
    parent->accepts = true;
    const Mat4& accepted = camera->getTransformationCVMInside(0);
    check(accepted.m[12] == 20 && accepted.m[13] == 30 && accepted.m[14] == 40,
          "accepted parent camera animation replaces absolute transform");
    check(surface.lastX == 20 && surface.lastZ == 40, "surface probes use animated x/z");
    const int queries = parent->queries, heights = surface.groundCalls;
    cameraTestDedicated = true;
    const Mat4& dedicated = camera->getTransformationCVMInside(0);
    check(dedicated.m[13] == 7 && parent->queries == queries && surface.groundCalls == heights,
          "runtime dedicated mode skips parent and surfaces");
    cameraTestDedicated = false;
    camera->parent(0);
    camera->m_explosionShakeRoll = 10; camera->m_soldierShakeRoll = 20;
    camera->m_cameraRecoilY = 25; camera->m_speedShakeRoll = 35;
    const Mat4& rolled = camera->getTransformationCVMInside(0);
    // The line basis uses forward cross up: for +Z it reflects X, so conjugating
    // the Z rotation yields +sin at m[1] and -sin at m[4].
    check(near(rolled.m[0], 0) && near(rolled.m[1], 1) && near(rolled.m[4], -1) && near(rolled.m[5], 0),
          "all four roll sources rotate about forward axis by ninety degrees");
    check(rolled.m[12] == 3 && rolled.m[13] == 7 && rolled.m[14] == 11 && rolled.m[15] == 1,
          "rotation preserves translation and fourth column");
    camera->m_explosionShakeRoll = camera->m_soldierShakeRoll = camera->m_cameraRecoilY = camera->m_speedShakeRoll = 0;
    surface.ground = 10;
    check(near(camera->getTransformationCVMInside(0).m[13], 10.09f), "terrain clamps upward with 0.09 clearance");
    surface.ground = -100; surface.water = 0;
    camera->absolute.m[13] = 0.024f;
    check(near(camera->getTransformationCVMInside(0).m[13], 0.09f), "near-surface above-water view moves to clearance");
    camera->absolute.m[13] = -0.024f;
    check(near(camera->getTransformationCVMInside(0).m[13], 0.09f), "near-surface below-water view moves to clearance");
    camera->absolute.m[13] = 0.025f;
    check(camera->getTransformationCVMInside(0).m[13] == 0.025f, "water threshold equality leaves camera unchanged");
    camera->absolute.m[13] = -1;
    check(camera->getTransformationCVMInside(0).m[13] == -1, "deep underwater view is not pushed above water");
    heightmapCluster = saved;
    std::printf("Camera inside: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
