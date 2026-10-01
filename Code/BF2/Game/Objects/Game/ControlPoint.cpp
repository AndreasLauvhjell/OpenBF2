// Control-point capture, ownership, flag motion, and replication.

#include "ControlPoint.h"
#include "TriggerManager.h"

#include "../../../Core/Ids.h"
#include "../../IGame.h"                     // dice::hfe::game, for the server test
#include "../../IGameLogic.h"                // gameLogic, addControlPoint (+0x170)
#include "../../SingletonServer.h"           // the nametag manager init/setFlag/destroy touch
#include "../ObjectLayoutAccess.h"           // init's m_createdOnLoad write (+0x78)
#include "../../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"   // addFlags' lookups
#include "../../../Scene/Object/ObjectUtil.h"                               // setNetworkableComp
#include "../../../Core/Debug.h"
#include "../../../Core/System.h"            // WorldPref::getWorldTime - the rate time stamp
#include "../../../Scene/Object/PlayerManager.h"   // setTeam's player walks
#include "../../GameLogic/SpawnSystem/SpawnManager.h"   // getSpawnGroups
#include "../../GameLogic/SpawnSystem/SpawnGroup.h"     // ISpawnGroup::setTeam / getControlPointId
#include "../../Common/GameEvent.h"          // PostRemoteEvent - the HUD events
#include "../../Python/PythonHost.h"         // buildArgs<IObject*,int> + handleEvent(ScriptEvent) - event 25
#include "../../Python/PyRef.h"
#include "../IObjectSpawner.h"               // the spawners' setTeam through the interface
#include "../IPlayer.h"                      // getTeam / getId
#include "../../../Math/LocalBoundingBox.h"  // reset's pole height
#include <cmath>

namespace dice {
namespace hfe {
namespace world {

} // namespace world
extern IBase* spawnManager;                  // ds:0x8AD3A00 - Core/Globals.cpp (setTeam walks its groups)
namespace world {

// Starts on the authored team on a server. Clients use team -1 until updated unless the template fixes team ownership.
ControlPoint::ControlPoint(const ControlPointTemplate* objectTemplate)
    : Bundle(objectTemplate)
{
    m_refCount = 1;                                  // +0x140
    m_template = objectTemplate;                     // +0x144
    m_team = objectTemplate->getStartTeam();         // +0x148 <- template +0x188

    m_takeOverChangePerSecond = 0.0f;                // +0x14C (a dword 0 store)
    m_takeOverChangeTime = 0.0f;                     // +0x150
    m_takeOverCounter = 0.0f;                        // +0x154
    m_isNeutral = false;                             // +0x158
    // m_flagObjects (+0x15C..+0x164) and m_capturingState (+0x16C): NOT written by the ctor -
    // addFlags, setFlag and setCapturingState own them. Preserved: no zeroing here that the ELF
    // does not do.
    m_currentFlagTeam = 0;                           // +0x168
    m_allowCaptureByTeam[0] = true;                  // +0x170 (team 1)
    m_allowCaptureByTeam[1] = true;                  // +0x171 (team 2)
    m_flagPoleHeight = 3.0f;                         // +0x174
    m_player = -1;                                   // +0x178

    if (game->queryInterface(IID_IGameServer))
    {
        Object::updateFlags(0x800, 0);               // the server-only bit
    }
    else if (!objectTemplate->getUnableToChangeTeam())   // template +0x1A0
    {
        m_team = -1;                                 // client: forget the authored team
    }

    Object::updateFlags(0x602, 1);                   // both paths
}

// 49 bytes (D1) / 62 (D0) - the size is Bundle's chain being run, all of it implicit here.
ControlPoint::~ControlPoint() {}

void ControlPoint::addRef()  { ++m_refCount; }
int  ControlPoint::getRef()  { return m_refCount; }
int  ControlPoint::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

// , four cases. Written out rather than forwarded, and note the offsets: on the object side
// IConsoleSaveable sits at +0x80, where the template side puts it at +0x08. The two hierarchies
// are not parallel.
void* ControlPoint::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)   return const_cast<ControlPoint*>(this);
    if (iid == IID_IObject) return const_cast<ControlPoint*>(this);
    if (iid == IID_IControlPoint)
        return static_cast<IControlPoint*>(const_cast<ControlPoint*>(this));   // the +0x13C sub-object
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<ControlPoint*>(this));   // this + 0x80
    return 0;
}

// Initializes the control point, marks it loaded and registers its nametag and group state.
void ControlPoint::init()
{
    typedef object_detail::LayoutAccess L;

    Bundle::init();
    L::createdOnLoad(this) = true;                   // byte +0x78

    gameLogic->addControlPoint(this, m_template->groupId);       // IGameLogic slot +0x170

    if (SingletonServer::m_iNametagManager)
        SingletonServer::m_iNametagManager->addTouchItem(this);  // slot +0x20 - server no-op

    addFlags();

    if (game->queryInterface(IID_IGameServer))
        updateFlags(0x602, 1);                       // through the vtable, as shipped
}

// One flag per team slot (0 = neutral, 1, 2): resolve the template by name - the control point's
// OWN flagTemplateTeamN if content set it, else GameLogic's per-team default. Stock Init.con files
// populate those defaults through the recovered gameLogic.setTeamFlag console command. A resolved
// flag is spawned, initialised, made a
// child of the control point (IObject slot 61, addChild), and parked at the top of the pole:
// relative (0, hoistMax * poleHeight, 0). Then hoist the current owner's flag.
void ControlPoint::addFlags()
{
    for (int i = 0; i <= 2; ++i)
    {
        const std::string& name = m_template->m_flagTemplateTeam[i].empty()
                                ? gameLogic->getTeamFlag(i)
                                : m_template->m_flagTemplateTeam[i];

        IObjectTemplate* t = objectTemplateManager->getTemplate(name);
        IObject* flag = t ? t->createObject() : 0;
        m_flagObjects[i] = flag;

        if (flag)
        {
            flag->init();                            // IObject slot +0x20
            addChild(flag);                          // IObject slot 61 - the flag rides the point
            Vec3 pos;
            pos.x = 0.0f;
            pos.y = m_template->hoistMinMax.y * m_flagPoleHeight;
            pos.z = 0.0f;
            flag->setRelativePosition(pos);          // IObject slot 57
        }
    }

    setFlag(m_team, true);                           // hoist the owner's flag (vtable, as shipped)
}

// The early-out is (team unchanged AND not a forced raise). Then: refresh the nametag touch item,
// hide every flag (updateFlags(1, 0) on each - bit 0, the same "disabled" bit the physics loops
// skip on), and if the team is in range: show its flag (updateFlags(0, 1) clears the bit), record
// it, fire the authored trigger if the template names one (signalTriggerId != -1, guarded on the
// TriggerManager global exactly as shipped - null in this tree, so silently skipped), record the
// shown team, and reposition.
void ControlPoint::setFlag(int team, bool raise)
{
    if (team == m_currentFlagTeam && !raise)
        return;

    if (SingletonServer::m_iNametagManager)
        SingletonServer::m_iNametagManager->addTouchItem(this);

    for (int i = 0; i <= 2; ++i)
        if (m_flagObjects[i])
            m_flagObjects[i]->updateFlags(1, 0);     // hide - set bit 0

    if (static_cast<unsigned int>(team) <= 2)
    {
        if (m_flagObjects[team])
            m_flagObjects[team]->updateFlags(0, 1);  // show - clear bit 0

        m_currentFlagTeam = team;

        if (m_template->signalTriggerId != -1)
        {
            if (triggerManager)
                triggerManager->handleEvent(TriggerType_ControlPoint, this, m_template->signalTriggerId);
        }

        m_capturingState = team;                     // +0x16C the same field get/setCapturingState own
        updateFlagPosition();
    }
}

// Sets flag height from clamped capture progress, authored hoist limits and pole height.
// Clients replace the initial height sentinel with their measured bounding-box height.
void ControlPoint::updateFlagPosition()
{
    if (m_flagPoleHeight == 3.0f && !game->queryInterface(IID_IGameServer))
        m_flagPoleHeight = getFullBoundingBox()->m_max.y;                    // vslot +0xA0, box +0x10

    IObject* flag = m_flagObjects[m_currentFlagTeam];
    if (!flag)
        return;

    const float span = m_template->hoistMinMax.y - m_template->hoistMinMax.x;
    float t = m_takeOverCounter;
    if (t < 0.0f)      t = 0.0f;
    else if (t > 1.0f) t = 1.0f;

    Vec3 pos;
    pos.x = 0.0f;
    pos.y = (t * span + m_template->hoistMinMax.x) * m_flagPoleHeight;
    pos.z = 0.0f;
    flag->setRelativePosition(pos);
}

// the template's controlPointId, raw read - identity lives on the authored side.
int ControlPoint::getControlerId()
{
    return m_template->controlPointId;
}

int   ControlPoint::getTeam()                          { return m_team; }                        // +0x148
float ControlPoint::getTakeOverChangePerSecond() const { return m_takeOverChangePerSecond; }     // +0x14C
float ControlPoint::getTakeOverCounter()               { return m_takeOverCounter; }             // +0x154
void  ControlPoint::setCapturingState(int state)       { m_capturingState = state; }             // +0x16C
int   ControlPoint::getCapturingState() const          { return m_capturingState; }
void  ControlPoint::setPlayer(int player)              { m_player = player; }                    // +0x178
int   ControlPoint::getPlayer()                        { return m_player; }
int   ControlPoint::getFlag() const                    { return m_currentFlagTeam; }             // +0x168

// 31 / 29 B: `byte [this + 0x16F + team]`, and only for team 1 or 2 (`(team - 1) <= 1` unsigned) -
// any other team reads false / writes nothing. The client's setNetUpdate calls the setter with team
// 0 for the first flag (a dice slip - it is silently ignored here, as shipped).
bool ControlPoint::getAllowCaptureByTeam(int team)
{
    if (static_cast<unsigned int>(team - 1) <= 1u)
        return m_allowCaptureByTeam[team - 1];
    return false;
}
void ControlPoint::setAllowCaptureByTeam(int team, bool allow)
{
    if (static_cast<unsigned int>(team - 1) <= 1u)
        m_allowCaptureByTeam[team - 1] = allow;
}

// the template's byte at +0x1BC.
bool ControlPoint::getShowOnMinimap()
{
    return m_template->showOnMinimap;
}

// Unregisters the control point and its nametag item, then delegates to SimpleObject destruction.
void ControlPoint::destroy(bool force)
{
    gameLogic->removeControlPoint(this);             // IGameLogic slot +0x17C

    if (SingletonServer::m_iNametagManager)
        SingletonServer::m_iNametagManager->removeItem(2, this);   // slot +0x4C

    SimpleObject::destroy(force);
}

// Attaches control-point replication outside precaching. Failure triggers a fatal diagnostic.
void ControlPoint::initNetworkableComp()
{
    if (m_template && !m_template->getNetworkableInfo().empty()
        && !setNetworkableComp(this, CID_ControlPointNetworkable))
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Game/ControlPoint.cpp", 147, __FUNCTION__)
            << "Failed to create ControlPointNetworkable";
    }
}

// Restores the starting team and capture state, measures pole height and updates the displayed flag.

// Capture progress, ownership and round-reset behavior.
namespace {
// The two HUD events setTeam raises through PostRemoteEvent(ECHud, id, ...) with the payload
// {controlPointId, player}: 64 to the players of the NEW team, 65 to everyone else (and to the OLD
// team when a point is lost). Names [CHOSEN]; ids read.
const unsigned int kHudControlPointCaptured = 64;
const unsigned int kHudControlPointLost     = 65;
// ScriptEvent 25 - the capture-status handler ("this control point, 1 = captured / 0 = neutralised");
// enumerator name [UNRESOLVED], value read.
const int kScriptEventControlPointStatus = 25;
} // namespace

// . Server only, and only while the game status is 1 (playing). Nothing while the
// rate is (near) zero (|rate| <= 1e-5). counter += rate * dt: past 1.0 -> clamp to 1.0, rate 0,
// event 25 (this, 1); below 0.0 -> counter 0, rate 0, event 25 (this, 0); in between -> just the
// flag. The event args tuple is a PyRef - released after the call. Then updateFlagPosition, always.
void ControlPoint::handleUpdate(float dt, unsigned int)
{
    if (!game->queryInterface(IID_IGameServer))
        return;
    if (gameLogic->getGameStatus() != 1)
        return;
    if (!(fabsf(m_takeOverChangePerSecond) > 9.9999997e-6f))    // +0x14C
        return;
    const float counter = m_takeOverChangePerSecond * dt + m_takeOverCounter;   // +0x154
    m_takeOverCounter = counter;
    if (counter > 1.0f)
    {
        m_takeOverCounter = 1.0f;
        m_takeOverChangePerSecond = 0.0f;
        IObject* self = this;
        python::PyRef args = python::buildArgs(self, 1);
        python::pythonHost()->handleEvent(static_cast<ScriptEvent>(kScriptEventControlPointStatus), args.get());
    }
    else if (counter >= 0.0f)
    {
        updateFlagPosition();
        return;
    }
    else
    {
        m_takeOverCounter = 0.0f;
        m_takeOverChangePerSecond = 0.0f;
        IObject* self = this;
        python::PyRef args = python::buildArgs(self, 0);
        python::pythonHost()->handleEvent(static_cast<ScriptEvent>(kScriptEventControlPointStatus), args.get());
    }
    updateFlagPosition();
}

// Sends capture or loss HUD events before storing the new team.
// Then updates nametags and, on servers, resets capture rate and transfers associated spawn groups and object spawners.
void ControlPoint::setTeam(unsigned int team)
{
    if (gameLogic->getGameStatus() == 1 && m_team != -1 && gameLogic->isServer())
    {
        IPlayer* localHuman = playerManager->getLocalHumanPlayer();          // +0x30
        int payload[2];
        payload[0] = getControlerId();
        payload[1] = m_player;                                               // +0x178
        bool notifyEveryone = false;
        bool notifyOldTeam  = false;
        if (team != 0)
            notifyEveryone = (m_team == 0);
        else if (m_team == 0)
            notifyEveryone = true;
        else
            notifyOldTeam = true;

        if (notifyEveryone)
        {
            const std::list<IPlayer*>& players = playerManager->getPlayers();          // +0x3C
            if (!players.empty())
            {
                for (std::list<IPlayer*>::const_iterator it = players.begin(); it != players.end(); ++it)
                {
                    IPlayer* player = *it;
                    if (!player)
                        continue;
                    const unsigned int id = (static_cast<unsigned int>(player->getTeam()) == team)
                                          ? kHudControlPointCaptured : kHudControlPointLost;
                    PostRemoteEvent* event = new PostRemoteEvent(ECHud, id, localHuman == player, payload, 8, 0.0f);
                    game->sendGameEventIfPlaying(static_cast<unsigned int>(player->getId()), event);   // IGame +0x88
                }
            }
        }
        else if (notifyOldTeam)
        {
            const std::list<IPlayer*>& players = playerManager->getPlayers(m_team);   // +0x48
            if (!players.empty())
            {
                for (std::list<IPlayer*>::const_iterator it = players.begin(); it != players.end(); ++it)
                {
                    IPlayer* player = *it;
                    if (!player)
                        continue;
                    PostRemoteEvent* event = new PostRemoteEvent(ECHud, kHudControlPointLost, localHuman == player, payload, 8, 0.0f);
                    game->sendGameEventIfPlaying(static_cast<unsigned int>(player->getId()), event);
                }
            }
        }
    }

    m_team = static_cast<int>(team);                                         // +0x148
    if (SingletonServer::m_iNametagManager)
        SingletonServer::m_iNametagManager->addTouchItem(this);              // slot +0x20 - server no-op
    if (!gameLogic->isServer())
        return;
    m_takeOverChangePerSecond = 0.0f;                                        // +0x14C

    const std::map<int, ISpawnGroup*>& groups = static_cast<SpawnManager*>(spawnManager)->getSpawnGroups();   // +0x40
    for (std::map<int, ISpawnGroup*>::const_iterator it = groups.begin(); it != groups.end(); ++it)
    {
        ISpawnGroup* group = it->second;
        if (group->getControlPointId() == static_cast<unsigned int>(m_template->controlPointId))   // slot 19 vs +0x190
            group->setTeam(m_team);                                          // slot 6
    }
    const std::map<unsigned int, IObject*>& spawners =
        *static_cast<const std::map<unsigned int, IObject*>*>(gameLogic->getObjectSpawnerMap());   // +0x19C
    for (std::map<unsigned int, IObject*>::const_iterator it = spawners.begin(); it != spawners.end(); ++it)
    {
        IObjectSpawner* spawner = static_cast<IObjectSpawner*>(it->second->queryInterface(IID_IObjectSpawner));
        if (spawner->getControlPointId() == m_template->controlPointId)      // IObjectSpawner slot 13
            spawner->setTeam(m_team);                                        // slot 4
    }
}

// the banked read, verbatim: back to the template's start team, the counter
// hoisted (1.0) for an owned point / 0.0 for a neutral one, the rate zeroed, the neutral flag, the
// flag raised for the team, the pole height taken from the full bounding box's max.y
// unconditionally, and the hoist applied.
void ControlPoint::reset()
{
    setTeam(static_cast<unsigned int>(m_template->getStartTeam()));         // vslot +0x19C, template +0x188
    m_takeOverCounter = (m_team != 0) ? 1.0f : 0.0f;                        // +0x154
    m_takeOverChangePerSecond = 0.0f;                                        // +0x14C
    m_isNeutral = (m_team == 0);                                             // +0x158
    setFlag(m_team, true);                                                   // vslot +0x1BC
    m_flagPoleHeight = getFullBoundingBox()->m_max.y;                        // vslot +0xA0, box +0x10
    updateFlagPosition();
}

// / 0x816F5E0, 34 B: each spawned flag is unlinked (removeChild, IObject slot 62)
// and destroyed (destroy(true), slot 13); refresh = remove then add.
void ControlPoint::removeFlags()
{
    for (int i = 0; i <= 2; ++i)
    {
        if (m_flagObjects[i])
        {
            removeChild(m_flagObjects[i]);                                   // slot 62
            m_flagObjects[i]->destroy(true);                                 // slot 13
        }
    }
}

void ControlPoint::refreshFlags()
{
    removeFlags();
    addFlags();
}

// a fully captured point (counter >= 1) refuses a positive rate; a rate is
// accepted when the counter is above zero, or when the rate itself is >= 0 - i.e. an empty counter
// refuses a negative rate. An accepted rate is stored with the world time.
void ControlPoint::setTakeOverChangePerSecond(float rate)
{
    if (m_takeOverCounter >= 1.0f && rate > 0.0f)
        return;
    if (0.0f < m_takeOverCounter || 0.0f <= rate)
    {
        m_takeOverChangePerSecond = rate;                                    // +0x14C
        m_takeOverChangeTime = static_cast<float>(WorldPref::getWorldTime()); // +0x150
    }
}

// -1 without a rate; else the seconds to reach 1.0 - the client extrapolates the
// counter by the rate over the time since the rate was set (the server's counter is live).
float ControlPoint::getTimeUntilTakeOver()
{
    if (m_takeOverChangePerSecond == 0.0f)
        return -1.0f;
    float extrapolated = 0.0f;
    if (!gameLogic->isServer())
    {
        const float now = static_cast<float>(WorldPref::getWorldTime());
        extrapolated = (now - m_takeOverChangeTime) * m_takeOverChangePerSecond;
    }
    return (1.0f - (extrapolated + m_takeOverCounter)) / fabsf(m_takeOverChangePerSecond);
}

// / 0x816F640, 36 B each: the template's area value for the team - while the point is
// neutral (m_isNeutral); 0 otherwise.
unsigned int ControlPoint::getAreaValueTeam1() { return m_isNeutral ? m_template->areaValueTeam1 : 0u; }   // +0x1B0
unsigned int ControlPoint::getAreaValueTeam2() { return m_isNeutral ? m_template->areaValueTeam2 : 0u; }   // +0x1B4

// teams 1 / 2 only - `(team - 1) <= 1` unsigned - the template's list is copied out.
bool ControlPoint::getSupplyGroupsNeeded(int team, std::vector<SupplyGroup>& out)
{
    if (static_cast<unsigned int>(team - 1) <= 1u)
    {
        out = m_template->m_supplyGroupsNeeded[team - 1];                    // template +0x1B8 + 12*team
        return true;
    }
    return false;
}

// the template's name, by value.
std::string ControlPoint::getName()
{
    return std::string(m_template->getName());                               // template +0x0C
}

} // namespace world
} // namespace hfe
} // namespace dice
