// Owns six parallel lists for one damage state: ordinary and spectacular template/position/direction triples.
// Each writer must append to all three lists in its triple; consumers advance them positionally.
// The original ELF size is 24 bytes. Source placement is [INFERRED].
#ifndef GAME_OBJECTS_GAMELOGICCOMP_DAMAGEEFFECTS_H
#define GAME_OBJECTS_GAMELOGICCOMP_DAMAGEEFFECTS_H

#include <list>

#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectTemplate;

class DamageEffects
{
public:
    // . Walks the normal effect list and then the spectacular one, calling
    // IObjectTemplate::preCache() (slot +0x6C) on every template in each. The two Vec3 triples are
    // not touched - only the templates need warming.
    void preCache() const;

    // THE triple selectors, BF2.exe 0x685B40 / 0x685B60 / 0x685B80 (thiscall,
    // one bool argument, `ret 4`; out of line in the client, among ArmorGLComp's code) and inlined
    // into ELF ArmorGLComp::playEffect 0x821A2B9..0x821A334. Each answers the spectacular list of
    // its kind only when asked for it AND that list is non-empty, else the plain one - and each
    // list is tested on its own (client: the VC7.1 size word at +0x2C / +0x38 / +0x44 of the 72-byte
    // client object; ELF: head->next != head). ArmorGLComp::playEffect is the one caller.
    // [INFERRED] names and constness; the bodies are read.
    const std::list<const IObjectTemplate*>& getEffects(bool spectacular) const
    {
        return (spectacular && !m_spectacularEffects.empty()) ? m_spectacularEffects : m_effects;
    }
    const std::list<Vec3>& getPositions(bool spectacular) const
    {
        return (spectacular && !m_spectacularPositions.empty()) ? m_spectacularPositions : m_positions;
    }
    const std::list<Vec3>& getDirections(bool spectacular) const
    {
        return (spectacular && !m_spectacularDirections.empty()) ? m_spectacularDirections : m_directions;
    }

    /*  0 */ std::list<const IObjectTemplate*> m_effects;
    /*  4 */ std::list<Vec3>                   m_positions;
    /*  8 */ std::list<Vec3>                   m_directions;

    /* 12 */ std::list<const IObjectTemplate*> m_spectacularEffects;
    /* 16 */ std::list<Vec3>                   m_spectacularPositions;
    /* 20 */ std::list<Vec3>                   m_spectacularDirections;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAMELOGICCOMP_DAMAGEEFFECTS_H
