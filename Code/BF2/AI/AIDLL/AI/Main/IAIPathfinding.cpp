
#include "IAIPathfinding.h"

namespace dice {
namespace hfe {
namespace ai {

IAIPathfinding* IAIPathfinding::instance = 0;

// Unconditionally replaces the pathfinding singleton. This interface uses no ClassManager registration.
IAIPathfinding::IAIPathfinding()
{
    instance = this;
}

// Clears the singleton unconditionally, even when destroying a displaced instance.
IAIPathfinding::~IAIPathfinding()
{
    instance = 0;
}

// Setup has its own singleton pointer, replaced on construction and cleared on destruction.
// An AIPathfinding instance publishes both interfaces.
IAIPathfindingSetup* IAIPathfindingSetup::instance = 0;

IAIPathfindingSetup::IAIPathfindingSetup()
{
    instance = this;
}

IAIPathfindingSetup::~IAIPathfindingSetup()
{
    instance = 0;
}

// Stateless map-container interface with no registry entry or custom lifecycle work.
IAIPathfindingSetup::IMapContainer::IMapContainer()
{
}

IAIPathfindingSetup::IMapContainer::~IMapContainer()
{
}

} // namespace ai
} // namespace hfe
} // namespace dice
