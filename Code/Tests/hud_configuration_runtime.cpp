// [TEMP] [CHOSEN, not recovered] Focused configuration dispatch fixture.
// Actual HUD constructor/configuration bodies, real radio storage and lifetime.
// Recording subclasses only model callback/query results for the stated cases.
// No init, event publication, widget destruction, real setMode transition or
// Ref2 resource initialization is exercised. /FORCE symbols stay uncalled.
#include <cstdint>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <new>
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/HudRadio.h"
#include "../BF2/Menu/Ref2System.h"
#undef protected
#undef private

using namespace dice::hfe;
using namespace dice::hfe::hud;
static std::vector<std::string> calls;
static std::vector<int> previousAnswers;
static unsigned previousIndex;
static bool chat;
static std::string number(int n) { return std::to_string(n); }

// [TEMP] Intercept transition to verify arguments/order, not emulate its body.
struct RecordingHud : BF2HudManager {
    int getMode() override { calls.push_back("mode:"+number(m_field250)); return m_field250; }
    int getPreviousMode() override {
        const int n=previousIndex<previousAnswers.size()?previousAnswers[previousIndex++]:m_field254;
        calls.push_back("previous:"+number(n)); return n;
    }
    void setMode(int n,bool save) override {
        calls.push_back("set:"+number(n)+(save?":1":":0")); m_field250=n;
    }
    bool isChatActive() override { calls.push_back(chat?"chat:1":"chat:0"); return chat; }
};
struct RecordingRadio : HudRadio {
    explicit RecordingRadio(const char* n):name(n) {}
    const char* name;
    void reset() override { calls.push_back(std::string(name)+":reset"); }
    void setEnabled(bool b) override {
        calls.push_back(std::string(name)+(b?":enabled1":":enabled0")); HudRadio::setEnabled(b);
    }
    bool isEnabled() override {
        calls.push_back(std::string(name)+":enabled?"); return HudRadio::isEnabled();
    }
    bool isActive() override {
        calls.push_back(std::string(name)+":active?"); return HudRadio::isActive();
    }
    void setActive(bool b) override {
        calls.push_back(std::string(name)+(b?":active1":":active0")); HudRadio::setActive(b);
    }
};
struct TextureSystem : menu::Ref2System {
    TextureSystem():Ref2System(800,600),hud(nullptr),next(100),replaceAfterFirst(false) {}
    BF2HudManager* hud;
    int next;
    bool replaceAfterFirst;
    menu::Ref2System* replacement;
    std::vector<std::string> paths;
    int loadTexture(const char* path,bool flag) override {
        paths.push_back(path); calls.push_back(flag?"texture:1":"texture:0");
        if(replaceAfterFirst) hud->m_field0f4=replacement;
        return next++;
    }
};
static void clearCalls() { calls.clear(); previousAnswers.clear(); previousIndex=0; chat=false; }
static bool sequence(std::initializer_list<const char*> expected) {
    if(calls.size()!=expected.size()) return false;
    unsigned i=0; for(const char* s:expected) if(calls[i++]!=s) return false;
    return true;
}
// Exact construction-only ownership cleanup used by hud_manager_runtime.
// Do not call incomplete BF2HudManager destructor; reclaim only constructed fields.
static void destroyConstructedHud(RecordingHud* h) {
    delete h->m_field2dc;
    using W=std::wstring; using S=std::string; using L=std::list<std::string>;
    using I=std::list<int>; using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    h->m_field33c.~L(); h->m_field32c.~V(); h->m_field304.~S();
    h->m_field2e0.~L(); h->m_field0e8.~I(); h->m_field0c0.~S(); h->m_field08c.~S();
    h->m_field060.~W(); h->m_field038.~W(); h->m_field010.~W();
    ::operator delete(h);
}
int main() {
    unsigned checks=0,failures=0;
    auto check=[&](bool ok,const char* text) {
        ++checks; if(!ok) {++failures;std::printf("FAIL %s\n",text);}
    };
    RecordingHud* h=new RecordingHud;
    BF2HudManager* api=h;
    h->m_field250=11;clearCalls();api->toggleMode(11,14);
    check(sequence({"mode:11","set:14:1"}),"toggle first branch dispatch");
    h->m_field250=9;clearCalls();api->toggleMode(11,14);
    check(sequence({"mode:9","set:11:1"}),"toggle alternate branch dispatch");
    h->m_field250=33;clearCalls();
    check(api->unknown1E8() && sequence({"mode:33"}),"discarded mode query precedes true");

    h->m_field254=24;clearCalls();
    check(api->wasCommanderMode() && sequence({"previous:24"}),"commander previous query once");
    h->m_field254=9;clearCalls();
    check(!api->wasCommanderMode() && sequence({"previous:9"}),"noncommander previous query once");
    clearCalls();check(api->isCommanderMode(15)&&calls.empty(),"direct commander skips callbacks");
    h->m_field254=21;clearCalls();
    check(api->isCommanderMode(27)&&sequence({"previous:21"}),"mode27 skips chat query");
    clearCalls();
    check(api->isCommanderMode(9)&&sequence({"chat:0","previous:21"}),"mode9 checks chat before previous");
    clearCalls();check(!api->isCommanderMode(2)&&sequence({"chat:0"}),"false chat skips previous");
    clearCalls();chat=true;
    check(api->isCommanderMode(2)&&sequence({"chat:1","previous:21"}),"active chat uses previous mode");

    clearCalls();check(!api->activateRadio(nullptr)&&calls.empty(),"null radio activation has no callbacks");
    RecordingRadio infantry("I"),vehicle("V"),squad("S"),leader("L"),commander("C");
    clearCalls();check(api->activateRadio(&infantry),"non-null radio activation");
    check(sequence({"I:reset","I:active1","I:enabled0"}),"activation exact reset/active/enable order");
    check(infantry.HudRadio::isActive()&&!infantry.HudRadio::isEnabled(),"real radio state changed");
    h->m_field250=15;clearCalls();api->resetRadio(&infantry);
    check(sequence({"mode:15","I:enabled1","I:active0"}),"reset commander skips transition");
    h->m_field250=9;clearCalls();api->resetRadio(&infantry);
    check(sequence({"mode:9","set:0:1","I:enabled1","I:active0"}),"reset mode before radio callbacks");
    clearCalls();api->setSpottedMenuActive();
    check(sequence({"set:6:0"}),"spotted transition suppresses previous save");

    h->m_field108=&infantry;h->m_field10c=&vehicle;h->m_field110=&squad;
    h->m_field114=&leader;h->m_field118=&commander;
    clearCalls();check(!api->updateRadioMode(),"enabled radios do not finish mode");
    check(sequence({"I:enabled?","V:enabled?","S:enabled?","L:enabled?","C:enabled?"}),
          "enabled radio skips active query and checks next");
    infantry.HudRadio::setEnabled(false);infantry.HudRadio::setActive(true);
    vehicle.HudRadio::setEnabled(false);vehicle.HudRadio::setActive(false);
    h->m_field250=5;h->m_field254=8;clearCalls();check(api->updateRadioMode(),"second radio chosen after active first");
    check(sequence({"I:enabled?","I:active?","V:enabled?","V:active?","mode:5","previous:8","previous:8","set:8:1"}),
          "vehicle repeated previous virtual calls preserved");
    infantry.HudRadio::setActive(false);h->m_field250=19;clearCalls();
    check(api->updateRadioMode(),"first eligible radio succeeds even without mode change");
    check(sequence({"I:enabled?","I:active?","mode:19","mode:19","mode:19"}),
          "first eligible radio blocks later radio checks");

    h->m_field250=4;clearCalls();previousAnswers={31,31,30,30,3,3};
    check(api->updateRadioMode(),"infantry chained mode transitions");
    check(sequence({"I:enabled?","I:active?","mode:4","previous:31","previous:31","set:31:1",
                    "mode:31","previous:30","previous:30","set:30:1",
                    "mode:30","previous:3","previous:3","set:3:1"}),
          "all three mode checks use callback-updated state");
    h->m_field250=4;h->m_field254=4;clearCalls();api->updateRadioMode();
    check(sequence({"I:enabled?","I:active?","mode:4","previous:4","mode:4","mode:4"}),
          "equal previous avoids second query and transition");
    h->m_field108=nullptr;h->m_field10c=nullptr;h->m_field110=nullptr;
    h->m_field114=nullptr;h->m_field118=nullptr;clearCalls();
    check(!api->updateRadioMode()&&calls.empty(),"null radio chain has no callbacks");

    TextureSystem first,second; first.hud=h;first.replaceAfterFirst=true;first.replacement=&second;
    h->m_field0f4=&first;h->m_field164=11;h->m_field168=12;h->m_field16c=13;
    h->m_field170=14;h->m_field174=15;h->m_field178=16;clearCalls();
    check(api->setMouseTextureEmpty(std::string("empty\0suffix",12)),"first texture setter true");
    check(first.paths.size()==1&&first.paths[0]=="empty"&&h->m_field164==100,
          "c-string callback consumes prefix and stores returned handle");
    check(h->m_field168==12&&h->m_field16c==13&&h->m_field170==14&&h->m_field174==15&&h->m_field178==16,
          "callback changes only selected texture field");
    check(api->setMouseTextureFull("full")&&api->setMouseTextureArtillery("artillery")&&
          api->setMouseTextureUAV("uav")&&api->setMouseTextureSupply("supply")&&
          api->setMouseTextureCancel("cancel"),"remaining texture setters true");
    check(second.paths==std::vector<std::string>({"full","artillery","uav","supply","cancel"}),
          "each setter reloads the replaced root in order");
    check(h->m_field168==100&&h->m_field16c==101&&h->m_field170==102&&h->m_field174==103&&h->m_field178==104,
          "texture results routed to six distinct fields");
    check(sequence({"texture:1","texture:1","texture:1","texture:1","texture:1","texture:1"}),
          "all texture callbacks receive true");
    h->m_field0f4=nullptr;clearCalls();
    check(api->setMouseTextureEmpty("ignored")&&api->setMouseTextureFull("ignored")&&
          api->setMouseTextureArtillery("ignored")&&api->setMouseTextureUAV("ignored")&&
          api->setMouseTextureSupply("ignored")&&api->setMouseTextureCancel("ignored"),
          "absent root preserves shipped true return");
    check(calls.empty()&&h->m_field164==100&&h->m_field168==100&&h->m_field16c==101&&
          h->m_field170==102&&h->m_field174==103&&h->m_field178==104,
          "absent root leaves prior handles intact");
    destroyConstructedHud(h);
    std::printf("HUD configuration: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
