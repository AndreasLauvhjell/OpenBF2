// Path: [INFERRED] beside the persistence cluster, no __FILE__ attestation.
// ELF primary 0x88C6EA8; PE primary 0x7E0360 / HTTP secondary 0x7E0344.
#ifndef GAME_PERSISTENCE_PLAYERAWARDS_H
#define GAME_PERSISTENCE_PLAYERAWARDS_H
#include "../../Core/IBase.h"
#include "HTTPRequest.h"
#include "KeyParser.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace dice { namespace hfe {
class IPlayerAwards : public IBase
{
public:
    // Member names [INFERRED] from getAwardInfoEx/addAward and the response keys.
    // ELF size 36; original MSVC size 108. Strings use the compiling STL.
    struct Award
    {
        int type, priority;
        std::string name, description;
        int id, level;
        long when, first; // both shipped x86 ports use signed 32-bit time
        std::string mod;
    };
    struct AwardList
    {
        long timestamp;
        int profileId;
        std::vector<Award> awards;
    };
    virtual bool getAwards(int, AwardList*, bool, bool) = 0;
    virtual void tick() = 0;
    virtual void getAwardInfoEx(Award&) = 0;
    virtual AwardList sortList(const AwardList&) = 0;
    virtual AwardList sortListDate(const AwardList&, bool) = 0;
    virtual void getAllAwards(AwardList&) = 0;
    virtual void addAward(int, int, int, int, std::string) = 0;
    virtual bool matchModAward(int, std::string) = 0;
protected:
    ~IPlayerAwards() {}
};
class PlayerAwards : public IPlayerAwards, public HTTPRequest
{
public:
    PlayerAwards();
    static IBase* create(unsigned int, IBase*);
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int) const override;
    bool getAwards(int, AwardList*, bool, bool) override;
    void tick() override;
    void getAwardInfoEx(Award&) override;
    AwardList sortList(const AwardList&) override;
    AwardList sortListDate(const AwardList&, bool) override;
    void getAllAwards(AwardList&) override;
    void addAward(int, int, int, int, std::string) override;
    bool matchModAward(int, std::string) override;
    ~PlayerAwards() override;
    void onGetComplete(const std::string&) override;
    void onGetFailure(const std::string&) override;
    void onPostComplete(const std::string&) override;
    void onPostFailure(const std::string&) override;
private:
    int m_refCount;                    // ELF/PE +1C
    KeyParser m_parser;                // +20
    AwardList* m_result;               // ELF +2C / PE +30
    std::map<std::uint64_t, Award> m_awards; // ELF +30 / PE +34
    bool m_notifyPython;               // ELF +3C / PE +40
    int m_profileId;                   // ELF +40 / PE +44
};
std::uint64_t getId(const IPlayerAwards::Award&);
} }
#endif
