// Path: [INFERRED]. ELF primary 0x88C7028 / PE 0x7E0644.
#ifndef GAME_PERSISTENCE_PLAYERRANKINFO_H
#define GAME_PERSISTENCE_PLAYERRANKINFO_H
#include "../../Core/IBase.h"
#include "HTTPRequest.h"
#include "KeyParser.h"
#include <cstdint>
namespace dice { namespace hfe {
class IPlayerRankInfo : public IBase
{
public:
    struct RankInfo { std::int32_t timestamp; int profileId, rank, change, decrease; };
    virtual bool getRankInfo(int, RankInfo&) = 0;
    virtual bool clearRankNotification(int) = 0;
    virtual int getNumKeyMaps() = 0;
    virtual int getNumKeys() = 0;
    virtual std::string getKeyValue(unsigned int, const std::string&) = 0;
    virtual bool getKey(unsigned int, int, std::string&, std::string&) = 0;
protected:
    ~IPlayerRankInfo() {}
};
class PlayerRankInfo : public IPlayerRankInfo, public HTTPRequest
{
public:
    PlayerRankInfo();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool getRankInfo(int, RankInfo&) override;
    bool clearRankNotification(int) override;
    int getNumKeyMaps() override;
    int getNumKeys() override;
    std::string getKeyValue(unsigned int, const std::string&) override;
    bool getKey(unsigned int, int, std::string&, std::string&) override;
    ~PlayerRankInfo() override;
    void onGetComplete(const std::string&) override;
    void onGetFailure(const std::string&) override;
    void onPostComplete(const std::string&) override;
    void onPostFailure(const std::string&) override;
    KeyMap* getKeyMap(unsigned int);
private:
    int m_refCount;
    KeyParser m_parser;
    bool m_notification;
};
} }
#endif
