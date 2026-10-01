// [TEMP] Actual commander actions, ClientVoteManager lifetime/issue helpers,
// and VoteEvent lifetime. No map object is fabricated. The sole message-display
// boundary observer throws after recording entry; production remains declaration-only.
#define main cameraRegressionMain
#include "hud_camera_runtime.cpp"
#undef main
#include "../BF2/Game/Common/ClientVoteManager.h"
#include "../BF2/Game/Common/VoteEvent.h"
#include "../BF2/Game/Objects/ICameraObject.h"
#include <cstring>
#include <limits>
using world::ICameraObject;
using world::CameraViewMode;
using world::IObject;
struct DisplayBoundary {};
static int displayCalls,displayCode;
static bool displayForced;
namespace dice { namespace hfe {
void ClientVoteManager::displayMessage(ClientVoteMessage code,bool force) {
    std::fprintf(stderr,"[FRONTIER] fixture observed dice::hfe::ClientVoteManager::displayMessage\n");
    std::fflush(stderr);
    trace+='M';++displayCalls;displayCode=static_cast<int>(code);displayForced=force;
    throw DisplayBoundary{};
}
} }
struct ResignCamera : ICameraObject {
    CameraViewMode current=static_cast<CameraViewMode>(7),received=static_cast<CameraViewMode>(0);
    IObject* subject=reinterpret_cast<IObject*>(1);
    bool forced=false;
    int writes=0;
    std::function<void()> onRead;
    void addRef() override {actionUnsupported("camera addRef");}
    int getRef() override {actionUnsupported("camera getRef");}
    int release() override {actionUnsupported("camera release");}
    void* queryInterface(unsigned) const override {actionUnsupported("camera query");}
    CameraViewMode getViewMode() const override {
        trace+='V';if(onRead)onRead();return current;
    }
    bool setViewMode(CameraViewMode value,IObject* object,bool force) override {
        trace+='S';received=value;subject=object;forced=force;++writes;return false;
    }
    const Mat4& getTransformation(float) override { actionUnsupported("camera unused method"); }
    void setInterpolatedMode(bool) override { actionUnsupported("camera unused method"); }
    bool isActive() const override { actionUnsupported("camera unused method"); }
    void setIsActive(bool) override { actionUnsupported("camera unused method"); }
    void setCameraId(int id) override { actionUnsupported("camera unused method"); }
    int  getCameraId() override { actionUnsupported("camera unused method"); }
    void prevViewMode() override { actionUnsupported("camera unused method"); }
    void setTraceObjectId(unsigned int id) override { actionUnsupported("camera unused method"); }
    unsigned int getTraceObjectId() override { actionUnsupported("camera unused method"); }
    void setExternCameraTrans(Mat4 transform) override { actionUnsupported("camera unused method"); }
    Mat4* getExternCameraTrans() override { actionUnsupported("camera unused method"); }
    bool getToggleMouseLook() override { actionUnsupported("camera unused method"); }
    int  getPcoId() override { actionUnsupported("camera unused method"); }
    void handleExplosion(float) override { actionUnsupported("camera unused method"); }
    void resetCameraView(bool) override { actionUnsupported("camera unused method"); }
    void updateCameraView(bool) override { actionUnsupported("camera unused method"); }
    float getActiveFOV() const override { actionUnsupported("camera unused method"); }
    bool getAllowDynamicLights() const override { actionUnsupported("camera unused method"); }
    bool getAllowDynamicSun() const override { actionUnsupported("camera unused method"); }
    const Vec3& getStaticSunDirection() const override { actionUnsupported("camera unused method"); }
    const Vec3& getStaticSunColor() const override { actionUnsupported("camera unused method"); }
    const Vec3& getStaticSunDamageColor() const override { actionUnsupported("camera unused method"); }
    float getInsideDamageFrequency() const override { actionUnsupported("camera unused method"); }
    float getInsideDamageThreshold() const override { actionUnsupported("camera unused method"); }
    const Vec3& getStaticSunAmbientColor() const override { actionUnsupported("camera unused method"); }
    bool allowCameraModeCycle() override { actionUnsupported("camera unused method"); }
    bool isOverheadCamera() const override { actionUnsupported("camera unused method"); }
};
struct FinishPlayer : Player {
    bool participates=true;
    ICameraObject* camera=nullptr;
    int disabled=-1;
    std::function<void()> onParticipates,onDisable;
    bool getParticipateInVoting() override {trace+='B';if(onParticipates)onParticipates();return participates;}
    void disableVOHelpMessage(int id) override {trace+='H';disabled=id;if(onDisable)onDisable();}
    ICameraObject* getCamera() const override {trace+='K';return camera;}
};
struct FinishPlayers : Players {
    std::vector<world::IPlayer*> sequence;
    std::size_t index=0;
    world::IPlayer* getLocalHumanPlayer() override {
        trace+='P';if(index<sequence.size())return sequence[index++];return local;
    }
    void reset(world::IPlayer* value){local=value;sequence.clear();index=0;}
};
struct FinishSquads : Squads {
    int resigned=-1,commander=42,teamForCommander=-1;
    std::function<void()> onResign;
    void resignCommander(int team) override {trace+='R';resigned=team;if(onResign)onResign();}
    int getCommander(int team) override {trace+='C';teamForCommander=team;return commander;}
};
struct FinishEvents : EventManager {
    GameEvent* posted=nullptr;
    EventCategory category=ECHud;
    unsigned eventId=0;
    unsigned postEventNextUpdate(EventCategory value,unsigned id,void* data) override {
        trace+='N';category=value;eventId=id;posted=static_cast<GameEvent*>(data);return 123;
    }
};
static bool nullLocalFault(ClientVoteManager* votes) {
    __try {votes->sendMutinyIssue();return false;}
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {return true;}
}
static void freeHudStorage(BF2HudManager* hud) {
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();
    hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();
    ::operator delete(hud);
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    setvbuf(stdout,nullptr,_IONBF,0);
    if(cameraRegressionMain())return 1;
    checks=failures=0;trace.clear();trackEvent=false;
    FinishPlayers players;FinishPlayer player,other;
    FinishSquads squads,replacement;Logic logic;Game observedGame;FinishEvents events;
    ResignCamera firstCamera,secondCamera;
    world::playerManager=&players;squadManager=&squads;gameLogic=&logic;game=&observedGame;eventManager=&events;
    auto* hud=new BF2HudManager;hudManager=hud;hud->m_field0f8=nullptr;
    players.reset(nullptr);trace.clear();hud->commanderResign();
    check(trace=="P","resign null local player stops before all other dependencies");
    players.reset(&player);player.team=-3;player.camera=nullptr;
    trace.clear();hud->commanderResign();
    check(trace=="PTRHK"&&squads.resigned==-3&&player.disabled==23,"resign forwards signed team then disables help23 and reads camera");
    player.camera=&firstCamera;firstCamera.current=static_cast<CameraViewMode>(-8);
    trace.clear();hud->commanderResign();
    check(trace=="PTRHKVS","resign camera get precedes set with no commander eligibility query");
    check(firstCamera.received==static_cast<CameraViewMode>(-8)&&!firstCamera.subject&&firstCamera.forced,"resign preserves mode and supplies null object/true");
    check(firstCamera.writes==1,"resign ignores false camera setter result");
    player.onDisable=[&]{player.camera=&secondCamera;players.local=&other;};
    secondCamera.current=static_cast<CameraViewMode>(3);
    secondCamera.onRead=[&]{player.camera=&firstCamera;};
    trace.clear();hud->commanderResign();
    check(trace=="PTRHKVS"&&secondCamera.writes==1&&secondCamera.received==static_cast<CameraViewMode>(3),"resign reads camera after help callback then retains camera across mode callback");
    player.onDisable=nullptr;secondCamera.onRead=nullptr;players.reset(&player);
    squads.onResign=[&]{squadManager=&replacement;};
    trace.clear();hud->commanderResign();
    check(trace=="PTRHKVS"&&player.disabled==23,"resign retains player across squad callback");
    squads.onResign=nullptr;squadManager=&squads;

    alignas(ClientVoteManager) unsigned char native[sizeof(ClientVoteManager)];
    std::memset(native,0x5a,sizeof(native));
    auto* constructed=new(native) ClientVoteManager;
    check(constructed->m_messagesEnabled&&!constructed->m_messageDisplayed,"vote constructor initializes two separate native bools");
    check(native[2]==0x5a&&native[3]==0x5a,"vote constructor leaves bool padding untouched");
    check(constructed->m_dword04==0&&constructed->m_dword08==254&&constructed->m_disallowedTime==0,"vote constructor initializes active type, item and float cooldown");
    check(constructed->m_string0C.empty()&&constructed->m_string14.empty(),"vote constructor initializes both real strings");
    constructed->~ClientVoteManager();
    ClientVoteManager::release();world::playerManager=nullptr;trace.clear();hud->commanderMutiny();
    auto* votes=ClientVoteManager::getInstance();
    check(votes&&trace.empty(),"HUD mutiny lazily constructs real vote manager before no-manager early return");
    check(ClientVoteManager::getInstance()==votes,"vote singleton reuses its native lifetime");
    check(!votes->sendMutinyIssue()&&trace.empty(),"missing player manager denies with no callbacks");
    world::playerManager=&players;players.reset(&player);player.participates=false;
    trace.clear();check(!votes->sendMutinyIssue()&&trace=="PB","nonparticipant denial stops after first local player");
    player.participates=true;player.onParticipates=[&]{world::playerManager=nullptr;};
    trace.clear();check(!votes->sendMutinyIssue()&&trace=="PB","vote path rechecks manager after participation callback");
    player.onParticipates=nullptr;world::playerManager=&players;
    votes->m_dword04=3;trace.clear();
    check(!votes->sendMutinyIssue()&&trace=="PB","active vote denies after participant query");
    votes->m_dword04=0;
    for(float cooldown : {0.01f,1.0f,std::numeric_limits<float>::infinity()}) {
        votes->m_disallowedTime=cooldown;trace.clear();const int before=displayCalls;
        bool boundary=false;try {hud->commanderMutiny();}catch(const DisplayBoundary&){boundary=true;}
        check(boundary&&displayCalls==before+1&&displayCode==0&&!displayForced&&trace=="PBM","positive cooldown reaches exact declaration-only display boundary before event allocation");
    }
    player.id=19;player.team=2;other.id=260;other.team=-7;
    for(float cooldown : {0.0f,-1.0f,-std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()}) {
        votes->m_disallowedTime=cooldown;players.reset(&player);
        players.sequence={&player,&other,&other,&player};logic.server=false;
        observedGame.sent=nullptr;trace.clear();
        check(votes->sendMutinyIssue(),"zero negative and unordered cooldown permit actual vote issue");
        auto* event=static_cast<VoteEvent*>(observedGame.sent);
        check(trace=="PBPTCPDLPDG","mutiny reloads local player for team, caster and transport ID");
        check(squads.teamForCommander==-7&&observedGame.target==19,"mutiny uses second local team and fourth local ID");
        check(event&&event->m_voteType==3&&event->m_voteStatus==0&&event->m_voteItem==42&&event->m_voteCaster==4,"actual vote payload truncates caster while preserving issue kind");
        check(event->getType()==39&&event->getRef()==1,"actual VoteEvent base and virtual type are active");
        event->release();observedGame.sent=nullptr;
    }
    votes->m_disallowedTime=0;players.reset(&player);logic.server=true;squads.commander=-1;
    trace.clear();check(votes->sendMutinyIssue(),"server branch accepts real vote event");
    auto* queued=static_cast<VoteEvent*>(events.posted);
    check(trace=="PBPTCPDLN"&&events.category==ECGameLogic&&events.eventId==15,"server branch queues category0 event15 without extra local-player read");
    check(queued&&queued->m_voteItem==255&&queued->m_voteCaster==19,"minus-one commander retains VoteEvent sentinel mapping");
    queued->release();events.posted=nullptr;
    players.reset(nullptr);trace.clear();
    check(nullLocalFault(votes)&&trace=="P","retail participation path still faults on missing local player");
    ClientVoteManager::release();freeHudStorage(hud);hudManager=nullptr;
    world::playerManager=nullptr;gameLogic=nullptr;game=nullptr;squadManager=nullptr;eventManager=nullptr;
    std::printf("HUD commander finish: %u/%u checks passed (ClientVoteManager size %u)\n",checks-failures,checks,unsigned(sizeof(ClientVoteManager)));
    return failures?1:0;
}
