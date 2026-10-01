// [TEMP] [CHOSEN, not recovered] Whole production BFSquad.cpp with typed boundary observers.
// This tests adapter calls and bool propagation, not actual engine squad admission or bot state.
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../../BF2/AI/AIInterface/BFKnowledge/BFSquad.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/AIPlayer.h"
#include "../../BF2/AI/AIInterface/SingletonServer.h"
#include "../../BF2/AI/AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../BF2/Game/GameLogic/SquadManager/ISquadManager.h"

[[noreturn]] static void unexpected() { std::fputs("[TEMP] unexpected bridge dependency\n", stderr); std::abort(); }
static void check(bool condition) { if (!condition) unexpected(); }
static std::string trace;
#define UNUSED(...) __VA_ARGS__ override { unexpected(); }

namespace dice { namespace hfe {
class ClassManager;
ClassManager* classManager = 0;
IBase* squadManager = 0;
namespace ai {
void* CommonSingletonServer::m_iBFSquad = 0;
// Test interface lifetime only; production concrete AIPlayer is outside this adapter fixture.
IAIPlayer::~IAIPlayer() {}
// A pre-existing registration makes both actual BFSquad constructor/destructor skip refresh.
void BFSingletonServer::updateStaticMethods() { unexpected(); }

struct ObservedPlayer : IAIPlayer
{
    int id = 7;
    int side = 2;
    bool changeIdOnSide = false;
    int getId() const override { trace += 'I'; return id; }
    int getSide() override { trace += 'S'; if (changeIdOnSide) id = 9; return side; }
    UNUSED(void reset())
    UNUSED(world::IPlayer* getPlayer() const)
    UNUSED(void setPlayer(int))
    UNUSED(void setPlayer(world::IPlayer*))
    UNUSED(const std::string& getName() const)
    UNUSED(int getInputId() const)
    UNUSED(bool getIsAlive() const)
    UNUSED(void setIsAlive(bool))
    UNUSED(bool isManDown() const)
    UNUSED(bool isClimbingLadder() const)
    UNUSED(bool isParachuting() const)
    UNUSED(void setSpawnGroup(int))
    UNUSED(void setSpawnKit(int))
    UNUSED(int getSpawnGroup() const)
    UNUSED(int getSpawnKit() const)
    UNUSED(unsigned int getPlayerControlObjectId())
    UNUSED(IAIObject* getSoldier())
    UNUSED(IAIObject* getDefaultVehicle())
    UNUSED(IAIObject* getCurrentVehicle())
    UNUSED(float getFieldOfView())
    UNUSED(float getFieldOfViewModifier())
    UNUSED(float getStandardFieldOfView())
    UNUSED(float getViewDistance())
    UNUSED(void setViewDistance(float))
    UNUSED(void setSide(int))
    UNUSED(int getNumShotsFired(IWeapon*))
    UNUSED(void removePlayerFromGame())
    UNUSED(void setName(const std::string&))
    UNUSED(int getScore())
    UNUSED(void setIsAIPlayer(bool))
    UNUSED(bool getIsAIPlayer())
    UNUSED(const void* getCameraTransformation() const)
    UNUSED(const void* getPlayerInput() const)
};
} // ai

struct ObservedSquadManager : ISquadManager
{
    bool admission = false;
    int leader = 7;
    int expectedId = 7;
    ai::ObservedPlayer* changePlayerDuringLeader = 0;
    bool addToSquad(int team, int squad, int id) override
    { trace += 'J'; check(team == 2 && squad == 3 && id == expectedId); return admission; }
    int getSquadLeader(int team, int squad) override
    { trace += 'L'; check(team == 2 && squad == 3); if (changePlayerDuringLeader) changePlayerDuringLeader->id = leader; return leader; }
    UNUSED(void addRef())
    UNUSED(int getRef())
    UNUSED(int release())
    UNUSED(void* queryInterface(unsigned int) const)
    UNUSED(void reset())
    UNUSED(void reAddDynamicSpawnGroups())
    UNUSED(void sendSquadManagerState(int))
    UNUSED(bool setSquadName(int, int, int, const std::string&, bool))
    UNUSED(void setIsPrivate(int, int, bool, bool))
    UNUSED(bool setIsAccepted(int, int, bool, OrderType, SquadOrderType, bool))
    UNUSED(ISquadInfo* getSquadInfo(int, int))
    UNUSED(void inviteToSquad(int, int, int, bool, bool))
    UNUSED(void removeFromSquad(int, int, int, bool))
    UNUSED(void setSquadLeader(int, int, int, bool))
    UNUSED(int getFirstUnusedSquad(int) const)
    UNUSED(void issueSquadOrder(int, int, SquadOrder&, bool))
    UNUSED(void issueCommanderOrder(int, int, SquadOrder&, bool))
    UNUSED(void issueRequest(int, int, SquadOrder&, bool))
    UNUSED(void resetSquad(int, int))
    UNUSED(void findNewSquadLeader(int, int, int))
    UNUSED(void applyForCommander(int, int))
    UNUSED(int popNextCommanderApplicant(int))
    UNUSED(void acceptCommander(int, int))
    UNUSED(void declineCommander(int, int))
    UNUSED(void resignCommander(int))
    UNUSED(void setCommander(int, int))
    UNUSED(int getCommander(int))
    UNUSED(void* getCommanderCam(int))
    UNUSED(void setCommanderToSoldierMode(int))
    UNUSED(void setCommanderToOverheadCamMode(int))
    UNUSED(void setCommanderOverheadCamZoomMode(int, bool))
    UNUSED(void removeCommanderApplicant(int, int))
    UNUSED(void addOrderMarker(int, SquadOrder&))
    UNUSED(void removeAllOrderMarkers())
    UNUSED(void removeOrderMarker(int))
    UNUSED(void updateOrderMarkers(world::IPlayer*))
};
} }
#undef UNUSED

int main()
{
    using namespace dice::hfe;
    using namespace dice::hfe::ai;
    static_assert(sizeof(void*) == 4, "MSVC x86 fixture");
    int existingRegistration = 0;
    CommonSingletonServer::m_iBFSquad = &existingRegistration;
    ObservedSquadManager manager;
    squadManager = &manager;
    ObservedPlayer player;
    BFSquad adapter;
    IBFSquad* const boundary = &adapter;
    trace.clear();
    check(!boundary->joinSquad(3, &player) && trace == "ISJ");
    manager.admission = true;
    player.changeIdOnSide = true;
    trace.clear();
    check(boundary->joinSquad(3, &player) && trace == "ISJ" && player.id == 9);
    player.changeIdOnSide = false;
    player.id = 7;
    trace.clear();
    check(boundary->isSquadLeader(3, &player) && trace == "SLI");
    manager.leader = 8;
    trace.clear();
    check(!boundary->isSquadLeader(3, &player) && trace == "SLI");
    manager.changePlayerDuringLeader = &player;
    trace.clear();
    check(boundary->isSquadLeader(3, &player) && trace == "SLI" && player.id == 8);
    manager.leader = -1;
    trace.clear();
    check(boundary->isSquadLeader(3, &player) && trace == "SLI" && player.id == -1);
    std::puts("PASS squad bridge: 6 cases, bool propagation, exact identifiers, PE call ordering");
}
