// [TEMP] [CHOSEN, not recovered] Focused x86 runtime validation for the recovered
// WindAffector / WindAffectorTemplate / WindManager family.  This is harness-only
// scaffolding and is not part of any BF2 image.

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <type_traits>

#include "../BF2/Core/Ids.h"
#include "../BF2/Game/Objects/IObject.h"
#include "../BF2/Game/Objects/IWindAffector.h"
#include "../BF2/Game/Objects/WindAffector.h"
#include "../BF2/Game/Objects/WindAffectorTemplate.h"
#include "../BF2/Scene/Object/WindManager.h"

using dice::hfe::IBase;
using dice::hfe::Mat4;
using dice::hfe::Vec3;
using dice::hfe::world::CID_WindAffector;
using dice::hfe::world::CID_WindAffectorTemplate;
using dice::hfe::world::IID_IWindAffector;
using dice::hfe::world::IID_IWindAffectorTemplate;
using dice::hfe::world::IObject;
using dice::hfe::world::IObjectComponent;
using dice::hfe::world::IWindAffector;
using dice::hfe::world::IWindAffectorTemplate;
using dice::hfe::world::WindAffector;
using dice::hfe::world::WindAffectorTemplate;
using dice::hfe::world::WindManager;

// The production global lives in Core/Globals.cpp.  This focused executable deliberately
// supplies only the singleton slot exercised by the two source files under test.
namespace dice {
namespace hfe {
namespace world {
IBase* windManager = 0;
// These two address-bearing ids are intentionally extern in Ids.h and normally live in
// Core/Globals.cpp.  Supplying their attested values here keeps this harness from importing that
// file's unrelated singleton surface.
extern const unsigned int IID_IObjectComponent = 50043u;
extern const unsigned int IID_IObjectTemplateComponent = 50379u;
} // namespace world
} // namespace hfe
} // namespace dice

namespace {

struct Checks
{
    Checks() : total(0), failures(0) {}

    void expect(bool condition, const char* message)
    {
        ++total;
        if (!condition)
        {
            ++failures;
            std::printf("FAIL: %s\n", message);
        }
    }

    int total;
    int failures;
};

bool near(float lhs, float rhs, float epsilon = 0.0001f)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

bool nearVec(const Vec3& value, float x, float y, float z, float epsilon = 0.0001f)
{
    return near(value.x, x, epsilon)
        && near(value.y, y, epsilon)
        && near(value.z, z, epsilon);
}

void setIdentity(Mat4& matrix)
{
    for (int i = 0; i < 16; ++i)
        matrix.m[i] = 0.0f;
    matrix.m[0] = 1.0f;
    matrix.m[5] = 1.0f;
    matrix.m[10] = 1.0f;
    matrix.m[15] = 1.0f;
}

// [TEMP]
// Raw x86 IObject observer implements position +0x68 and transform +0x80; all other slots abort.
// MSVC slots 26/32 correspond to logical slots 27/33 with Itanium's extra destructor entry.
// __fastcall receives self in ECX for the __thiscall boundary.
struct ObjectAbiFixture
{
    void** vtable;
    Vec3 position;
    Mat4 transformation;
};

void __fastcall unexpectedObjectVirtual(ObjectAbiFixture*)
{
    std::fprintf(stderr, "[TEMP] unexpected IObject virtual reached by WindAffector test\n");
    std::fflush(stderr);
    std::abort();
}

const Vec3& __fastcall fixtureAbsolutePosition(const ObjectAbiFixture* fixture)
{
    return fixture->position;
}

const Mat4& __fastcall fixtureAbsoluteTransformation(const ObjectAbiFixture* fixture)
{
    return fixture->transformation;
}

class FixtureObject
{
public:
    FixtureObject()
    {
        for (int i = 0; i < 90; ++i)
            m_slots[i] = reinterpret_cast<void*>(&unexpectedObjectVirtual);
        m_slots[26] = reinterpret_cast<void*>(&fixtureAbsolutePosition);
        m_slots[32] = reinterpret_cast<void*>(&fixtureAbsoluteTransformation);
        m_storage.vtable = m_slots;
        m_storage.position.x = 0.0f;
        m_storage.position.y = 0.0f;
        m_storage.position.z = 0.0f;
        setIdentity(m_storage.transformation);
    }

    IObject* object() { return reinterpret_cast<IObject*>(&m_storage); }
    Vec3& position() { return m_storage.position; }
    Mat4& transformation() { return m_storage.transformation; }

private:
    void* m_slots[90];
    ObjectAbiFixture m_storage;
};

IWindAffector* createAffector(WindAffectorTemplate& objectTemplate, Checks& checks)
{
    IObjectComponent* const component = objectTemplate.createComponent();
    checks.expect(component != 0, "WindAffectorTemplate::createComponent returns a component");
    if (!component)
        return 0;

    IWindAffector* const affector = static_cast<IWindAffector*>(
        component->queryInterface(IID_IWindAffector));
    checks.expect(affector != 0, "created component exposes IID_IWindAffector");
    return affector;
}

void retireAffector(IWindAffector* affector, Checks& checks)
{
    if (!affector)
        return;
    affector->destroy();
    checks.expect(affector->release() == 0, "final WindAffector release deletes the runtime");
}

void testTemplateAndRuntimeDefaults(Checks& checks, FixtureObject& owner)
{
    static_assert(sizeof(void*) == 4, "WindAffector runtime test must be compiled x86");
    static_assert(sizeof(WindAffector) == 0x18,
                  "WindAffector must preserve the recovered x86 0x18-byte layout");

    WindAffectorTemplate objectTemplate;
    checks.expect(objectTemplate.getRef() == 1, "template starts with one reference");
    checks.expect(objectTemplate.getClassID() == CID_WindAffectorTemplate,
                  "template returns CID_WindAffectorTemplate");
    checks.expect(static_cast<IWindAffectorTemplate*>(&objectTemplate)->getInterfaceID()
                      == IID_IWindAffectorTemplate,
                  "template interface returns IID_IWindAffectorTemplate");
    checks.expect(objectTemplate.queryInterface(IID_IWindAffectorTemplate)
                      == static_cast<IWindAffectorTemplate*>(&objectTemplate),
                  "template exposes its primary interface");
    checks.expect(objectTemplate.queryInterface(dice::hfe::io::IID_IConsoleSaveable) != 0,
                  "template exposes IConsoleSaveable");
    checks.expect(objectTemplate.queryInterface(0xFFFFFFFFu) == 0,
                  "template rejects an unknown interface");
    checks.expect(objectTemplate.queryImplementation(CID_WindAffectorTemplate)
                      == &objectTemplate,
                  "template exposes its concrete implementation id");
    checks.expect(objectTemplate.queryImplementation(CID_WindAffector) == 0,
                  "template rejects a different concrete id");

    checks.expect(objectTemplate.getWindIsDynamic(), "windIsDynamic defaults true");
    checks.expect(objectTemplate.getWindTypeEnum() == 0
                      && objectTemplate.getWindType() == "Directional",
                  "windType defaults to Directional / enum zero");
    checks.expect(nearVec(objectTemplate.getWindDirection(), 0.0f, -1.0f, 0.0f),
                  "windDirection defaults to (0,-1,0)");
    checks.expect(near(objectTemplate.getWindSpeed(), 0.0f), "windSpeed defaults to zero");
    checks.expect(near(objectTemplate.getWindFalloff(), 10.0f), "windFalloff defaults to ten");
    checks.expect(near(objectTemplate.getWindRadialFalloff(), 1.0f),
                  "windRadialFalloff defaults to one");
    checks.expect(near(objectTemplate.getWindBlowTime(), -1.0f),
                  "windBlowTime defaults to minus one");

    static const char* const names[] = {
        "dIrEcTiOnAl", "pOiNt", "rAdIaLdIr", "tUrBuLeNcEfIeLd"
    };
    for (int i = 0; i < 4; ++i)
    {
        objectTemplate.setWindType(names[i]);
        checks.expect(objectTemplate.getWindTypeEnum() == i,
                      "windType lookup is case-insensitive and maps in retail order");
        checks.expect(objectTemplate.getWindType() == names[i],
                      "windType retains the authored spelling");
    }
    objectTemplate.setWindType("not-a-retail-wind-type");
    checks.expect(objectTemplate.getWindTypeEnum() == 3
                      && objectTemplate.getWindType() == names[3],
                  "invalid windType preserves both prior enum and authored string");

    IWindAffector* const affector = createAffector(objectTemplate, checks);
    if (!affector)
        return;
    checks.expect(affector->getRef() == 1, "runtime starts with one reference");
    checks.expect(affector->getClassID() == CID_WindAffector,
                  "runtime returns CID_WindAffector");
    checks.expect(affector->getInterfaceID() == IID_IWindAffector,
                  "runtime returns IID_IWindAffector");
    checks.expect(affector->queryInterface(dice::hfe::IID_IBase) == affector
                      && affector->queryInterface(dice::hfe::world::IID_IObjectComponent)
                             == affector,
                  "runtime exposes IBase and IObjectComponent at the primary address");
    checks.expect(affector->queryInterface(0xFFFFFFFFu) == 0,
                  "runtime rejects an unknown interface");
    checks.expect(affector->getObject() == 0, "runtime object starts null");
    checks.expect(affector->getIsActive(), "runtime starts active");
    checks.expect(near(affector->getBoundingRadius(), objectTemplate.getWindFalloff()),
                  "runtime bounding radius reads the template falloff");

    // init() is the first legitimate read/write boundary for the deliberately-uninitialised timer.
    affector->init(owner.object());
    checks.expect(affector->getObject() == owner.object(), "init stores the owner object");
    checks.expect(&affector->getAbsolutePosition() == &owner.position(),
                  "runtime delegates absolute position to its owner");
    affector->setObject(0);
    checks.expect(affector->getObject() == 0, "setObject replaces the owner pointer");
    affector->setObject(owner.object());
    retireAffector(affector, checks);
}

void testLifecycleAndRange(Checks& checks, FixtureObject& owner, WindManager& manager)
{
    WindAffectorTemplate finiteTemplate;
    finiteTemplate.setWindBlowTime(1.0f);
    IWindAffector* finite = createAffector(finiteTemplate, checks);
    if (finite)
    {
        finite->init(owner.object());
        checks.expect(manager.getAllWindAffectors().size() == 1,
                      "init registers a runtime with WindManager");
        manager.addWindAffector(finite);
        checks.expect(manager.getAllWindAffectors().size() == 1,
                      "duplicate registration is ignored by the set");
        manager.update(0.4f);
        checks.expect(finite->getIsActive()
                          && manager.getAllWindAffectors().count(finite) == 1,
                      "positive blow time remains active before expiry");
        manager.update(0.7f);
        checks.expect(!finite->getIsActive()
                          && manager.getAllWindAffectors().count(finite) == 0,
                      "crossing blow-time zero deactivates and unregisters");
        finite->destroy();
        checks.expect(manager.getAllWindAffectors().empty(),
                      "destroy after expiry tolerates an already-absent runtime");
        checks.expect(finite->release() == 0, "expired runtime releases cleanly");
    }

    WindAffectorTemplate zeroTemplate;
    zeroTemplate.setWindBlowTime(0.0f);
    IWindAffector* zero = createAffector(zeroTemplate, checks);
    if (zero)
    {
        zero->init(owner.object());
        manager.update(1000.0f);
        checks.expect(zero->getIsActive()
                          && manager.getAllWindAffectors().count(zero) == 1,
                      "zero blow time is indefinite");
        zero->setIsActive(false);
        const Vec3 here = { 0.0f, 0.0f, 0.0f };
        checks.expect(manager.getAllWindAffectors().count(zero) == 1,
                      "manual deactivation does not unregister the runtime");
        checks.expect(manager.getWindAffectors(here, 0.0f).empty(),
                      "range sampling filters inactive runtimes");
        zero->setIsActive(true);
        zeroTemplate.setWindFalloff(4.0f);
        const Vec3 boundary = { 4.0f, 0.0f, 0.0f };
        checks.expect(manager.getWindAffectors(boundary, 0.0f).empty(),
                      "range test excludes the exact bounding-radius boundary");
        checks.expect(manager.getWindAffectors(boundary, 0.01f).count(zero) == 1,
                      "query radius expands the local-wind range");
        retireAffector(zero, checks);
        checks.expect(manager.getAllWindAffectors().empty(),
                      "destroy unregisters an active runtime");
    }

    WindAffectorTemplate negativeTemplate;
    negativeTemplate.setWindBlowTime(-1.0f);
    IWindAffector* negative = createAffector(negativeTemplate, checks);
    if (negative)
    {
        negative->init(owner.object());
        manager.update(1000.0f);
        checks.expect(negative->getIsActive()
                          && manager.getAllWindAffectors().count(negative) == 1,
                      "negative blow time is indefinite");
        retireAffector(negative, checks);
    }
}

void testWindMathAndManager(Checks& checks, FixtureObject& owner, WindManager& manager)
{
    checks.expect(near(manager.getGlobalWindSpeed(), 0.0f),
                  "WindManager global speed defaults to zero");
    checks.expect(nearVec(manager.getGlobalWindDirection(), 0.0f, 0.0f, 1.0f),
                  "WindManager global direction defaults to +Z");
    checks.expect(nearVec(manager.getGlobalWind(), 0.0f, 0.0f, 0.0f),
                  "WindManager cached global wind defaults to zero");

    WindAffectorTemplate objectTemplate;
    objectTemplate.setWindIsDynamic(false); // Retail runtime deliberately ignores this property.
    objectTemplate.setWindType("Directional");
    objectTemplate.setWindDirection(Vec3{ 1.0f, 0.0f, 0.0f });
    objectTemplate.setWindSpeed(8.0f);
    objectTemplate.setWindFalloff(4.0f);
    IWindAffector* affector = createAffector(objectTemplate, checks);
    if (!affector)
        return;
    affector->init(owner.object());

    Vec3 direction = { 9.0f, 8.0f, 7.0f };
    float speed = -100.0f;
    const Vec3 centre = { 0.0f, 0.0f, 0.0f };
    const Vec3 half = { 2.0f, 0.0f, 0.0f };
    const Vec3 edge = { 4.0f, 0.0f, 0.0f };
    affector->getAbsoluteWindDirectionAndSpeed(centre, direction, speed);
    checks.expect(nearVec(direction, 1.0f, 0.0f, 0.0f) && near(speed, 8.0f),
                  "Directional wind is full strength at its centre");
    affector->getAbsoluteWindDirectionAndSpeed(half, direction, speed);
    checks.expect(nearVec(direction, 1.0f, 0.0f, 0.0f) && near(speed, 6.0f),
                  "Directional wind uses squared-distance falloff");
    affector->getAbsoluteWindDirectionAndSpeed(edge, direction, speed);
    checks.expect(near(speed, 0.0f), "Directional wind reaches zero at its falloff boundary");

    // Local +X becomes world +Y.  The result changes after init even with windIsDynamic false,
    // proving the shipped runtime always fetches the owner's current transformation.
    setIdentity(owner.transformation());
    owner.transformation().m[0] = 0.0f;
    owner.transformation().m[1] = 1.0f;
    owner.transformation().m[4] = -1.0f;
    owner.transformation().m[5] = 0.0f;
    affector->getAbsoluteWindDirectionAndSpeed(centre, direction, speed);
    checks.expect(nearVec(direction, 0.0f, 1.0f, 0.0f),
                  "Directional wind follows the live owner transformation");
    setIdentity(owner.transformation());

    objectTemplate.setWindType("Point");
    affector->getAbsoluteWindDirectionAndSpeed(half, direction, speed);
    checks.expect(nearVec(direction, 1.0f, 0.0f, 0.0f) && near(speed, 6.0f),
                  "Point wind normalises centre-to-query and uses squared falloff");
    affector->getAbsoluteWindDirectionAndSpeed(centre, direction, speed);
    checks.expect(nearVec(direction, 0.0f, 0.0f, 0.0f) && near(speed, 8.0f),
                  "Point wind safely zeroes its direction at the centre");

    objectTemplate.setWindType("RadialDir");
    objectTemplate.setWindDirection(Vec3{ 0.0f, 1.0f, 0.0f });
    objectTemplate.setWindSpeed(8.0f);
    objectTemplate.setWindFalloff(10.0f);
    objectTemplate.setWindRadialFalloff(4.0f);
    const Vec3 radial = { 2.0f, 5.0f, 0.0f };
    affector->getAbsoluteWindDirectionAndSpeed(radial, direction, speed);
    checks.expect(nearVec(direction, 0.45f, 0.55f, 0.0f) && near(speed, 4.0f),
                  "RadialDir blends radial/axis direction and attenuates axially");
    const Vec3 behind = { 2.0f, -5.0f, 0.0f };
    affector->getAbsoluteWindDirectionAndSpeed(behind, direction, speed);
    checks.expect(near(speed, 12.0f),
                  "RadialDir does not lower-clamp attenuation behind its origin");
    const Vec3 radialBoundary = { 4.0f, 5.0f, 0.0f };
    affector->getAbsoluteWindDirectionAndSpeed(radialBoundary, direction, speed);
    checks.expect(near(speed, 0.0f) && nearVec(direction, 4.0f, 5.0f, 0.0f),
                  "RadialDir excludes its strict radial boundary and retains raw delta");

    affector->setIsActive(false);
    direction = Vec3{ 9.0f, 8.0f, 7.0f };
    speed = 123.0f;
    affector->getAbsoluteWindDirectionAndSpeed(centre, direction, speed);
    checks.expect(nearVec(direction, 9.0f, 8.0f, 7.0f) && near(speed, 0.0f),
                  "inactive direction/speed sampling writes only zero speed");
    checks.expect(nearVec(affector->getAbsoluteWind(centre), 0.0f, 0.0f, 0.0f),
                  "inactive absolute wind is the zero vector");
    affector->setIsActive(true);

    objectTemplate.setWindType("TurbulenceField");
    direction = Vec3{ 9.0f, 8.0f, 7.0f };
    speed = 123.0f;
    affector->getAbsoluteWindDirectionAndSpeed(centre, direction, speed);
    checks.expect(nearVec(direction, 9.0f, 8.0f, 7.0f) && near(speed, 123.0f),
                  "TurbulenceField preserves both output arguments");

    // Manager combination and both observable retail defects.
    objectTemplate.setWindType("Directional");
    objectTemplate.setWindDirection(Vec3{ 1.0f, 0.0f, 0.0f });
    objectTemplate.setWindSpeed(2.0f);
    objectTemplate.setWindFalloff(10.0f);
    manager.setGlobalWindSpeed(1.0f); // default direction is +Z
    checks.expect(nearVec(manager.getGlobalWind(), 0.0f, 0.0f, 1.0f),
                  "global speed setter refreshes cached global wind");
    checks.expect(nearVec(manager.getWind(centre), 2.0f, 0.0f, 1.0f),
                  "WindManager adds global and in-range local wind vectors");
    checks.expect(near(manager.getWindSpeed(centre), std::sqrt(5.0f)),
                  "WindManager wind speed is the combined vector magnitude");
    checks.expect(nearVec(manager.getWindDirection(centre), 0.0f, 0.0f, 1.0f),
                  "getWindDirection preserves the shipped global-only defect");
    manager.getWindDirectionAndSpeed(centre, direction, speed);
    const float rootFive = std::sqrt(5.0f);
    checks.expect(near(speed, rootFive)
                      && nearVec(direction, 2.0f * rootFive, 0.0f, rootFive),
                  "combined direction preserves the shipped multiply-by-magnitude defect");

    objectTemplate.setWindSpeed(0.0f);
    direction = Vec3{ 0.0f, 0.0f, 0.0f };
    speed = -1.0f;
    manager.getWindDirectionAndSpeed(centre, direction, speed);
    checks.expect(nearVec(direction, 0.0f, 0.0f, 1.0f) && near(speed, 1.0f),
                  "zero local accumulation falls back to global direction and speed");

    const Vec3 newGlobalDirection = { 0.0f, 1.0f, 0.0f };
    manager.setGlobalWindDirection(newGlobalDirection);
    checks.expect(nearVec(manager.getGlobalWind(), 0.0f, 1.0f, 0.0f),
                  "global direction setter refreshes cached global wind");

    retireAffector(affector, checks);
}

} // namespace

int main()
{
    static_assert(std::is_base_of<dice::hfe::world::IWindManager, WindManager>::value,
                  "WindManager must retain its RTTI-proven IWindManager base");

    Checks checks;
    FixtureObject owner;

    // Constructor/default behavior with no singleton present also covers the manager-null guards.
    testTemplateAndRuntimeDefaults(checks, owner);

    WindManager manager;
    dice::hfe::world::windManager = &manager;
    checks.expect(manager.getRef() == 1, "WindManager starts with one reference");
    checks.expect(manager.queryInterface(dice::hfe::IID_IBase)
                      == reinterpret_cast<void*>(&manager)
                      && manager.queryInterface(dice::hfe::world::IID_IWindManager)
                             == reinterpret_cast<void*>(&manager),
                  "WindManager exposes its two recovered interface ids");
    checks.expect(manager.queryInterface(0xFFFFFFFFu) == 0,
                  "WindManager rejects an unknown interface");

    IBase* const createdManager = WindManager::create(
        dice::hfe::world::IID_IWindManager, 0);
    checks.expect(createdManager != 0 && createdManager->release() == 0,
                  "WindManager factory returns a real IBase and release preserves zero after delete");

    testLifecycleAndRange(checks, owner, manager);
    testWindMathAndManager(checks, owner, manager);
    checks.expect(manager.getAllWindAffectors().empty(),
                  "all focused runtimes are unregistered before manager teardown");
    dice::hfe::world::windManager = 0;

    std::printf("wind affector runtime: %d checks, %d failures\n",
                checks.total, checks.failures);
    return checks.failures == 0 ? 0 : 1;
}
