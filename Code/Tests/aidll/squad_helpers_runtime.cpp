// [TEMP] Three verbatim production methods run in actual.cpp. This TU supplies
// chosen native lifetimes and typed collaborators, not retail lifecycle behavior.
#include "squad_helpers_runtime.h"
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <limits>
#include <new>
#include <string>
#include <windows.h>
#include <crtdbg.h>
using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*)==4,"MSVC x86 only");
static_assert(sizeof(SquadManager::CommanderMessage)==24,"Native message extent");
static_assert(offsetof(SquadManager::CommanderMessage,m_dueTime)==16,"Native double alignment");
static unsigned checks;
[[noreturn]] static void unexpected(const char* name) { std::fprintf(stderr,"FAIL %s\n",name); std::fflush(stderr); std::abort(); }
static void require(bool value,const char* why) { ++checks; if(!value) unexpected(why); }
static std::string mode,calls;
static void trace(const std::string& s) { if(!calls.empty()) calls+=','; calls+=s; }
static void expect(const std::string& wanted) { if(calls!=wanted) { std::fprintf(stderr,"TRACE got=%s expected=%s\n",calls.c_str(),wanted.c_str()); unexpected("callback order"); } ++checks; calls.clear(); }
using Message=SquadManager::CommanderMessage;
static std::list<Message*>* queue;
static std::map<int,double>* positions;
struct Allocation { void* p; int deletes; };
static Allocation allocations[64];
static int allocationCount;
void* operator new(std::size_t n) { if(void* p=std::malloc(n?n:1)) return p; throw std::bad_alloc(); }
void operator delete(void* p) noexcept {
    if(!p) return;
    for(int i=0;i<allocationCount;++i) if(allocations[i].p==p && !allocations[i].deletes) {
        ++allocations[i].deletes; trace("D"+std::to_string(i)); break;
    }
    std::free(p);
}
void operator delete(void* p,std::size_t) noexcept { ::operator delete(p); }
static Message* message(int type,int squad,int side,double deadline,bool append=true) {
    Message* p=new Message{}; p->m_type=static_cast<SquadManager::CommanderMessageType>(type);
    p->m_squadIndex=squad; p->m_sideIndex=side; p->m_dueTime=deadline;
    require(allocationCount<64,"allocation capacity"); allocations[allocationCount++]={p,0};
    if(append) queue->push_back(p); return p;
}
static double clockValue=10.0;
static bool commanders[2];
static SAI* strategies[2];
static IAIMain* nextMain;
static IBFCommander* nextCommander;
static IAIPlayerFactory* nextFactory;
static std::function<void()> clockCallback,commandCallback,aiCallback,lookupCallback;
static int commandCount;
static SAI* observeSAI(int identity,int side) {
    trace("S"+std::to_string(identity)+":"+std::to_string(side));
    require(side==1 || side==2,"one-based side");
    if(nextMain) { IAIMain::m_instance=nextMain; nextMain=nullptr; }
    return strategies[side-1];
}
static double observeTime() { trace("T"); if(clockCallback) clockCallback(); return clockValue; }
static void observeCommand(int identity,int type,int side,int squad) {
    trace("C"+std::to_string(identity)+":"+std::to_string(type)+":"+std::to_string(side)+":"+std::to_string(squad));
    require(!queue->empty(),"node exists during callback");
    ++commandCount;
    if(commandCallback) commandCallback();
    if(nextCommander) { CommonSingletonServer::m_iBFCommander=nextCommander; nextCommander=nullptr; }
}
static std::vector<IAIPlayer*> roster;
static std::map<int,IAIPlayer*> byId;
static const std::vector<IAIPlayer*>& observeRoster(int identity) { trace("R"+std::to_string(identity)); return roster; }
static IAIPlayer* observeLookup(int identity,int id) {
    trace("L"+std::to_string(identity)+":"+std::to_string(id));
    if(lookupCallback) lookupCallback();
    if(nextFactory) { CommonSingletonServer::m_iaiPlayerFactory=nextFactory; nextFactory=nullptr; }
    auto i=byId.find(id); return i==byId.end()?nullptr:i->second;
}
struct PlayerState { int id=0; bool ai=false,alive=true,down=false; int aiCalls=0,idCalls=0; IAIObject* vehicle=nullptr; std::vector<bool> aiAnswers; };
static PlayerState states[8];
static int observeId(int n) { trace("I"+std::to_string(n)); ++states[n].idCalls; return states[n].id; }
static bool observeAI(int n) {
    trace("A"+std::to_string(n)); int at=states[n].aiCalls++;
    if(aiCallback) aiCallback();
    return at<static_cast<int>(states[n].aiAnswers.size())?states[n].aiAnswers[at]:states[n].ai;
}
static bool observeAlive(int n) { trace("V"+std::to_string(n)); return states[n].alive; }
static bool observeDown(int n) { trace("N"+std::to_string(n)); return states[n].down; }
static IAIObject* observeVehicle(int n) { trace("O"+std::to_string(n)); return states[n].vehicle; }
static int observeSide(int n) {
    trace("E"+std::to_string(n));
    if(mode=="human_vehicle_stop") require(calls=="R0,A0,V0,N0,O0,E0" && positions->at(0)==42,"side query precedes stop with timestamp untouched");
    return -731;
}
#include "mocks.inl"
namespace dice { namespace hfe { namespace ai {
void* CommonSingletonServer::m_iBFTimer=nullptr;
void* CommonSingletonServer::m_iBFCommander=nullptr;
void* CommonSingletonServer::m_iaiPlayerFactory=nullptr;
IAIMain* IAIMain::m_instance=nullptr;
IAIMain::IAIMain() {} IAIMain::~IAIMain() {}
IBFTimer::IBFTimer() {} IBFTimer::~IBFTimer() {}
IBFCommander::IBFCommander() {} IBFCommander::~IBFCommander() {}
IAIPlayerFactory::IAIPlayerFactory() {} IAIPlayerFactory::~IAIPlayerFactory() {}
IAIPlayer::~IAIPlayer() {}
ISAI::~ISAI() {}
SAI::SAI(int side,int,int,Vec2,Vec2):m_side(side) {}
SAI::~SAI() {}
bool SAI::isPlayerCommander() { trace("P"+std::to_string(m_side)); return commanders[m_side-1]; }
SquadManager::SquadManager() { queue=&m_commanderMessages; positions=&m_playerValidPositions; }
} } }
static void finish() {
    queue->clear();
    for(int i=0;i<allocationCount;++i) if(!allocations[i].deletes) delete static_cast<Message*>(allocations[i].p);
    for(int i=0;i<allocationCount;++i) require(allocations[i].deletes==1,"all fixture messages deleted once");
    std::printf("PASS %s %u checks\n",mode.c_str(),checks);
}
int main(int argc,char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX); _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE); _CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
    require(argc==2,"mode required"); mode=argv[1];
    SquadManager manager;
    SAI s1(1,0,0,{},{}),s2(2,0,0,{},{}); strategies[0]=&s1; strategies[1]=&s2;
    ObservedMain main0,main1; main1.identity=1; IAIMain::m_instance=&main0;
    ObservedTimer timer; CommonSingletonServer::m_iBFTimer=&timer;
    ObservedCommander commander0,commander1; commander1.identity=1; CommonSingletonServer::m_iBFCommander=&commander0;
    ObservedFactory factory0,factory1; factory1.identity=1; CommonSingletonServer::m_iaiPlayerFactory=&factory0;
    ObservedPlayer players[8]; for(int i=0;i<8;++i) { players[i].identity=i; states[i].id=i; }
    const std::string prefix="S0:1,P1,S0:2,P2,T";
    if(mode.compare(0,8,"message_")==0) {
        std::string wanted=prefix;
        if(mode=="message_empty") {}
        else if(mode=="message_purge0" || mode=="message_purge1" || mode=="message_purge_both") {
            commanders[0]=mode!="message_purge1"; commanders[1]=mode!="message_purge0";
            message(0,2,0,100); message(1,3,1,100); Message* invalid=message(-1,4,-3,100);
            wanted="S0:1,P1"; if(commanders[0]) wanted+=",D0";
            wanted+=",S0:2,P2"; if(commanders[1]) wanted+=",D1"; wanted+=",T";
            manager.updateCommanderMessages(); expect(wanted);
            require(queue->size()==3-commanders[0]-commanders[1] && queue->back()==invalid,"purge exact side only");
            finish(); return 0;
        }
        else if(mode=="message_expiry") {
            message(0,2,0,9); message(1,3,1,9); message(2,5,0,9); message(-1,6,0,9); message(3,7,0,9);
            Message* equal=message(0,8,0,10); message(0,9,0,11); message(0,10,0,std::numeric_limits<double>::quiet_NaN());
            wanted+=",C0:0:1:3,D0,C0:1:2:4,D1,D2,D3,D4";
            manager.updateCommanderMessages(); expect(wanted);
            require(queue->size()==3 && queue->front()==equal && commandCount==2,"strict expiry and only two recognized types"); finish(); return 0;
        }
        else if(mode=="message_time_nan") { clockValue=std::numeric_limits<double>::quiet_NaN(); message(0,0,0,-100); }
        else if(mode=="message_fresh_main") { nextMain=&main1; wanted="S0:1,P1,S1:2,P2,T"; }
        else if(mode=="message_fresh_commander") { message(0,0,0,9); message(1,4,1,9); nextCommander=&commander1; wanted+=",C0:0:1:1,D0,C1:1:2:5,D1"; }
        else if(mode=="message_reload_payload") {
            Message* original=message(0,2,0,9); Message* replacement=message(7,7,7,100,false);
            commandCallback=[=] { require(queue->front()==original,"original payload visible before mutation"); queue->front()=replacement; };
            wanted+=",C0:0:1:3,D1";
        }
        else if(mode=="message_append") {
            message(0,0,0,9); commandCallback=[] { if(commandCount==1) message(1,7,1,9); };
            wanted+=",C0:0:1:1,D0,C0:1:2:8,D1";
        }
        else if(mode=="message_clock_mutation") {
            message(0,0,0,11); clockCallback=[] { queue->front()->m_dueTime=9; }; wanted+=",C0:0:1:1,D0";
        } else unexpected("unknown message mode");
        manager.updateCommanderMessages(); expect(wanted);
        if(mode=="message_time_nan") require(queue->size()==1 && allocations[0].deletes==0,"NaN clock preserves record");
        else require(queue->empty(),"all due records removed");
        if(mode=="message_reload_payload") require(allocations[0].deletes==0 && allocations[1].deletes==1,"delete rereads callback replacement");
    } else if(mode.compare(0,7,"setter_")==0) {
        states[0].id=mode=="setter_full_id"?INT_MIN:17;
        if(mode=="setter_insert" || mode=="setter_full_id") {
            manager.setPlayerOnValidPosition(&players[0],false,1.0000000000000002);
            manager.setPlayerOnValidPosition(&players[0],false,9.0);
            require(positions->size()==1 && positions->at(states[0].id)==1.0000000000000002,"first double timestamp retained"); expect("I0,I0");
        } else if(mode=="setter_erase") {
            (*positions)[17]=1; (*positions)[18]=2; manager.setPlayerOnValidPosition(&players[0],true,9); manager.setPlayerOnValidPosition(&players[0],true,10);
            require(positions->size()==1 && positions->count(18)==1,"valid clears only matching id and tolerates absent"); expect("I0,I0");
        } else if(mode=="setter_nan") {
            manager.setPlayerOnValidPosition(&players[0],false,std::numeric_limits<double>::quiet_NaN());
            manager.setPlayerOnValidPosition(&players[0],false,3); require(std::isnan(positions->at(17)),"first NaN retained"); expect("I0,I0");
        } else unexpected("unknown setter mode");
        require(states[0].idCalls==2,"one id query per invocation");
    } else {
        std::string wanted="R0";
        if(mode=="human_empty") {}
        else if(mode=="human_null") roster={nullptr};
        else if(mode=="human_prune") {
            for(int i=0;i<5;++i) (*positions)[i]=20+i;
            byId[1]=&players[1]; states[1].ai=true;
            byId[2]=&players[2]; states[2].alive=false;
            byId[3]=&players[3]; states[3].down=true;
            byId[4]=&players[4];
            wanted+=",L0:0,L0:1,A1,L0:2,A2,V2,L0:3,A3,V3,N3,L0:4,A4,V4,N4";
        } else if(mode=="human_fresh_factory") {
            (*positions)[0]=1; (*positions)[1]=2; nextFactory=&factory1; wanted+=",L0:0,L1:1";
        } else if(mode=="human_live_map") {
            (*positions)[0]=1;
            lookupCallback=[done=false]() mutable { if(!done) { done=true; (*positions)[2]=3; } }; wanted+=",L0:0,L0:2";
        } else {
            roster={&players[0]}; (*positions)[0]=42;
            if(mode=="human_ai") { states[0].ai=true; byId[0]=&players[0]; wanted+=",A0,A0,L0:0,A0"; }
            else if(mode=="human_dead") { states[0].alive=false; wanted+=",A0,V0,A0,I0"; }
            else if(mode=="human_down") { states[0].down=true; wanted+=",A0,V0,N0,A0,I0"; }
            else if(mode=="human_vehicle_null") wanted+=",A0,V0,N0,O0,A0,I0";
            else if(mode=="human_repeat_ai") { states[0].aiAnswers={true,false}; wanted+=",A0,A0,I0"; }
            else if(mode=="human_copy") {
                roster.push_back(&players[1]); states[0].alive=states[1].alive=false;
                aiCallback=[] { roster.clear(); roster.reserve(200); roster.push_back(nullptr); };
                wanted+=",A0,V0,A0,I0,A1,V1,A1,I1";
            } else if(mode=="human_vehicle_stop") {
                // Opaque identity only: genuine storage is never dereferenced as IAIObject.
                static int token; states[0].vehicle=reinterpret_cast<IAIObject*>(&token);
            } else unexpected("unknown human mode");
        }
        manager.updateHumanPlayers(9.000000000000002);
        if(mode=="human_vehicle_stop") unexpected("frontier returned");
        expect(wanted);
        if(mode=="human_prune") require(positions->size()==1 && positions->at(4)==24,"only live nondown human timestamp survives");
        else require(positions->empty(),"nonvehicle validity or cleanup removes entries");
        if(mode=="human_repeat_ai") require(states[0].aiCalls==2 && states[0].idCalls==1,"fresh second AI result controls setter");
        if(mode=="human_copy") require(states[1].aiCalls==2 && roster.size()==1 && roster[0]==nullptr,"local roster copy survives external replacement");
    }
    finish(); return 0;
}
