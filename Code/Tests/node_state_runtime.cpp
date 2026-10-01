// [TEMP] Assembly-derived NodeState goldens and real System::update ordering.
// Only Node event/update hooks and System/Pointer callbacks are recorders.
// updateNodeState, Node traversal, System::update and EventQueue execute unchanged.
#include <vector>
#include <string>
#include <cstdio>
#include <cmath>
#include <limits>
#include <Windows.h>
#define private public
#define protected public
#include "../BF2/External/BFMeme/NodeState.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/Pointer.h"
#include "../BF2/External/BFMeme/EventQueue.h"
#undef protected
#undef private
using namespace dice::meme;
static unsigned checks, failures;
static void check(bool pass,const char* name)
{
    ++checks;
    if(!pass) { ++failures;std::printf("FAIL %s\n",name); }
}
static bool same(float a,float b) { return (std::isnan(a)&&std::isnan(b)) || a==b; }
struct Seen { int node,type;float target,current; };
static std::vector<Seen> seen;
static std::vector<std::string> trace;
static NodeState* expectedState;
static const Rect* expectedRect;
static System* expectedSystem;
struct SpyNode : Node
{
    explicit SpyNode(int number):id(number) {}
    int id, stateQueries=0, mutation=0;
    bool ownsState=false, updateResult=true, zeroUpdateResult=true;
    NodeState owned={1,1};
    Node* replaceOnEvent=nullptr;
    Pointer* replacePointerOnUpdate=nullptr;
    NodeState* getState() override { ++stateQueries;return ownsState?&owned:nullptr; }
    bool dispatchEvent(System* system,const Rect* rect,const NodeState* state,const NodeInputEvent* event) override
    {
        check(system==expectedSystem && rect==expectedRect && state==expectedState,"event forwards exact context pointers");
        seen.push_back({id,event->type,state->target,state->current});
        trace.push_back("E"+std::to_string(event->type)+":"+std::to_string(id));
        if(event->type>=7) check(event->index==0 && event->first==0 && event->second==0 && event->field10==0,"state-event zero payload");
        if(mutation==1 && event->type==8) expectedState->target=0;
        if(mutation==2 && event->type==21) expectedState->target=-4;
        if(mutation==3 && event->type==7) {
            expectedState->target=-4;
            system->m_pointerX=9;
        }
        if(mutation==4) ownsState=true;
        if(replaceOnEvent) system->getRootNode()=replaceOnEvent;
        return false; // StateEventVisitor must ignore this result.
    }
    bool updateSelf(System* system,const NodeState* state,const Rect* rect,float delta) override
    {
        check(system==expectedSystem && state==expectedState && rect==expectedRect,"update forwards live System fields");
        trace.push_back(std::string(delta==0?"Z":"U")+std::to_string(id));
        if(replacePointerOnUpdate) system->m_field020=replacePointerOnUpdate;
        return delta==0?zeroUpdateResult:updateResult;
    }
};
static std::vector<int> eventTypes()
{
    std::vector<int> types;for(const auto& event:seen) types.push_back(event.type);return types;
}
struct Golden
{
    const char* name;float target,current,delta;bool inside,enabled;int lock;bool moved;
    float expectedTarget,expectedCurrent;std::vector<int> events;
};
static void nodeStateGoldens()
{
    const float nan=std::numeric_limits<float>::quiet_NaN();
    // Values and event sequences are literal branch goldens from838FB0.
    const Golden cases[]={
        {"disabled fade",2,1,.25f,false,false,0,false,0,1.75f,{20}},
        {"disabled inside",-3,-2,.25f,true,false,0,false,-1,1.75f,{8,20}},
        {"disabled locked inside",-3,-2,.25f,true,false,1,false,-1,-2.75f,{8,20}},
        {"enabled from zero",0,0,0,false,true,0,false,1,0,{21}},
        {"enabled enter from zero",0,0,0,true,true,0,false,-3,-1,{21,7,9}},
        {"enter",1,1,0,true,true,0,false,-3,-2,{7,9}},
        {"stay inside",-3,-2,.25f,true,true,0,false,-3,-2.75f,{}},
        {"move inside",-3,-2,.25f,true,true,0,true,-3,-2.75f,{11}},
        {"leave",-3,-2,0,false,true,0,false,1,1,{8,10}},
        {"locked leave",-3,-2,0,false,true,1,false,1,-2,{10}},
        {"negative lock also locks",-3,-2,0,false,true,-1,false,1,-2,{10}},
        {"locked move outside",1,-2,0,false,true,1,true,1,-2,{12}},
        {"pressed inside",-4,-4,0,true,true,0,false,-4,-4,{}},
        {"enter pressed",3,3,0,true,true,0,false,-4,-4,{7,9}},
        {"leave pressed",-4,-4,0,false,true,0,false,3,3,{8,10}},
        {"falling clamp",1,2,1,false,true,0,false,1,1,{}},
        {"rising clamp",2,1,1,false,true,0,false,2,2,{}},
        {"negative delta",2,1,-.25f,false,true,0,false,2,.25f,{}},
        {"disabled unordered target",nan,1,0,false,false,0,false,0,1,{20}},
        {"enabled unordered target",nan,1,0,false,true,0,false,nan,1,{}},
        {"unordered current enter",1,nan,0,true,true,0,false,-3,nan,{9}},
        {"unordered delta",2,1,nan,false,true,0,false,2,nan,{}},
        {"encoded zero exit",-1,-1,0,false,false,0,false,0,0,{8}},
        {"stored interpolation float",3,1,1.0f/3.0f,false,true,0,false,3,2,{}},
        {"infinite delta clamps",2,1,std::numeric_limits<float>::infinity(),false,true,0,false,2,2,{}},
    };
    for(const auto& row:cases) {
        System system(800,600);
        system.m_lockCount=row.lock;
        system.m_pointerX=row.moved?4.0f:0.0f;
        NodeState state={row.target,row.current};
        Rect rect={-400,-300,800,600,0};
        SpyNode node(1);
        expectedState=&state;expectedRect=&rect;expectedSystem=&system;
        seen.clear();trace.clear();
        updateNodeState(&state,&system,nullptr,&node,&rect,row.delta,row.inside,row.enabled);
        check(same(state.target,row.expectedTarget),row.name);
        check(same(state.current,row.expectedCurrent),row.name);
        check(eventTypes()==row.events,row.name);
        check(node.stateQueries==0,"first node skips state query");
    }
    System system(800,600);Rect rect={0,0,10,10,0};SpyNode node(1);
    expectedSystem=&system;expectedRect=&rect;
    NodeState state={-3,-3};expectedState=&state;
    system.m_pointerX=nan;seen.clear();
    updateNodeState(&state,&system,nullptr,&node,&rect,0,true,true);
    check(eventTypes()==std::vector<int>{11},"unordered pointer comparison counts as movement");
    // Event callback changes the fields that later branches must reload.
    for(int mutation=1;mutation<=3;++mutation) {
        node.mutation=mutation;system.m_pointerX=0;seen.clear();
        state=mutation==1?NodeState{-3,-2}:mutation==2?NodeState{0,0}:NodeState{1,1};
        updateNodeState(&state,&system,nullptr,&node,&rect,0,mutation==3,mutation!=1);
        check(eventTypes()==(mutation==1?std::vector<int>{8}:mutation==2?std::vector<int>{21,10}:std::vector<int>{7,11}),"callback mutation controls subsequent events");
        check(state.target==(mutation==1?0.0f:mutation==2?1.0f:-4.0f),"callback target flag reread");
    }
}
static void visitorBoundaries()
{
    System system(800,600);Rect rect={0,0,10,10,0};NodeState state={-3,-2};
    SpyNode root(1),middle(2),leaf(3);
    root.setChild(0,&middle);middle.setChild(0,&leaf);
    root.ownsState=true;middle.ownsState=true;
    expectedSystem=&system;expectedRect=&rect;expectedState=&state;
    seen.clear();
    updateNodeState(&state,&system,nullptr,&root,&rect,0,false,false);
    check(eventTypes()==std::vector<int>({8,8,20,20}),"each visitor crosses first state-owning node and stops at next");
    check(seen[0].node==1 && seen[1].node==2 && seen[2].node==1 && seen[3].node==2,"fresh visitor for second event");
    check(root.stateQueries==0 && middle.stateQueries==2 && leaf.stateQueries==0,"state boundary query order");
    middle.ownsState=false;middle.mutation=4;state={-3,-2};seen.clear();
    updateNodeState(&state,&system,nullptr,&root,&rect,0,false,false);
    check(seen.size()==4 && middle.ownsState,"visitor queries state after event callback");
    middle.mutation=0;middle.ownsState=false;state={-3,-2};seen.clear();
    updateNodeState(&state,&system,nullptr,&root,&rect,0,false,false);
    check(eventTypes()==std::vector<int>({8,8,8,20,20,20}),"stateless intermediate node allows descent");
}
struct SpySystem : System
{
    SpySystem():System(800,600) {}
    bool before=true,after=true;
    Node* replaceBefore=nullptr;
    bool preUpdate(float) override {
        trace.push_back("P");if(replaceBefore)m_field018=replaceBefore;return before;
    }
    bool postUpdate(float) override { trace.push_back("O");return after; }
};
struct SpyPointer : Pointer
{
    explicit SpyPointer(int number):id(number) { m_enabled=true; }
    int id;bool result=true;
    bool update(System* system,float delta) override {
        trace.push_back("K"+std::to_string(id));
        Pointer::update(system,delta); // Real previous-position copy, no graphic.
        return result;
    }
};
static void systemOrdering()
{
    for(int stop=0;stop<5;++stop) {
        SpySystem system;EventQueue queue(0);SpyNode node(1);SpyPointer pointer(1);
        system.m_field018=&node;system.m_field020=&pointer;system.m_eventQueue=&queue;
        system.before=stop!=1;node.updateResult=stop!=2;pointer.result=stop!=3;system.after=stop!=4;
        node.zeroUpdateResult=false; // Queue's post-event update result is ignored.
        system.m_pointerX=17;system.m_pointerY=23;
        expectedSystem=&system;expectedState=system.getNodeState();expectedRect=system.getBounds();
        system.enqueueEvent(4,2,6,3,0);seen.clear();trace.clear();
        const bool result=system.System::update(.25f);
        check(result==(stop==0),"System update failure propagation");
        const std::vector<std::string> expected=stop==1?std::vector<std::string>{"P"}:
            stop==2?std::vector<std::string>{"P","U1"}:
            stop==3?std::vector<std::string>{"P","U1","E4:1","Z1","K1"}:
            std::vector<std::string>{"P","U1","E4:1","Z1","K1","O"};
        check(trace==expected,"pre state root queue pointer post ordering");
        check(system.m_field02c==(stop==1?2.0f:-3.0f),"NodeState runs after successful preUpdate");
        check((queue.m_first!=nullptr)==(stop==1 || stop==2),"early failure leaves queue pending");
        if(stop==0 || stop>=3) check(system.m_previousPointerX==17 && system.m_previousPointerY==23,"real Pointer update follows queue");
        system.m_field018=nullptr;system.m_field020=nullptr;system.m_eventQueue=nullptr;
    }
    SpySystem system;EventQueue queue(0);SpyNode first(1),next(2);SpyPointer oldPointer(1),newPointer(2);
    system.m_field018=&first;system.m_field020=&oldPointer;system.m_eventQueue=&queue;
    first.replaceOnEvent=&next;first.replacePointerOnUpdate=&newPointer;
    expectedSystem=&system;expectedState=system.getNodeState();expectedRect=system.getBounds();
    system.enqueueEvent(4,1,6,3,0);seen.clear();trace.clear();
    check(system.System::update(.25f),"update accepts callback mutations");
    check(trace==std::vector<std::string>({"P","U1","E4:1","Z2","K2","O"}),"queue root and pointer reloaded after callbacks");
    system.replaceBefore=&first;system.m_field018=&next;first.replaceOnEvent=nullptr;trace.clear();
    check(system.System::update(.25f),"preUpdate may replace root");
    check(trace==std::vector<std::string>({"P","U1","K2","O"}),"root reloaded after preUpdate");
    system.replaceBefore=nullptr;system.m_field018=nullptr;system.m_field020=nullptr;trace.clear();
    check(system.System::update(.25f) && trace==std::vector<std::string>({"P","O"}),"null node and pointer guards");
    system.m_field020=&oldPointer;oldPointer.m_enabled=false;trace.clear();
    check(system.System::update(.25f) && trace==std::vector<std::string>({"P","O"}),"disabled pointer wrapper guard");
    system.m_field020=nullptr;system.m_eventQueue=nullptr;
}
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    static_assert(sizeof(void*)==4,"native x86 validation");
    static_assert(sizeof(NodeState)==8 && sizeof(NodeVisitor)==8 && sizeof(System)==88,"client layouts");
    nodeStateGoldens();visitorBoundaries();systemOrdering();
    std::printf("NodeState/System update: %u checks, %u failures\n",checks,failures);
    return failures?1:0;
}
