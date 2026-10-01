// Guard-position order and bot recruitment state.
// [INFERRED] Source placement.
#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_GUARDPOSITION_H
#define AI_AIDLL_AI_AISTRATEGICENTITY_GUARDPOSITION_H

#include "../../../../Math/Vec3.h"
#include "../AIStrategic/StatusMap.h"
#include <string>
#include <vector>

namespace dice { namespace hfe { namespace ai {

class Bot;
enum CombatStrengthType : int;

class GuardPosition
{
public:
    // Attested signatures, declaration-only: no default or fixture constructor in production.
    GuardPosition(const Vec2&, CombatStrengthType, int side, int value36);
    // > : only AreaCondition vector/string cleanup.
    // The borrowed bot is not deleted and no callback is invoked.
    ~GuardPosition() = default;

    bool active() const { return m_active; }       // byte +28
    int getSide() const { return m_side; }         // int +32
    bool needBot() const
    {
        // Both images test the byte first. Neither active nor side participates in this predicate.
        return !m_flag48 && m_bot == 0;
    }

private:
    // Area abort condition. New entries start with areaIndex -1.
    // [INFERRED] Field names; preserve member order and types.
    struct AreaCondition
    {
        std::string areaName;
        int areaIndex;
        StatusMap::Status status;
    };

    // Offsets are ELF reference offsets; the prefix through +48 agrees with the Win32 PE.
    /* +0  */ Vec3 m_position;
    /* +12 */ Vec3 m_rotation;
    /* +24 */ CombatStrengthType m_strengthType;
    /* +28 */ bool m_active;
    /* +32 */ int m_side;
    /* +36 */ int m_value36;                 // [UNRESOLVED] constructor's final parameter
    /* +40 */ float m_minDistance;
    /* +44 */ Bot* m_bot;
    /* +48 */ bool m_flag48;                 // [UNRESOLVED] name; recruit sets true
    /* +52 */ std::vector<AreaCondition> m_areaConditions;
};

} } }

#endif
