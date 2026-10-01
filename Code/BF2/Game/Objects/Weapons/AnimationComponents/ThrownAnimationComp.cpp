// Path:     [INFERRED] from the AnimationComponents symbol cluster in the BF2 1.50 Linux ELF.
// Contents: complete ThrownAnimationComp template/runtime family recovered from ELF
// ..0x827BF1A and cross-checked against the Win32 PE trigger call slots.

#include "ThrownAnimationComp.h"

#include <sstream>
#include <string>

#include "../../../../Animation/BoneAnimation/AnimationSystem.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

ThrownAnimationComp::ThrownAnimationComp(ThrownAnimationCompTemplate* objectTemplate)
    : AnimationComp(objectTemplate)
    , m_thrownRefCount(1)
    , m_throwStarted(false)
    , m_thrownTemplate(objectTemplate)
{
}

ThrownAnimationComp::~ThrownAnimationComp()
{
}

void ThrownAnimationComp::addRef() { ++m_thrownRefCount; }
int ThrownAnimationComp::getRef() { return m_thrownRefCount; }

int ThrownAnimationComp::release()
{
    const int remaining = --m_thrownRefCount;
    if (remaining == 0 && this)
        delete this;
    return remaining;
}

void* ThrownAnimationComp::queryInterface(unsigned int iid) const
{
    ThrownAnimationComp* const self = const_cast<ThrownAnimationComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponAnimationComp)
        return static_cast<IWeaponAnimationComp*>(self);
    return 0;
}

unsigned int ThrownAnimationComp::getInterfaceID() const
{
    return IID_IWeaponAnimationComp;
}

void ThrownAnimationComp::updateOnFiring(FireState state)
{
    if (!m_activeAnimationSystem)
        return;

    if (static_cast<int>(state) == 2)
    {
        if (!m_throwStarted)
            m_activeAnimationSystem->setTriggerOnceMessage(16);
        m_throwStarted = true;
    }

    if (m_throwStarted)
        m_activeAnimationSystem->clearTriggerMessage(8);
    else
        m_activeAnimationSystem->setTriggerMessage(8);
}

void ThrownAnimationComp::postFire(WeaponFireEvent& event)
{
    if (!m_activeAnimationSystem)
        return;

    // / client PE 0x60C750: only an ordered value below the
    // threshold selects message 4. An unordered value takes message 1 too.
    m_activeAnimationSystem->setTriggerOnceMessage(
        event.fireStrength < 0.050000001f ? 4u : 1u);
}

void ThrownAnimationComp::reload(ReloadState state)
{
    if (m_activeAnimationSystem && static_cast<int>(state) != 0)
    {
        m_activeAnimationSystem->disable(false);
        m_activeAnimationSystem->setTriggerOnceMessage(2);
        m_throwStarted = false;
    }
}

void ThrownAnimationComp::enable()
{
    m_throwStarted = false;
}

void ThrownAnimationComp::disable()
{
    if (m_activeAnimationSystem)
        m_activeAnimationSystem->disable(false);
    m_activeAnimationSystem = 0;
}

bool ThrownAnimationComp::getSignalDelayShiftStarted() const
{
    return false;
}

void ThrownAnimationComp::updateOnZooming(int)
{
}

void ThrownAnimationComp::setAmmoLeft(int)
{
}

ThrownAnimationCompTemplate::ThrownAnimationCompTemplate()
    : AnimationCompTemplate()
    , m_refCount(1)
{
}

void ThrownAnimationCompTemplate::addRef() { ++m_refCount; }
int ThrownAnimationCompTemplate::getRef() { return m_refCount; }

int ThrownAnimationCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        operator delete(this);
    return remaining;
}

void* ThrownAnimationCompTemplate::queryInterface(unsigned int iid) const
{
    ThrownAnimationCompTemplate* const self =
        const_cast<ThrownAnimationCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IWeaponCompTemplate)
        return static_cast<IWeaponCompTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int ThrownAnimationCompTemplate::getClassID() const
{
    return CID_ThrownAnimationCompTemplate;
}

IObjectComponent* ThrownAnimationCompTemplate::createComponent()
{
    return new ThrownAnimationComp(this);
}

void* ThrownAnimationCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ThrownAnimationCompTemplate || classId == CID_AnimationCompTemplate)
        return this;
    return 0;
}

bool ThrownAnimationCompTemplate::makeScript(io::IStream* stream)
{
    if (!AnimationCompTemplate::makeScript(stream))
        return false;

    std::ostringstream script;
    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
