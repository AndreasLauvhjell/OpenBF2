// [TEMP] Focused native fixture for the recovered Warning/Help HUD component families.
// It verifies serialization, by-value getter overrides, runtime selection/state and
// the shipped makeScript direct-member-read behavior.  Not part of a BF2 image.

#include "../BF2/Game/Objects/WarningHudCompTemplate.h"
#include "../BF2/Game/Objects/HelpHudCompTemplate.h"
#include "../BF2/Core/Ids.h"
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

namespace {

class CaptureStream : public dice::hfe::io::IStream
{
public:
    CaptureStream() : m_refCount(1), m_shortWrite(false) {}

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
    }

    const std::string& text() const { return m_text; }

private:
    int m_refCount;
    bool m_shortWrite;
    std::string m_text;
};

class WarningGetterProbe : public dice::hfe::world::WarningHudCompTemplate
{
public:
    WarningGetterProbe() : getterCalls(0) {}

    virtual int getWarningType() { ++getterCalls; return -99; }
    virtual std::string getFirstWarningSound() { ++getterCalls; return "override-first"; }
    virtual std::string getSecondWarningSound() { ++getterCalls; return "override-second"; }
    virtual std::string getWarningIcon() { ++getterCalls; return "override-icon"; }

    int getterCalls;
};

class HelpGetterProbe : public dice::hfe::world::HelpHudCompTemplate
{
public:
    HelpGetterProbe() : getterCalls(0) {}

    virtual std::string getHelpStringKey() { return hit(); }
    virtual std::string getHelpSoundKey() { return hit(); }
    virtual std::string getAltHelpStringKey() { return hit(); }
    virtual std::string getAltHelpSoundKey() { return hit(); }
    virtual std::string getLowArmorHelpSoundKey() { return hit(); }
    virtual std::string getLowArmorHelpStringKey() { return hit(); }
    virtual std::string getLowAmmoHelpSoundKey() { return hit(); }
    virtual std::string getLowAmmoHelpStringKey() { return hit(); }
    virtual std::string getCloseToHelpStringKeyNeutral() { return hit(); }
    virtual std::string getCloseToHelpSoundKeyNeutral() { return hit(); }
    virtual std::string getCloseToHelpStringKeyFriendly() { return hit(); }
    virtual std::string getCloseToHelpSoundKeyFriendly() { return hit(); }
    virtual std::string getCloseToHelpStringKeyEnemy() { return hit(); }
    virtual std::string getCloseToHelpSoundKeyEnemy() { return hit(); }
    virtual int getCloseToDistance() { ++getterCalls; return -99; }

    int getterCalls;

private:
    std::string hit() { ++getterCalls; return "override"; }
};

bool check(bool condition, const char* what)
{
    if (!condition)
        std::fprintf(stderr, "warning_help_hudcomp_runtime: %s\n", what);
    return condition;
}

} // namespace

int main()
{
    using namespace dice::hfe;
    using namespace dice::hfe::world;

    bool ok = true;
    CaptureStream stream;

    WarningGetterProbe warning;
    ok &= check(warning.getRef() == 1, "warning initial refcount");
    warning.addRef();
    ok &= check(warning.release() == 1, "warning refcount round trip");
    ok &= check(warning.queryInterface(IID_IWarningHudCompTemplate) ==
                    static_cast<IWarningHudCompTemplate*>(&warning),
                "warning template interface");
    ok &= check(warning.queryInterface(io::IID_IConsoleSaveable) ==
                    static_cast<io::IConsoleSaveable*>(&warning),
                "warning saveable adjustment");
    warning.setWarningType(2);
    warning.setFirstWarningSound("first");
    warning.setSecondWarningSound("second");
    warning.setWarningIcon("icon");
    ok &= check(warning.makeScript(&stream), "warning full write");
    ok &= check(stream.text() ==
                    "ObjectTemplate.WarningHud.warningType 2\n"
                    "ObjectTemplate.WarningHud.firstWarningSound first\n"
                    "ObjectTemplate.WarningHud.secondWarningSound second\n"
                    "ObjectTemplate.WarningHud.warningIcon icon\n",
                "warning script bytes/order");
    ok &= check(warning.getterCalls == 0, "warning makeScript must not dispatch getters");
    stream.reset(true);
    ok &= check(!warning.makeScript(&stream), "warning short write");

    IObjectComponent* warningBase = warning.createComponent();
    IWarningHudComp* warningComp = static_cast<IWarningHudComp*>(
        warningBase->queryInterface(IID_IWarningHudComp));
    ok &= check(warningComp != 0, "warning runtime interface");
    ok &= check(warningComp->getHudInfo()->warningType == 2,
                "warning runtime borrowed info");
    ok &= check(*warningComp->getHudIcons() == "icon", "warning runtime borrowed icon");
    warningComp->setWarningState(static_cast<WarningState>(7));
    ok &= check(static_cast<int>(warningComp->getWarningState()) == 7,
                "warning runtime state store");
    warningBase->release();

    HelpGetterProbe help;
    help.setHelpStringKey("hstr");
    help.setHelpSoundKey("hsnd");
    help.setAltHelpStringKey("astr");
    help.setAltHelpSoundKey("asnd");
    help.setLowArmorHelpSoundKey("lasnd");
    help.setLowArmorHelpStringKey("lastr");
    help.setLowAmmoHelpSoundKey("lmsnd");
    help.setLowAmmoHelpStringKey("lmstr");
    help.setCloseToHelpStringKeyNeutral("nstr");
    help.setCloseToHelpSoundKeyNeutral("nsnd");
    help.setCloseToHelpStringKeyFriendly("fstr");
    help.setCloseToHelpSoundKeyFriendly("fsnd");
    help.setCloseToHelpStringKeyEnemy("estr");
    help.setCloseToHelpSoundKeyEnemy("esnd");
    help.setCloseToDistance(25);

    stream.reset();
    ok &= check(help.makeScript(&stream), "help full write");
    ok &= check(stream.text() ==
                    "ObjectTemplate.HelpHud.helpStringKey \"hstr\"\n"
                    "ObjectTemplate.HelpHud.helpSoundKey \"hsnd\"\n"
                    "ObjectTemplate.HelpHud.altHelpStringKey \"astr\"\n"
                    "ObjectTemplate.HelpHud.altHelpSoundKey \"asnd\"\n"
                    "ObjectTemplate.HelpHud.lowArmorHelpSoundKey \"lasnd\"\n"
                    "ObjectTemplate.HelpHud.lowArmorHelpStringKey \"lastr\"\n"
                    "ObjectTemplate.HelpHud.lowAmmoHelpSoundKey \"lmsnd\"\n"
                    "ObjectTemplate.HelpHud.lowAmmoHelpStringKey \"lmstr\"\n"
                    "ObjectTemplate.HelpHud.closeToHelpStringKeyNeutral \"nstr\"\n"
                    "ObjectTemplate.HelpHud.closeToHelpSoundKeyNeutral \"nsnd\"\n"
                    "ObjectTemplate.HelpHud.closeToHelpStringKeyFriendly \"fstr\"\n"
                    "ObjectTemplate.HelpHud.closeToHelpSoundKeyFriendly \"fsnd\"\n"
                    "ObjectTemplate.HelpHud.closeToHelpStringKeyEnemy \"estr\"\n"
                    "ObjectTemplate.HelpHud.closeToHelpSoundKeyEnemy \"esnd\"\n"
                    "ObjectTemplate.HelpHud.closeToDistance 25\n",
                "help script bytes/order/quoting");
    ok &= check(help.getterCalls == 0, "help makeScript must not dispatch getters");

    IObjectComponent* helpBase = help.createComponent();
    IHelpHudComp* helpComp = static_cast<IHelpHudComp*>(helpBase->queryInterface(IID_IHelpHudComp));
    ok &= check(helpComp != 0, "help runtime interface");
    ok &= check(helpComp->getHudInfo()->helpStringKey == "hstr", "help borrowed info");
    ok &= check(helpComp->getIsCloseToActive(1, 0),
                "help neutral key setter activates category");
    ok &= check(helpComp->getIsCloseToActive(1, 1),
                "help friendly key setter activates category");
    ok &= check(helpComp->getIsCloseToActive(1, 2),
                "help enemy key setter activates category");
    helpComp->setCloseToIsActive(1, 0, false);
    helpComp->setCloseToIsActive(1, 1, false);
    helpComp->setCloseToIsActive(1, 2, false);
    ok &= check(!helpComp->getIsCloseToActive(1, 0), "help neutral clear selection");
    ok &= check(!helpComp->getIsCloseToActive(1, 1), "help friendly clear selection");
    ok &= check(!helpComp->getIsCloseToActive(1, 2), "help enemy clear selection");
    helpComp->setCloseToIsActive(1, 0, true);
    helpComp->setCloseToIsActive(1, 1, true);
    helpComp->setCloseToIsActive(1, 2, true);
    ok &= check(helpComp->getIsCloseToActive(1, 0), "help neutral selection");
    ok &= check(helpComp->getIsCloseToActive(1, 1), "help friendly selection");
    ok &= check(helpComp->getIsCloseToActive(1, 2), "help enemy selection");
    ok &= check(helpComp->getCloseToHelpStringKey(1, 0) == "nstr",
                "help neutral string selection");
    ok &= check(helpComp->getCloseToHelpSoundKey(1, 1) == "fsnd",
                "help friendly sound selection");
    ok &= check(helpComp->getCloseToHelpStringKey(1, 2) == "estr",
                "help enemy string selection");
    helpBase->release();

    std::puts(ok ? "warning_help_hudcomp_runtime: PASS" :
                   "warning_help_hudcomp_runtime: FAIL");
    return ok ? 0 : 1;
}
