// Code/Tools/BF2Editor/App/EditorGame.cpp - see EditorGame.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/EditorGame.h"

#include "Framework/EditorLog.h"

#include "Core/EventManager.h"
#include "Core/Ids.h"
#include "Core/SettingsRepostitory.h"
#include "Game/Common/GameEvent.h"
#include "Game/GameLogic/ClientGameLogic.h"
#include "Game/MapInfo.h"
#include "Game/Objects/Base/Camera.h"
#include "Game/Objects/Game/Ladder.h"
#include "Game/Objects/Game/Soldier.h"
#include "Game/Objects/ICameraObject.h"
#include "Game/Objects/IObject.h"
#include "Game/Objects/IObjectTemplate.h"
#include "Game/Objects/IPlayer.h"
#include "Game/Objects/IPlayerControlObjectTemplate.h"
#include "Game/Objects/ISoldier.h"
#include "IO/Input/Input.h"
#include "Math/Mat4.h"
#include "Physics/PhysicsUtil.h"
#include "Scene/Object/ObjectManager.h"
#include "Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "Scene/Object/PlayerManager.h"

namespace dice { namespace hfe { namespace io { extern IBase* inputManager; } } }

namespace bf2editor
{

using namespace dice::hfe;

namespace
{

// The input manager the game switches control maps through. The editor starts neither the game's input devices nor
// Controls.con (App/EngineHost.cpp), so the manager may be absent or hold no maps: a missing map is a null player map,
// which the manager accepts (ControlMapManager::applyActive does nothing without one).
void setPlayerControlMap(const char* name)
{
    if (io::inputManager)
        static_cast<io::IInputManager*>(static_cast<void*>(io::inputManager))->setPlayerControlMap(std::string(name));
}

bool isLocalPlayer(world::IPlayer* player)
{
    return world::playerManager && player == world::playerManager->getLocalHumanPlayer();
}

void clearPendingExit(world::IPlayer* player)
{
    if (isLocalPlayer(player) && gameLogic)
        static_cast<ClientGameLogic*>(gameLogic)->clearPendingExit();
}

} // namespace

EditorGame::EditorGame()
    : m_refCount(1)
    , m_simulationRate(30)
{
}

EditorGame::~EditorGame()
{
}

void EditorGame::setLevel(const std::string& levelDirectory, const std::string& name, const std::string& gameMode,
                          int maxPlayers)
{
    m_mapInfo.setLevelPath(levelDirectory);
    m_mapInfo.setMapName(name);
    m_mapInfo.setGameMode(gameMode);
    m_mapInfo.setMaxNumberOfPlayers(maxPlayers);
}

void EditorGame::addRef()
{
    ++m_refCount;
}

int EditorGame::getRef()
{
    return m_refCount;
}

int EditorGame::release()
{
    // Never self-deletes: the editor owns the object and deletes it after unregistering "Game".
    return --m_refCount;
}

void* EditorGame::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IGame)
        return static_cast<IGame*>(const_cast<EditorGame*>(this));
    return 0;                                   // no IGameServer / IGameClient: the editor is neither
}

bool EditorGame::init()                         { return true; }
void EditorGame::shutdown()                     {}
bool EditorGame::unload()                       { return true; }   // the editor's level closes through GameLogic
StatsMonitor* EditorGame::getStatsMonitor() const { return 0; }
bool EditorGame::update(int, float)             { return true; }   // the editor's frame loop updates the world
bool EditorGame::say(const std::string&)        { return false; }

bool EditorGame::radioMessage(TargetChannel, const std::string&, world::IPlayer*, unsigned int, bool, bool)
{
    return false;
}

bool EditorGame::radioMessageFromPlayer(TargetChannel, const std::string&, unsigned int, unsigned int, bool, bool)
{
    return false;
}

bool EditorGame::gameInfoMessage(TargetChannel, const std::string&, unsigned int, unsigned int, bool)
{
    return false;
}

void EditorGame::confirmGhost(int, unsigned int) {}
void EditorGame::confirmReset(int)                {}

// The play-test's object maker (soldier mode, PlayTest.cpp): GameServer::createObjectOnAllClients (GameServer.cpp)
// without the network - the transform from the rotation's negated ZXY angles and the position, the object, its
// 0x20000 flag, the control object's default team and the "object created" event. No networkable is registered and
// no creation event is sent: the editor has no clients.
world::IObject* EditorGame::createObjectOnAllClients(const world::IObjectTemplate* objectTemplate, Math::Vec3 position,
                                                     Math::Vec3 rotation, bool, int, int defaultTeam)
{
    if (!objectTemplate || !world::objectManager)
        return 0;
    Mat4 transform = Mat4::getIdentity();
    if (objectTemplate->getClassID() == world::CID_ZiplineTemplate)
    {
        transform.m[4] = rotation.x;            // a zipline carries its second point in row 1, as the server's does
        transform.m[5] = rotation.y;
        transform.m[6] = rotation.z;
    }
    else
    {
        setRotationZXY(transform, -rotation.z, -rotation.y, -rotation.x);
    }
    transform.m[12] = position.x;
    transform.m[13] = position.y;
    transform.m[14] = position.z;

    world::IObject* const object = world::objectManager->createObject(objectTemplate, 0, &transform);
    if (!object)
        return 0;
    object->updateFlags(0x20000u, 0);
    for (world::IObject* o = object; o; o = o->getParent())
    {
        if (world::IPlayerControlObject* pco = static_cast<world::IPlayerControlObject*>(
                o->queryInterface(world::IID_IPlayerControlObject)))
        {
            pco->setDefaultTeam(defaultTeam);
            break;
        }
    }
    eventManager->postEvent(ECGame, 3, object, 0.0f);
    return object;
}

// No connection: the event is released, exactly as GameClient's send slots do without one
// (Game/GameClient/GameClient.cpp).
void EditorGame::sendGameEventToAllPlaying(GameEvent* event)                   { if (event) event->release(); }
void EditorGame::sendGameEventIfPlaying(unsigned int, GameEvent* event)        { sendGameEventToAllPlaying(event); }
void EditorGame::sendGameEventToAll(GameEvent* event)                          { sendGameEventToAllPlaying(event); }
void EditorGame::sendGameEvent(unsigned int, GameEvent* event)                 { sendGameEventToAllPlaying(event); }
void EditorGame::sendGameEventToAllConnected(GameEvent* event)                 { sendGameEventToAllPlaying(event); }
void EditorGame::sendGameEventIfConnected(unsigned int, GameEvent* event)      { sendGameEventToAllPlaying(event); }
void EditorGame::sendMissileMessageToOccupyingPlayers(world::IObject*, world::MissileLockMessage) {}

// Seating a player. Every new Player reaches
// this from its constructor (Player::allocateFreeCamera -> _setVehicle(0, 0) -> enterVehicle(player, freeCamera)),
// and Player::setVehicle aborts (Player.cpp line 901) for a player put into a real vehicle while holding none - so
// the editor's game seats for real. The body is GameClient::enterVehicle's (GameClient.cpp; the editor runs the
// client's game logic): the control object at or above the target, a player on a ladder taken off it, the outgoing
// camera's pitch carried across for a template that asks, the old control object told what is entered, the new one
// entered, the default vehicle, the local player's physics hand-over and control map, the "entered vehicle" event
// and the pending exit cleared. Where the client's shipped body stops on a retail INT3 (a default vehicle that is not
// a soldier, a ladder id the registry does not hold), the editor refuses and logs.
bool EditorGame::enterVehicle(world::IPlayer* player, world::IObject* object, bool setAsDefaultVehicle)
{
    if (!player || !object)
        return false;
    world::IPlayerControlObject* const pco = world::findOwningPlayerControlObject(object);
    if (!pco)
        return false;

    // Off a ladder first.
    world::IObject* const current = player->getVehicle();
    if (current && current->getTemplate()->getClassID() == world::CID_LadderContainerTemplate)
    {
        world::IObject* const defaultVehicle = player->getDefaultVehicle();
        world::ISoldier* const soldier = defaultVehicle
            ? static_cast<world::ISoldier*>(defaultVehicle->queryInterface(world::IID_ISoldier)) : 0;
        std::map<int, world::Ladder*>::iterator ladder =
            soldier ? world::ladderMap.find(static_cast<int>(soldier->getLadderId())) : world::ladderMap.end();
        if (ladder == world::ladderMap.end() || !ladder->second)
        {
            logLine(LogLevel::Warning, "play-test: %s is on a ladder the editor cannot find; not seated",
                    player->getName().c_str());
            return false;
        }
        ladder->second->detachSoldier(soldier);
    }

    world::IPlayerControlObjectTemplate* const pcoTemplate = static_cast<world::IPlayerControlObjectTemplate*>(
        object->getTemplate()->queryInterface(world::IID_IPlayerControlObjectTemplate));

    // The outgoing camera: its pitch (the one angle the client carries) and its view mode.
    world::ICameraObject* oldCamera = 0;
    float carriedRotation = 0.0f;
    if (world::IObject* const defaultVehicle = player->getDefaultVehicle())
    {
        world::IPlayerControlObject* const defaultPco = static_cast<world::IPlayerControlObject*>(
            defaultVehicle->queryInterface(world::IID_IPlayerControlObject));
        oldCamera = defaultPco ? defaultPco->getActiveCamera() : 0;
        if (oldCamera)
        {
            world::IObject* const cameraObject = static_cast<world::IObject*>(oldCamera->queryInterface(world::IID_IObject));
            float rotZ = 0.0f, rotX = 0.0f, rotY = 0.0f;
            getRotationZXY(cameraObject->getRelativeTransformation(), rotZ, rotX, rotY);
            carriedRotation = -rotX;
        }
    }
    const int viewMode = oldCamera ? static_cast<int>(oldCamera->getViewMode()) : 8;

    // The object being left is told what is entered, then the new control object takes the player.
    if (world::IPlayerControlObject* const oldPco = world::findOwningPlayerControlObject(player->getVehicle()))
        oldPco->onEnter(object);
    pco->enter(player);
    if (setAsDefaultVehicle)
        player->setDefaultVehicle(player->getVehicle());

    // The local player's vehicle leaves the general physics pass: the play-test step simulates it (PlayTest.cpp,
    // as GameClient::stepWorld does), and its control map follows the vehicle type.
    if (!player->getIsRemote())
    {
        disablePhysics(player->getVehicle(), false);
        enableObject(object, false);
        if (pcoTemplate)
        {
            const int category = pcoTemplate->getControlsCategory() == 4 ? pcoTemplate->getVehicleCategory()
                                                                           : pcoTemplate->getControlsCategory();
            const char* controlMap = "LandPlayerInputControlMap";
            if (pco->isChildSoldierActive())
                controlMap = "InfantryPlayerInputControlMap";
            else if (category == 2)
                controlMap = "AirPlayerInputControlMap";
            else if (category == 3)
                controlMap = "HelicopterPlayerInputControlMap";
            else if (category == 1)
                controlMap = "SeaPlayerInputControlMap";
            setPlayerControlMap(controlMap);
        }
    }

    // The camera's pitch carried across, for a template that maintains the camera on enter (the parachute).
    world::ICameraObject* const newCamera = pco->getActiveCamera();
    if (newCamera && pcoTemplate && pcoTemplate->getMaintainCameraOnEnter())
    {
        if (settings && settings->boolGet(std::string("GSDeveloper"), false) && object != player->getFreeCamera())
            newCamera->setViewMode(static_cast<world::CameraViewMode>(viewMode), 0, false);
        world::IObject* const cameraObject = static_cast<world::IObject*>(newCamera->queryInterface(world::IID_IObject));
        Mat4 transform = cameraObject->getParent() ? cameraObject->getRelativeTransformation()
                                                   : cameraObject->getAbsoluteTransformation();
        setRotationZXY(transform, 0.0f, -carriedRotation, 0.0f);
        if (cameraObject->getParent())
            cameraObject->setRelativeTransformation(transform);
        else
            cameraObject->setAbsoluteTransformation(transform);
        Vec3 rotation;
        rotation.x = 0.0f;
        rotation.y = carriedRotation;
        rotation.z = 0.0f;
        static_cast<world::Camera*>(cameraObject)->setCurrentRotation(rotation);
        if (world::IObject* const occupied = pco->getOccupingObject())
        {
            if (world::ISoldier* const soldier = static_cast<world::ISoldier*>(occupied->queryInterface(world::IID_ISoldier)))
                static_cast<world::Soldier*>(soldier)->setLookUpDownAngle(carriedRotation);
        }
    }

    eventManager->postEvent(ECGame, 9, player, 0.0f);
    clearPendingExit(player);
    return true;
}

// Unseating a player (soldier mode's E key and leaving, the free camera handed over when a soldier is entered).
// GameServer::exitVehicle's shape (GameServer.cpp) - a living player's control object exit()s, which seats its
// soldier back in the world; a dead one's exitPlayer()s - with the client's local-player physics hand-over, the
// infantry control map and the pending exit cleared, and no replication or script event (the editor has neither).
bool EditorGame::exitVehicle(world::IPlayer* player, bool, bool)
{
    if (!player)
        return false;
    world::IObject* const oldVehicle = player->getVehicle();
    if (!oldVehicle || !world::findOwningPlayerControlObject(oldVehicle))
        return false;

    if (isLocalPlayer(player))
        setPlayerControlMap("InfantryPlayerInputControlMap");
    if (!player->getIsRemote())
        reenablePhysics(oldVehicle, false);
    if (player->getIsAlive())
    {
        Game::internal_exitPco(oldVehicle);
        if (world::IObject* const now = player->getVehicle())
        {
            if (!player->getIsRemote())
            {
                disablePhysics(now, false);
                resetPhysics(now, false);
            }
        }
    }
    else
    {
        Game::internal_exitPlayerPco(oldVehicle);
    }
    // Not done: the client points its free camera's trace object at what was left (FreeCamera::m_traceObject, which
    // only GameClient may write). The editor's free camera never enters trace mode.

    const unsigned int classId = oldVehicle->getTemplate()->getClassID();
    if (classId != world::CID_FreeCameraTemplate && classId != world::CID_LadderContainerTemplate)
        eventManager->postEvent(ECGame, 11, player, 0.0f);
    clearPendingExit(player);
    return true;
}

std::string EditorGame::listPlayers() const                           { return std::string(); }
void EditorGame::recordDemo(DemoRecorder*)                            {}
void EditorGame::stopRecording()                                      {}
bool EditorGame::isPlayingDemo()                                      { return false; }
void EditorGame::setSimulationRate(int rate)                          { m_simulationRate = rate; }
int  EditorGame::getSimulationRate()                                  { return m_simulationRate; }
void EditorGame::debugShowActiveGhosts(int)                           {}
const io::BasicNetworkStats* EditorGame::getNetworkStats() const      { return 0; }
void EditorGame::sendUDPPacket(const char*, unsigned short, void*, int) {}

void EditorGame::addTextToChatWindow(const char* text)
{
    if (text)
        logLine(LogLevel::Info, "game: %s", text);
}

bool EditorGame::isPunkBusterEnabled() const  { return false; }
void EditorGame::destroyPlayer(world::IPlayer*) {}

} // namespace bf2editor
