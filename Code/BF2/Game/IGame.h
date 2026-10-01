// Shared game lifecycle, networking and player-action interface implemented by
// Game, GameServer and GameClient. Preserve declaration order, including the
// virtual destructor after IBase.
// [UNRESOLVED] Return types explicitly marked at declarations remain provisional.

#ifndef GAME_IGAME_H            // [INFERRED] guard style
#define GAME_IGAME_H

#include "../Core/IBase.h"
#include <string>

// Math::Vec2 / Math::Vec3 are structs with dice::hfe-level typedefs (Math/Vec3.h lines 50-51), so
// they cannot be forward-declared here - a local `class Vec2;` collides with the typedef.
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {

// Chat/radio target channels. Values 12 and 13 are server-only and are rejected
// from client string blocks. LocalRevive and AutoLocalRevive share server formatting.
enum TargetChannel
{
    TCGlobal                = 0,
    TCTeam                  = 1,
    TCCommander             = 2,
    TCSquad                 = 3,
    TCSquadLeader           = 4,
    TCSquadId               = 5,
    TCLocalTeamMedic        = 6,
    TCLocalGroundVehicle    = 7,
    TCLocalAirVehicle       = 8,
    TCLocal                 = 9,
    TCLocalTeam             = 10,
    TCVehicle               = 11,
    TCServerMessage         = 12,   // server-only - a client may not send this
    TCServerTeamMessage     = 13,   // server-only - a client may not send this
    TCPlayer                = 14,
    TCLocalRevive           = 15,
    TCAutoLocalRevive       = 16,
    TCTeamAndLocalEnemies   = 17,
    TCSquadAndLocalEnemies  = 18,
    TCInvalid               = 19
};
enum ChannelMessageType : int { ChannelMessageType_UNRECOVERED = 0 };   // `: int` matches the opaque
                                                                        //   declarations in AIAdapterTypes.h
enum HudNodeTypes        { HudNodeTypes_UNRECOVERED = 0 };
class MapInfo;
class StatsMonitor;
class GameEvent;
class DemoRecorder;
namespace io { class PlayerInput; class BasicNetworkStats; }   // BasicNetworkStats: getNetworkStats' return
namespace world {
class IPlayer;
class IObject;
class IObjectTemplate;
// By-value 16-byte message in both retail ABIs. Semantic names remain unrecovered; +0x04 is
// compared with -1 and +0x08 is used as an object-grid id by GameServer.
struct MissileLockMessage
{
    unsigned int field00;
    int          field04;
    unsigned int field08;
    unsigned int field0C;
};
enum VehicleType : int  { VehicleType_UNRECOVERED = 0 };   // `: int` matches IPlayerControlObjectTemplate.h
}

class IGame : public IBase
{
public:
    virtual void  addRef() = 0;                                     // +0x00
    virtual int   getRef() = 0;                                     // +0x04
    virtual int   release() = 0;                                    // +0x08
    virtual void* queryInterface(unsigned int) const = 0;           // +0x0C

    virtual ~IGame() = 0;                                           // +0x10 / +0x14 pure in retail

    virtual bool  init() = 0;                                       // +0x18  bool attested by use
    virtual void  shutdown() = 0;                                   // +0x1C
    virtual bool  load() = 0;                                       // +0x20  [CHOSEN] return
    virtual bool  loadPath(const std::string&) = 0;                 // +0x24  [CHOSEN] return
    virtual bool  unload() = 0;                                     // +0x28  return forwarded by GameServer
    virtual StatsMonitor* getStatsMonitor() const = 0;              // +0x2C  [CHOSEN] return
    virtual bool  update(int steps, float dt) = 0;                  // +0x30  bool
                                                                    // A false update result causes BF2Engine to shut down the game.
    virtual bool  say(const std::string&) = 0;                      // +0x34

    virtual TargetChannel      getCurrentChannel() = 0;             // +0x38
    virtual void               setCurrentChannel(TargetChannel) = 0;// +0x3C
    virtual int                getCurrentFlags() = 0;               // +0x40
    virtual void               setCurrentFlags(int) = 0;            // +0x44
    virtual ChannelMessageType getCurrentType() = 0;                // +0x48
    virtual void               setCurrentType(ChannelMessageType) = 0; // +0x4C
    virtual MapInfo            getMapInfo() const = 0;              // +0x50  by value (hidden sret)

    // SIX arguments each - from the GameServer mangled names,
    virtual bool radioMessage(TargetChannel, const std::string&, world::IPlayer*,
                              unsigned int, bool, bool) = 0;                                  // +0x54
    virtual bool radioMessageFromPlayer(TargetChannel, const std::string&, unsigned int,
                                        unsigned int, bool, bool) = 0;                        // +0x58
    virtual bool localRadioMessage(TargetChannel, const std::string&) = 0;                     // +0x5C
    virtual bool gameInfoMessage(TargetChannel, const std::string&, unsigned int,
                                 unsigned int, bool) = 0;                                     // +0x60
    virtual bool localGameInfoMessage(TargetChannel, const std::string&) = 0;                  // +0x64

    virtual void addPlayerInput(const io::PlayerInput&) = 0;        // +0x68
    virtual int  getCurrentInputIndex() = 0;                        // +0x6C  [CHOSEN] return
    virtual void confirmGhost(int, unsigned int) = 0;               // +0x70
    virtual void confirmReset(int) = 0;                             // +0x74
    // Returns the created object. Parameters select its template, transform, recipient filter, and default team.
    virtual world::IObject* createObjectOnAllClients(const world::IObjectTemplate*, Math::Vec3, Math::Vec3,
                                          bool, int, int) = 0;                                // +0x78
    virtual world::IObject* getObjectFromNetworkableId(const unsigned short&) const = 0;      // +0x7C
    virtual void createAndDestroyObject(const world::IObjectTemplate*) = 0;                   // +0x80

    virtual void sendGameEventToAllPlaying(GameEvent*) = 0;         // +0x84
    virtual void sendGameEventIfPlaying(unsigned int, GameEvent*) = 0;  // +0x88
    virtual void sendGameEventToAll(GameEvent*) = 0;                // +0x8C
    virtual void sendGameEvent(unsigned int, GameEvent*) = 0;       // +0x90
    virtual void sendGameEventToAllConnected(GameEvent*) = 0;       // +0x94
    virtual void sendGameEventIfConnected(unsigned int, GameEvent*) = 0;  // +0x98
    virtual void sendMissileMessageToOccupyingPlayers(world::IObject*,
                                                      world::MissileLockMessage) = 0;         // +0x9C

    virtual bool destroyObject(world::IObject*) = 0;                // +0xA0
    virtual bool enterVehicle(world::IPlayer*, world::IObject*, bool) = 0;  // +0xA4
    virtual bool exitVehicle(world::IPlayer*, bool, bool) = 0;      // +0xA8
    virtual std::string listPlayers() const = 0;                    // +0xAC

    virtual void joinSquad(int) = 0;                                // +0xB0
    virtual void leaveSquad(int) = 0;                               // +0xB4
    virtual std::string listSquadMembers(int, int) = 0;             // +0xB8
    virtual std::string listSquads(int) = 0;                        // +0xBC
    virtual void changeSquadName(const std::string&, bool) = 0;     // +0xC0
    virtual void makeMeCommander() = 0;                             // +0xC4
    virtual void popNextCommanderApplicant(int) = 0;                // +0xC8

    virtual std::string getSpottedMessage(unsigned short) = 0;      // +0xCC
    virtual void sendSpottedMessage(world::IPlayer*, unsigned short, Vec2,
                                    world::VehicleType) = 0;                                  // +0xD0
    virtual void callForSpecialist(world::IPlayer*, HudNodeTypes) = 0;  // +0xD4

    // The demo group - reached from Game/Main/demo.cpp. These are ordinary IGame slots; the
    // "beyond the vtable" shim that used to wrap them is gone.
    virtual void recordDemo(DemoRecorder*) = 0;                     // +0xD8
    virtual void stopRecording() = 0;                               // +0xDC
    virtual bool isPlayingDemo() = 0;                               // +0xE0  [CHOSEN] return
    virtual void setSimulationRate(int) = 0;                        // +0xE4
    virtual int  getSimulationRate() = 0;                           // +0xE8  int by use (mainLoop
                                                                    //   feeds Demo::handlePlayerInput(int))
    virtual void debugShowActiveGhosts(int) = 0;                    // +0xEC

    virtual const std::string& getCurrentMapName() const = 0;       // +0xF0
    virtual const std::string& getCurrentMapGameMode() const = 0;   // +0xF4
    virtual const std::string& getCurrentMapLevelPath() const = 0;  // +0xF8
    virtual int  getCurrentMapMaxPlayers() const = 0;               // +0xFC
    virtual int  getCurrentMapSize() const = 0;                     // +0x100
    virtual void setCurrentMapSize(unsigned int) = 0;               // +0x104

    // Returns borrowed network statistics for the status monitor.
    virtual const io::BasicNetworkStats* getNetworkStats() const = 0; // +0x108
    virtual void sendUDPPacket(const char*, unsigned short, void*, int) = 0;  // +0x10C
    virtual void addTextToChatWindow(const char*) = 0;              // +0x110
    virtual void failedExitVehicle(int) = 0;                        // +0x114
    virtual bool isPunkBusterEnabled() const = 0;                   // +0x118 [CHOSEN] return
    virtual bool getAbortLoadLevel() const = 0;                     // +0x11C bool by use (mainLoop
                                                                    // > shutdownGame(true))

};

// A pure virtual destructor still needs a body for derived destruction. Keeping that body inline
// avoids inventing an unattested IGame.cpp while the IGame vtable entries remain purecall targets.
inline IGame::~IGame() {}

// ds:0x8AD45A0 = _ZN4dice3hfe4gameE. IDA types it GameServer*, but it is registered under IID_IGame
// and Demo::stop takes IGame*, so IGame* is the source-level type. See BF2Engine.cpp.
extern IGame* game;

} // namespace hfe
} // namespace dice

#endif // GAME_IGAME_H
