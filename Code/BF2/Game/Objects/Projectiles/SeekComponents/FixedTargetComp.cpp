// Delayed weak-target tracking for artillery projectiles.

#include "FixedTargetComp.h"

#include <cstdlib>
#include <new>
#include <sstream>
#include <string>

#include "../ProjectileCompTemplates.h"
#include "../../Game/TargetObject.h"
#include "../../IObject.h"
#include "../../../IGameLogic.h"                 // complete TargetType definition
#include "../../../../Core/Ids.h"
#include "../../../../Core/StringUtil.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

// ; PE createComponent at 0x5BA8F0 inlines the same stores. Both reference counts
// start at one, both object pointers at zero, the weak handle is empty, and the template pointer
// is borrowed.
FixedTargetComp::FixedTargetComp(FixedTargetCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_targetObject()
    , m_template(objectTemplate)
{
    ProjectileComp::m_refCount = 1;
    m_object = 0;
    m_initObject = 0;
}

// The generated member/base destruction sequence releases the weak control block before restoring
// the two base vtables, exactly matching ELF 0x82931F0 and PE 0x5BAA60.
FixedTargetComp::~FixedTargetComp() {}

void FixedTargetComp::addRef() { ++m_refCount; }
int FixedTargetComp::getRef() { return m_refCount; }

int FixedTargetComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* FixedTargetComp::queryInterface(unsigned int iid) const
{
    FixedTargetComp* const self = const_cast<FixedTargetComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IProjectileComp || iid == IID_IProjectileSeekComp)
        return static_cast<IProjectileSeekComp*>(self);
    return 0;
}

unsigned int FixedTargetComp::getInterfaceID() const
{
    return IID_IProjectileSeekComp;
}

bool FixedTargetComp::hasUpdate() const
{
    return true;
}

// Both arguments are genuinely unused. The deadline compare is unsigned in the ELF; casting before
// addition also preserves x86 wraparound without relying on signed-overflow behaviour.
void FixedTargetComp::updateComponent(float, unsigned int)
{
    const unsigned int launchTick = static_cast<unsigned int>(
        static_cast<GenericProjectile*>(m_initObject)->m_dword1BC);
    const unsigned int delay = static_cast<unsigned int>(m_template->getTrackingDelayTick());
    if (launchTick + delay <= static_cast<unsigned int>(WorldPref::getTickCount()))
        checkTarget();
}

void FixedTargetComp::activate()
{
    m_targetObject = WeakPtr<IObject>();
}

// The boolean is present in the ABI but unused by this implementation. WeakPtr assignment retains
// the incoming control block before releasing the old one, including on self-assignment.
void FixedTargetComp::setTargetObject(IObject* target, bool)
{
    m_targetObject = target ? target->getWeakPtr() : WeakPtr<IObject>();
}

IObject* FixedTargetComp::getTargetObject() const
{
    return m_targetObject.get();
}

int FixedTargetComp::getTrackingDelayTick() const
{
    return m_template->getTrackingDelayTick();
}

float FixedTargetComp::getMaxTargetDistance() const
{
    return 9999.0f;
}

float FixedTargetComp::getMaxTargetAngle() const
{
    return 0.78539818525314331055f; // exact float 0x3F490FDB
}

TargetType FixedTargetComp::getTargetType() const
{
    return m_template->getTargetType();
}

// No template/class check is made in retail: a non-null weak target is treated as TargetObject.
void FixedTargetComp::checkTarget()
{
    TargetObject* const target = static_cast<TargetObject*>(m_targetObject.get());
    if (target && !target->getTargetActive())
        m_targetObject = WeakPtr<IObject>();
}

// TargetType streaming. Numeric spellings are accepted only when the token is exactly one byte;
// invalid and unknown spellings leave the destination untouched and do not manufacture failbit.
std::ostream& operator<<(std::ostream& stream, TargetType type)
{
    switch (static_cast<int>(type))
    {
    case 0: stream << "TTNone";   break;
    case 1: stream << "TTLaser";  break;
    case 2: stream << "TTHeat";   break;
    case 3: stream << "TTUnique"; break;
    default:                       break;
    }
    return stream;
}

std::istream& operator>>(std::istream& stream, TargetType& type)
{
    std::string token;
    stream >> token;
    if (token.length() == 1)
    {
        std::ostringstream normalized;
        normalized << static_cast<TargetType>(std::atoi(token.c_str()));
        token = normalized.str();
    }

    if (equalsIgnoreCase(token, "TTNone"))
        type = static_cast<TargetType>(0);
    else if (equalsIgnoreCase(token, "TTHeat"))
        type = static_cast<TargetType>(2);
    else if (equalsIgnoreCase(token, "TTLaser"))
        type = static_cast<TargetType>(1);
    else if (equalsIgnoreCase(token, "TTUnique"))
        type = static_cast<TargetType>(3);
    return stream;
}

// FixedTargetCompTemplate. The class has no virtual destructor slot; release performs the shipped
// direct operator delete when its reference count reaches zero.
FixedTargetCompTemplate::FixedTargetCompTemplate()
    : m_refCount(1)
    , m_targetType(static_cast<TargetType>(2))
{
    setTrackingDelay(0.0f);
}

void FixedTargetCompTemplate::addRef() { ++m_refCount; }
int FixedTargetCompTemplate::getRef() { return m_refCount; }

int FixedTargetCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        ::operator delete(this);
    return remaining;
}

void* FixedTargetCompTemplate::queryInterface(unsigned int iid) const
{
    FixedTargetCompTemplate* const self = const_cast<FixedTargetCompTemplate*>(this);
    if (iid == IID_IBase
        || iid == IID_IObjectTemplateComponent
        || iid == IID_IProjectileCompTemplate)
    {
        return static_cast<IObjectTemplateComponent*>(self);
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int FixedTargetCompTemplate::getClassID() const
{
    return CID_FixedTargetCompTemplate;
}

unsigned int FixedTargetCompTemplate::getInterfaceID()
{
    return IID_IProjectileSeekCompTemplate;
}

IObjectComponent* FixedTargetCompTemplate::createComponent()
{
    return new FixedTargetComp(this);
}

void FixedTargetCompTemplate::objectCreated(IObject*) {}
void FixedTargetCompTemplate::objectDestroyed(IObject*) {}
void FixedTargetCompTemplate::setTemplate(IObjectTemplate*) {}

void* FixedTargetCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

void FixedTargetCompTemplate::setTrackingDelay(float seconds)
{
    m_trackingDelay = seconds;
    m_trackingDelayTick = static_cast<int>(seconds * 30.0f);
}

bool FixedTargetCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    script << "ObjectTemplate.seek.targetType " << m_targetType << std::endl;
    script << "ObjectTemplate.seek.trackingDelay " << m_trackingDelay << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
