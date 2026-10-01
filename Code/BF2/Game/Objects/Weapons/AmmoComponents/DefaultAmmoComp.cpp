// Default magazine component and template.
// [CHOSEN, not recovered] Split from the original AmmoComp translation unit for readability.

#include "DefaultAmmoComp.h"

#include <sstream>

#include "../../DefaultAmmoCompTemplate.h"
#include "../../IObjectComponent.h"
#include "../../../../Core/Debug.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

// DefaultAmmoComp - the whole class. Construction chains to AmmoComp, swaps in this leaf's two
// vtables, and starts its own reference count at 1.
DefaultAmmoComp::DefaultAmmoComp(DefaultAmmoCompTemplate* objectTemplate)
    : AmmoComp(objectTemplate)
    , m_refCount(1)
    , m_defaultTemplate(objectTemplate)
{
}

DefaultAmmoComp::~DefaultAmmoComp() {}

void DefaultAmmoComp::addRef() { ++m_refCount; }
int  DefaultAmmoComp::getRef() { return m_refCount; }

// . IT decrements THE leaf'S count AT +0x5C, NOT WeaponComp's AT +0x04. The base
// count is set to 1 by AmmoComp's constructor and never touched again; this is the live one.
// The null test on `this` is dice's, and the delete goes through the deleting destructor slot.
int DefaultAmmoComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// . TWO IDS return THE primary pointer AND TWO return THE +0x10 face. Getting that
// split wrong is exactly the bug that makes a component appear to work and then dispatch into the
// wrong table.
void* DefaultAmmoComp::queryInterface(unsigned int iid) const
{
    DefaultAmmoComp* const self = const_cast<DefaultAmmoComp*>(this);

    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponAmmoComp)
        return static_cast<IWeaponAmmoComp*>(self);
    return 0;
}

// DefaultAmmoCompTemplate - 76 bytes. The factory-registered half.
DefaultAmmoCompTemplate::DefaultAmmoCompTemplate()
    : AmmoCompTemplate()
    , m_refCount(1)
{
}

void DefaultAmmoCompTemplate::addRef() { ++m_refCount; }
int  DefaultAmmoCompTemplate::getRef() { return m_refCount; }

int DefaultAmmoCompTemplate::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// . IID_IWeaponAmmoCompTemplate IS NOT accepted here - three ids return the
// primary pointer and the io::IConsoleSaveable face at +4 is the only adjusted one. The ammo-
// template id is answered by queryImplementation's CID path instead.
void* DefaultAmmoCompTemplate::queryInterface(unsigned int iid) const
{
    DefaultAmmoCompTemplate* const self = const_cast<DefaultAmmoCompTemplate*>(this);

    if (iid == IID_IBase ||
        iid == IID_IObjectTemplateComponent ||
        iid == IID_IWeaponCompTemplate)
    {
        return static_cast<IWeaponCompTemplate*>(self);
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int DefaultAmmoCompTemplate::getClassID() const
{
    return CID_DefaultAmmoCompTemplate;
}

void* DefaultAmmoCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_DefaultAmmoCompTemplate || classId == CID_AmmoCompTemplate)
        return this;
    return 0;
}

// createComponent - 0x8259690, 47 B. THE ONE line that makes ammo exist. operator new(0x64) -
// 100 bytes - then the constructor. AmmoCompTemplate's own createComponent is a Debug refusal
// ("Virtual type", AmmoComp.h:154); this override is what every non-replenishing weapon reaches.
IObjectComponent* DefaultAmmoCompTemplate::createComponent()
{
    return new DefaultAmmoComp(this);
}

// (1,037 B) / PE 0x5A41D0 (164 B).  The leaf owns no properties: retail still
// constructs an empty stream, delegates the base writer, then performs a second (zero-byte) write.
// Preserve that observable write and its return-value check rather than returning the base result.
bool DefaultAmmoCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!AmmoCompTemplate::makeScript(stream))
        return false;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
