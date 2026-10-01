// Path: [INFERRED], beside HelpHudCompTemplate.h.
// Contents: complete template and runtime component recovered from BF2.exe 1.50 and
// cross-checked against the symbolized Linux image.

#include "HelpHudCompTemplate.h"

#include "../../Core/Ids.h"
#include "../../IO/Stream/IStream.h"

#include <sstream>

namespace dice {
namespace hfe {
namespace world {

// .  The template is borrowed and m_object is
// deliberately untouched until init() or setObject().
HelpHudComp::HelpHudComp(HelpHudCompTemplate* componentTemplate)
    : m_refCount(1)
    , m_componentTemplate(componentTemplate)
{
}

HelpHudComp::~HelpHudComp()
{
}

void HelpHudComp::addRef()
{
    ++m_refCount;
}

int HelpHudComp::getRef()
{
    return m_refCount;
}

int HelpHudComp::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}

void* HelpHudComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent || iid == IID_IHelpHudComp)
        return const_cast<HelpHudComp*>(this);
    return 0;
}

unsigned int HelpHudComp::getInterfaceID() const
{
    return IID_IHelpHudComp;
}

void HelpHudComp::init(IObject* object)
{
    m_object = object;
}

void HelpHudComp::setObject(IObject* object)
{
    m_object = object;
}

IObject* HelpHudComp::getObject() const
{
    return m_object;
}

bool HelpHudComp::hasUpdate() const
{
    return false;
}

void HelpHudComp::handleUpdate(float)
{
}

HelpHudInfo* HelpHudComp::getHudInfo()
{
    return &m_componentTemplate->m_helpHudInfo;
}

void HelpHudComp::setIsActive(bool active)
{
    m_componentTemplate->m_isActive = active;
}

void HelpHudComp::setAltIsActive(bool active)
{
    m_componentTemplate->m_altIsActive = active;
}

void HelpHudComp::setLowArmorIsActive(bool active)
{
    m_componentTemplate->m_lowArmorIsActive = active;
}

void HelpHudComp::setLowAmmoIsActive(bool active)
{
    m_componentTemplate->m_lowAmmoIsActive = active;
}

// .  The second id selects neutral when zero,
// friendly when it equals the first id, and enemy otherwise.
void HelpHudComp::setCloseToIsActive(int referenceTeam, int otherTeam, bool active)
{
    if (otherTeam == 0)
        m_componentTemplate->m_closeToNeutralIsActive = active;
    else if (otherTeam == referenceTeam)
        m_componentTemplate->m_closeToFriendlyIsActive = active;
    else
        m_componentTemplate->m_closeToEnemyIsActive = active;
}

bool HelpHudComp::getIsCloseToActive(int referenceTeam, int otherTeam)
{
    if (otherTeam == 0)
        return m_componentTemplate->m_closeToNeutralIsActive;
    if (otherTeam == referenceTeam)
        return m_componentTemplate->m_closeToFriendlyIsActive;
    return m_componentTemplate->m_closeToEnemyIsActive;
}

const std::string& HelpHudComp::getCloseToHelpStringKey(int referenceTeam, int otherTeam)
{
    if (otherTeam == 0)
        return m_componentTemplate->m_helpHudInfo.closeToHelpStringKeyNeutral;
    if (otherTeam == referenceTeam)
        return m_componentTemplate->m_helpHudInfo.closeToHelpStringKeyFriendly;
    return m_componentTemplate->m_helpHudInfo.closeToHelpStringKeyEnemy;
}

const std::string& HelpHudComp::getCloseToHelpSoundKey(int referenceTeam, int otherTeam)
{
    if (otherTeam == 0)
        return m_componentTemplate->m_helpHudInfo.closeToHelpSoundKeyNeutral;
    if (otherTeam == referenceTeam)
        return m_componentTemplate->m_helpHudInfo.closeToHelpSoundKeyFriendly;
    return m_componentTemplate->m_helpHudInfo.closeToHelpSoundKeyEnemy;
}

// .  C++ default-constructs all sixteen strings
// before the body.  Retail then assigns empty to the fourteen named strings in
// this exact order, clears the seven active bytes and distance, and does not touch
// the alignment byte at +0x4F (ELF).
HelpHudCompTemplate::HelpHudCompTemplate()
    : m_refCount(1)
    , m_firstComponentPending(true)
{
    m_helpHudInfo.helpStringKey.assign("");
    m_helpHudInfo.helpSoundKey.assign("");
    m_helpHudInfo.altHelpStringKey.assign("");
    m_helpHudInfo.altHelpSoundKey.assign("");
    m_helpHudInfo.lowArmorHelpStringKey.assign("");
    m_helpHudInfo.lowArmorHelpSoundKey.assign("");
    m_helpHudInfo.lowAmmoHelpStringKey.assign("");
    m_helpHudInfo.lowAmmoHelpSoundKey.assign("");
    m_helpHudInfo.closeToHelpStringKeyNeutral.assign("");
    m_helpHudInfo.closeToHelpStringKeyFriendly.assign("");
    m_helpHudInfo.closeToHelpStringKeyEnemy.assign("");
    m_helpHudInfo.closeToHelpSoundKeyNeutral.assign("");
    m_helpHudInfo.closeToHelpSoundKeyFriendly.assign("");
    m_helpHudInfo.closeToHelpSoundKeyEnemy.assign("");

    m_isActive = false;
    m_altIsActive = false;
    m_lowArmorIsActive = false;
    m_lowAmmoIsActive = false;
    m_closeToNeutralIsActive = false;
    m_closeToFriendlyIsActive = false;
    m_closeToEnemyIsActive = false;
    m_closeToDistance = 0;
}

HelpHudCompTemplate::~HelpHudCompTemplate()
{
}

void HelpHudCompTemplate::addRef()
{
    ++m_refCount;
}

int HelpHudCompTemplate::getRef()
{
    return m_refCount;
}

int HelpHudCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}

void* HelpHudCompTemplate::queryInterface(unsigned int iid) const
{
    HelpHudCompTemplate* self = const_cast<HelpHudCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent ||
        iid == IID_IHelpHudCompTemplate)
    {
        return self;
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int HelpHudCompTemplate::getClassID() const
{
    return CID_HelpHudCompTemplate;
}

unsigned int HelpHudCompTemplate::getInterfaceID()
{
    return IID_IHelpHudCompTemplate;
}

IObjectComponent* HelpHudCompTemplate::createComponent()
{
    if (m_firstComponentPending)
        m_firstComponentPending = false;
    return new HelpHudComp(this);
}

void HelpHudCompTemplate::objectCreated(IObject*)
{
}

void HelpHudCompTemplate::objectDestroyed(IObject*)
{
}

void HelpHudCompTemplate::setTemplate(IObjectTemplate*)
{
}

void* HelpHudCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

void HelpHudCompTemplate::setHelpStringKey(std::string key)
{
    m_helpHudInfo.helpStringKey = key;
    m_isActive = true;
}

std::string HelpHudCompTemplate::getHelpStringKey()
{
    return m_helpHudInfo.helpStringKey;
}

void HelpHudCompTemplate::setHelpSoundKey(std::string key)
{
    m_helpHudInfo.helpSoundKey = key;
    m_isActive = true;
}

std::string HelpHudCompTemplate::getHelpSoundKey()
{
    return m_helpHudInfo.helpSoundKey;
}

void HelpHudCompTemplate::setAltHelpStringKey(std::string key)
{
    m_helpHudInfo.altHelpStringKey = key;
    m_altIsActive = true;
}

std::string HelpHudCompTemplate::getAltHelpStringKey()
{
    return m_helpHudInfo.altHelpStringKey;
}

void HelpHudCompTemplate::setAltHelpSoundKey(std::string key)
{
    m_helpHudInfo.altHelpSoundKey = key;
    m_altIsActive = true;
}

std::string HelpHudCompTemplate::getAltHelpSoundKey()
{
    return m_helpHudInfo.altHelpSoundKey;
}

void HelpHudCompTemplate::setLowArmorHelpSoundKey(std::string key)
{
    m_helpHudInfo.lowArmorHelpSoundKey = key;
    m_lowArmorIsActive = true;
}

std::string HelpHudCompTemplate::getLowArmorHelpSoundKey()
{
    return m_helpHudInfo.lowArmorHelpSoundKey;
}

void HelpHudCompTemplate::setLowArmorHelpStringKey(std::string key)
{
    m_helpHudInfo.lowArmorHelpStringKey = key;
    m_lowArmorIsActive = true;
}

std::string HelpHudCompTemplate::getLowArmorHelpStringKey()
{
    return m_helpHudInfo.lowArmorHelpStringKey;
}

void HelpHudCompTemplate::setLowAmmoHelpSoundKey(std::string key)
{
    m_helpHudInfo.lowAmmoHelpSoundKey = key;
    m_lowAmmoIsActive = true;
}

std::string HelpHudCompTemplate::getLowAmmoHelpSoundKey()
{
    return m_helpHudInfo.lowAmmoHelpSoundKey;
}

void HelpHudCompTemplate::setLowAmmoHelpStringKey(std::string key)
{
    m_helpHudInfo.lowAmmoHelpStringKey = key;
    m_lowAmmoIsActive = true;
}

std::string HelpHudCompTemplate::getLowAmmoHelpStringKey()
{
    return m_helpHudInfo.lowAmmoHelpStringKey;
}

void HelpHudCompTemplate::setCloseToHelpStringKeyNeutral(std::string key)
{
    m_helpHudInfo.closeToHelpStringKeyNeutral = key;
    m_closeToNeutralIsActive = true;
}

std::string HelpHudCompTemplate::getCloseToHelpStringKeyNeutral()
{
    return m_helpHudInfo.closeToHelpStringKeyNeutral;
}

void HelpHudCompTemplate::setCloseToHelpSoundKeyNeutral(std::string key)
{
    m_helpHudInfo.closeToHelpSoundKeyNeutral = key;
    m_closeToNeutralIsActive = true;
}

std::string HelpHudCompTemplate::getCloseToHelpSoundKeyNeutral()
{
    return m_helpHudInfo.closeToHelpSoundKeyNeutral;
}

void HelpHudCompTemplate::setCloseToHelpStringKeyFriendly(std::string key)
{
    m_helpHudInfo.closeToHelpStringKeyFriendly = key;
    m_closeToFriendlyIsActive = true;
}

std::string HelpHudCompTemplate::getCloseToHelpStringKeyFriendly()
{
    return m_helpHudInfo.closeToHelpStringKeyFriendly;
}

void HelpHudCompTemplate::setCloseToHelpSoundKeyFriendly(std::string key)
{
    m_helpHudInfo.closeToHelpSoundKeyFriendly = key;
    m_closeToFriendlyIsActive = true;
}

std::string HelpHudCompTemplate::getCloseToHelpSoundKeyFriendly()
{
    return m_helpHudInfo.closeToHelpSoundKeyFriendly;
}

void HelpHudCompTemplate::setCloseToHelpStringKeyEnemy(std::string key)
{
    m_helpHudInfo.closeToHelpStringKeyEnemy = key;
    m_closeToEnemyIsActive = true;
}

std::string HelpHudCompTemplate::getCloseToHelpStringKeyEnemy()
{
    return m_helpHudInfo.closeToHelpStringKeyEnemy;
}

void HelpHudCompTemplate::setCloseToHelpSoundKeyEnemy(std::string key)
{
    m_helpHudInfo.closeToHelpSoundKeyEnemy = key;
    m_closeToEnemyIsActive = true;
}

std::string HelpHudCompTemplate::getCloseToHelpSoundKeyEnemy()
{
    return m_helpHudInfo.closeToHelpSoundKeyEnemy;
}

void HelpHudCompTemplate::setCloseToDistance(int distance)
{
    m_closeToDistance = distance;
}

int HelpHudCompTemplate::getCloseToDistance()
{
    return m_closeToDistance;
}

// .  Both bodies use direct field reads (no virtual
// getters), emit this exact order, quote every string, and write the materialized
// script once.  The distance is omitted unless strictly positive.
bool HelpHudCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!m_helpHudInfo.helpStringKey.empty())
        script << "ObjectTemplate.HelpHud.helpStringKey \""
               << m_helpHudInfo.helpStringKey << "\"" << std::endl;
    if (!m_helpHudInfo.helpSoundKey.empty())
        script << "ObjectTemplate.HelpHud.helpSoundKey \""
               << m_helpHudInfo.helpSoundKey << "\"" << std::endl;
    if (!m_helpHudInfo.altHelpStringKey.empty())
        script << "ObjectTemplate.HelpHud.altHelpStringKey \""
               << m_helpHudInfo.altHelpStringKey << "\"" << std::endl;
    if (!m_helpHudInfo.altHelpSoundKey.empty())
        script << "ObjectTemplate.HelpHud.altHelpSoundKey \""
               << m_helpHudInfo.altHelpSoundKey << "\"" << std::endl;
    if (!m_helpHudInfo.lowArmorHelpSoundKey.empty())
        script << "ObjectTemplate.HelpHud.lowArmorHelpSoundKey \""
               << m_helpHudInfo.lowArmorHelpSoundKey << "\"" << std::endl;
    if (!m_helpHudInfo.lowArmorHelpStringKey.empty())
        script << "ObjectTemplate.HelpHud.lowArmorHelpStringKey \""
               << m_helpHudInfo.lowArmorHelpStringKey << "\"" << std::endl;
    if (!m_helpHudInfo.lowAmmoHelpSoundKey.empty())
        script << "ObjectTemplate.HelpHud.lowAmmoHelpSoundKey \""
               << m_helpHudInfo.lowAmmoHelpSoundKey << "\"" << std::endl;
    if (!m_helpHudInfo.lowAmmoHelpStringKey.empty())
        script << "ObjectTemplate.HelpHud.lowAmmoHelpStringKey \""
               << m_helpHudInfo.lowAmmoHelpStringKey << "\"" << std::endl;
    if (!m_helpHudInfo.closeToHelpStringKeyNeutral.empty())
        script << "ObjectTemplate.HelpHud.closeToHelpStringKeyNeutral \""
               << m_helpHudInfo.closeToHelpStringKeyNeutral << "\"" << std::endl;
    if (!m_helpHudInfo.closeToHelpSoundKeyNeutral.empty())
        script << "ObjectTemplate.HelpHud.closeToHelpSoundKeyNeutral \""
               << m_helpHudInfo.closeToHelpSoundKeyNeutral << "\"" << std::endl;
    if (!m_helpHudInfo.closeToHelpStringKeyFriendly.empty())
        script << "ObjectTemplate.HelpHud.closeToHelpStringKeyFriendly \""
               << m_helpHudInfo.closeToHelpStringKeyFriendly << "\"" << std::endl;
    if (!m_helpHudInfo.closeToHelpSoundKeyFriendly.empty())
        script << "ObjectTemplate.HelpHud.closeToHelpSoundKeyFriendly \""
               << m_helpHudInfo.closeToHelpSoundKeyFriendly << "\"" << std::endl;
    if (!m_helpHudInfo.closeToHelpStringKeyEnemy.empty())
        script << "ObjectTemplate.HelpHud.closeToHelpStringKeyEnemy \""
               << m_helpHudInfo.closeToHelpStringKeyEnemy << "\"" << std::endl;
    if (!m_helpHudInfo.closeToHelpSoundKeyEnemy.empty())
        script << "ObjectTemplate.HelpHud.closeToHelpSoundKeyEnemy \""
               << m_helpHudInfo.closeToHelpSoundKeyEnemy << "\"" << std::endl;
    if (m_closeToDistance > 0)
        script << "ObjectTemplate.HelpHud.closeToDistance "
               << m_closeToDistance << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

#if defined(_MSC_VER) && defined(_M_IX86)
// The template contains std::strings and therefore cannot honestly assert BF2.exe's
// old 28-byte-string allocation size with the rebuild's modern MSVC STL.
static_assert(sizeof(HelpHudComp) == 0x10, "BF2.exe HelpHudComp size");
#elif defined(__i386__)
static_assert(sizeof(HelpHudComp) == 0x10, "32-bit ELF HelpHudComp size");
#endif

} // namespace world
} // namespace hfe
} // namespace dice
