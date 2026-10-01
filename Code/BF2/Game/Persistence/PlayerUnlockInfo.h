// Path: [INFERRED] persistence cluster; ELF primary 0x88C82E8 / PE 0x7E05E4.
#ifndef GAME_PERSISTENCE_PLAYERUNLOCKINFO_H
#define GAME_PERSISTENCE_PLAYERUNLOCKINFO_H
#include "../../Core/IBase.h"
#include "HTTPRequest.h"
#include "KeyParser.h"
#include <vector>
namespace dice { namespace hfe {
class IPlayerUnlockInfo : public IBase
{
public:
    struct Item { unsigned int id, kit; std::string name, description; }; // [INFERRED] field names
    virtual bool init() = 0;
    virtual bool getItemsByKit(unsigned int, std::vector<Item>&) = 0;
    virtual bool getItemById(unsigned int, Item&) = 0;
    virtual bool getItems(std::vector<Item>&) = 0;
    virtual float getVersion() = 0;
    virtual void setVersion(float) = 0;
    virtual bool getRanked() = 0;
    virtual void setRanked(bool) = 0;
protected:
    ~IPlayerUnlockInfo() {}
};
class PlayerUnlockInfo : public IPlayerUnlockInfo, public HTTPRequest
{
public:
    PlayerUnlockInfo();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool init() override;
    bool getItemsByKit(unsigned int, std::vector<Item>&) override;
    bool getItemById(unsigned int, Item&) override;
    bool getItems(std::vector<Item>&) override;
    float getVersion() override;
    void setVersion(float) override;
    bool getRanked() override;
    void setRanked(bool) override;
    ~PlayerUnlockInfo() override;
    void onGetComplete(const std::string&) override;
    void onGetFailure(const std::string&) override;
    void onPostComplete(const std::string&) override;
    void onPostFailure(const std::string&) override;
private:
    int m_refCount;
    KeyParser m_parser;
    std::vector<Item> m_items;
    float m_version;
    bool m_ranked;
};
} }
#endif
