// [TEMP] [CHOSEN, not recovered] Native x86 actual-body Messages tests.
// Typed observers model registration callbacks; no production method is replaced.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <new>
#include <limits>
#define private public
#include "../BF2/Menu/HudMessages.h"
#include "../BF2/Core/EventManager.h"
#undef private
#include "../BF2/Game/HudInformationLayer.h"
namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
IBase* g_hudInformationLayer = nullptr;
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
static int checks, failures;
static const void* watched[2];
static int freed;
static bool unregisterComplete;
static void check(bool value, const char* label) {
    ++checks; if (!value) { ++failures; std::fprintf(stderr,"FAIL %s\n",label); }
}
// Observe only the two explicit test-owned strings; normal allocation semantics.
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void* p) noexcept {
    if (p && (p == watched[0] || p == watched[1])) {
        ++freed; check(unregisterComplete,"owned strings are freed after both unregister calls");
    }
    ::operator delete(p);
}
struct Events : EventManager {
    HudMessages* constructing = nullptr;
    HudMessages* destroying = nullptr;
    std::vector<int> registrations, removals;
    bool registerEventHandler(EventCategory c, IEventListener* listener, int priority) override {
        if (constructing) {
            registrations.push_back(c);
            check(listener == static_cast<IEventListener*>(constructing),"event listener is primary this");
            check(priority == 0,"event priority0");
            check(reinterpret_cast<std::uintptr_t>(constructing->m_items[0].icon)==0xa5a5a5a5,"item initialization follows both event registrations");
            check(constructing->m_remainingTime==10 && !constructing->m_visible,"core fields initialized before event registration");
        }
        return EventManager::registerEventHandler(c,listener,priority);
    }
    bool unregisterEventHandler(EventCategory c, IEventListener* listener) override {
        if (destroying && listener == static_cast<IEventListener*>(destroying)) {
            removals.push_back(c);
            unregisterComplete = removals.size() == 2;
        }
        return EventManager::unregisterEventHandler(c,listener);
    }
};
struct Binding { char kind; std::string name; const void* address; };
static std::vector<Binding> bindings;
struct Information : HudInformationLayer {
    Information* next = nullptr;
    int calls = 0;
    int record(char kind,const std::string& name,const void* p) {
        ++calls; bindings.push_back({kind,name,p});
        if (next) g_hudInformationLayer = next;
        return -1; // Deliberate observed callback result, discarded by Messages.
    }
    int registerBool(const std::string& name,bool* p) override { return record('B',name,p); }
    int registerInt(const std::string& name,int* p) override { return record('I',name,p); }
    int registerFloat(const std::string& name,float* p) override { return record('F',name,p); }
    int registerString(const std::string& name,const char** p) override { return record('S',name,p); }
};
static std::uint32_t bits(float f) { std::uint32_t value; std::memcpy(&value,&f,4); return value; }
static bool hasListener(Events& e, int category, IEventListener* p) {
    for (auto* h=e.m_handlers[category];h;h=h->m_next) if(h->m_listener==p) return true;
    return false;
}
int main() {
    static_assert(sizeof(void*) == 4,"x86 fixture required");
    static_assert(sizeof(HudMessages) == 0xf8,"non-STL layout is retail248 bytes");
    Events events; eventManager=&events;
    Information first, second; first.next=&second; g_hudInformationLayer=&first;
    alignas(HudMessages) unsigned char memory[sizeof(HudMessages)]; std::memset(memory,0xa5,sizeof(memory));
    events.constructing=reinterpret_cast<HudMessages*>(memory);
    auto* messages=new(memory) HudMessages(); events.constructing=nullptr;
    check(events.registrations==std::vector<int>({1,2}),"registration category order");
    check(messages->m_field004==0xa5a5a5a5,"ctor preserves unknown word");
    check(messages->m_enabled && !messages->m_visible && messages->m_selectedItem==-1,"ctor Boolean and selected defaults");
    check(messages->m_field010==0 && messages->m_itemCount==0,"ctor counts");
    check(messages->m_duration==10 && messages->m_remainingTime==10,"ctor time defaults");
    check(messages->m_field020==0.3 && messages->m_field028==0,"ctor doubles");
    const char* sentinel = messages->m_items[0].icon;
    for(auto& item:messages->m_items) {
        check(item.icon==sentinel && item.icon[0]==0 && item.id==0 && !item.active,"all item defaults share zero sentinel identity");
        check(bits(item.backgroundAlpha)==0xa5a5a5a5 && bits(item.activeAlpha)==0xa5a5a5a5,"ctor leaves alpha fields untouched");
    }
    check(hasListener(events,1,messages)&&hasListener(events,2,messages),"real EventManager stores both listeners");
    messages->handleEvent(ECGame,5,nullptr,nullptr);
    messages->init();
    check(!messages->slot_0x0c(),"slot0C is retail false");
    check(messages->registerData(),"all binding failures are ignored");
    check(bindings.size()==42 && first.calls==1 && second.calls==41,"42 bindings reload the information global after callbacks");
    check(bindings[0].kind=='B' && bindings[0].name=="ShowItemSelect" && bindings[0].address==&messages->m_visible,"visibility binding");
    check(bindings[1].kind=='I' && bindings[1].name=="ItemSelectActive" && bindings[1].address==&messages->m_selectedItem,"selected index binding");
    for(int i=0;i<10;++i) {
        const auto k=static_cast<std::size_t>(2+4*i); const std::string n=std::to_string(i);
        check(bindings[k].kind=='B' && bindings[k].name=="ItemSlot"+n+"Active" && bindings[k].address==&messages->m_items[i].active,"per-item active binding");
        check(bindings[k+1].kind=='S' && bindings[k+1].name=="Item"+n+"Icon" && bindings[k+1].address==&messages->m_items[i].icon,"per-item icon pointer binding");
        check(bindings[k+2].kind=='F' && bindings[k+2].name=="ItemSlot"+n+"BackActiveAlpha" && bindings[k+2].address==&messages->m_items[i].backgroundAlpha,"per-item background alpha binding");
        check(bindings[k+3].kind=='F' && bindings[k+3].name=="ItemSlot"+n+"ActiveAlpha" && bindings[k+3].address==&messages->m_items[i].activeAlpha,"per-item active alpha binding");
    }
    check(!messages->update(5) && messages->m_remainingTime==10,"hidden update retains timer");
    messages->m_visible=true; messages->m_remainingTime=1;
    check(messages->update(2) && messages->m_remainingTime==-1,"timer overshoot stays visible for this call");
    check(!messages->update(0) && messages->m_remainingTime==-1,"negative timer hides only on next call");
    messages->m_visible=true; messages->m_remainingTime=0;
    check(messages->update(0) && messages->m_visible,"zero timer remains visible");
    check(messages->update(-3) && messages->m_remainingTime==3,"negative delta increases timer");
    messages->m_remainingTime=std::numeric_limits<float>::quiet_NaN();
    check(!messages->update(1),"unordered NaN timer hides");
    messages->m_selectedItem=2; messages->m_items[2].id=42; messages->m_items[0].id=12; messages->m_itemCount=3;
    check(messages->getSelectedItem()==42,"selected item getter");
    check(messages->findItem(42)==2 && messages->findItem(12)==0 && messages->findItem(999)==0,"find returns first matching index or zero");
    messages->m_itemCount=-1; check(messages->findItem(42)==0,"signed nonpositive count returns zero");
    messages->m_visible=true; check(messages->confirmSelection()==42 && !messages->m_visible,"confirmation hides and returns selected id");
    auto* icon=new char[8]; std::strcpy(icon,"anicon");
    auto* empty=new char[1]; empty[0]=0;
    watched[0]=icon; watched[1]=empty;
    messages->m_items[1].icon=icon; messages->m_items[7].icon=empty;
    messages->m_items[4].icon=nullptr;
    events.destroying=messages; messages->~HudMessages(); events.destroying=nullptr;
    check(events.removals==std::vector<int>({1,2}),"dtor unregister category order");
    check(freed==2,"dtor frees owned normal and empty strings by identity");
    check(!hasListener(events,1,messages)&&!hasListener(events,2,messages),"dtor removes both real listeners");
    std::printf("HUD messages: %d/%d checks passed\n",checks-failures,checks);
    return failures?1:0;
}
