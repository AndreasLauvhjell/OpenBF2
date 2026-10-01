// Path: [INFERRED]. ELF primary 0x88C81A8 / PE primary 0x7E0240.
// Member names are [INFERRED]; constructor field order and all 175 keys are paired evidence.
#ifndef GAME_PERSISTENCE_PLAYERSTATSMANAGER_H
#define GAME_PERSISTENCE_PLAYERSTATSMANAGER_H
#include "../../Core/IBase.h"
#include <string>
#include <vector>
namespace dice { namespace hfe {
struct PlayerStatKey
{
    int type;
    std::string wireCode;
    std::string longName;
    std::string textValue;
    int numberValue;
    PlayerStatKey(const char* code, const char* name)
        : type(0), wireCode(code), longName(name), numberValue(0) {}
    PlayerStatKey(const char* code, const char* name, const char* text)
        : type(1), wireCode(code), longName(name), textValue(text) {}
    void set(const std::string&);
};
class IPlayerStatsManager : public IBase
{
public:
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() = 0;
    virtual bool isUpdating() = 0;
    virtual bool update() = 0;
    virtual void downloadCallback(const char*) = 0;
    virtual void downloadFailed() = 0;
    virtual PlayerStatKey* findKey(const std::string&) = 0;
    // [INFERRED] return constness: both bodies return the member vector by address.
    virtual std::vector<PlayerStatKey>& getKeys() = 0;
    virtual ~IPlayerStatsManager() {}
};
class PlayerStatsManager : public IPlayerStatsManager
{
public:
    PlayerStatsManager();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool connect() override;
    void disconnect() override;
    bool isConnected() override;
    bool isUpdating() override;
    bool update() override;
    void downloadCallback(const char*) override;
    void downloadFailed() override;
    PlayerStatKey* findKey(const std::string&) override;
    std::vector<PlayerStatKey>& getKeys() override;
    ~PlayerStatsManager() override;
private:
    int m_refCount;
    bool m_connected;
    bool m_updating;
    std::string m_profile;
    std::string m_query;
    std::vector<PlayerStatKey> m_keys;
};
} }
#endif
