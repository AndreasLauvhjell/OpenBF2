// Path: [INFERRED] beside the persistence cluster; no __FILE__ attestation.
// ELF primary table 0x88C8488; PE primary 0x7E03EC, HTTP secondary 0x7E03D0.
#ifndef GAME_PERSISTENCE_PLAYERUNLOCKS_H
#define GAME_PERSISTENCE_PLAYERUNLOCKS_H
#include "../../Core/IBase.h"
#include "HTTPRequest.h"
#include "KeyParser.h"
#include <vector>
namespace dice { namespace hfe {
namespace world { class IPlayer; }
class IPlayerUnlocks : public IBase
{
public:
    // Member names are [INFERRED]; Item's pair<int,int> contents and cache
    // integers followed by vector<Item> are read from both completion bodies.
    struct Item { int id; int status; };
    struct UnlockCache
    {
        int field0, field4;
        int available, selected; // [INFERRED] metadata names
        std::vector<Item> items;
    };
    virtual bool getUnlocks(int, bool, std::vector<int>&) = 0;
    virtual bool setUnlocks(int, int) = 0;
    virtual bool getUnlocks(int, UnlockCache&) = 0;
protected:
    ~IPlayerUnlocks() {}
};
class PlayerUnlocks : public IPlayerUnlocks, public HTTPRequest
{
public:
    PlayerUnlocks();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool getUnlocks(int, bool, std::vector<int>&) override;
    bool setUnlocks(int, int) override;
    bool getUnlocks(int, UnlockCache&) override;
    ~PlayerUnlocks() override;
    void onGetComplete(const std::string&) override;
    void onGetFailure(const std::string&) override;
    void onPostComplete(const std::string&) override;
    void onPostFailure(const std::string&) override;
    void setPlayerUnlocks(world::IPlayer*, std::vector<int>*);
private:
    int m_refCount;                 // ELF/PE +1C
    std::vector<int>* m_unlocks;    // +20
    UnlockCache* m_cache;           // +24
    KeyParser m_parser;            // +28
    bool m_asynchronous;           // ELF +34 / PE +38
    int m_profileId;               // ELF +38 / PE +3C
    bool m_cacheMode;              // ELF +3C / PE +40
};
} }
#endif
