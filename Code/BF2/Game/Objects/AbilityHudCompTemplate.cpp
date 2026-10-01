// Path: [INFERRED], beside AbilityHudCompTemplate.h.
// Contents: complete template and runtime component recovered from the symbolized Linux ELF
// (template 0x8225230..0x8225DD0, runtime 0x8224CB0..0x8225220) and cross-checked against
// BF2.exe (template vtables 0x906518/0x906500, runtime vtable 0x90BAA0).

#include "AbilityHudCompTemplate.h"

#include "IAbilityHudComp.h"
#include "../../Core/Ids.h"
#include "../../Sound/BfSound.h"     // g_bfSound - the ability sounds
#include "../../IO/Stream/IStream.h"

#include <sstream>
#include <string>

namespace dice {
namespace hfe {
namespace world {

// Ability sounds are played through BfSound.

// ELF ctor 0x8224CB0 / PE ctor 0x68AB50. The template is borrowed and the object pointer is
// deliberately untouched until init() or setObject(), exactly as in both shipped constructors.
class AbilityHudComp : public IAbilityHudComp
{
public:
    explicit AbilityHudComp(AbilityHudCompTemplate* componentTemplate);

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getInterfaceID() const;
    virtual void init(IObject*);
    virtual void setObject(IObject*);
    virtual IObject* getObject() const;
    virtual bool hasUpdate() const;
    virtual void handleUpdate(float);
    virtual AbilityHudInfo* getHudInfo();
    virtual void setHealingActive(bool);
    virtual void setRepairingActive(bool);
    virtual void setAmmoActive(bool);
    virtual void setGetHealActive(bool);
    virtual void setGetRepairActive(bool);
    virtual void setGetAmmoActive(bool);
    virtual bool isPlayingHealSound() const;
    virtual bool isPlayingRepairSound() const;
    virtual bool isPlayingAmmoSound() const;
    virtual void forceStopAllSound();
    virtual ~AbilityHudComp();

private:
    int m_refCount;                              // +0x04
    AbilityHudCompTemplate* m_template;          // +0x08 borrowed
    IObject* m_object;                           // +0x0C ctor deliberately leaves indeterminate
    bool m_isPlayingHealSound;                   // +0x10
    bool m_isPlayingRepairSound;                 // +0x11
    bool m_isPlayingAmmoSound;                   // +0x12
    bool m_getHealActive;                        // +0x13
    bool m_getRepairActive;                      // +0x14
    bool m_getAmmoActive;                        // +0x15
};

AbilityHudComp::AbilityHudComp(AbilityHudCompTemplate* componentTemplate)
    : m_refCount(1)
    , m_template(componentTemplate)
    , m_isPlayingHealSound(false)
    , m_isPlayingRepairSound(false)
    , m_isPlayingAmmoSound(false)
    , m_getHealActive(false)
    , m_getRepairActive(false)
    , m_getAmmoActive(false)
{
}

AbilityHudComp::~AbilityHudComp()
{
}

void AbilityHudComp::addRef()
{
    ++m_refCount;
}

int AbilityHudComp::getRef()
{
    return m_refCount;
}

int AbilityHudComp::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}

void* AbilityHudComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent || iid == IID_IAbilityHudComp)
        return const_cast<AbilityHudComp*>(this);
    return 0;
}

unsigned int AbilityHudComp::getInterfaceID() const
{
    return IID_IAbilityHudComp;
}

void AbilityHudComp::init(IObject* object)
{
    m_object = object;
}

void AbilityHudComp::setObject(IObject* object)
{
    m_object = object;
}

IObject* AbilityHudComp::getObject() const
{
    return m_object;
}

// Named overrides in both ports rather than inherited IObjectComponent defaults.
bool AbilityHudComp::hasUpdate() const
{
    return false;
}

void AbilityHudComp::handleUpdate(float)
{
}

AbilityHudInfo* AbilityHudComp::getHudInfo()
{
    // Borrowed block beginning at the first authored string; +0x10 in both 32-bit ports.
    return reinterpret_cast<AbilityHudInfo*>(&m_template->m_healingSound);
}

void AbilityHudComp::setHealingActive(bool active)
{
    if (active)
    {
        if (!m_isPlayingHealSound)
        {
            g_bfSound->playSound(m_template->m_healingSound);
            m_isPlayingHealSound = true;
        }
    }
    else if (m_isPlayingHealSound)
    {
        g_bfSound->stopSound(m_template->m_healingSound);
        m_isPlayingHealSound = false;
    }
}

void AbilityHudComp::setRepairingActive(bool active)
{
    if (active)
    {
        if (!m_isPlayingRepairSound)
        {
            g_bfSound->playSound(m_template->m_repairingSound);
            m_isPlayingRepairSound = true;
        }
    }
    else if (m_isPlayingRepairSound)
    {
        g_bfSound->stopSound(m_template->m_repairingSound);
        m_isPlayingRepairSound = false;
    }
}

void AbilityHudComp::setAmmoActive(bool active)
{
    if (active)
    {
        if (!m_isPlayingAmmoSound)
        {
            g_bfSound->playSound(m_template->m_ammoSound);
            m_isPlayingAmmoSound = true;
        }
    }
    else if (m_isPlayingAmmoSound)
    {
        g_bfSound->stopSound(m_template->m_ammoSound);
        m_isPlayingAmmoSound = false;
    }
}

void AbilityHudComp::setGetHealActive(bool active)
{
    if (active)
    {
        if (!m_getHealActive)
        {
            g_bfSound->playSound(m_template->m_healingSound);
            m_getHealActive = true;
        }
    }
    else if (m_getHealActive)
    {
        g_bfSound->stopSound(m_template->m_healingSound);
        m_getHealActive = false;
    }
}

void AbilityHudComp::setGetRepairActive(bool active)
{
    if (active)
    {
        if (!m_getRepairActive)
        {
            g_bfSound->playSound(m_template->m_repairingSound);
            m_getRepairActive = true;
        }
    }
    else if (m_getRepairActive)
    {
        g_bfSound->stopSound(m_template->m_repairingSound);
        m_getRepairActive = false;
    }
}

void AbilityHudComp::setGetAmmoActive(bool active)
{
    if (active)
    {
        if (!m_getAmmoActive)
        {
            g_bfSound->playSound(m_template->m_ammoSound);
            m_getAmmoActive = true;
        }
    }
    else if (m_getAmmoActive)
    {
        g_bfSound->stopSound(m_template->m_ammoSound);
        m_getAmmoActive = false;
    }
}

bool AbilityHudComp::isPlayingHealSound() const
{
    return m_isPlayingHealSound;
}

bool AbilityHudComp::isPlayingRepairSound() const
{
    return m_isPlayingRepairSound;
}

bool AbilityHudComp::isPlayingAmmoSound() const
{
    return m_isPlayingAmmoSound;
}

// . This is deliberately unconditional and deliberately clears only
// bytes +0x11, +0x13 and +0x15 in that order. The asymmetric shipped state mutation is preserved.
void AbilityHudComp::forceStopAllSound()
{
    g_bfSound->stopSound(m_template->m_repairingSound);
    m_isPlayingRepairSound = false;

    g_bfSound->stopSound(m_template->m_healingSound);
    m_getHealActive = false;

    g_bfSound->stopSound(m_template->m_ammoSound);
    m_getAmmoActive = false;
}

AbilityHudCompTemplate::AbilityHudCompTemplate()
    : m_refCount(1)
    , m_needsBuild(true)
{
    // The shipped ctor first default-constructs all five strings, then assigns empty to these
    // three authored strings. _padD and the last two string purposes remain untouched/unnamed.
    m_healingSound.assign("");
    m_repairingSound.assign("");
    m_ammoSound.assign("");
}

AbilityHudCompTemplate::~AbilityHudCompTemplate()
{
}

void AbilityHudCompTemplate::addRef()
{
    ++m_refCount;
}

int AbilityHudCompTemplate::getRef()
{
    return m_refCount;
}

int AbilityHudCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}

void* AbilityHudCompTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent ||
        iid == IID_IAbilityHudCompTemplate)
    {
        return const_cast<AbilityHudCompTemplate*>(this);
    }
    if (iid == io::IID_IConsoleSaveable)
    {
        return static_cast<io::IConsoleSaveable*>(
            const_cast<AbilityHudCompTemplate*>(this));
    }
    return 0;
}

unsigned int AbilityHudCompTemplate::getClassID() const
{
    return CID_AbilityHudCompTemplate;
}

unsigned int AbilityHudCompTemplate::getInterfaceID()
{
    return IID_IAbilityHudCompTemplate;
}

IObjectComponent* AbilityHudCompTemplate::createComponent()
{
    if (m_needsBuild)
        m_needsBuild = false;
    return new AbilityHudComp(this);
}

void AbilityHudCompTemplate::objectCreated(IObject*)
{
}

void AbilityHudCompTemplate::objectDestroyed(IObject*)
{
}

void AbilityHudCompTemplate::setTemplate(IObjectTemplate*)
{
}

void* AbilityHudCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

void AbilityHudCompTemplate::setHealingSound(std::string value)
{
    m_healingSound = value;
}

std::string AbilityHudCompTemplate::getHealingSound()
{
    return m_healingSound;
}

void AbilityHudCompTemplate::setRepairingSound(std::string value)
{
    m_repairingSound = value;
}

std::string AbilityHudCompTemplate::getRepairingSound()
{
    return m_repairingSound;
}

void AbilityHudCompTemplate::setAmmoSound(std::string value)
{
    m_ammoSound = value;
}

std::string AbilityHudCompTemplate::getAmmoSound()
{
    return m_ammoSound;
}

// emit nonempty sounds in repairing/ammo/healing order, then one write.
bool AbilityHudCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!m_repairingSound.empty())
        script << "ObjectTemplate.AbilityHud.repairingSound " << m_repairingSound << std::endl;
    if (!m_ammoSound.empty())
        script << "ObjectTemplate.AbilityHud.ammoSound " << m_ammoSound << std::endl;
    if (!m_healingSound.empty())
        script << "ObjectTemplate.AbilityHud.healingSound " << m_healingSound << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(AbilityHudComp) == 0x18, "BF2.exe AbilityHudComp size");
#elif defined(__i386__)
static_assert(sizeof(AbilityHudComp) == 0x18, "32-bit ELF AbilityHudComp size");
#endif

} // namespace world
} // namespace hfe
} // namespace dice
