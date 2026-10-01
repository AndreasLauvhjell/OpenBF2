// Handles player orders in the AI module.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_MAIN_AIPLAYERCOURIER_H
#define AI_AIDLL_AI_MAIN_AIPLAYERCOURIER_H

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include "AIDllTypes.h"

namespace dice {
namespace hfe {
namespace ai {

// Registers the first IAIPlayerCourier in the simple-singleton registry.
// It has no roster static, so teardown resolves it by name.
class IAIPlayerCourier
{
public:
    IAIPlayerCourier();
    virtual ~IAIPlayerCourier();
    virtual int getSelectedVehicle(int) = 0;
    virtual int getRepairTarget(int) = 0;
    virtual int getReviveTarget(int) = 0;
    virtual int getMineDefuseTarget(int) = 0;
};

// AIPlayerCourier - sizeof == 4, from BF2AI::init step 12.
class AIPlayerCourier : public IAIPlayerCourier
{
public:
    AIPlayerCourier();
    virtual ~AIPlayerCourier();

    // IAIPlayerCourier
    virtual int getSelectedVehicle(int);
    virtual int getRepairTarget(int);
    virtual int getReviveTarget(int);
    virtual int getMineDefuseTarget(int);

};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_AIPLAYERCOURIER_H
