// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BFPathfindingCourier operations.

#include "BFPathfindingCourier.h"

#include <string>

#include "../../../../Core/ClassManager.h"
#include "CommonSingletonServer.h"
#include "AIDllFrontier.h"
#include "IAIPathfinding.h"            // getVehicleNumberFromName's one forward
#include "AISettingsInterfaces.h"      // IAISettings::getVehicle - findClosestValidInfantryPolygon

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// IAIPathfindingCourier
IAIPathfindingCourier::IAIPathfindingCourier()
{
    // the guard reads the roster static, not the registry. Reproduced.
    if (!CommonSingletonServer::m_iaiPathfindingCourier)
        classManager->registerSimpleSingleton("IAIPathfindingCourier", this);
}

IAIPathfindingCourier::~IAIPathfindingCourier()
{
    if (CommonSingletonServer::m_iaiPathfindingCourier == this)
        classManager->unregisterSimpleSingleton("IAIPathfindingCourier");
}

// Stateless pathfinding courier; construction requires only the base-class initialization.
BFPathfindingCourier::BFPathfindingCourier()
{
}

// The vptr reset and the base call, which again is what C++ emits.
BFPathfindingCourier::~BFPathfindingCourier()
{
}

int BFPathfindingCourier::getNumberOfVehicles() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFPathfindingCourier::getNumberOfVehicles");
}

bool BFPathfindingCourier::isVehicleUsed(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFPathfindingCourier::isVehicleUsed");
}

bool BFPathfindingCourier::isValidPosition(int, int, const Vec3&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFPathfindingCourier::isValidPosition");
}

bool BFPathfindingCourier::isValidPosition(const Vec3&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFPathfindingCourier::isValidPosition");
}

void BFPathfindingCourier::getLastValidSquadleaderPosition(const Vec3&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFPathfindingCourier::getLastValidSquadleaderPosition");
}

// Returns the case-insensitive pathfinding map id, or -1 for an unknown name.
// Requires IAIPathfinding::instance to exist.
int BFPathfindingCourier::getVehicleNumberFromName(const std::string& name) const
{
    return IAIPathfinding::instance->getMapIdFromName(name);
}

// Finds the nearest valid infantry polygon using the vehicle settings for Infantery.
bool BFPathfindingCourier::findClosestValidInfantryPolygon(Vec3& closest, const Vec3& from,
                                                           const Vec3& to, unsigned int limit) const
{
    const int vehicle = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)
                            ->getVehicle(std::string("Infantery"));
    unsigned int quadTree;
    unsigned int polygon;
    return IAIPathfinding::instance->findClosestValidPolygon(
        quadTree, polygon, closest, IAIPathfinding::instance->getTraceMapIndex(), from, to, limit,
        vehicle);
}

} // namespace ai
} // namespace hfe
} // namespace dice
