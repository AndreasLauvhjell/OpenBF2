// Path:     [INFERRED] beside the attested DefaultAnimationComp.cpp translation unit.
// Contents: complete AnimationComp and AnimationCompTemplate bases recovered from ELF
// ..0x827A61C and cross-checked against the PE template writer/call slots.

#include "AnimationComp.h"

#include <sstream>
#include <string>

#include "../../../../Animation/BoneAnimation/AnimationSystem.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

IWeaponAnimationComp::~IWeaponAnimationComp()
{
}

AnimationComp::AnimationComp(AnimationCompTemplate* objectTemplate)
    : m_activeAnimationSystem(0)
    , m_animationTemplate(objectTemplate)
{
    // WeaponComp has no emitted constructor.  Retail seeds only the primary count here and leaves
    // both attachment pointers untouched until Object::setComponent calls setObject/init.
    WeaponComp::m_refCount = 1;
}

AnimationComp::~AnimationComp()
{
}

unsigned int AnimationComp::getInterfaceID() const { return IID_IWeaponAnimationComp; }
void AnimationComp::initComponent(WeaponInitEvent&) {}
bool AnimationComp::preUpdate() { return false; }
void AnimationComp::updateComponent(float, unsigned int) {}
bool AnimationComp::isSelectable() const { return true; }
void AnimationComp::fireStop(bool) {}
bool AnimationComp::requestFire(bool, bool) { return true; }
void AnimationComp::postFire(WeaponFireEvent&) {}
void AnimationComp::fireComplete(unsigned int) {}

void AnimationComp::postSwitchFireRate(FireRate)
{
    if (m_activeAnimationSystem)
        m_activeAnimationSystem->setTriggerOnceMessage(8);
}

bool AnimationComp::requestReload(bool) { return true; }

void AnimationComp::reload(ReloadState)
{
    if (m_activeAnimationSystem)
        m_activeAnimationSystem->setTriggerOnceMessage(2);
}

void AnimationComp::postReload() {}
void AnimationComp::outOfAmmo(bool) {}
void AnimationComp::stoppedTracking() {}
void AnimationComp::postChangeTarget(IObject*) {}
void AnimationComp::projectileWasReset(IObject*) {}
void AnimationComp::enable() {}

void AnimationComp::disable()
{
    m_activeAnimationSystem = 0;
}

void AnimationComp::stop() {}
bool AnimationComp::busy() { return false; }
void AnimationComp::exit() {}
void AnimationComp::destroy() {}
void AnimationComp::pickup() {}
void AnimationComp::reset() {}

void AnimationComp::setActiveAnimationSystem(anim::IAnimationSystem* system)
{
    m_activeAnimationSystem = system;
}

void AnimationComp::updateOnZooming(int zoom)
{
    if (!m_animationTemplate->getAnimateOnZoom() || !m_activeAnimationSystem)
        return;

    if (zoom)
        m_activeAnimationSystem->setTriggerMessage(4);
    else
        m_activeAnimationSystem->clearTriggerMessage(4);
}

void AnimationComp::updateOnFiring(FireState)
{
}

AnimationCompTemplate::AnimationCompTemplate()
    : m_animateOnZoom(true)
{
}

unsigned int AnimationCompTemplate::getClassID() const
{
    return CID_AnimationCompTemplate;
}

unsigned int AnimationCompTemplate::getInterfaceID()
{
    return IID_IWeaponAnimationCompTemplate;
}

void AnimationCompTemplate::objectCreated(IObject*) {}
void AnimationCompTemplate::objectDestroyed(IObject*) {}
void AnimationCompTemplate::setTemplate(IObjectTemplate*) {}

void* AnimationCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == CID_AnimationCompTemplate ? this : 0;
}

bool AnimationCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!m_animateOnZoom)
        script << "ObjectTemplate.animation.animateOnZoom " << m_animateOnZoom << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

void AnimationCompTemplate::setAnimateOnZoom(bool value)
{
    m_animateOnZoom = value;
}

bool AnimationCompTemplate::getAnimateOnZoom()
{
    return m_animateOnZoom;
}

} // namespace world
} // namespace hfe
} // namespace dice
