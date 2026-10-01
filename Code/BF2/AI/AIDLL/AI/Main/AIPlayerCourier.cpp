// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIPlayerCourier operations.

#include "AIPlayerCourier.h"

#include <string>

#include "../../../../Core/ClassManager.h"
#include "CommonSingletonServer.h"
#include "AIDllFrontier.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// IAIPlayerCourier
IAIPlayerCourier::IAIPlayerCourier()
{
    if (!classManager->getSimpleSingleton("IAIPlayerCourier"))
        classManager->registerSimpleSingleton("IAIPlayerCourier", this);
}

IAIPlayerCourier::~IAIPlayerCourier()
{
    if (classManager->getSimpleSingleton("IAIPlayerCourier") == this)
        classManager->unregisterSimpleSingleton("IAIPlayerCourier");
}

// Stateless player courier; construction requires only the base-class initialization.
AIPlayerCourier::AIPlayerCourier()
{
}

// The vptr reset and the base call, which again is what C++ emits.
AIPlayerCourier::~AIPlayerCourier()
{
}

int AIPlayerCourier::getSelectedVehicle(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerCourier::getSelectedVehicle");
}

int AIPlayerCourier::getRepairTarget(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerCourier::getRepairTarget");
}

int AIPlayerCourier::getReviveTarget(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerCourier::getReviveTarget");
}

int AIPlayerCourier::getMineDefuseTarget(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIPlayerCourier::getMineDefuseTarget");
}

} // namespace ai
} // namespace hfe
} // namespace dice
