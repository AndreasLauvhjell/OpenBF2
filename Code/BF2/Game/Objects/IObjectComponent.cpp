// Default object-component behavior; update and event hooks are inert unless overridden.

#include "IObjectComponent.h"
#include "../../Core/Ids.h"     // IID_IObjectComponent - the single definition site for all 750 ids

namespace dice {
namespace hfe {
namespace world {

// Returns the base component interface ID used for component registration.
unsigned int IObjectComponent::getInterfaceID() const
{
    return IID_IObjectComponent;
}

// 5 bytes. Empty — the runtime counterpart of a template component receives its object
// here, but the base does nothing with it.
void IObjectComponent::init(IObject*)
{
}

// 5 bytes. Empty.
void IObjectComponent::destroy()
{
}

// / 0x815CA00 — `xor eax,eax / retn` and an empty body.
// This is the simulation channel. A component opting in gets ticked with a delta time.
bool IObjectComponent::hasUpdate() const
{
    return false;
}

void IObjectComponent::handleUpdate(float)
{
}

// Frame updates are independent of the regular update channel; both default to disabled.
bool IObjectComponent::hasFrameUpdate() const
{
    return false;
}

void IObjectComponent::handleFrameUpdate(float)
{
}

// 5 bytes. Empty. The third channel — event-driven rather than polled, and it carries the
// acting player. [UNRESOLVED] the TemplateMessage enumerators and what the trailing bool selects.
void IObjectComponent::handleMessage(TemplateMessage, IPlayer*, bool)
{
}

} // namespace world
} // namespace hfe
} // namespace dice
