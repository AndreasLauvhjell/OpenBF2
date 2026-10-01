// Path: [INFERRED] persistence cluster; ELF 0x82F8E10..0x82F9580, PE 0x5C7C70..0x5C80B0.
#ifndef GAME_PERSISTENCE_STATSMANAGER_H
#define GAME_PERSISTENCE_STATSMANAGER_H
#include "IStatsManager.h"
namespace dice { namespace hfe {
class StatsManager : public IStatsManager
{
public:
    StatsManager();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool connect() override;
    void disconnect() override;
    bool newGame() override;
    bool endGame() override;
    bool isConnected() override;
    const std::string& getChallenge() override;
    statsgame_s* getGame() const override;
    bool think() override;
    bool sendSnapshot(const std::string&) override;
    bool sendFinal(const std::string&) override;
    bool authenticatePlayer(world::IPlayer*) override;
    ~StatsManager() override;
    std::string getErrorString(int) const;
private:
    int m_refCount;
    statsgame_s* m_game;
    std::string m_challenge;
};
} }
#endif
