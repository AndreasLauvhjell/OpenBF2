// Damage-state effect lookup and lifetime management.

#include "DamageEffects.h"

#include "../IObjectTemplate.h"

namespace dice {
namespace hfe {
namespace world {

// 131 bytes, read in full. Two walks, one per effect set. The ELF spells both out as raw
// SGI list traversals (`for (i = head->next; i != head; i = i->next)`) reading the node's value at
// +8 and calling slot +0x6C on it; IObjectTemplate::preCache is that slot, already declared in this
// tree, so the dispatch is named rather than a ghost hook.
void DamageEffects::preCache() const
{
    for (std::list<const IObjectTemplate*>::const_iterator it = m_effects.begin();
         it != m_effects.end(); ++it)
        (*it)->preCache();

    for (std::list<const IObjectTemplate*>::const_iterator it = m_spectacularEffects.begin();
         it != m_spectacularEffects.end(); ++it)
        (*it)->preCache();
}

} // namespace world
} // namespace hfe
} // namespace dice
