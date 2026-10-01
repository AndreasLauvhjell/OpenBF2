// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AILandingZoneManager operations.

#include "AILandingZoneManager.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

AILandingZoneManager* AILandingZoneManager::m_instance = 0;

// Initializes an inactive landing-zone manager and explicitly resizes its new vector to zero.
AILandingZoneManager::AILandingZoneManager()
    : m_unresolved1C(0)
    , m_unresolved20(0)
    , m_activeZoneName("No valid zone is active")
{
    m_zones.resize(0);
}

// Constructs the landing-zone manager at DLL load. BF2AI::clear owns its later destruction.
namespace {

struct AILandingZoneManagerInstaller
{
    AILandingZoneManagerInstaller()
    {
        AILandingZoneManager::m_instance = new AILandingZoneManager();
    }
};

AILandingZoneManagerInstaller g_aiLandingZoneManagerInstaller;

} // namespace

// [FRONTIER] - the rest of the class. Addresses and sizes are in the header.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]

AILandingZoneManager::~AILandingZoneManager()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AILandingZoneManager::~AILandingZoneManager");
}

void AILandingZoneManager::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AILandingZoneManager::reset");
}

} // namespace ai
} // namespace hfe
} // namespace dice
