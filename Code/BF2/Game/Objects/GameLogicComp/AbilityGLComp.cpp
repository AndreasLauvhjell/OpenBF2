// Runtime ability flags and radar settings.

#include "AbilityGLComp.h"

#include "../AbilityGLCompTemplate.h"
#include "../../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace world {

// (C1) / 0x8216880 (C2), 26 B each: vptr, refCount = 1, template = the argument.
// m_object is NOT initialised - as shipped. init()/setObject() store it before any reader runs
// on the retail component path (SimpleObjectTemplate::createComponent -> setComponent -> init).
AbilityGLComp::AbilityGLComp(AbilityGLCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_template(objectTemplate)
{
}

// vptr store only; no members need teardown.
AbilityGLComp::~AbilityGLComp()
{
}

// IBase - the standard component quartet (0x8216990 / 0x82169A0 / 0x82169B0 / 0x82169F0).
// release() dispatches the deleting destructor through the vtable (+0x40, the Itanium D0), and
// tests `this` for null after having decremented through it - the same shipped shape as
// ArmorGLCompTemplate::release; `delete this` is the typed spelling.
void AbilityGLComp::addRef()  { ++m_refCount; }
int  AbilityGLComp::getRef()  { return m_refCount; }

int AbilityGLComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// Three ids answer with `this` (single inheritance - no second base here): IBase, the component
// base interface, and its own interface id. Everything else is null.
void* AbilityGLComp::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent || iid == IID_IAbilityGLComp)
        return const_cast<AbilityGLComp*>(this);
    return 0;
}

unsigned int AbilityGLComp::getInterfaceID() const  { return IID_IAbilityGLComp; }

// the base default is empty; this override stores the object, same as setObject.
void AbilityGLComp::init(IObject* object)       { m_object = object; }
void AbilityGLComp::setObject(IObject* object)  { m_object = object; }
IObject* AbilityGLComp::getObject() const       { return m_object; }

void AbilityGLComp::handleUpdate(float)         {}                                   // empty

// hands out THE template'S payload block: the address of m_hasHealingAbility
// (template+0x10; three ability bools then the radar-radius int). The template header makes this
// class a friend for exactly this field-address take.
AbilityGLInfo* AbilityGLComp::getGLInfo()
{
    return &m_template->m_info;
}

// the appended NON-const slot (+0x44), constant false. The +0x24 hasUpdate() const
// slot stays IObjectComponent's default (also false).
bool AbilityGLComp::hasUpdate()
{
    return false;
}

} // namespace world
} // namespace hfe
} // namespace dice
