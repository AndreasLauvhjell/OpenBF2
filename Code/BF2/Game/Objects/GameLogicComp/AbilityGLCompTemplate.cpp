// Ability template lifecycle and console serialization. Source placement is [INFERRED].

#include "../AbilityGLCompTemplate.h"

#include "AbilityGLComp.h"
#include "../../../Core/Ids.h"
#include "../../../IO/Stream/IStream.h"
#include <sstream>

namespace dice {
namespace hfe {
namespace world {

// both vptrs, refCount 1, needsBuild true, the two [UNRESOLVED] strings default-
// construct (empty-rep attach), and then the four ability fields are zeroed IN THE body - the ELF
// stores them after the +0x18/+0x1C strings, which only a body assignment produces (the fields
// sit before the strings in the layout, so an init-list would have stored them first).
// _byteD/_byteE/_byteF are NOT written - as shipped.
AbilityGLCompTemplate::AbilityGLCompTemplate()
    : m_refCount(1)
    , m_needsBuild(true)
{
    m_info.hasHealingAbility   = false;
    m_info.hasRepairingAbility = false;
    m_info.hasAmmoAbility      = false;
    m_info.radarRadius         = 0;
}

// D1/D0 at vtable +0x4C/+0x50 - the compiler's member teardown only (the two strings drop their
// reps, in reverse declaration order). No hand-written body in the ELF beyond the vptr restores.
AbilityGLCompTemplate::~AbilityGLCompTemplate()
{
}

// IBase - release() dispatches the deleting destructor through the vtable (+0x50, the Itanium D0)
// and tests `this` after decrementing through it, as shipped; `delete this` is the typed spelling.
void AbilityGLCompTemplate::addRef()  { ++m_refCount; }
int  AbilityGLCompTemplate::getRef()  { return m_refCount; }

int AbilityGLCompTemplate::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// Three ids answer with `this`; IConsoleSaveable answers with the second base at +4, which is what
// the RTTI says (IAbilityGLCompTemplate @ +0, io::IConsoleSaveable @ +4).
void* AbilityGLCompTemplate::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IAbilityGLCompTemplate)
        return const_cast<AbilityGLCompTemplate*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<AbilityGLCompTemplate*>(this));
    return 0;
}

unsigned int AbilityGLCompTemplate::getClassID() const  { return CID_AbilityGLCompTemplate; }   // 50354
unsigned int AbilityGLCompTemplate::getInterfaceID()    { return IID_IAbilityGLCompTemplate; }  // 50355

// "Give me the implementation if you ARE this class" - asks its own getClassID through the vtable
// rather than returning the constant, so a subclass answers for itself (Armor's exact shape).
void* AbilityGLCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == getClassID())
        return this;
    return 0;
}

// createComponent - 0x8216CB0, 63 B. THE RW-6b slot: clears the needs-build flag (the same
// test-then-clear the constructor armed, Armor's m_needsBuild semantics) and news the runtime
// component bound to this template. SimpleObjectTemplate::createComponent keys the result into
// the object by getInterfaceID.
IObjectComponent* AbilityGLCompTemplate::createComponent()
{
    if (m_needsBuild)
        m_needsBuild = false;
    return new AbilityGLComp(this);
}

// Three five-byte bodies - every template component gets these hooks; this one declines all three.
void AbilityGLCompTemplate::objectCreated(IObject*)          {}
void AbilityGLCompTemplate::objectDestroyed(IObject*)        {}
void AbilityGLCompTemplate::setTemplate(IObjectTemplate*)    {}

// the four console-backed accessors (trivial stores/loads, 10-17 B each)
void AbilityGLCompTemplate::setHasHealingAbility(bool v)    { m_info.hasHealingAbility = v; }
bool AbilityGLCompTemplate::getHasHealingAbility()          { return m_info.hasHealingAbility; }
void AbilityGLCompTemplate::setHasRepairingAbility(bool v)  { m_info.hasRepairingAbility = v; }
bool AbilityGLCompTemplate::getHasRepairingAbility()        { return m_info.hasRepairingAbility; }
void AbilityGLCompTemplate::setHasAmmoAbility(bool v)       { m_info.hasAmmoAbility = v; }
bool AbilityGLCompTemplate::getHasAmmoAbility()             { return m_info.hasAmmoAbility; }
void AbilityGLCompTemplate::setRadarRadius(int v)           { m_info.radarRadius = v; }
int  AbilityGLCompTemplate::getRadarRadius()                { return m_info.radarRadius; }

// omit zero values, retain property order and test the one write.
bool AbilityGLCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (m_info.hasHealingAbility) out << "ObjectTemplate.Ability.hasHealingAbility " << m_info.hasHealingAbility << std::endl;
    if (m_info.hasRepairingAbility) out << "ObjectTemplate.Ability.hasRepairingAbility " << m_info.hasRepairingAbility << std::endl;
    if (m_info.hasAmmoAbility) out << "ObjectTemplate.Ability.hasAmmoAbility " << m_info.hasAmmoAbility << std::endl;
    if (m_info.radarRadius) out << "ObjectTemplate.Ability.radarRadius " << m_info.radarRadius << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
