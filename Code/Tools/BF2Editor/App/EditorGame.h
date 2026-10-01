// Publishes the editor's Game interface for shared level loading and local play testing.
// Network sends release their events and demo requests fail. Vehicle entry/exit updates local seats;
// createObjectOnAllClients creates a local object without networking.
#ifndef BF2EDITOR_APP_EDITORGAME_H
#define BF2EDITOR_APP_EDITORGAME_H

#include "Framework/EditorConfig.h"

#include "Game/Main/Game.h"

#include <string>

namespace bf2editor
{

class EditorGame : public dice::hfe::Game
{
public:
    EditorGame();
    ~EditorGame() override;

    // The level the editor opens: what GameLogic::load reads back through getCurrentMap*.
    void setLevel(const std::string& levelDirectory, const std::string& name, const std::string& gameMode,
                  int maxPlayers);

    // IBase. The editor owns this object (registered with flags 4: the ClassManager neither creates
    // nor releases it).
    void  addRef() override;
    int   getRef() override;
    int   release() override;
    void* queryInterface(unsigned int iid) const override;

    // IGame slots Game leaves pure.
    bool init() override;
    void shutdown() override;
    bool unload() override;
    dice::hfe::StatsMonitor* getStatsMonitor() const override;
    bool update(int steps, float dt) override;
    bool say(const std::string&) override;
    bool radioMessage(dice::hfe::TargetChannel, const std::string&, dice::hfe::world::IPlayer*, unsigned int,
                      bool, bool) override;
    bool radioMessageFromPlayer(dice::hfe::TargetChannel, const std::string&, unsigned int, unsigned int, bool,
                                bool) override;
    bool gameInfoMessage(dice::hfe::TargetChannel, const std::string&, unsigned int, unsigned int, bool) override;
    void confirmGhost(int, unsigned int) override;
    void confirmReset(int) override;
    dice::hfe::world::IObject* createObjectOnAllClients(const dice::hfe::world::IObjectTemplate*, dice::hfe::Math::Vec3,
                                                       dice::hfe::Math::Vec3, bool, int, int) override;
    void sendGameEventToAllPlaying(dice::hfe::GameEvent* event) override;
    void sendGameEventIfPlaying(unsigned int, dice::hfe::GameEvent* event) override;
    void sendGameEventToAll(dice::hfe::GameEvent* event) override;
    void sendGameEvent(unsigned int, dice::hfe::GameEvent* event) override;
    void sendGameEventToAllConnected(dice::hfe::GameEvent* event) override;
    void sendGameEventIfConnected(unsigned int, dice::hfe::GameEvent* event) override;
    void sendMissileMessageToOccupyingPlayers(dice::hfe::world::IObject*, dice::hfe::world::MissileLockMessage) override;
    bool enterVehicle(dice::hfe::world::IPlayer*, dice::hfe::world::IObject*, bool) override;
    bool exitVehicle(dice::hfe::world::IPlayer*, bool, bool) override;
    std::string listPlayers() const override;
    void recordDemo(dice::hfe::DemoRecorder*) override;
    void stopRecording() override;
    bool isPlayingDemo() override;
    void setSimulationRate(int) override;
    int  getSimulationRate() override;
    void debugShowActiveGhosts(int) override;
    const dice::hfe::io::BasicNetworkStats* getNetworkStats() const override;
    void sendUDPPacket(const char*, unsigned short, void*, int) override;
    void addTextToChatWindow(const char*) override;
    bool isPunkBusterEnabled() const override;
    void destroyPlayer(dice::hfe::world::IPlayer*) override;

private:
    EditorGame(const EditorGame&);
    EditorGame& operator=(const EditorGame&);

    int m_refCount;
    int m_simulationRate;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_EDITORGAME_H
