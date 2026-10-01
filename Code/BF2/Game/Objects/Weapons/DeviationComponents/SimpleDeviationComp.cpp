// Path:     [INFERRED] Game/Objects/Weapons/DeviationComponents/SimpleDeviationComp.cpp.
// Contents: complete SimpleDeviationComp family recovered from ELF 0x825D430..0x825DE6A and
// cross-checked against PE 0x5A9C30..0x5A9ED6.

#include "SimpleDeviationComp.h"

#include <sstream>
#include <string>

#include "../GenericFireArm.h"
#include "../../IObject.h"
#include "../../IPlayer.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

SimpleDeviationComp::SimpleDeviationComp(SimpleDeviationCompTemplate* objectTemplate)
    : DeviationComp(objectTemplate)
    , m_simpleRefCount(1)
    , m_simpleRandom()
    , m_aiDeviation(0.0f)
    , m_simpleTemplate(objectTemplate)
{
}

SimpleDeviationComp::~SimpleDeviationComp()
{
}

void SimpleDeviationComp::addRef() { ++m_simpleRefCount; }
int SimpleDeviationComp::getRef() { return m_simpleRefCount; }

int SimpleDeviationComp::release()
{
    const int remaining = --m_simpleRefCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* SimpleDeviationComp::queryInterface(unsigned int iid) const
{
    SimpleDeviationComp* const self = const_cast<SimpleDeviationComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponDeviationComp)
        return static_cast<IWeaponDeviationComp*>(self);
    return 0;
}

unsigned int SimpleDeviationComp::getInterfaceID() const
{
    return IID_IWeaponDeviationComp;
}

void SimpleDeviationComp::updateComponent(float, unsigned int)
{
}

// , 118 B.  The root-parent lookup is an observable but
// deliberately discarded call.  Only the final bool controls decay; the PlayerInput and first
// bool are unused in both ports.
void SimpleDeviationComp::updateDeviation(io::PlayerInput, bool, bool firing)
{
    (void)getRootParent(m_initObject);

    if (!firing)
    {
        m_fireDeviation -= m_simpleTemplate->getFireDevSub();
        if (m_fireDeviation < 0.0f)
            m_fireDeviation = 0.0f;
    }

    m_deviation = m_fireDeviation + m_simpleTemplate->getMinDev();
    if (m_initObject)
    {
        IPlayer* const player = firearm()->getPlayer();
        if (player && player->getIsAIPlayer())
            m_deviation += m_aiDeviation;
    }
}

float SimpleDeviationComp::getMaxDeviation() const
{
    return m_simpleTemplate->getFireDevMax() + m_simpleTemplate->getMinDev();
}

void SimpleDeviationComp::setAIDeviation(float deviation)
{
    m_aiDeviation = deviation;
}

SimpleDeviationCompTemplate::SimpleDeviationCompTemplate()
    : DeviationCompTemplate()
    , m_simpleRefCount(1)
{
}

void SimpleDeviationCompTemplate::addRef() { ++m_simpleRefCount; }
int SimpleDeviationCompTemplate::getRef() { return m_simpleRefCount; }

int SimpleDeviationCompTemplate::release()
{
    const int remaining = --m_simpleRefCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* SimpleDeviationCompTemplate::queryInterface(unsigned int iid) const
{
    SimpleDeviationCompTemplate* const self =
        const_cast<SimpleDeviationCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent
        || iid == IID_IWeaponCompTemplate)
        return static_cast<IWeaponCompTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int SimpleDeviationCompTemplate::getClassID() const
{
    return CID_SimpleDeviationCompTemplate;
}

unsigned int SimpleDeviationCompTemplate::getInterfaceID()
{
    return IID_IWeaponDeviationCompTemplate;
}

IObjectComponent* SimpleDeviationCompTemplate::createComponent()
{
    return new SimpleDeviationComp(this);
}

void SimpleDeviationCompTemplate::objectCreated(IObject*) {}
void SimpleDeviationCompTemplate::objectDestroyed(IObject*) {}
void SimpleDeviationCompTemplate::setTemplate(IObjectTemplate*) {}

void* SimpleDeviationCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_SimpleDeviationCompTemplate || classId == CID_DeviationCompTemplate)
        return this;
    return 0;
}

SimpleDeviationCompTemplate::~SimpleDeviationCompTemplate()
{
}

// .  This unusual leaf delegates all serialization to its base and
// then still performs its own zero-length write, preserving the retail IStream call.
bool SimpleDeviationCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!DeviationCompTemplate::makeScript(stream))
        return false;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
