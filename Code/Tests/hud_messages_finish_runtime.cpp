// [TEMP] [CHOSEN, not recovered] Actual Messages bodies against typed callbacks.
// [RETIRE-WHEN: HUD-MESSAGES-SELECTION-FIXTURE] The real Object constructors run;
// world objects are retained because complete world teardown is outside this test.
#define HUD_PLAYER_FILTER_NO_MAIN
#include "hud_player_filter_runtime.cpp"
#include <cstring>
struct MessagePco : FilterPco {
    std::vector<world::IObject*> slots[2];
    std::function<void(int)> onList;
    void* getWeapons(int slot) override {
        check(slot==0||slot==1,"weapon slot is zero or one");
        trace.push_back("list"+std::to_string(slot));
        if(onList)onList(slot);
        return &slots[slot];
    }
};
struct MessageSound : Sound {
    std::function<void()> onPlay;
    void playSound(const std::string& name) override {
        trace.push_back("sound");check(name=="weaponOver","selection sound key");
        if(onPlay)onPlay();
    }
};
namespace dice { namespace hfe { namespace hud {
struct HudMessagesFixture {
static void run() {
    EventManager events;eventManager=&events;
    Players players;world::playerManager=&players;
    FilterPlayer player;MessageSound sound;g_bfSound=&sound;
    HudMessages m;
    const char* sentinel=m.m_items[0].icon;
    m.m_duration=3.75f;m.m_remainingTime=0;m.m_itemCount=3;
    m.m_items[0].id=7;m.m_items[1].id=11;m.m_items[2].id=11;
    trace.clear();m.selectItem(11,false);
    check(m.m_selectedItem==1&&m.m_remainingTime==3.75f,"selection chooses first equal id and resets timer");
    check(trace==std::vector<std::string>({"player"}),"silent null-player selection only queries player");
    m.m_visible=false;trace.clear();m.selectItem(99,false);
    check(m.m_selectedItem==0&&!m.m_visible,"missing id falls back to zero without changing visibility");
    sound.onPlay=[&]{check(m.m_selectedItem==1&&m.m_remainingTime==3.75f,"selection stores precede sound callback");players.local=&player;};
    trace.clear();m.selectItem(11,true);
    check(trace==std::vector<std::string>({"sound","player","vehicle"}),"player lookup follows sound and observes callback replacement");
    sound.onPlay=nullptr;players.local=nullptr;
    m.m_itemCount=-5;trace.clear();m.selectItem(11,false);
    check(m.m_selectedItem==0,"negative signed item count selects zero");
    m.m_itemCount=3;m.m_field010=9;m.m_items[0].active=true;
    trace.clear();check(!m.refreshItems(),"refresh without local player returns false");
    check(m.m_itemCount==0&&m.m_field010==9&&m.m_items[0].active,"null player clears only item count");
    players.local=&player;trace.clear();check(!m.refreshItems(),"refresh without vehicle returns false");
    check(trace==std::vector<std::string>({"player","vehicle"})&&m.m_field010==9,"null vehicle preserves secondary-list flag");
    auto* vehicle=new FilterObject;auto* other=new FilterObject;auto* ancestor=new FilterObject;
    MessagePco control, fallback;vehicle->control=&control;other->control=&fallback;
    player.vehicle=vehicle;player.defaultVehicle=vehicle;
    control.slots[0]={nullptr,nullptr};
    trace.clear();check(!m.refreshItems(),"nonnull vector with null entries does not use fallback");
    check(trace==std::vector<std::string>({"player","vehicle","query","list0"}),"nonempty current list avoids default-vehicle callbacks");
    check(m.m_field010==0&&m.m_itemCount==0&&!m.m_items[0].active,"refresh clears flags while population skips null entries");
    control.slots[0].clear();player.defaultVehicle=other;fallback.slots[0]={nullptr};
    trace.clear();check(m.refreshItems(),"empty current list selects fallback vehicle list");
    check(trace==std::vector<std::string>({"player","vehicle","query","list0","default","default","query","list0"}),"fallback repeats the default vehicle lookup and replaces vector");
    fallback.slots[0].clear();trace.clear();check(m.refreshItems(),"fallback result is true even when fallback list is empty");
    // Current PCO retained for slot1 selection even when slot0 came from fallback.
    FilterVehicleComp comp;comp.info.useSelectionIcons=true;vehicle->component(&comp);
    control.slots[1]={nullptr,nullptr,nullptr};
    trace.clear();check(m.refreshItems()&&m.m_field010==1,"larger secondary list sets the selection-list flag");
    check(std::count(trace.begin(),trace.end(),"list1")==2,"secondary list getter is repeated for assignment");
    check(std::count(trace.begin(),trace.end(),"default")==4,"fallback and vehicle-HUD checks retain all default-vehicle calls");
    control.slots[0]={nullptr};control.slots[1]={nullptr};
    trace.clear();check(!m.refreshItems()&&m.m_field010==0,"equal-sized secondary list does not replace primary");
    check(std::count(trace.begin(),trace.end(),"list1")==1,"comparison-only list lookup occurs once");
    comp.info.useSelectionIcons=false;trace.clear();m.refreshItems();
    check(std::count(trace.begin(),trace.end(),"list1")==0,"disabled vehicle selection bypasses secondary list");
    vehicle->component(nullptr);vehicle->control=nullptr;vehicle->parent(ancestor);ancestor->control=&control;
    trace.clear();m.refreshItems();
    check(std::count(trace.begin(),trace.end(),"query")==2,"control lookup walks the real parent chain");
    // The second vehicle callback can change its result; selection then skips PCO lookup.
    player.onVehicle=[&]{if(std::count(trace.begin(),trace.end(),"vehicle")==2)player.vehicle=other;};
    trace.clear();m.selectItem(7,false);
    check(trace==std::vector<std::string>({"player","vehicle","vehicle"}),"changed vehicle prevents weapon-vector lookup");
    player.onVehicle=nullptr;player.vehicle=vehicle;control.slots[0]={nullptr};
    trace.clear();m.selectItem(7,false);
    check(trace==std::vector<std::string>({"player","vehicle","vehicle","query","query","list0"}),"stable selection copies current weapon list through ancestor");
    // Icon operations: equality retains allocation, null source is ignored, and
    // compaction copies the icon while leaving source storage and alphas intact.
    HudMessages::assignIcon(m.m_items[0].icon,"alpha");
    const char* alpha=m.m_items[0].icon;
    HudMessages::assignIcon(m.m_items[0].icon,"alpha");
    check(m.m_items[0].icon==alpha,"equal icon preserves allocation identity");
    HudMessages::assignIcon(m.m_items[0].icon,nullptr);
    check(m.m_items[0].icon==alpha,"null icon source retains value");
    HudMessages::assignIcon(m.m_items[0].icon,"");
    check(m.m_items[0].icon!=sentinel&&m.m_items[0].icon[0]==0,"empty replacement remains owned, not shared sentinel");
    for(int i=0;i<10;++i){m.m_items[i].active=false;m.m_items[i].id=100+i;m.m_items[i].backgroundAlpha=float(i);m.m_items[i].activeAlpha=float(20+i);}
    m.m_items[2].active=true;m.m_items[2].id=6;HudMessages::assignIcon(m.m_items[2].icon,"six");
    m.m_items[5].active=true;m.m_items[5].id=8;HudMessages::assignIcon(m.m_items[5].icon,"eight");
    const char* six=m.m_items[2].icon;const char* eight=m.m_items[5].icon;m.m_itemCount=17;
    m.compactItems();
    check(m.m_items[0].active&&m.m_items[0].id==6&&std::strcmp(m.m_items[0].icon,"six")==0,"first hole receives first active item");
    check(m.m_items[1].active&&m.m_items[1].id==8&&std::strcmp(m.m_items[1].icon,"eight")==0,"compaction preserves item order across gaps");
    check(m.m_items[2].icon==six&&m.m_items[5].icon==eight&&m.m_items[0].icon!=six,"compaction copies icons and retains source allocation");
    check(!m.m_items[2].active&&!m.m_items[5].active&&m.m_items[2].id==-1&&m.m_items[5].id==-1,"moved records clear only id and active");
    check(m.m_itemCount==17,"compaction does not recompute the count");
    for(int i=0;i<10;++i)check(m.m_items[i].backgroundAlpha==float(i)&&m.m_items[i].activeAlpha==float(20+i),"compaction preserves slot alpha fields");
    m.populateItems({nullptr,nullptr});check(m.m_itemCount==17,"by-value population ignores null entries");
    // Tests wheel selection through real steppers and sound forwarding with no local player.
    // Weapon/template and underwater-alpha branches are excluded.
    players.local=nullptr;sound.onPlay=nullptr;
    for(int i=0;i<10;++i){m.m_items[i].active=false;m.m_items[i].id=200+i;}
    m.m_itemCount=0;m.m_selectedItem=2;trace.clear();m.selectNext(0);m.selectPrevious(0);
    check(trace.empty()&&m.m_selectedItem==2,"an empty list: neither step selects");
    m.m_itemCount=4;m.m_items[0].active=m.m_items[1].active=m.m_items[3].active=true;
    m.m_selectedItem=0;trace.clear();m.selectNext(0);
    check(m.m_selectedItem==1&&trace==std::vector<std::string>({"sound","player"}),"next takes the following active slot, with the sound");
    m.m_selectedItem=1;m.selectNext(0);
    check(m.m_selectedItem==0,"next over an inactive slot wraps to an active [0]");
    m.m_selectedItem=3;m.selectNext(0);
    check(m.m_selectedItem==0,"next from the last slot wraps");
    m.m_items[0].active=false;m.m_selectedItem=3;trace.clear();m.selectNext(0);
    check(m.m_selectedItem==3&&trace.empty(),"next wraps only onto an ACTIVE [0]");
    m.m_items[0].active=true;m.m_selectedItem=2;m.selectPrevious(0);
    check(m.m_selectedItem==1,"previous takes the slot before when it is active");
    m.m_selectedItem=0;m.selectPrevious(0);
    check(m.m_selectedItem==3,"previous from [0] takes the last slot when it is active");
    m.m_items[3].active=false;m.m_items[0].active=false;m.m_selectedItem=0;trace.clear();m.selectPrevious(0);
    check(m.m_selectedItem==0&&trace==std::vector<std::string>({"sound","player"}),"previous falls back to [0] WITHOUT testing it - the retail asymmetry");
    m.m_items[0].active=true;m.m_selectedItem=-1;m.selectNext(0);
    check(m.m_selectedItem==0,"the constructor's -1 steps to [0]");
    world::playerManager=nullptr;g_bfSound=nullptr;
}
};
} } }
int main(){HudMessagesFixture::run();std::printf("HUD message selection: %u/%u checks passed\n",checks-failures,checks);return failures?1:0;}
