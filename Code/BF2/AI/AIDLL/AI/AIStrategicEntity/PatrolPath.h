// Patrol route and bot recruitment state.
// [INFERRED] Source placement.
#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_PATROLPATH_H
#define AI_AIDLL_AI_AISTRATEGICENTITY_PATROLPATH_H

#include "../../../../Math/Vec3.h"
#include <list>
#include <vector>

namespace dice { namespace hfe { namespace ai {

class Bot;
enum CombatStrengthType : int;

class PatrolPath
{
public:
    // Attested signatures, declaration-only: no default or fixture constructor in production.
    PatrolPath(CombatStrengthType, int side);
    PatrolPath(const std::vector<Vec3>&, CombatStrengthType, int);
    // list nodes/sentinel, then the Vec3 vector buffer.
    // Ordinary reverse member destruction; bot pointers in the list are not deleted.
    ~PatrolPath() = default;

    bool active() const { return m_active; }  // +16 / +20
    int getSide() const { return m_side; }    // +20 / +24
    int neededResources() const
    {
        // Signed required-minus-current, with no clamp. recruitBot tests the result > 0.
        return m_requiredResources - m_currentResources;
    }

private:
    // Member names [INFERRED] from the constructor, getters and recruitment increment.
    /* ELF +0  / PE +0  */ std::vector<Vec3> m_path;
    /* ELF +12 / PE +16 */ CombatStrengthType m_strengthType;
    /* ELF +16 / PE +20 */ bool m_active;
    /* ELF +20 / PE +24 */ int m_side;
    /* ELF +24 / PE +28 */ int m_currentResources;
    /* ELF +28 / PE +32 */ int m_requiredResources;
    /* ELF +32 / PE +36 */ float m_minDistance;
    /* ELF +36 / PE +40 */ float m_patrolSpeed;
    /* ELF +40 / PE +44 */ std::list<Bot*> m_bots;
};

} } }

#endif
