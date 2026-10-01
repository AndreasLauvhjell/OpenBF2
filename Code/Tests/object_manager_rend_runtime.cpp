// [TEMP] [CHOSEN, not recovered] focused observer fixture for the recovered ObjectManager render
// query family.  It uses the production ObjectManager bodies with culler/object collaborators that
// record only the tested contract.  This is not a boot, renderer, map-load, or client acceptance.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <cmath>
#include <cstdio>
#include <vector>

#define private public
#define protected public
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Game/Objects/Object.h"
#undef protected
#undef private

#include "../BF2/Core/ClassManager.h"
#include "../BF2/Math/Frustum.h"
#include "../BF2/Scene/Object/ICuller.h"
#include "../BF2/Scene/Object/ObjectPredicator.h"

using namespace dice::hfe;
using namespace dice::hfe::world;

namespace dice { namespace hfe { extern ClassManager* classManager; } }

namespace {

int g_checks;
int g_failures;

void check(bool condition, const char* name)
{
    ++g_checks;
    if (!condition)
        ++g_failures;
    std::printf("%s %s\n", condition ? "PASS" : "FAIL", name);
}

bool nearValue(float a, float b)
{
    return std::fabs(a - b) < 0.0001f;
}

LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, 0, TRUE);
    std::fprintf(stderr, "Fixture exception %08lx at %p\n",
                 fault->ExceptionRecord->ExceptionCode,
                 fault->ExceptionRecord->ExceptionAddress);
    CONTEXT context = *fault->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip;
    frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i != 16; ++i)
    {
        char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
            std::fprintf(stderr, "  %08llx %s+%llu\n", frame.AddrPC.Offset,
                         symbol->Name, displacement);
        IMAGEHLP_LINE64 line = {};
        line.SizeOfStruct = sizeof(line);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line))
            std::fprintf(stderr, "             %s:%lu\n", line.FileName, line.LineNumber);
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &frame, &context,
                         0, SymFunctionTableAccess64, SymGetModuleBase64, 0))
            break;
    }
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}

struct TestCuller : ICuller
{
    int refs = 1;
    int sphereCalls = 0;
    int frustumCalls = 0;
    std::vector<IObject*> sphereObjects;
    std::vector<IObject*> frustumVisibleObjects;
    std::vector<IObject*> frustumAllVisibleObjects;

    void addRef() override { ++refs; }
    int getRef() override { return refs; }
    int release() override { --refs; return 0; }
    void* queryInterface(unsigned int) const override
    {
        return const_cast<TestCuller*>(this);
    }
    unsigned int getClassID() const override { return 0; }
    CullerObject* createObject(const Vec3&, float, IObject*) override { return 0; }
    int getNumberOfObjects() override { return 0; }
    void initQuadTree(const Vec3&, float, float, float) override {}
    void destroyObject(CullerObject*) override {}
    CullerObject* updateObject(CullerObject*, const Vec3&, float) override { return 0; }
    void optimize() override {}
    void clear() override {}
    void getAllObjects(std::vector<IObject*>&) override {}
    void getObjects(const Frustum&, std::vector<IObject*>& visible,
                    std::vector<IObject*>& allVisible, ObjectPredicator*) override
    {
        ++frustumCalls;
        visible.insert(visible.end(), frustumVisibleObjects.begin(), frustumVisibleObjects.end());
        allVisible.insert(allVisible.end(), frustumAllVisibleObjects.begin(),
                          frustumAllVisibleObjects.end());
    }
    void getObjects(const Vec3&, const Vec3&, std::vector<IObject*>&,
                    ObjectPredicator*) override {}
    void getObjects(const Vec3&, float, std::vector<IObject*>& out,
                    ObjectPredicator*) override
    {
        ++sphereCalls;
        out.insert(out.end(), sphereObjects.begin(), sphereObjects.end());
    }
    void getObjects(const Vec3&, const Vec3&,
                    std::vector<std::pair<float, IObject*> >&) override {}
    void clearGetObjects() override {}
    void appendGetObjects(const Vec3&, float, std::vector<IObject*>&,
                          ObjectPredicator*) override {}
};

struct TestClasses : ClassManager
{
    std::vector<TestCuller*> created;

    IBase* createInstance(unsigned int, unsigned int, IBase*) override
    {
        TestCuller* culler = new TestCuller;
        created.push_back(culler);
        return culler;
    }
};

struct Probe : Object
{
    explicit Probe(int identity)
        : Object(0), id(identity), visualRadius(1.0f)
    {
        position.x = position.y = position.z = 0.0f;
        transformation = Mat4::getIdentity();
    }

    int id;
    float visualRadius;
    Vec3 position;
    Mat4 transformation;

    const Vec3& getAbsolutePosition() const override { return position; }
    const Mat4& getAbsoluteTransformation() const override { return transformation; }
    float getVisualBoundingRadius() const override { return visualRadius; }

    void setQueryFlags(unsigned int flags) { m_flags = flags; }
    void setGeometryPresent(bool present)
    {
        m_geometry = present ? reinterpret_cast<void*>(static_cast<uintptr_t>(1)) : 0;
    }
    void setChildLink(IObject* child) { m_child = child; }
    void setSiblingLink(IObject* sibling) { m_sibling = sibling; }
};

struct Predicate : ObjectPredicator
{
    std::vector<IObject*> accepted;
    std::vector<IObject*> visited;

    bool predicate(IObject* object) override
    {
        visited.push_back(object);
        for (std::vector<IObject*>::const_iterator it = accepted.begin(); it != accepted.end(); ++it)
            if (*it == object)
                return true;
        return false;
    }
    unsigned int getFlags() const override { return 0; }
    bool performCulling(IObject*) override { return true; }
};

bool sequence(const std::vector<IObject*>& actual,
              IObject* a, IObject* b = 0, IObject* c = 0, IObject* d = 0)
{
    IObject* expected[4] = { a, b, c, d };
    std::size_t count = 1;
    while (count != 4 && expected[count])
        ++count;
    if (actual.size() != count)
        return false;
    for (std::size_t i = 0; i != count; ++i)
        if (actual[i] != expected[i])
            return false;
    return true;
}

} // anonymous namespace

int main()
{
    static_assert(sizeof(void*) == 4, "ObjectManager retail ABI is x86");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    SetUnhandledExceptionFilter(reportFault);

    TestClasses* classes = new TestClasses;
    dice::hfe::classManager = classes;
    ObjectManager* manager = new ObjectManager;

    TestCuller* const dynamicCollision = static_cast<TestCuller*>(manager->m_quadTreeCuller);
    TestCuller* const staticCollision = static_cast<TestCuller*>(manager->m_gridCuller);
    TestCuller* const staticNoCollision =
        static_cast<TestCuller*>(manager->m_noCollisionGridCuller);
    TestCuller* const dynamicNoCollision =
        static_cast<TestCuller*>(manager->m_noCollisionQuadTreeCuller);

    Probe* a = new Probe(1);
    Probe* b = new Probe(2);
    Probe* c = new Probe(3);
    Probe* d = new Probe(4);
    Probe* e = new Probe(5);
    const Vec3 centre = { 0.0f, 0.0f, 0.0f };
    Frustum frustum;

    // Validate the actual MSVC ObjectManager slots, not merely overload resolution at a source
    // call site: PE +0x78 sphere-static, +0x7C Frustum-static, +0x80 sphere-dynamic,
    // +0x84 Frustum-dynamic.
    typedef void (__thiscall *SphereQuery)(ObjectManager*, const Vec3&, float,
        std::vector<IObject*>&, LocalBoundingBox*, ObjectPredicator*, unsigned int, bool);
    typedef void (__thiscall *FrustumQuery)(ObjectManager*, const Frustum&,
        std::vector<IObject*>&, LocalBoundingBox*, ObjectPredicator*, unsigned int, bool);
    void** const table = *reinterpret_cast<void***>(manager);
    std::vector<IObject*> out;

    staticCollision->sphereObjects.push_back(a);
    staticNoCollision->sphereObjects.push_back(b);
    reinterpret_cast<SphereQuery>(table[0x78 / sizeof(void*)])(
        manager, centre, 10.0f, out, 0, 0, 7, false);
    check(sequence(out, a, b), "PE +0x78 dispatches static sphere and preserves culler order");
    check(staticCollision->sphereCalls == 1 && staticNoCollision->sphereCalls == 1
              && dynamicCollision->sphereCalls == 0 && dynamicNoCollision->sphereCalls == 0,
          "static sphere consults only the two static cullers");

    out.clear();
    staticCollision->frustumVisibleObjects.push_back(a);
    staticCollision->frustumAllVisibleObjects.push_back(c);
    staticNoCollision->frustumVisibleObjects.push_back(b);
    staticNoCollision->frustumAllVisibleObjects.push_back(d);
    reinterpret_cast<FrustumQuery>(table[0x7C / sizeof(void*)])(
        manager, frustum, out, 0, 0, 0, false);
    check(sequence(out, a, b, c, d),
          "PE +0x7C dispatches static Frustum and appends wholly-visible mode-0 objects");

    out.clear();
    dynamicCollision->sphereObjects.push_back(c);
    dynamicNoCollision->sphereObjects.push_back(d);
    reinterpret_cast<SphereQuery>(table[0x80 / sizeof(void*)])(
        manager, centre, 10.0f, out, 0, 0, 1, false);
    check(sequence(out, c, d), "PE +0x80 dispatches dynamic sphere");
    check(dynamicCollision->sphereCalls == 1 && dynamicNoCollision->sphereCalls == 1,
          "dynamic sphere consults both dynamic cullers");

    out.clear();
    dynamicCollision->frustumVisibleObjects.push_back(c);
    dynamicNoCollision->frustumVisibleObjects.push_back(d);
    reinterpret_cast<FrustumQuery>(table[0x84 / sizeof(void*)])(
        manager, frustum, out, 0, 0, 0, false);
    check(sequence(out, c, d), "PE +0x84 dispatches dynamic Frustum");

    // Sphere visual-bound arm: starts with first position +/-0.1, then expands each qualifying
    // object's sphere by visualRadius * sqrt(2).  Existing output is intentionally retained.
    staticCollision->sphereObjects.clear();
    staticNoCollision->sphereObjects.clear();
    staticCollision->sphereObjects.push_back(a);
    staticNoCollision->sphereObjects.push_back(b);
    a->position = Vec3{ 1.0f, 2.0f, 3.0f };
    a->visualRadius = 2.0f;
    a->setQueryFlags(0x04000000u);
    b->position = Vec3{ -4.0f, 5.0f, 6.0f };
    b->visualRadius = 1.0f;
    b->setQueryFlags(0x04000000u);
    LocalBoundingBox bounds;
    out.clear();
    out.push_back(e);
    manager->getStaticObjectsFromGridRend(centre, 10.0f, out, &bounds, 0, 9, false);
    const float rootTwo = static_cast<float>(std::sqrt(2.0));
    check(sequence(out, e, a, b), "sphere render query appends without clearing caller output");
    check(nearValue(bounds.m_min.x, -4.0f - rootTwo)
              && nearValue(bounds.m_min.y, 2.0f - 2.0f * rootTwo)
              && nearValue(bounds.m_min.z, 3.0f - 2.0f * rootTwo)
              && nearValue(bounds.m_max.x, 1.0f + 2.0f * rootTwo)
              && nearValue(bounds.m_max.y, 5.0f + rootTwo)
              && nearValue(bounds.m_max.z, 6.0f + rootTwo),
          "visual bounds use shipped radius-times-sqrt-two expansion");

    // Single-child traversal is depth-first. Disabled/invisible nodes suppress their whole child
    // subtree, while a predicate-rejected eligible node still contributes its descendants.
    Probe* root = new Probe(10);
    Probe* rejected = new Probe(11);
    Probe* grandchild = new Probe(12);
    Probe* disabled = new Probe(13);
    Probe* suppressedChild = new Probe(14);
    Probe* lastSibling = new Probe(15);
    Probe* hierarchy[] = { root, rejected, grandchild, disabled, suppressedChild, lastSibling };
    for (unsigned int i = 0; i != sizeof(hierarchy) / sizeof(hierarchy[0]); ++i)
    {
        hierarchy[i]->setQueryFlags(2u);
        hierarchy[i]->setGeometryPresent(true);
    }
    root->setChildLink(rejected);
    rejected->setChildLink(grandchild);
    root->setSiblingLink(disabled);
    disabled->setQueryFlags(3u);
    disabled->setChildLink(suppressedChild);
    disabled->setSiblingLink(lastSibling);
    Predicate predicate;
    predicate.accepted.push_back(root);
    predicate.accepted.push_back(grandchild);
    predicate.accepted.push_back(suppressedChild);
    predicate.accepted.push_back(lastSibling);
    out.clear();
    manager->addChildrenRend(root, out, 0, &predicate);
    check(sequence(out, root, grandchild, lastSibling),
          "single addChildrenRend preserves depth-first predicate and subtree rules");
    check(predicate.visited.size() == 4
              && predicate.visited[0] == root
              && predicate.visited[1] == rejected
              && predicate.visited[2] == grandchild
              && predicate.visited[3] == lastSibling,
          "disabled sibling suppresses its child before predicate dispatch");

    // Vector roots bypass the predicate; bit 23 alone controls entry to the single-object walk.
    rejected->setQueryFlags(2u | 0x00800000u);
    predicate.visited.clear();
    out.clear();
    std::vector<IObject*> roots(1, rejected);
    manager->addChildrenRend(roots, out, 0, &predicate);
    check(sequence(out, rejected, grandchild),
          "vector addChildrenRend keeps roots and predicate-filters only descendants");

    std::printf("%d checks, %d failures\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
