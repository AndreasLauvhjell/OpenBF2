// Warning-HUD configuration and runtime sound state.

#include "WarningHudCompTemplate.h"

#include "../../Core/Ids.h"
#include "../../Sound/BfSound.h"     // g_bfSound - the warning sounds
#include "../../IO/Stream/IStream.h"

#include <sstream>

namespace dice {
namespace hfe {
namespace world {


// .  The template is borrowed.  m_object is
// deliberately untouched until init() or setObject(), as in both shipped constructors.
WarningHudComp::WarningHudComp(WarningHudCompTemplate* componentTemplate)
    : m_refCount(1)
    , m_componentTemplate(componentTemplate)
    , m_warningState(static_cast<WarningState>(0))
{
}

WarningHudComp::~WarningHudComp()
{
}

void WarningHudComp::addRef()
{
    ++m_refCount;
}

int WarningHudComp::getRef()
{
    return m_refCount;
}

int WarningHudComp::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}

void* WarningHudComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent || iid == IID_IWarningHudComp)
        return const_cast<WarningHudComp*>(this);
    return 0;
}

unsigned int WarningHudComp::getInterfaceID() const
{
    return IID_IWarningHudComp;
}

void WarningHudComp::init(IObject* object)
{
    m_object = object;
}

void WarningHudComp::setObject(IObject* object)
{
    m_object = object;
}

IObject* WarningHudComp::getObject() const
{
    return m_object;
}

bool WarningHudComp::hasUpdate() const
{
    return false;
}

void WarningHudComp::handleUpdate(float)
{
}

// Preserves warning transition order, including repeated sound stops before the state update.
void WarningHudComp::setWarningState(WarningState state)
{
    const int next = static_cast<int>(state);
    const int current = static_cast<int>(m_warningState);

    if (next == 0 && current != 0)
    {
        g_bfSound->stopSound(m_componentTemplate->m_hudInfo.firstWarningSound);
        g_bfSound->stopSound(m_componentTemplate->m_hudInfo.secondWarningSound);
    }
    else if (next == 2 && current != 2)
    {
        g_bfSound->stopSound(m_componentTemplate->m_hudInfo.firstWarningSound);
        g_bfSound->playSound(m_componentTemplate->m_hudInfo.secondWarningSound);
    }
    else if (next == 1 && current != 1)
    {
        g_bfSound->stopSound(m_componentTemplate->m_hudInfo.secondWarningSound);
        g_bfSound->playSound(m_componentTemplate->m_hudInfo.firstWarningSound);
    }

    m_warningState = state;
}

WarningState WarningHudComp::getWarningState()
{
    return m_warningState;
}

WarningHudInfo* WarningHudComp::getHudInfo()
{
    return &m_componentTemplate->m_hudInfo;
}

std::string* WarningHudComp::getHudIcons()
{
    return &m_componentTemplate->m_warningIcon;
}

// .  All five strings are default-constructed before
// this body.  Retail then assigns empty only to the three authored strings and leaves
// the trailing dword untouched.
WarningHudCompTemplate::WarningHudCompTemplate()
    : m_refCount(1)
    , m_firstComponentPending(true)
{
    m_hudInfo.warningType = 1;
    m_hudInfo.firstWarningSound.assign("");
    m_hudInfo.secondWarningSound.assign("");
    m_warningIcon.assign("");
}

WarningHudCompTemplate::~WarningHudCompTemplate()
{
}

void WarningHudCompTemplate::addRef()
{
    ++m_refCount;
}

int WarningHudCompTemplate::getRef()
{
    return m_refCount;
}

int WarningHudCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        delete this;
    return remaining;
}

void* WarningHudCompTemplate::queryInterface(unsigned int iid) const
{
    WarningHudCompTemplate* self = const_cast<WarningHudCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent ||
        iid == IID_IWarningHudCompTemplate)
    {
        return self;
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int WarningHudCompTemplate::getClassID() const
{
    return CID_WarningHudCompTemplate;
}

unsigned int WarningHudCompTemplate::getInterfaceID()
{
    return IID_IWarningHudCompTemplate;
}

IObjectComponent* WarningHudCompTemplate::createComponent()
{
    if (m_firstComponentPending)
        m_firstComponentPending = false;
    return new WarningHudComp(this);
}

void WarningHudCompTemplate::objectCreated(IObject*)
{
}

void WarningHudCompTemplate::objectDestroyed(IObject*)
{
}

void WarningHudCompTemplate::setTemplate(IObjectTemplate*)
{
}

void* WarningHudCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

void WarningHudCompTemplate::setWarningType(int warningType)
{
    m_hudInfo.warningType = warningType;
}

int WarningHudCompTemplate::getWarningType()
{
    return m_hudInfo.warningType;
}

void WarningHudCompTemplate::setFirstWarningSound(std::string sound)
{
    m_hudInfo.firstWarningSound = sound;
}

std::string WarningHudCompTemplate::getFirstWarningSound()
{
    return m_hudInfo.firstWarningSound;
}

void WarningHudCompTemplate::setSecondWarningSound(std::string sound)
{
    m_hudInfo.secondWarningSound = sound;
}

std::string WarningHudCompTemplate::getSecondWarningSound()
{
    return m_hudInfo.secondWarningSound;
}

void WarningHudCompTemplate::setWarningIcon(std::string icon)
{
    m_warningIcon = icon;
}

std::string WarningHudCompTemplate::getWarningIcon()
{
    return m_warningIcon;
}

// .  Both ports read members directly (zero virtual
// getter calls), emit in this order, materialize once and perform one stream write.
bool WarningHudCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (m_hudInfo.warningType != 1)
        script << "ObjectTemplate.WarningHud.warningType "
               << m_hudInfo.warningType << std::endl;
    if (!m_hudInfo.firstWarningSound.empty())
        script << "ObjectTemplate.WarningHud.firstWarningSound "
               << m_hudInfo.firstWarningSound << std::endl;
    if (!m_hudInfo.secondWarningSound.empty())
        script << "ObjectTemplate.WarningHud.secondWarningSound "
               << m_hudInfo.secondWarningSound << std::endl;
    if (!m_warningIcon.empty())
        script << "ObjectTemplate.WarningHud.warningIcon "
               << m_warningIcon << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

#if defined(_MSC_VER) && defined(_M_IX86)
// No STL-containing retail-size assertion: the rebuild's modern MSVC std::string
// layout intentionally differs from BF2.exe's 28-byte string.  This POD-only size is stable.
static_assert(sizeof(WarningHudComp) == 0x14, "BF2.exe WarningHudComp size");
#elif defined(__i386__)
static_assert(sizeof(WarningHudComp) == 0x14, "32-bit ELF WarningHudComp size");
#endif

} // namespace world
} // namespace hfe
} // namespace dice
