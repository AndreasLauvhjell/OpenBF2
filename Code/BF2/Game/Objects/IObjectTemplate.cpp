// Path:     [INFERRED] - interface implementation beside IObjectTemplate.h.
// Contents: fully recovered lifecycle and weak-reference members from bf2.i64 0x8307330-0x8307420
// and the corresponding PE bodies.

#include "IObjectTemplate.h"
#include "../../Core/StringUtil.h"

namespace dice {
namespace hfe {
namespace world {

IObjectTemplate::IObjectTemplate()
    : m_weakRef(0)
{
}

IObjectTemplate::~IObjectTemplate()
{
    WeakPtr<IObjectTemplate>::Ctrl* control =
        static_cast<WeakPtr<IObjectTemplate>::Ctrl*>(m_weakRef);
    if (control)
    {
        control->target = 0;
        if (control->refCount-- == 1)
            ::operator delete(control);
    }
}

WeakPtr<IObjectTemplate> IObjectTemplate::getWeakPtr()
{
    typedef WeakPtr<IObjectTemplate> TemplateWeakPtr;
    TemplateWeakPtr::Ctrl* control = static_cast<TemplateWeakPtr::Ctrl*>(m_weakRef);
    if (!control)
    {
        control = static_cast<TemplateWeakPtr::Ctrl*>(::operator new(sizeof(TemplateWeakPtr::Ctrl)));
        control->refCount = 1;          // the template object's own control-block reference
        control->target = this;
        m_weakRef = control;
    }

    TemplateWeakPtr result;
    result.m_ctrl = control;
    ++control->refCount;                // the returned handle's reference
    return result;
}

bool IObjectTemplate::writeBinary(io::IStream*) const
{
    return false;
}

bool IObjectTemplate::readBinary(io::IStream*)
{
    return false;
}

void IObjectTemplate::setCollisionMesh(const std::string&)
{
}

const std::string& IObjectTemplate::getCollisionMesh() const
{
    return getEmptyString();
}

IObjectTemplate::ComponentMap* IObjectTemplate::getComponentMap()
{
    return 0;
}

} // namespace world
} // namespace hfe
} // namespace dice
