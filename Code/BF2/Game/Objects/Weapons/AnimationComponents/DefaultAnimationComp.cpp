// Path:     attested - the PE Debug site names
// Game/Objects/Weapons/AnimationComponents/DefaultAnimationComp.cpp.
// Contents: complete template/runtime family recovered from ELF 0x827A620..0x827B47C and
// cross-checked against PE 0x5AACxx..0x5AB17D.

#include "DefaultAnimationComp.h"

#include <cmath>
#include <sstream>
#include <string>

#include "../../../../Animation/BoneAnimation/AnimationSystem.h"
#include "../../../../Core/Debug.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"
#include "../GenericFireArm.h"

namespace dice {
namespace hfe {
namespace world {

DefaultAnimationComp::DefaultAnimationComp(DefaultAnimationCompTemplate* objectTemplate)
    : AnimationComp(objectTemplate)
    , m_defaultRefCount(1)
    , m_defaultTemplate(objectTemplate)
    , m_shiftPending(false)
    , m_shiftDelayStartedAtTick(0)
    , m_signalDelayShiftStarted(false)
    , m_ammoLeft(0)
{
}

DefaultAnimationComp::~DefaultAnimationComp()
{
}

void DefaultAnimationComp::addRef() { ++m_defaultRefCount; }
int DefaultAnimationComp::getRef() { return m_defaultRefCount; }

int DefaultAnimationComp::release()
{
    const int remaining = m_defaultRefCount - 1;
    const bool last = m_defaultRefCount == 1;
    m_defaultRefCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* DefaultAnimationComp::queryInterface(unsigned int iid) const
{
    DefaultAnimationComp* const self = const_cast<DefaultAnimationComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponAnimationComp)
        return static_cast<IWeaponAnimationComp*>(self);
    return 0;
}

unsigned int DefaultAnimationComp::getInterfaceID() const
{
    return IID_IWeaponAnimationComp;
}

bool DefaultAnimationComp::requestFire(bool, bool)
{
    return !m_shiftPending && m_shiftDelayStartedAtTick == 0;
}

void DefaultAnimationComp::updateComponent(float, unsigned int)
{
    if (m_shiftPending && !firearm()->busy(false))
    {
        shift();
        m_shiftPending = false;
        m_shiftDelayStartedAtTick = static_cast<unsigned int>(WorldPref::getTickCount());
        m_signalDelayShiftStarted = true;
    }

    if (m_shiftDelayStartedAtTick)
    {
        const unsigned int now = static_cast<unsigned int>(WorldPref::getTickCount());
        if (now >= m_shiftDelayStartedAtTick
                 + static_cast<unsigned int>(m_defaultTemplate->getShiftDelayInTicks()))
        {
            m_shiftDelayStartedAtTick = 0;
        }
    }
}

void DefaultAnimationComp::postFire(WeaponFireEvent&)
{
    if (!m_activeAnimationSystem)
        return;

    if (m_defaultTemplate->getLoopingFire())
        m_activeAnimationSystem->setTriggerMessage(1);
    else
        m_activeAnimationSystem->setTriggerOnceMessage(1);
}

void DefaultAnimationComp::fireStop(bool alternate)
{
    if (m_activeAnimationSystem && m_defaultTemplate->getLoopingFire() && !alternate)
        m_activeAnimationSystem->clearTriggerMessage(1);
}

void DefaultAnimationComp::fireComplete(unsigned int)
{
    if (m_ammoLeft > 0 && m_defaultTemplate->getUseShiftAnimation())
        m_shiftPending = true;
}

void DefaultAnimationComp::shift()
{
    if (m_activeAnimationSystem)
        m_activeAnimationSystem->setTriggerOnceMessage(32);
}

void DefaultAnimationComp::reload(ReloadState state)
{
    if (!m_activeAnimationSystem)
        return;

    switch (static_cast<int>(state))
    {
    case 0:
    case 6:
        m_activeAnimationSystem->clearTriggerMessage(2);
        break;
    case 1:
    case 2:
    case 5:
        return;
    case 3:
        m_activeAnimationSystem->setTriggerOnceMessage(2);
        break;
    case 4:
        m_activeAnimationSystem->setTriggerMessage(2);
        break;
    default:
        Debug(DEBUGTYPE_6,
              "Game/Objects/Weapons/AnimationComponents/DefaultAnimationComp.cpp", 138, "")
            << "Unhandled reload state";
        break;
    }
}

bool DefaultAnimationComp::busy()
{
    return m_shiftDelayStartedAtTick != 0;
}

void DefaultAnimationComp::disable()
{
    m_activeAnimationSystem = 0;
}

void DefaultAnimationComp::setAmmoLeft(int rounds)
{
    m_ammoLeft = rounds;
}

bool DefaultAnimationComp::getSignalDelayShiftStarted() const
{
    const bool signalled = m_signalDelayShiftStarted;
    m_signalDelayShiftStarted = false;
    return signalled;
}

DefaultAnimationCompTemplate::DefaultAnimationCompTemplate()
    : AnimationCompTemplate()
    , m_refCount(1)
    , m_useShiftAnimation(false)
    , m_shiftDelay(0.0f)
    , m_shiftDelayInTicks(0)
    , m_loopingFire(false)
{
    setShiftDelay(0.0f);
}

void DefaultAnimationCompTemplate::addRef() { ++m_refCount; }
int DefaultAnimationCompTemplate::getRef() { return m_refCount; }

int DefaultAnimationCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        operator delete(this);
    return remaining;
}

void* DefaultAnimationCompTemplate::queryInterface(unsigned int iid) const
{
    DefaultAnimationCompTemplate* const self =
        const_cast<DefaultAnimationCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IWeaponCompTemplate)
        return static_cast<IWeaponCompTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int DefaultAnimationCompTemplate::getClassID() const
{
    return CID_DefaultAnimationCompTemplate;
}

IObjectComponent* DefaultAnimationCompTemplate::createComponent()
{
    return new DefaultAnimationComp(this);
}

void* DefaultAnimationCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_DefaultAnimationCompTemplate || classId == CID_AnimationCompTemplate)
        return this;
    return 0;
}

bool DefaultAnimationCompTemplate::makeScript(io::IStream* stream)
{
    if (!AnimationCompTemplate::makeScript(stream))
        return false;

    std::ostringstream script;
    if (m_useShiftAnimation)
    {
        script << "ObjectTemplate.animation.useShiftAnimation "
               << m_useShiftAnimation << std::endl;
    }
    if (std::fabs(m_shiftDelay) > 1.1920929e-7f)
        script << "ObjectTemplate.animation.shiftDelay " << m_shiftDelay << std::endl;
    if (m_loopingFire)
        script << "ObjectTemplate.animation.loopingFire " << m_loopingFire << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

void DefaultAnimationCompTemplate::setUseShiftAnimation(bool value)
{
    m_useShiftAnimation = value;
}

bool DefaultAnimationCompTemplate::getUseShiftAnimation()
{
    return m_useShiftAnimation;
}

void DefaultAnimationCompTemplate::setLoopingFire(bool value)
{
    m_loopingFire = value;
}

bool DefaultAnimationCompTemplate::getLoopingFire()
{
    return m_loopingFire;
}

void DefaultAnimationCompTemplate::setShiftDelay(float seconds)
{
    m_shiftDelay = seconds;
    m_shiftDelayInTicks = static_cast<int>(seconds * 30.0f);
}

float DefaultAnimationCompTemplate::getShiftDelay() const
{
    return m_shiftDelay;
}

int DefaultAnimationCompTemplate::getShiftDelayInTicks() const
{
    return m_shiftDelayInTicks;
}

} // namespace world
} // namespace hfe
} // namespace dice
