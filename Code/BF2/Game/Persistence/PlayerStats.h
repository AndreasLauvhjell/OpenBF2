// Path: [INFERRED] beside attested Game/Persistence/PlayerStats.cpp.
#ifndef GAME_PERSISTENCE_PLAYERSTATS_H
#define GAME_PERSISTENCE_PLAYERSTATS_H
#include "IPlayerStats.h"
#include "HTTPRequest.h"
#include "KeyParser.h"
namespace dice { namespace hfe {
class PlayerStats : public IPlayerStats, public HTTPRequest
{
public:
    PlayerStats();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool getStats(int, const std::string&, bool, bool) override;
    void tick() override;
    int getNumKeyMaps() override;
    int getNumKeys() override;
    bool getKey(unsigned int, int, std::string&, std::string&) override;
    std::string getKeyValue(unsigned int, const std::string&) override;
    ~PlayerStats() override;
    void onGetComplete(const std::string&) override;
    void onGetFailure(const std::string&) override;
    void onPostComplete(const std::string&) override;
    void onPostFailure(const std::string&) override;
    KeyMap* getKeyMap(unsigned int);
private:
    int m_refCount;                              // ELF +1C; PE +1C
    KeyParser m_parser;                         // ELF +20; PE +20
    bool m_notifyPython;                        // ELF +2C; PE +30 (STL differs)
    int m_profileId;                            // ELF +30; PE +34
};
} }
#endif
