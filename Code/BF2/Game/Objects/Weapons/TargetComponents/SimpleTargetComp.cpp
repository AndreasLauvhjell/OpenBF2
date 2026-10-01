// Path:     [INFERRED] beside the attested LPTargetComp.cpp.
// Contents: simple target runtime/template recovered from ELF 0x82770D0..0x827801A and
// cross-checked against the Win32 dedicated-server PE.

#include "SimpleTargetComp.h"

#include <cmath>
#include <sstream>

#include "../GenericFireArm.h"
#include "../../IObject.h"
#include "../../Projectiles/GenericProjectile.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

SimpleTargetComp::SimpleTargetComp(SimpleTargetCompTemplate* objectTemplate)
    : TargetComp(objectTemplate)
    , m_leafRefCount(1)
    , m_simpleTemplate(objectTemplate)
{
}

SimpleTargetComp::~SimpleTargetComp()
{
}

void SimpleTargetComp::addRef() { ++m_leafRefCount; }
int SimpleTargetComp::getRef() { return m_leafRefCount; }

int SimpleTargetComp::release()
{
    const int remaining = --m_leafRefCount;
    if (remaining == 0 && this)
        delete this;
    return remaining;
}

void* SimpleTargetComp::queryInterface(unsigned int iid) const
{
    SimpleTargetComp* const self = const_cast<SimpleTargetComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return self;
    if (iid == IID_IWeaponComp || iid == IID_IWeaponTargetComp)
        return static_cast<IWeaponTargetComp*>(self);
    return 0;
}

unsigned int SimpleTargetComp::getInterfaceID() const
{
    return IID_IWeaponTargetComp;
}

bool SimpleTargetComp::busy()
{
    return false;
}

void SimpleTargetComp::setTargetObject(IObject* target)
{
    m_targetObject = target ? target->getWeakPtr() : WeakPtr<IObject>();
}

void SimpleTargetComp::postFire(WeaponFireEvent& event)
{
    TargetComp::postFire(event);
    if (event.projectile)
    {
        event.projectile->queryInterface(IID_IProjectileObject);
        static_cast<GenericProjectile*>(event.projectile)->setTargetObject(getTargetObject());
    }
}

float SimpleTargetComp::getDynamicVelocity() const
{
    if (!m_simpleTemplate->getUseDynamicVelocity())
        return -1.0f;

    IObject* const target = getTargetObject();
    if (!target)
        return -1.0f;

    const Vec3& targetPosition = target->getAbsolutePosition();
    const Vec3& weaponPosition = firearm()->getAbsolutePosition();
    const float x = targetPosition.x - weaponPosition.x;
    const float y = targetPosition.y - weaponPosition.y;
    const float z = targetPosition.z - weaponPosition.z;
    const float value = static_cast<float>(std::sqrt(std::sqrt(x * x + y * y + z * z)))
        * m_simpleTemplate->getVelocityModifier();

    // and the PE twin compare value <= max first, then min <= value. Besides the
    // ordinary clamp, that ordering deliberately returns maxVelocity for a NaN value.
    const float maximum = m_simpleTemplate->getMaxVelocity();
    if (value <= maximum)
    {
        const float minimum = m_simpleTemplate->getMinVelocity();
        if (minimum <= value)
            return value;
        return minimum;
    }
    return maximum;
}

SimpleTargetCompTemplate::SimpleTargetCompTemplate()
    : TargetCompTemplate()
    , m_leafRefCount(1)
    , m_useDynamicVelocity(false)
    , m_minVelocity(0.0f)
    , m_maxVelocity(1000.0f)
    , m_velocityModifier(1.0f)
{
}

void SimpleTargetCompTemplate::addRef() { ++m_leafRefCount; }
int SimpleTargetCompTemplate::getRef() { return m_leafRefCount; }

int SimpleTargetCompTemplate::release()
{
    const int remaining = --m_leafRefCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* SimpleTargetCompTemplate::queryInterface(unsigned int iid) const
{
    SimpleTargetCompTemplate* const self = const_cast<SimpleTargetCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IWeaponCompTemplate)
        return static_cast<IObjectTemplateComponent*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int SimpleTargetCompTemplate::getClassID() const
{
    return CID_SimpleTargetCompTemplate;
}

IObjectComponent* SimpleTargetCompTemplate::createComponent()
{
    return new SimpleTargetComp(this);
}

void* SimpleTargetCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_SimpleTargetCompTemplate || classId == CID_TargetCompTemplate)
        return this;
    return 0;
}

bool SimpleTargetCompTemplate::makeScript(io::IStream* stream)
{
    if (!TargetCompTemplate::makeScript(stream))
        return false;

    std::ostringstream script;
    if (m_useDynamicVelocity)
    {
        script << "ObjectTemplate.target.useDynamicVelocity " << m_useDynamicVelocity << std::endl;
        script << "ObjectTemplate.target.minVelocity " << m_minVelocity << std::endl;
        script << "ObjectTemplate.target.maxVelocity " << m_maxVelocity << std::endl;
        script << "ObjectTemplate.target.velocityModifier " << m_velocityModifier << std::endl;
    }

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
