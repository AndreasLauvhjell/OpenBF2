// [TEMP] Focused Ref2 input behavior. Real Ref2System, Pointer,
// DynamicIndexPointer, System enqueue and EventQueue bodies execute.
// Private visibility is only for supplying snapshots and observing owned state.
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <limits>
#include <new>
#include <Windows.h>
#define private public
#define protected public
#include "../BF2/Menu/Ref2System.h"
#include "../BF2/External/BFMeme/DynamicIndexPointer.h"
#include "../BF2/External/BFMeme/EventQueue.h"
#undef protected
#undef private

using namespace dice::meme;
using namespace dice::hfe;
using menu::Ref2System;
static unsigned checks, failures;
static void check(bool pass, const char* message)
{
    ++checks;
    if (!pass) { ++failures; std::printf("FAIL %s\n", message); }
}
static std::vector<const QueuedEvent*> events(const EventQueue& queue)
{
    std::vector<const QueuedEvent*> result;
    for (auto* node=queue.m_first; node; node=node->next) result.push_back(node);
    return result;
}
static void clearInputs(Ref2System& root)
{
    for(auto& input:root.m_inputs) {
        input.mappingFlags=0;
        std::memset(input.inputs,0,sizeof(input.inputs));
    }
    std::memset(root.m_inputStates,0,sizeof(root.m_inputStates));
}
static void activate(Ref2System& root, unsigned user, io::GameInputMap input, float value=1.0f)
{
    root.m_inputs[user].setInputIsMapped(input,true);
    root.m_inputs[user].inputs[input]=value;
}
static void expect(const QueuedEvent* event,int type,int index,int code,int count)
{
    check(event->type==type && event->index==index && event->first==float(code)
        && event->second==float(count) && event->field14==0,"exact queued payload");
}

// Models only a pointer callback that returns false and changes the next input
// and pointer. Ref2System::preUpdate is never overridden or shadowed.
struct Callback : Pointer
{
    Ref2System* root=nullptr;
    Pointer* replacement=nullptr;
    int calls=0;
    bool event1(System* system,int code,int index,int count) override {
        ++calls;
        check(system==root && code==1 && index==1 && count==1,"callback receives source event");
        check(root->m_inputStates[0][0]==1.0f,"state stored before callback");
        root->m_field020=replacement;
        activate(*root,0,io::GIEnter);
        return false;
    }
};

int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    static_assert(sizeof(void*)==4,"x86 fixture only");
    static_assert(sizeof(Ref2System)==1680,"native Ref2System layout");
    static_assert(sizeof(io::GameInput)==320,"native input stride");
    Ref2System root(800,600);
    EventQueue queue(0);
    Pointer pointer;
    pointer.m_enabled=true;
    root.m_field020=&pointer;
    root.m_eventQueue=&queue;
    clearInputs(root);
    check(sizeof(root.m_inputStates)==256,"four sixteen-float state banks");
    check(root.preUpdate(0.25f) && events(queue).empty(),"empty snapshots enqueue nothing");

    const io::GameInputMap mappings[]={io::GIOk,io::GIAltOk,io::GIEnter,io::GITab,
        io::GICancel,io::GIUp,io::GIDown,io::GIPageUp,io::GIPageDown,
        io::GIMouseWheelUp,io::GIMouseWheelDown,io::GILeft,io::GIRight,io::GIDelete,io::GIBack};
    const int codes[]={1,21,18,16,3,6,8,11,12,19,20,5,7,14,3};
    const int stateIndices[]={0,1,2,3,4,8,7,14,15,10,11,5,6,12,13};
    for(unsigned user=0;user<4;++user) {
        root.m_inputStates[user][9]=-77;
        for(auto mapping:mappings) activate(root,user,mapping);
    }
    check(root.preUpdate(100.0f),"initial presses ignore huge delta");
    auto got=events(queue);
    check(got.size()==60,"all fifteen controls for four users");
    for(unsigned user=0;user<4;++user) {
        for(unsigned i=0;i<15;++i) {
            expect(got[user*15+i],1,int(user+1),codes[i],1);
            check(root.m_inputStates[user][stateIndices[i]]==1,"correct state index initialized");
        }
        check(root.m_inputStates[user][9]==-77,"state nine untouched");
    }
    check(root.preUpdate(0.2f) && events(queue).size()==60,"integer three does not repeat");
    check(root.preUpdate(0.1f),"repeat crossing processed");
    got=events(queue);
    check(got.size()==112,"only thirteen controls repeat per user");
    for(unsigned user=0;user<4;++user)
        for(unsigned i=2;i<15;++i) expect(got[60+user*13+i-2],1,int(user+1),codes[i],2);
    check(root.preUpdate(0) && events(queue).size()==112,"same integer does not repeat");
    check(root.preUpdate(0.35f),"long repeat delta processed");
    got=events(queue);
    check(got.size()==164,"one event per control across several integer boundaries");
    for(unsigned user=0;user<4;++user)
        for(unsigned i=2;i<15;++i) expect(got[112+user*13+i-2],1,int(user+1),codes[i],5);
    for(auto& input:root.m_inputs) input.mappingFlags=0; // Values stay held.
    check(root.preUpdate(0),"unmapping releases held controls");
    got=events(queue);
    check(got.size()==224,"one release per active control");
    for(unsigned user=0;user<4;++user)
        for(unsigned i=0;i<15;++i) expect(got[164+user*15+i],2,int(user+1),codes[i],0);
    check(root.preUpdate(0) && events(queue).size()==224,"repeated releases suppressed");

    clearInputs(root);
    activate(root,0,io::GIOk,0.5f);
    activate(root,1,io::GIOk,std::numeric_limits<float>::quiet_NaN());
    root.m_inputs[2].inputs[io::GIOk]=1; // No mapping bit.
    check(root.preUpdate(0) && events(queue).size()==224,"half threshold NaN and unmapped value are inactive");
    activate(root,0,io::GIEnter);
    root.m_inputStates[0][2]=8;
    check(root.preUpdate(-0.2f),"negative delta accepted directly");
    got=events(queue);
    check(got.size()==225 && root.m_inputStates[0][2]==6,"decreasing integer still repeats above threshold");
    expect(got.back(),1,1,18,4);
    root.m_inputStates[0][2]=4;
    check(root.preUpdate(-0.2f) && events(queue).size()==225,"decreasing below repeat threshold emits nothing");

    clearInputs(root);
    pointer.m_enabled=false;
    activate(root,3,io::GIOk);
    check(root.preUpdate(0) && events(queue).size()==225,"disabled pointer suppresses dispatch");
    check(root.m_inputStates[3][0]==1,"disabled pointer still advances transition state");
    pointer.m_enabled=true;
    check(root.preUpdate(0) && events(queue).size()==225,"reenabling does not synthesize a lost press");
    root.m_inputs[3].mappingFlags=0;
    root.preUpdate(0);
    got=events(queue);
    expect(got.back(),2,4,1,0);

    clearInputs(root);
    root.m_field020=nullptr;
    activate(root,0,io::GIEnter);
    check(root.preUpdate(0) && events(queue).size()==226,"null pointer retains real wrapper guard");
    check(root.m_inputStates[0][2]==1,"null pointer still updates state");

    clearInputs(root);
    Callback callback;
    callback.m_enabled=true;
    callback.root=&root;
    callback.replacement=&pointer;
    root.m_field020=&callback;
    activate(root,0,io::GIOk);
    check(root.preUpdate(0) && callback.calls==1,"false callback result ignored");
    got=events(queue);
    check(got.size()==227 && root.m_field020==&pointer,"next control sees callback input and pointer changes");
    expect(got.back(),1,1,18,1);

    clearInputs(root);
    DynamicIndexPointer dynamic;
    dynamic.m_enabled=true;
    root.m_field020=&dynamic;
    activate(root,2,io::GIOk);
    check(root.preUpdate(0),"real dynamic pointer receives input");
    got=events(queue);
    expect(got.back(),1,1,1,1); // Dynamic pointer remaps physical user3 to logical1.
    check(dynamic.m_lastInputIndex==2 && dynamic.m_indexOrder[2]==1,"dynamic pointer identity and state preserved");
    root.m_inputs[2].mappingFlags=0;
    root.preUpdate(0);
    expect(events(queue).back(),2,1,1,0);
    root.m_field020=nullptr;
    root.m_eventQueue=nullptr;
    std::printf("Ref2 input: %u checks, %u failures\n",checks,failures);
    return failures ? 1 : 0;
}
