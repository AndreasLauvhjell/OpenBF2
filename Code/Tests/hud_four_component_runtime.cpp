// [TEMP] Focused x86 client-side regression fixture for the recovered
// PostProcess/Ability/Vehicle/Weapon HUD component families. Not part of a BF2 image.

// It deliberately exercises only non-audible runtime paths. The production sound seams are
// separately named/reporting frontiers and are not invoked here.

#include "../BF2/Core/ClassFactory.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Game/Objects/AbilityHudCompTemplate.h"
#include "../BF2/Game/Objects/IAbilityHudComp.h"
#include "../BF2/Game/Objects/IPostProcessComp.h"
#include "../BF2/Game/Objects/IVehicleHudComp.h"
#include "../BF2/Game/Objects/IWeaponHudComp.h"
#include "../BF2/Game/Objects/PostProcessCompTemplate.h"
#include "../BF2/Game/Objects/VehicleHudCompTemplate.h"
#include "../BF2/Game/Objects/WeaponHudCompTemplate.h"
#include "../BF2/IO/Stream/IStream.h"

#include <cstdio>
#include <string>

// [TEMP]
// Provides local component IDs; tested transitions take no sound branch, so the facade stays null.
namespace dice { namespace hfe { class IBfSound; IBfSound* g_bfSound = 0; } }
namespace dice { namespace hfe { namespace world {
extern const unsigned int IID_IObjectTemplateComponent = 50379;
extern const unsigned int IID_IObjectComponent = 50043;
} } }

// The client icon resolver checks this pointer before making either virtual call. Keeping it null
// makes icon mutation deterministic without replacing the production resolver.
namespace dice { namespace hfe {
namespace hud { class BF2HudManager; }
hud::BF2HudManager* hudManager = 0;
} }

static unsigned int g_rootParentCalls;

// WeaponHudComp has a direct reference to this production helper. Every path selected below must
// return before reaching it; the counter turns an accidental traversal into a fixture failure.
namespace dice { namespace hfe { namespace world {
IObject* getRootParent(const IObject* object)
{
    ++g_rootParentCalls;
    return const_cast<IObject*>(object);
}
} } }

namespace {

class CaptureStream : public dice::hfe::io::IStream
{
public:
    CaptureStream()
        : m_refCount(1), m_shortWrite(false), m_writeCalls(0), m_requestedBytes(0)
    {
    }

    virtual void addRef() { ++m_refCount; }
    virtual int getRef() { return m_refCount; }
    virtual int release() { return --m_refCount; }
    virtual void* queryInterface(unsigned int) const
    {
        return const_cast<CaptureStream*>(this);
    }
    virtual unsigned int read(void*, unsigned int) { return 0; }
    virtual unsigned int write(const void* data, unsigned int size)
    {
        ++m_writeCalls;
        m_requestedBytes = size;
        const unsigned int accepted = m_shortWrite && size ? size - 1 : size;
        m_text.append(static_cast<const char*>(data), accepted);
        return accepted;
    }
    virtual int skip(unsigned int) { return 0; }
    virtual int getAvailableBytes() { return 0; }
    virtual dice::hfe::io::IStream* clone() { return 0; }
    virtual bool canRead() { return false; }
    virtual bool canWrite() { return true; }

    void reset(bool shortWrite = false)
    {
        m_text.clear();
        m_shortWrite = shortWrite;
        m_writeCalls = 0;
        m_requestedBytes = 0;
    }

    const std::string& text() const { return m_text; }
    unsigned int writeCalls() const { return m_writeCalls; }
    unsigned int requestedBytes() const { return m_requestedBytes; }

private:
    int m_refCount;
    bool m_shortWrite;
    unsigned int m_writeCalls;
    unsigned int m_requestedBytes;
    std::string m_text;
};

bool check(bool condition, const char* what)
{
    if (!condition)
        std::fprintf(stderr, "hud_four_component_runtime: %s\n", what);
    return condition;
}

bool checkScript(dice::hfe::io::IConsoleSaveable& object,
                 CaptureStream& stream,
                 const std::string& expected,
                 const char* fullWriteWhat,
                 const char* bytesWhat,
                 const char* callCountWhat)
{
    bool ok = true;
    stream.reset();
    ok &= check(object.makeScript(&stream), fullWriteWhat);
    ok &= check(stream.text() == expected, bytesWhat);
    ok &= check(stream.writeCalls() == 1 && stream.requestedBytes() == expected.size(),
                callCountWhat);
    return ok;
}

bool checkShortWrite(dice::hfe::io::IConsoleSaveable& object,
                     CaptureStream& stream,
                     const std::string& expected,
                     const char* resultWhat,
                     const char* bytesWhat)
{
    bool ok = true;
    stream.reset(true);
    ok &= check(!object.makeScript(&stream), resultWhat);
    ok &= check(stream.writeCalls() == 1 && stream.requestedBytes() == expected.size(),
                "short write call count/requested length");
    ok &= check(!expected.empty() && stream.text() == expected.substr(0, expected.size() - 1),
                bytesWhat);
    return ok;
}

template <typename Base>
class TemplateProbe : public Base
{
public:
    bool needsBuild() const { return this->m_needsBuild; }
};

template <typename Base>
class FactoryProbe : public Base
{
public:
    FactoryProbe() { ++s_constructed; }
    virtual ~FactoryProbe() { ++s_destroyed; }

    static void resetCounts()
    {
        s_constructed = 0;
        s_destroyed = 0;
    }

    static int s_constructed;
    static int s_destroyed;
};

template <typename Base> int FactoryProbe<Base>::s_constructed = 0;
template <typename Base> int FactoryProbe<Base>::s_destroyed = 0;

void checkTemplateIdentity(dice::hfe::world::IObjectTemplateComponent* primary,
                           dice::hfe::io::IConsoleSaveable* saveable,
                           unsigned int interfaceId,
                           unsigned int classId,
                           const char* family,
                           bool& ok)
{
    using namespace dice::hfe;
    using namespace dice::hfe::world;

    ok &= check(primary->getRef() == 1, family);
    primary->addRef();
    ok &= check(primary->release() == 1, "template refcount round trip");
    ok &= check(primary->queryInterface(IID_IBase) == primary, "template IBase QI");
    ok &= check(primary->queryInterface(IID_IObjectTemplateComponent) == primary,
                "template common QI");
    ok &= check(primary->queryInterface(interfaceId) == primary, "template family QI");
    ok &= check(primary->queryInterface(io::IID_IConsoleSaveable) == saveable,
                "template saveable QI");
    ok &= check(static_cast<void*>(primary) != static_cast<void*>(saveable),
                "IConsoleSaveable pointer adjustment");
    ok &= check(saveable->queryInterface(interfaceId) == primary,
                "secondary-base QI returns primary subobject");
    ok &= check(primary->queryInterface(0xFFFFFFFFu) == 0, "template mismatch QI");
    ok &= check(primary->getInterfaceID() == interfaceId, "template interface id");
    ok &= check(primary->getClassID() == classId, "template class id");
    ok &= check(primary->queryImplementation(classId) == primary,
                "template implementation query");
    ok &= check(primary->queryImplementation(0xFFFFFFFFu) == 0,
                "template implementation mismatch");
}

template <typename Base>
void checkGenericFactory(unsigned int interfaceId, const char* family, bool& ok)
{
    using namespace dice::hfe;
    typedef FactoryProbe<Base> Probe;

    Probe::resetCounts();

    IBase* result = world::ClassFactory<Probe>::create(
        interfaceId, reinterpret_cast<IBase*>(1));
    ok &= check(result == 0, family);
    ok &= check(Probe::s_constructed == 0 && Probe::s_destroyed == 0,
                "ClassFactory rejects outer before construction");

    result = world::ClassFactory<Probe>::create(0xFFFFFFFFu, 0);
    ok &= check(result == 0, "ClassFactory mismatch result");
    ok &= check(Probe::s_constructed == 1 && Probe::s_destroyed == 1,
                "ClassFactory mismatch cleanup");

    result = world::ClassFactory<Probe>::create(interfaceId, 0);
    ok &= check(result != 0, "ClassFactory primary success");
    if (result)
    {
        ok &= check(result->queryInterface(interfaceId) == result,
                    "ClassFactory primary pointer");
        ok &= check(result->getRef() == 1, "ClassFactory success refcount");
        ok &= check(result->release() == 0, "ClassFactory primary release");
    }
    ok &= check(Probe::s_constructed == 2 && Probe::s_destroyed == 2,
                "ClassFactory primary destruction");

    result = world::ClassFactory<Probe>::create(io::IID_IConsoleSaveable, 0);
    ok &= check(result != 0, "ClassFactory secondary success");
    if (result)
    {
        void* primary = result->queryInterface(interfaceId);
        ok &= check(primary != 0, "ClassFactory secondary can query primary");
        ok &= check(primary != static_cast<void*>(result),
                    "ClassFactory preserves adjusted secondary pointer");
        ok &= check(result->release() == 0, "ClassFactory secondary release");
    }
    ok &= check(Probe::s_constructed == 3 && Probe::s_destroyed == 3,
                "ClassFactory secondary destruction");
}

void checkComponentIdentity(dice::hfe::world::IObjectComponent* component,
                            unsigned int interfaceId,
                            const char* family,
                            bool& ok)
{
    using namespace dice::hfe;
    using namespace dice::hfe::world;

    ok &= check(component != 0, family);
    if (!component)
        return;
    ok &= check(component->getRef() == 1, "component initial refcount");
    component->addRef();
    ok &= check(component->release() == 1, "component refcount round trip");
    ok &= check(component->getInterfaceID() == interfaceId, "component interface id");
    ok &= check(component->queryInterface(IID_IBase) == component, "component IBase QI");
    ok &= check(component->queryInterface(IID_IObjectComponent) == component,
                "component common QI");
    ok &= check(component->queryInterface(interfaceId) == component, "component family QI");
    ok &= check(component->queryInterface(0xFFFFFFFFu) == 0, "component mismatch QI");
    component->setObject(0);
    ok &= check(component->getObject() == 0, "component object forwarding");
    ok &= check(!component->hasUpdate(), "component hasUpdate default");
    component->handleUpdate(0.25f);
    ok &= check(!component->hasFrameUpdate(), "component hasFrameUpdate default");
    component->handleFrameUpdate(0.25f);
}

bool testPostProcess(CaptureStream& stream)
{
    using namespace dice::hfe;
    using namespace dice::hfe::world;

    bool ok = true;
    TemplateProbe<PostProcessCompTemplate> object;
    checkTemplateIdentity(static_cast<IPostProcessCompTemplate*>(&object),
                          static_cast<io::IConsoleSaveable*>(&object),
                          IID_IPostProcessCompTemplate, CID_PostProcessCompTemplate,
                          "post-process template initial refcount", ok);
    ok &= check(object.needsBuild(), "post-process initially needs build");
    ok &= check(object.getTvInterference() == 0.05f &&
                object.getTvDistortionRoll() == 0.1f &&
                object.getTvDistortionFreq() == 0.5f &&
                object.getTvDistortionScale() == 0.5f,
                "post-process template defaults");
    ok &= checkScript(object, stream, "",
                      "post-process default full write",
                      "post-process default script bytes",
                      "post-process default one-write contract");

    object.setTvInterference(0.25f);
    object.setTvDistortionRoll(0.75f);
    object.setTvDistortionFreq(1.25f);
    object.setTvDistortionScale(2.5f);
    const std::string expected =
        "ObjectTemplate.postProcess.tvInterference 0.25\n"
        "ObjectTemplate.postProcess.tvDistortionRoll 0.75\n"
        "ObjectTemplate.postProcess.tvDistortionFreq 1.25\n"
        "ObjectTemplate.postProcess.tvDistortionScale 2.5\n";
    ok &= checkScript(object, stream, expected,
                      "post-process full write", "post-process script bytes/order",
                      "post-process one-write contract");
    ok &= checkShortWrite(object, stream, expected,
                          "post-process short write result",
                          "post-process short write bytes");

    IObjectComponent* base = object.createComponent();
    ok &= check(!object.needsBuild(), "post-process clears needs-build");
    checkComponentIdentity(base, IID_IPostProcessComp, "post-process component", ok);
    if (base)
    {
        IPostProcessComp* component = static_cast<IPostProcessComp*>(
            base->queryInterface(IID_IPostProcessComp));
        ok &= check(component != 0, "post-process concrete QI");
        if (component)
        {
            PostProcessInfo* info = component->getInfo();
            ok &= check(info && info->tvInterference == 0.25f &&
                        info->tvDistortionRoll == 0.75f &&
                        info->tvDistortionFreq == 1.25f &&
                        info->tvDistortionScale == 2.5f,
                        "post-process runtime forwards template info");
        }
        base->release();
    }
    base = object.createComponent();
    ok &= check(!object.needsBuild(), "post-process stays built after second component");
    if (base)
        base->release();

    checkGenericFactory<PostProcessCompTemplate>(IID_IPostProcessCompTemplate,
                                                  "post-process factory outer", ok);
    return ok;
}

bool testAbility(CaptureStream& stream)
{
    using namespace dice::hfe;
    using namespace dice::hfe::world;

    bool ok = true;
    TemplateProbe<AbilityHudCompTemplate> object;
    checkTemplateIdentity(static_cast<IAbilityHudCompTemplate*>(&object),
                          static_cast<io::IConsoleSaveable*>(&object),
                          IID_IAbilityHudCompTemplate, CID_AbilityHudCompTemplate,
                          "ability template initial refcount", ok);
    ok &= check(object.needsBuild(), "ability initially needs build");
    ok &= check(object.getHealingSound().empty() && object.getRepairingSound().empty() &&
                object.getAmmoSound().empty(), "ability template defaults");
    ok &= checkScript(object, stream, "",
                      "ability default full write", "ability default script bytes",
                      "ability default one-write contract");

    object.setHealingSound("heal");
    object.setRepairingSound("repair");
    object.setAmmoSound("ammo");
    ok &= check(object.getHealingSound() == "heal" &&
                object.getRepairingSound() == "repair" &&
                object.getAmmoSound() == "ammo", "ability template mutations");
    const std::string expected =
        "ObjectTemplate.AbilityHud.repairingSound repair\n"
        "ObjectTemplate.AbilityHud.ammoSound ammo\n"
        "ObjectTemplate.AbilityHud.healingSound heal\n";
    ok &= checkScript(object, stream, expected,
                      "ability full write", "ability script bytes/order",
                      "ability one-write contract");
    ok &= checkShortWrite(object, stream, expected,
                          "ability short write result", "ability short write bytes");

    IObjectComponent* base = object.createComponent();
    ok &= check(!object.needsBuild(), "ability clears needs-build");
    checkComponentIdentity(base, IID_IAbilityHudComp, "ability component", ok);
    if (base)
    {
        IAbilityHudComp* component = static_cast<IAbilityHudComp*>(
            base->queryInterface(IID_IAbilityHudComp));
        ok &= check(component != 0, "ability concrete QI");
        if (component)
        {
            ok &= check(component->getHudInfo() != 0, "ability borrowed HUD info");
            ok &= check(!component->isPlayingHealSound() &&
                        !component->isPlayingRepairSound() &&
                        !component->isPlayingAmmoSound(),
                        "ability runtime state defaults");
            // All six false/default arms are non-audible and must leave the public state false.
            component->setHealingActive(false);
            component->setRepairingActive(false);
            component->setAmmoActive(false);
            component->setGetHealActive(false);
            component->setGetRepairActive(false);
            component->setGetAmmoActive(false);
            ok &= check(!component->isPlayingHealSound() &&
                        !component->isPlayingRepairSound() &&
                        !component->isPlayingAmmoSound(),
                        "ability inactive branches stay silent/inactive");
        }
        base->release();
    }
    base = object.createComponent();
    ok &= check(!object.needsBuild(), "ability stays built after second component");
    if (base)
        base->release();

    checkGenericFactory<AbilityHudCompTemplate>(IID_IAbilityHudCompTemplate,
                                                 "ability factory outer", ok);
    return ok;
}

bool testVehicle(CaptureStream& stream)
{
    using namespace dice::hfe;
    using namespace dice::hfe::world;

    bool ok = true;
    TemplateProbe<VehicleHudCompTemplate> object;
    checkTemplateIdentity(static_cast<IVehicleHudCompTemplate*>(&object),
                          static_cast<io::IConsoleSaveable*>(&object),
                          IID_IVehicleHudCompTemplate, CID_VehicleHudCompTemplate,
                          "vehicle template initial refcount", ok);
    ok &= check(object.needsBuild(), "vehicle initially needs build");
    ok &= check(object.getGuiIndex() == 0 && object.getHudName().empty() &&
                object.getHudSid().empty() && object.getVehicleType() == -1,
                "vehicle identity defaults");
    ok &= check(object.getVehicleIconNumber() == 1 && object.getAbilityIconNumber() == 1 &&
                static_cast<std::string*>(object.getVehicleIcon(0))->empty() &&
                static_cast<std::string*>(object.getAbilityIcon(0))->empty(),
                "vehicle icon vector defaults");
    ok &= check(object.getShowCrossHair() && !object.getUsePlayerIcon() &&
                !object.getDisableOnSprint() && !object.getUseChildHud() &&
                object.getStandardHelpEnabled() && !object.getHasTurretIcon() &&
                object.getShowInfo() && !object.getIsCoDriver(),
                "vehicle boolean defaults");
    ok &= check(object.getShowOnCamMode(1) && !object.getShowOnCamMode(-1) &&
                object.getMiniMapIcon().empty() && object.getTypeIcon().empty() &&
                object.getMiniMapIconSize() == 16 &&
                object.getMiniMapIconLeaderSize() == "Medium" &&
                object.getSpottedIcon().empty() && object.getSpottedIconSize() == 16,
                "vehicle camera/map defaults");
    ok &= check(object.getMaxClimbSpeed() == 10 &&
                object.getEnablePostProcessingOnGuiIndex() == -1 &&
                object.getUseSelectionIcons() && !object.getDisplaySelectionOnEnter() &&
                object.getUseVehicleCommRose() && object.getPantingSound().empty() &&
                object.getInjurySound().empty() &&
                object.getVehicleIconPos().x == 0.0f && object.getVehicleIconPos().y == 0.0f,
                "vehicle remaining defaults");

    const std::string defaultExpected =
        "ObjectTemplate.vehicleHud.miniMapIconLeaderSize Medium\n";
    ok &= checkScript(object, stream, defaultExpected,
                      "vehicle default full write",
                      "vehicle default shipped leader-size quirk",
                      "vehicle default one-write contract");

    object.setHudSid("vehicle-hud");
    object.setTypeIcon("type");
    object.setMiniMapIcon("mini");
    object.setMiniMapIconSize(20);
    object.setSpottedIcon("spot");
    object.setSpottedIconSize(24);
    object.setMiniMapIconLeaderSize("LARGE");
    object.setHasTurretIcon(true);
    object.setShowInfo(false);
    object.setDisableOnSprint(true);
    object.setShowOnCamMode(2);
    object.addShowOnCamMode(3);
    object.setPantingSound("pant");
    object.setInjurySound("hurt");
    object.setIsCoDriver(true);
    object.setShowCrossHair(false);
    object.setUsePlayerIcon(true);
    object.setStandardHelpEnabled(false);
    object.setUseChildHud(true);
    object.setGuiIndex(7);
    object.setMaxClimbSpeed(11);
    Vec2 iconPosition = { 1.5f, -2.25f };
    object.setVehicleIconPos(iconPosition);
    object.setEnablePostProcessingOnGuiIndex(9);
    object.setVehicleType(4);
    object.setUseSelectionIcons(false);
    object.setDisplaySelectionOnEnter(true);
    object.setUseVehicleCommRose(false);
    object.setVehicleIcon("veh0");
    object.addVehicleIcon("veh1");
    object.setAbilityIcon("abil0");
    object.addAbilityIcon("abil1");

    ok &= check(object.getHudName() == "vehicle-hud" &&
                object.getHudSid() == "vehicle-hud" && object.getTypeIcon() == "type" &&
                object.getMiniMapIcon() == "mini" && object.getSpottedIcon() == "spot" &&
                object.getMiniMapIconLeaderSize() == "LARGE",
                "vehicle string mutations/by-value getters");
    ok &= check(object.getVehicleIconNumber() == 2 && object.getAbilityIconNumber() == 2 &&
                *static_cast<std::string*>(object.getVehicleIcon(1)) == "veh1" &&
                *static_cast<std::string*>(object.getAbilityIcon(1)) == "abil1",
                "vehicle icon mutations");

    const std::string expected =
        "ObjectTemplate.vehicleHud.hudName \"vehicle-hud\"\n"
        "ObjectTemplate.vehicleHud.typeIcon type\n"
        "ObjectTemplate.vehicleHud.miniMapIcon mini\n"
        "ObjectTemplate.vehicleHud.miniMapIconSize 20\n"
        "ObjectTemplate.vehicleHud.spottedIcon spot\n"
        "ObjectTemplate.vehicleHud.spottedIconSize 24\n"
        "ObjectTemplate.vehicleHud.hasTurretIcon 1\n"
        "ObjectTemplate.vehicleHud.showInfo 0\n"
        "ObjectTemplate.vehicleHud.disableOnSprint 1\n"
        "ObjectTemplate.vehicleHud.showOnCamMode 2\n"
        "ObjectTemplate.vehicleHud.addShowOnCamMode 3\n"
        "ObjectTemplate.AbilityHud.pantingSound pant\n"
        "ObjectTemplate.AbilityHud.injurySound hurt\n"
        "ObjectTemplate.vehicleHud.isCoDriver 1\n"
        "ObjectTemplate.vehicleHud.showCrosshair 0\n"
        "ObjectTemplate.vehicleHud.usePlayerIcon 1\n"
        "ObjectTemplate.vehicleHud.standardHelpEnabled 0\n"
        "ObjectTemplate.vehicleHud.useChildHud 1\n"
        "ObjectTemplate.vehicleHud.guiIndex 7\n"
        "ObjectTemplate.vehicleHud.maxClimbSpeed 11\n"
        "ObjectTemplate.vehicleHud.vehicleIconPos 1.5/-2.25\n"
        "ObjectTemplate.vehicleHud.enablePostProcessingOnGuiIndex 9\n"
        "ObjectTemplate.vehicleHud.vehicleType 4\n"
        "ObjectTemplate.vehicleHud.useSelectionIcons 0\n"
        "ObjectTemplate.vehicleHud.displaySelectionOnEnter 1\n"
        "ObjectTemplate.vehicleHud.useVehicleCommRose 0\n"
        "ObjectTemplate.vehicleHud.vehicleIcon veh0\n"
        "ObjectTemplate.vehicleHud.addVehicleIcon veh1\n"
        "ObjectTemplate.abilityHud.abilityIcon abil0\n"
        "ObjectTemplate.abilityHud.addAbilityIcon abil1\n";
    ok &= checkScript(object, stream, expected,
                      "vehicle full write", "vehicle script bytes/order/quirks",
                      "vehicle one-write contract");
    ok &= checkShortWrite(object, stream, expected,
                          "vehicle short write result", "vehicle short write bytes");

    IObjectComponent* base = object.createComponent();
    ok &= check(!object.needsBuild(), "vehicle clears needs-build");
    checkComponentIdentity(base, IID_IVehicleHudComp, "vehicle component", ok);
    if (base)
    {
        IVehicleHudComp* component = static_cast<IVehicleHudComp*>(
            base->queryInterface(IID_IVehicleHudComp));
        ok &= check(component != 0, "vehicle concrete QI");
        if (component)
        {
            VehicleHudInfo* info = component->getHudInfo();
            VehicleHudIcons* icons = component->getHudIcons();
            ok &= check(info && info->guiIndex == 7 && info->miniMapIconLeaderSizeValue == 2 &&
                        info->miniMapIconLeaderSize == "LARGE" &&
                        info->displaySelectionOnEnter,
                        "vehicle runtime forwards HUD info");
            ok &= check(icons && icons->vehicleIcons.size() == 2 &&
                        icons->abilityIcons.size() == 2 && icons->miniMapIcon == "mini" &&
                        icons->typeIcon == "type" && icons->spottedIcon == "spot",
                        "vehicle runtime forwards HUD icons");
            ok &= check(component->getVehicleIcon(0) == "veh0" &&
                        component->getVehicleIcon(1) == "veh1" &&
                        component->getVehicleIcon(99) == "veh0" &&
                        component->getAbilityIcon(0) == "abil0" &&
                        component->getAbilityIcon(1) == "abil1" &&
                        component->getAbilityIcon(99) == "abil0",
                        "vehicle runtime icon selection/fallback");
            ok &= check(component->getVehicleIconPtr(0) == 0 &&
                        component->getAbilityIconPtr(0) == 0,
                        "vehicle null HUD manager icon handles");
            ok &= check(component->getShowOnCamMode(2) &&
                        !component->getIsPlayingPantingSound(),
                        "vehicle non-audible runtime defaults");
            ok &= check(component->getGuiIndex(true) == 0,
                        "vehicle sprint-disable early branch");
            object.setDisableOnSprint(false);
            object.setUseChildHud(false);
            object.setShowOnCamMode(1);
            ok &= check(component->getGuiIndex(false) == 7,
                        "vehicle null-object own GUI branch");
            object.setShowOnCamMode(99);
            ok &= check(component->getGuiIndex(false) == 0,
                        "vehicle camera-mode rejection branch");
            ok &= check(component->getEnablePostProcessing(123) == 0,
                        "vehicle null-object post-process miss");
        }
        base->release();
    }
    base = object.createComponent();
    ok &= check(!object.needsBuild(), "vehicle stays built after second component");
    if (base)
        base->release();

    checkGenericFactory<VehicleHudCompTemplate>(IID_IVehicleHudCompTemplate,
                                                 "vehicle factory outer", ok);
    return ok;
}

bool testWeapon(CaptureStream& stream)
{
    using namespace dice::hfe;
    using namespace dice::hfe::world;

    bool ok = true;
    TemplateProbe<WeaponHudCompTemplate> object;
    checkTemplateIdentity(static_cast<IWeaponHudCompTemplate*>(&object),
                          static_cast<io::IConsoleSaveable*>(&object),
                          IID_IWeaponHudCompTemplate, CID_WeaponHudCompTemplate,
                          "weapon template initial refcount", ok);
    ok &= check(object.needsBuild(), "weapon initially needs build");
    ok &= check(object.getGuiIndex() == 5 && object.getAltGuiIndex() == -1 &&
                object.getHudName() == " " && object.getHudSid().empty(),
                "weapon identity defaults");
    ok &= check(!object.getEnableMouse() && object.getWeaponIcon().empty() &&
                object.getAltWeaponIcon().empty() && object.getSelectIcon().empty() &&
                object.getSpecialAbilityIcon().empty() && object.getCrosshairIcon().empty() &&
                object.getAltCrosshairIcon().empty() && object.getCrosshairIconSize() == 32,
                "weapon icon defaults");
    ok &= check(object.getAmmoBarFrontIcon().empty() &&
                object.getAmmoBarBackIcon().empty() && object.getClipIcon().empty() &&
                object.getAmmoIcon().empty(), "weapon non-serialized icon defaults");
    ok &= check(object.getHasFireRate() && object.getHasMag() &&
                !object.getHasRangeFinder() && !object.getSwitchGuiOnCamera() &&
                object.getShowOnCamMode(1) && object.getShowAmmo() && object.getShowClips(),
                "weapon boolean/camera defaults");
    ok &= check(object.getDeviationFactor() == 10 && object.getMaxVisualDeviation() == 1000 &&
                object.getMinVisualDeviation() == 0 &&
                object.getEnablePostProcessingOnGuiIndex() == -1 && object.getShowInfo() &&
                !object.getDisableOnSprint() && !object.getDisplaySelectOnActivation() &&
                object.getFirstLockSound().empty() && object.getSecondLockSound().empty() &&
                object.getOverheatSound().empty(), "weapon remaining defaults");
    ok &= checkScript(object, stream, "",
                      "weapon default full write", "weapon default script bytes",
                      "weapon default one-write contract");

    object.setWeaponIcon("weapon");
    object.setAltWeaponIcon("alt-weapon");
    object.setSelectIcon("select");
    object.setSpecialAbilityIcon("special");
    object.setCrosshairIcon("cross");
    object.setCrosshairIconSize(40);
    object.setAltCrosshairIcon("alt-cross");
    object.setAmmoBarFrontIcon("bar-front");
    object.setAmmoBarBackIcon("bar-back");
    object.setClipIcon("clip");
    object.setAmmoIcon("ammo-icon");
    object.setHudSid("weapon-hud");
    object.setGuiIndex(8);
    object.setAltGuiIndex(9);
    object.setDeviationFactor(11);
    object.setMaxVisualDeviation(999);
    object.setMinVisualDeviation(1);
    object.setHasMag(false);
    object.setHasFireRate(false);
    object.setHasRangeFinder(true);
    object.setSwitchGuiOnCamera(true);
    object.setShowOnCamMode(2);
    object.addShowOnCamMode(3);
    object.setEnableMouse(true);
    object.setShowAmmo(false);
    object.setShowClips(false);
    object.setShowInfo(false);
    object.setDisableOnSprint(true);
    object.setDisplaySelectOnActivation(true);
    object.setEnablePostProcessingOnGuiIndex(10);
    object.setFirstLockSound("lock-one");
    object.setSecondLockSound("lock-two");
    object.setOverheatSound("hot");

    ok &= check(object.getHudName() == "weapon-hud" &&
                object.getHudSid() == "weapon-hud" &&
                object.getAmmoBarFrontIcon() == "bar-front" &&
                object.getAmmoBarBackIcon() == "bar-back" &&
                object.getClipIcon() == "clip" && object.getAmmoIcon() == "ammo-icon",
                "weapon string mutations/by-value getters");

    const std::string expected =
        "ObjectTemplate.weaponHud.weaponIcon weapon\n"
        "ObjectTemplate.weaponHud.altWeaponIcon alt-weapon\n"
        "ObjectTemplate.weaponHud.selectIcon select\n"
        "ObjectTemplate.weaponHud.specialAbilityIcon special\n"
        "ObjectTemplate.weaponHud.crosshairIcon cross\n"
        "ObjectTemplate.weaponHud.crosshairIconSize 40\n"
        "ObjectTemplate.weaponHud.altCrosshairIcon alt-cross\n"
        "ObjectTemplate.weaponHud.hudName \"weapon-hud\"\n"
        "ObjectTemplate.weaponHud.guiIndex 8\n"
        "ObjectTemplate.weaponHud.altGuiIndex 9\n"
        "ObjectTemplate.weaponHud.deviationFactor 11\n"
        "ObjectTemplate.weaponHud.maxVisualDeviation 999\n"
        "ObjectTemplate.weaponHud.minVisualDeviation 1\n"
        "ObjectTemplate.weaponHud.hasMag 0\n"
        "ObjectTemplate.weaponHud.hasFireRate 0\n"
        "ObjectTemplate.weaponHud.hasRangeFinder 1\n"
        "ObjectTemplate.weaponHud.switchGuiOnCamera 1\n"
        "ObjectTemplate.weaponHud.showOnCamMode 2\n"
        "ObjectTemplate.weaponHud.addShowOnCamMode 3\n"
        "ObjectTemplate.weaponHud.enableMouse 1\n"
        "ObjectTemplate.weaponHud.showAmmo 0\n"
        "ObjectTemplate.weaponHud.showClips 0\n"
        "ObjectTemplate.weaponHud.showInfo 0\n"
        "ObjectTemplate.weaponHud.disableOnSprint 1\n"
        "ObjectTemplate.weaponHud.displaySelectOnActivation 1\n"
        "ObjectTemplate.weaponHud.enablePostProcessingOnGuiIndex 10\n"
        "ObjectTemplate.weaponHud.firstLockSound lock-one\n"
        "ObjectTemplate.weaponHud.secondLockSound lock-two\n"
        "ObjectTemplate.weaponHud.overheatSound hot\n";
    ok &= checkScript(object, stream, expected,
                      "weapon full write", "weapon script bytes/order/omissions",
                      "weapon one-write contract");
    ok &= check(stream.text().find("bar-front") == std::string::npos &&
                stream.text().find("bar-back") == std::string::npos &&
                stream.text().find("ammo-icon") == std::string::npos,
                "weapon non-serialized icon omission");
    ok &= checkShortWrite(object, stream, expected,
                          "weapon short write result", "weapon short write bytes");

    IObjectComponent* base = object.createComponent();
    ok &= check(!object.needsBuild(), "weapon clears needs-build");
    checkComponentIdentity(base, IID_IWeaponHudComp, "weapon component", ok);
    if (base)
    {
        IWeaponHudComp* component = static_cast<IWeaponHudComp*>(
            base->queryInterface(IID_IWeaponHudComp));
        ok &= check(component != 0, "weapon concrete QI");
        if (component)
        {
            WeaponHudInfo* info = component->getHudInfo();
            WeaponHudIcons* icons = component->getHudIcons();
            ok &= check(info && info->guiIndex == 8 && info->altGuiIndex == 9 &&
                        info->displaySelectOnActivation &&
                        info->enablePostProcessingOnGuiIndex == 10,
                        "weapon runtime forwards HUD info");
            ok &= check(icons && icons->weaponIcon == "weapon" &&
                        icons->altWeaponIcon == "alt-weapon" &&
                        icons->crosshairIcon == "cross" &&
                        icons->altCrosshairIcon == "alt-cross",
                        "weapon runtime forwards HUD icons");
            ok &= check(component->getGuiIndex(true) == 0,
                        "weapon display-selection early branch");
            object.setDisplaySelectOnActivation(false);
            object.setAltGuiIndex(-1);
            ok &= check(component->getGuiIndex(false) == 8,
                        "weapon no-alt GUI early branch");
            object.setAltGuiIndex(9);
            object.setSwitchGuiOnCamera(false);
            ok &= check(component->getGuiIndex(false) == 8,
                        "weapon null-object primary GUI branch");
            ok &= check(component->getCrossHairIcon() == "cross",
                        "weapon null-object primary crosshair branch");
            ok &= check(component->getShowOnCamMode(2),
                        "weapon authored camera-mode early branch");
            ok &= check(static_cast<int>(component->getLockState()) == 0,
                        "weapon lock-state default");
            component->setLockState(static_cast<WarningState>(7));
            component->setLockState(static_cast<WarningState>(7));
            ok &= check(static_cast<int>(component->getLockState()) == 7,
                        "weapon non-audible lock-state store/repeat branch");
        }
        base->release();
    }
    base = object.createComponent();
    ok &= check(!object.needsBuild(), "weapon stays built after second component");
    if (base)
        base->release();

    checkGenericFactory<WeaponHudCompTemplate>(IID_IWeaponHudCompTemplate,
                                                "weapon factory outer", ok);
    return ok;
}

} // namespace

int main()
{
    CaptureStream stream;
    bool ok = true;
    ok &= testPostProcess(stream);
    ok &= testAbility(stream);
    ok &= testVehicle(stream);
    ok &= testWeapon(stream);
    ok &= check(g_rootParentCalls == 0,
                "safe runtime branches unexpectedly traversed getRootParent seam");

    std::puts(ok ? "hud_four_component_runtime: PASS" :
                   "hud_four_component_runtime: FAIL");
    return ok ? 0 : 1;
}
