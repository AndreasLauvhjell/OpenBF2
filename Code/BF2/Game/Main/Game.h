// Abstract game implementation shared by GameServer and GameClient. Implements
// common map, message, squad, and object operations while leaving transport policy
// to derived classes. Preserve member order; STL-containing sizes vary by toolchain.

#ifndef GAME_MAIN_GAME_H     // [INFERRED] guard style
#define GAME_MAIN_GAME_H

#include "../IGame.h"
#include "../MapInfo.h"
#include "../../IO/Input/PlayerInput.h"

#include <iosfwd>
#include <list>
#include <string>

namespace dice {
namespace hfe {

// Parses TargetChannel tokens for console argument conversion.
std::istream& operator>>(std::istream& stream, TargetChannel& channel);

class Game : public IGame
{
public:
    Game();
    virtual ~Game();

    virtual bool load();
    virtual bool loadPath(const std::string&);

    virtual TargetChannel getCurrentChannel();
    virtual void setCurrentChannel(TargetChannel);
    virtual int getCurrentFlags();
    virtual void setCurrentFlags(int);
    virtual ChannelMessageType getCurrentType();
    virtual void setCurrentType(ChannelMessageType);
    virtual MapInfo getMapInfo() const;

    virtual bool localRadioMessage(TargetChannel, const std::string&);
    virtual bool localGameInfoMessage(TargetChannel, const std::string&);
    virtual void addPlayerInput(const io::PlayerInput&);
    virtual int getCurrentInputIndex();

    virtual world::IObject*
        getObjectFromNetworkableId(const unsigned short&) const;
    virtual void createAndDestroyObject(const world::IObjectTemplate*);
    virtual bool destroyObject(world::IObject*);

    virtual void joinSquad(int);
    virtual void leaveSquad(int);
    virtual std::string listSquadMembers(int, int);
    virtual std::string listSquads(int);
    virtual void changeSquadName(const std::string&, bool);
    virtual void makeMeCommander();
    virtual void popNextCommanderApplicant(int);
    virtual std::string getSpottedMessage(unsigned short);
    virtual void sendSpottedMessage(world::IPlayer*, unsigned short, Vec2,
                                    world::VehicleType);
    virtual void callForSpecialist(world::IPlayer*, HudNodeTypes);

    virtual const std::string& getCurrentMapName() const;
    virtual const std::string& getCurrentMapGameMode() const;
    virtual const std::string& getCurrentMapLevelPath() const;
    virtual int getCurrentMapMaxPlayers() const;
    virtual int getCurrentMapSize() const;
    virtual void setCurrentMapSize(unsigned int);
    virtual void failedExitVehicle(int);
    virtual bool getAbortLoadLevel() const;

    // Game extends IGame with this one pure slot; concrete server/client games own destruction.
    virtual void destroyPlayer(world::IPlayer*) = 0;                // +0x120 ELF / +0x11C PE

    // Recovered nonvirtual Game surface. Radio playback and the two target-channel output paths,
    // laser predicate and both PCO-exit helpers are executable.
    // Keeping every declaration here preserves the real shared base API without adding vtable slots.
    void playbackRadioMessage(TargetChannel, ChannelMessageType, int, int, int);
    void outputChatString(TargetChannel, ChannelMessageType, int, int,
                          const std::string&);
    void outputChatString(TargetChannel, ChannelMessageType, int, int,
                          const std::wstring&);
    void outputChatString(ChannelMessageType, const std::string&,
                          world::IPlayer*, int);
    void outputChatString(ChannelMessageType, const std::wstring&,
                          world::IPlayer*, int);
    bool playerControlsLaserGuidedWeapon(world::IPlayer*);
    void internal_exitPco(world::IObject*);
    void internal_exitPlayerPco(world::IObject*);

    // Registers game events once, after member construction and before scalar seeding.
    // Withheld makers remain omitted until their event virtual bodies are complete.
    bool registerGameEvents();

    void registerStaticNetworkables();
    void unregisterStaticNetworkables();
    float getDeltaTime();
    void updateTriggerManager(float);
    void updateTriggerableManager(float);
    void updateWorldCollision(float);
    void destroyAllPlayers();

protected:
    MapInfo m_mapInfo;
    // Intentionally not initialized by either retail constructor.
    unsigned int m_currentMapSize;
    std::list<io::PlayerInput> m_playerInputs;
    double m_lastDeltaTimeMark;
    TargetChannel m_currentChannel;
    ChannelMessageType m_currentType;
    int m_currentFlags;
    bool m_abortLoadingLevel;
    // Cached failed-exit-vehicle sound id; unused by the dedicated server.
    int m_failedExitVehicleSoundId;
};

// Formats the build number for the browser's local-server version record.
std::string getBF2Version();

// Dedicated images use constant mode flags; client images retain runtime flags for +dedicated.
// [INFERRED] Getter/setter names follow their roles.
#if DEDICATED
inline bool isDedicated() { return true; }
inline bool isLightmapRenderingNode() { return false; }
#else
bool isDedicated();                         // BF2.exe 0x440510, 6 B
void setDedicated(bool dedicated);          // BF2.exe 0x440520, 7 B
// [INFERRED] Client lightmap-worker flag. Enables distributed rendering and
// permits lightmap blocks; absent from dedicated images.
bool isLightmapRenderingNode();             // BF2.exe 0x440530, 6 B
void setLightmapRenderingNode(bool node);   // BF2.exe 0x440540, 7 B
#endif

} // namespace hfe
} // namespace dice

#endif // GAME_MAIN_GAME_H
