// [TEMP][CHOSEN, not recovered][RETIRE-WHEN: HUD-AWARD-CATALOGUE-FIXTURE]
// Actual native catalogue construction/destruction and native HUD constructors.
// Only localization callbacks are observed; no catalogue or STL body is shadowed.
// HUD observers stay allocated at exit because their wider lifetime is unrelated.
#include <windows.h>
#include <cstdio>
#include <string>
#include <vector>
#include <functional>
#include <new>
#include <type_traits>
#include "../BF2/Menu/HudAwardCatalogue.h"
#include "../BF2/Menu/BF2HudManager.h"
#include "hud_award_catalogue_golden.h"

using namespace dice::hfe::hud;
namespace dice { namespace hfe { hud::BF2HudManager* hudManager=nullptr; } }
namespace dice { namespace hfe { namespace hud {
struct HudAwardCatalogueFixture {
    static const std::map<std::uint64_t,HudAwardDescription>& entries(const HudAwardCatalogue& value) { return value.m_awards; }
    static void insertUnsignedEdges(HudAwardCatalogue& value) {
        value.m_awards[(std::uint64_t(0x80000000u)<<32)|0xffffffffu].message=L"high";
        value.m_awards[(std::uint64_t(0xffffffffu)<<32)|0x80000000u].message=L"last";
    }
};
} } }
static unsigned checks,failures;
static std::vector<std::string> trace;
static std::function<void()> onCall;
static void check(bool ok,const char* label) {
    ++checks;if(!ok){++failures;std::printf("FAIL %s\n",label);}
}
static std::wstring wide(const std::string& value) {return std::wstring(value.begin(),value.end());}
struct CatalogueHud : BF2HudManager {
    std::string marker;
    explicit CatalogueHud(const char* value):marker(value){}
    std::wstring getLocalizedString(std::string key) override {
        trace.push_back(marker+":"+key);
        if(onCall)onCall();
        return wide(marker+"/"+key);
    }
    std::wstring replaceWideToken(std::wstring text,std::string token,std::wstring replacement) override {
        trace.push_back(marker+":replace");
        check(token=="#AWARD#","every replacement receives exact token");
        if(onCall)onCall();
        return wide(marker+"/")+text+L"/"+replacement;
    }
};
static std::string keyName(const AwardGolden& value) {
    char key[80];
    if(value.level)std::sprintf(key,"AWARD_Name_%u_%u",value.id,value.level);
    else std::sprintf(key,"AWARD_Name_%u",value.id);
    return key;
}
static std::string iconName(const AwardGolden& value) {
    char key[128];
    if(value.level)std::sprintf(key,"Ingame/GeneralIcons/Medals/AWARD_NAME_%u_%u.tga",value.id,value.level);
    else std::sprintf(key,"Ingame/GeneralIcons/Medals/AWARD_NAME_%u.tga",value.id);
    return key;
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);setvbuf(stdout,nullptr,_IONBF,0);
    static_assert(sizeof(void*)==4,"x86 only");
    static_assert(!std::is_polymorphic<HudAwardCatalogue>::value,"catalogue has no fabricated virtual table");
    static_assert(sizeof(BF2HudManager)==824,"native HUD pointer field unchanged");
    auto* first=new CatalogueHud("first");auto* second=new CatalogueHud("second");
    dice::hfe::hudManager=first;
    {
        HudAwardCatalogue catalogue;
        const auto& entries=HudAwardCatalogueFixture::entries(catalogue);
        check(entries.size()==138,"all138 distinct real native map entries are constructed");
        check(trace.size()==414,"each record performs exactly two lookups and one replacement");
        for(unsigned i=0;i<138;++i) {
            const auto& expected=awardGolden[i];const auto key=(std::uint64_t(expected.level)<<32)|expected.id;
            auto found=entries.find(key);
            check(found!=entries.end(),"all PE uint64 keys match native map entries");
            check(found!=entries.end()&&catalogue.findAward(expected.id,expected.level)==&found->second,"real lookup returns the exact stored native payload");
            check(trace[i*3]=="first:"+keyName(expected)&&trace[i*3+1]=="first:HUD_HUD_AWARDED"&&trace[i*3+2]=="first:replace","exact construction and right-to-left localization order");
            if(found!=entries.end()) {
                check(found->second.message==wide("first/first/HUD_HUD_AWARDED/first/"+keyName(expected)),"full localized wide message stored");
                check(found->second.iconPath==iconName(expected),"exact case-sensitive native icon path stored");
                check(found->second.notification=="AUTO_RULES_youbeenrecommendedforaward","exact notification key stored");
            }
        }
        check(entries.begin()->first==2020419,"unsigned64 ordering sorts level-zero before tiered records");
        check(entries.rbegin()->first==((std::uint64_t(3)<<32)|1262612),"last ordered key keeps full high-word level");
        check(entries.count(1220118)==0&&entries.count((std::uint64_t(1)<<32)|1220118)==1,"tiered award does not invent a level-zero entry");
        check(sizeof(HudAwardCatalogue)==sizeof(entries),"owner contains only the native map, no retail-sized padding");
        check(!catalogue.findAward(1220118,0)&&!catalogue.findAward(1220118,4)&&!catalogue.findAward(0,0)&&!catalogue.findAward(0xffffffffu,0xffffffffu),"missing level/key lookups return null including unsigned edges");
        check(entries.size()==138,"failed lookup never inserts a default award");
        HudAwardCatalogueFixture::insertUnsignedEdges(catalogue);
        auto* high=catalogue.findAward(0xffffffffu,0x80000000u);
        auto* last=catalogue.findAward(0x80000000u,0xffffffffu);
        check(high&&high->message==L"high"&&last&&last->message==L"last","lookup preserves all bits of both unsigned key words");
        check(!catalogue.findAward(0x80000000u,0x80000000u)&&!catalogue.findAward(0xffffffffu,0),"high-word comparison does not alias other lower-bound candidates");
    }
    // Every nested callback replaces the singleton with another same-dynamic-type
    // HUD. PE reloads each receiver; this does not model replacing a cached vptr.
    trace.clear();dice::hfe::hudManager=first;
    onCall=[&]{dice::hfe::hudManager=dice::hfe::hudManager==first?second:first;};
    {
        HudAwardCatalogue catalogue;
        check(trace.size()==414,"replacement callbacks do not add or skip catalogue work");
        for(unsigned i=0;i<138;++i) {
            const std::string a=i%2?"second":"first",b=i%2?"first":"second";
            check(trace[i*3]==a+":"+keyName(awardGolden[i])&&trace[i*3+1]==b+":HUD_HUD_AWARDED"&&trace[i*3+2]==a+":replace","each callback receiver reloads the current singleton");
            const auto& record=HudAwardCatalogueFixture::entries(catalogue).at((std::uint64_t(awardGolden[i].level)<<32)|awardGolden[i].id);
            check(record.message==wide(a+"/"+b+"/HUD_HUD_AWARDED/"+a+"/"+keyName(awardGolden[i])),"replacement return payload follows actual callback receivers");
        }
    }
    onCall=nullptr;
    std::printf("HUD award catalogue: %u/%u checks passed; native catalogue=%u payload=%u\n",checks-failures,checks,unsigned(sizeof(HudAwardCatalogue)),unsigned(sizeof(HudAwardDescription)));
    return failures?1:0;
}
