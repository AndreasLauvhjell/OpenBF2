// [INFERRED] Source placement.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFGame operations.

#include "BFGame.h"

#include "../../../Core/Debug.h"        // the no-settings report

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../../../Game/GameServer/ServerSettings.h"
#include "../../../Game/IGame.h"       // the `game` global and queryInterface (+0x0C)
#include "../../../Game/IGameLogic.h"  // the `gameLogic` global - the fog and view-distance reads
#include "../../../Core/Ids.h"         // IID_IGameServer
#include "../../../Core/SettingsRepostitory.h"   // the `settings` global - getLevelPath's two keys
#include "../../../Game/Objects/IObject.h"                  // getAbsolutePosition, getTemplate
#include "../../../Game/Objects/ControlPointTemplate.h"     // getRadius, getUnableToChangeTeam
#include "../../../Game/Objects/Game/ControlPoint.h"        // getTeam, getTakeOverCounter
#include "../../../Game/GameLogic/ScoreSystem/ScoreManager.h" // scoreManager, TeamScore::m_tickets
#include "../../../Game/GameLogic/SpawnSystem/SpawnManager.h"
#include "../../../Game/GameLogic/SpawnSystem/SpawnGroup.h"

#include <map>                                              // the control-point map's own type

namespace dice {
namespace hfe {

extern ClassManager* classManager;
extern IBase* spawnManager; // Core/Globals.cpp; SpawnManager's primary IBase subobject.
namespace world { extern IBase* scoreManager; } // Core/Globals.cpp's shared storage

namespace ai {

// First wins in, exact match out.
IAIGame::IAIGame()
{
    if (!CommonSingletonServer::m_iaiGame)
        classManager->registerSimpleSingleton(std::string("IAIGame"), this);
}

IAIGame::~IAIGame()
{
    if (CommonSingletonServer::m_iaiGame == this)
        classManager->unregisterSimpleSingleton(std::string("IAIGame"));
}

// (!) it starts precaching true. The constructor's only store is `[this+4] = 1`.
BFGame::BFGame()
    : m_isPrecaching(true)
{
}

BFGame::~BFGame()
{
}

// - a tail jump to ServerSettings::getMaxPlayers
// on the serverSettings global (ds:).
int BFGame::getMaxNumberOfPlayers() const
{
    return serverSettings->getMaxPlayers();
}

// Copies game settings into the AI configuration, including bot population and team balance.
// [RETIRE-WHEN: aidll-01-adapters] Remove when the corresponding implementation is complete and validated.
int BFGame::getAISettingsFromGame(int& a, bool& b, float& c) const
{
    if (classManager->getSingleton(std::string("GeneralSettings")))
    {
        BF2_AI_FRONTIER("dice::hfe::ai::BFGame::getAISettingsFromGame"
                        " - the MENU arm; this image has no GeneralSettings slots");
    }

    // no settings and no menu is A reported failure, not A silent zero - and the caller honours
    // the false by leaving the script's values alone.
    if (!serverSettings)
    {
        Debug(DebugTypeError, "AI/AIInterface/BFKnowledge/BFGame.cpp", 104)
            << "GeneralSettings are not available, no settings importade from menu.";
        return 0;
    }

    a = serverSettings->getCoopBotCount();
    b = false;
    // the scale is 0.0099999998f in the binary - the nearest float to 1/100, not the double 0.01.
    // Difficulty is authored 0..100 and the AI wants 0..1.
    c = serverSettings->getCoopBotDifficulty() * 0.0099999998f;
    return 1;
}

// Returns whether the current game exposes the server interface; false when no game exists.
bool BFGame::isServer() const
{
    if (!game)
        return false;

    return game->queryInterface(IID_IGameServer) != 0;
}

// getTeamScore is the typed +0x10
// dispatch on both ports; the returned TeamScore's +0x84 is read without a guard.
int BFGame::getTickets(int team) const
{
    return static_cast<world::IScoreManager*>(world::scoreManager)->getTeamScore(team)->m_tickets;
}

int BFGame::getClosestHostileMine(IAIPlayer* player, Vec3 from, bool& found, Vec3& position, Vec3& normal, unsigned int& id, std::vector<unsigned int>& ignore) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGame::getClosestHostileMine");
}

// Only gameLogic is guarded.
// The map is captured once and traversed in key order; every point's live team
// is queried once. PE uses the adjusted IControlPoint base (+0x1C), whereas ELF
// uses ControlPoint's primary +0x1A0; the native typed call handles that difference.
int BFGame::getNumberOfControlPoints(int team) const
{
    if (!gameLogic)
        return 0;

    typedef std::map<unsigned int, world::IObject*> ControlPointMap;
    ControlPointMap* const points = gameLogic->getControlPointMap();
    int count = 0;
    for (ControlPointMap::iterator it = points->begin(); it != points->end(); ++it)
    {
        if (static_cast<world::ControlPoint*>(it->second)->getTeam() == team)
            ++count;
    }
    return count;
}

// Control-point queries borrow the game logic's map.
// The game logic and requested control-point ID must be valid.

// Map lookup, then vtable slot +0x1A0 on the object's primary table - which
// `SpawnPoint::getActive` already recorded as `ControlPoint::getTeam`, with the warning that
// `IControlPoint`'s own +0x1C is a different slot. The typed call below sidesteps that entirely.
int BFGame::getOwnerOfControlPoint(unsigned int index) const
{
    typedef std::map<unsigned int, world::IObject*> ControlPointMap;

    ControlPointMap* const points = gameLogic->getControlPointMap();

    // no `end` check. See the block above.
    const ControlPointMap::iterator found = points->find(index);

    return static_cast<world::ControlPoint*>(found->second)->getTeam();
}

// The result is the
// control point's float capture progress, forwarded in ST0 without conversion.
// ELF calls the primary +0x1AC slot; PE adjusts to IControlPoint and calls +0x5C.
// Like getOwnerOfControlPoint, neither the game/map nor find's result is guarded.
float BFGame::getControlPointTakeOverCounter(unsigned int index) const
{
    typedef std::map<unsigned int, world::IObject*> ControlPointMap;

    ControlPointMap* const points = gameLogic->getControlPointMap();
    const ControlPointMap::iterator found = points->find(index);

    return static_cast<world::ControlPoint*>(found->second)->getTakeOverCounter();
}

// Returns whether the control point can change teams and writes that value to active.
// Writes the live position and template radius. The control-point ID must exist.
bool BFGame::getControlPointInfo(unsigned int index, Vec3& position, float& radius, bool& active) const
{
    typedef std::map<unsigned int, world::IObject*> ControlPointMap;

    ControlPointMap* const points = gameLogic->getControlPointMap();

    // no `end` check. See the block above.
    const ControlPointMap::iterator found = points->find(index);

    world::IObject* const object = found->second;

    position = object->getAbsolutePosition();

    const world::ControlPointTemplate* const controlPointTemplate =
        static_cast<const world::ControlPointTemplate*>(object->getTemplate());

    radius = controlPointTemplate->getRadius();

    active = !controlPointTemplate->getUnableToChangeTeam();

    return active;
}

// [FRONTIER] Finds a control point within the horizontal box defined by a and b.
// [RETIRE-WHEN: aidll-01-adapters] Implement and validate insideCentreBox before enabling this query.
int BFGame::findControlPoint(const Vec2& a, const Vec2& b) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGame::findControlPoint"
                    " - the walk is read; dice::hfe::insideCentreBox is not");
}

// The returned list is
// the manager's borrowed scratch storage, not an owned copy. No manager null guard.
const std::list<int>& BFGame::getSpawnPointList(int team) const
{
    return static_cast<SpawnManager*>(spawnManager)->getGroupsForTeam(team);
}

// Both calls dispatch
// virtually; the group and returned position are borrowed, with no null guard.
const Vec3& BFGame::getSpawnPointPosition(int index) const
{
    return static_cast<SpawnManager*>(spawnManager)->getSpawnGroup(index)->getPos();
}

// retain the returned
// group's identity for its virtual query. No manager/group guard; preserve the
// full 32-bit result, including the caller's -1 sentinel.
int BFGame::getControlPointId(int index) const
{
    return static_cast<int>(static_cast<SpawnManager*>(spawnManager)->getSpawnGroup(index)->getControlPointId());
}

int BFGame::getSpawnPointPositions(std::vector<Vec3>& out, unsigned char team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGame::getSpawnPointPositions");
}

void BFGame::getControlPointPositions(std::vector<Vec3>& out, unsigned char team) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGame::getControlPointPositions");
}

// Capture team before
// getActive(true), but compare it afterwards. A matching team's human-only query
// still runs when the captured active flag is false.
bool BFGame::isEligibleSpawnpoint(int team, int groupId) const
{
    if (groupId == 0)
        return false;

    ISpawnGroup* const group = static_cast<SpawnManager*>(spawnManager)->getSpawnGroup(groupId);
    if (!group)
        return false;

    const int groupTeam = group->getTeam();
    const bool active = group->getActive(true);
    if (groupTeam != team)
        return false;
    if (group->getOnlyForHuman())
        return false;
    return active;
}

bool BFGame::isEnterOnSpawnSpawnpoint(int index) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGame::isEnterOnSpawnSpawnpoint");
}

int BFGame::getEnterOnSpawnSpawnpointObject(int a, int b) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGame::getEnterOnSpawnSpawnpointObject");
}

// Returns the level directory joined with the configured level name.
std::string BFGame::getLevelPath() const
{
    const std::string& level =
        settings->stringGet(std::string("GSLoadLevel"), std::string(""));
    const std::string& directory =
        settings->stringGet(std::string("GSLevelDirectory"), std::string(""));

    return directory + level;
}

// - `mov eax, 0Ah`. A constant: ten squads.
int BFGame::getMaxNumberOfSquads() const
{
    return 10;
}

// - `mov eax, 6`. A constant: six per squad.
int BFGame::getMaxNumberOfSquadMembers() const
{
    return 6;
}

// Computes the recommended AI view distance from the fog settings.
float BFGame::getRecomendedViewDistance() const
{
    const Vec4& fog = gameLogic->getFogStartEndAndBase();

    float t = 0.0f;
    for (;;)
    {
        const float linear  = fog.z * t;
        const float density = (fog.w > linear) ? linear : (t * t * t + fog.w);

        if (density >= 0.8f)
            break;

        t += 0.01f;
        if (t > 1.0f)
            break;
    }

    const float fogged = t * (fog.y - fog.x) + fog.x;

    float distance = gameLogic->getMaximumLevelViewDistance();
    if (distance > fogged)
        distance = fogged;
    if (distance > 300.0f)
        distance = 300.0f;

    return distance;
}

// Returns whether the game is in the playing state.
bool BFGame::isPlaying() const
{
    return gameLogic->getGameStatus() == GameStatus_Playing;
}

void BFGame::setIsPrecaching(bool precaching)
{
    m_isPrecaching = precaching;
}

bool BFGame::getIsPrecaching() const
{
    return m_isPrecaching;
}

void BFGame::enemySpottedByAI(world::IPlayer* player, world::IObject* object) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFGame::enemySpottedByAI");
}

} // namespace ai
} // namespace hfe
} // namespace dice

#if 0   // Disabled subsystem declarations.

#endif
