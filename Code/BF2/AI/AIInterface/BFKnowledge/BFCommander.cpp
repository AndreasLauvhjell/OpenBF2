// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFCommander operations.

#include "BFCommander.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IBFCommander::IBFCommander()
{
    if (!CommonSingletonServer::m_iBFCommander)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->registerSimpleSingleton(std::string("IBFCommander"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IBFCommander::~IBFCommander()
{
    if (CommonSingletonServer::m_iBFCommander == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IBFCommander"));
        BFSingletonServer::updateStaticMethods();
    }
}

BFCommander::BFCommander()
{
}

BFCommander::~BFCommander()
{
}

int BFCommander::applyAsCommander(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::applyAsCommander");
}

int BFCommander::retireAsCommander(IAIPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::retireAsCommander");
}

int BFCommander::giveAttackOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::giveAttackOrder");
}

int BFCommander::giveMoveOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::giveMoveOrder");
}

int BFCommander::giveDefendOrder(const Vec3& position, int a, int b, IAIObject* target)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::giveDefendOrder");
}

void BFCommander::sendAttackMessage(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::sendAttackMessage");
}

void BFCommander::sendDefendMessage(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::sendDefendMessage");
}

int BFCommander::fireArtillery(int a, int b, Vec2 position)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::fireArtillery");
}

int BFCommander::sendUav(int a, int b, Vec2 position)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::sendUav");
}

int BFCommander::dropSupply(int a, int b, Vec2 position)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::dropSupply");
}

int BFCommander::dropVehicle(int a, int b, Vec2 position)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::dropVehicle");
}

int BFCommander::denyArtilleryRequest(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::denyArtilleryRequest");
}

bool BFCommander::gameHasArtillery(int team)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::gameHasArtillery");
}

bool BFCommander::anyArtilleryPiecesLeft(int team)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::anyArtilleryPiecesLeft");
}

// dice's capital A. Frozen - it is the mangled name.
int BFCommander::ArtilleryUnavailable(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::ArtilleryUnavailable");
}

int BFCommander::denySupplyRequest(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::denySupplyRequest");
}

int BFCommander::denyVehicleDropRequest(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::denyVehicleDropRequest");
}

int BFCommander::denyUavRequest(int a, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCommander::denyUavRequest");
}

} // namespace ai
} // namespace hfe
} // namespace dice
