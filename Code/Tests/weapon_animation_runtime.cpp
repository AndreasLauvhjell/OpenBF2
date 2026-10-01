// [TEMP] [CHOSEN, not recovered] strict observers for production weapon animation bodies.
// No game, animation assets, renderer device, or server is created.
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <list>
#define private public
#define protected public
#include "../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../BF2/Game/Objects/Weapons/AnimationComponents/DefaultAnimationComp.h"
#include "../BF2/Game/Objects/Weapons/AnimationComponents/ThrownAnimationComp.h"
#include "../BF2/Game/Objects/Weapons/AmmoComponents/AmmoComp.h"
#undef protected
#undef private
#include "../BF2/Animation/BoneAnimation/AnimationSystem.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/Core/System.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <limits>
using namespace dice;
using namespace dice::hfe;
using namespace dice::hfe::world;
static_assert(sizeof(void*)==4,"x86 only");
namespace {
int checks,failed,baseGeometryCalls,geometryCalls,diagnostics;
const Mat4* publishedTransform;
std::string order;
void check(bool value,const char* description)
{ ++checks; if(!value){++failed;std::printf("FAIL %s\n",description);} }
struct SystemObserver : anim::IAnimationSystem {
    std::string events;
    unsigned int persistent=0,once=0;
    void clear(){events.clear();persistent=once=0;}
    anim::AnimationSystemTemplate* getTemplate(){return 0;}
    void reset(){} void update(IObject*,float){}
    void applyOnSkeleton(anim::Skeleton*,bool){}
    void disable(bool b){events+=b?"D1;":"D0;";}
    bool playTrigger(const std::string&,bool){return false;}
    bool playPostTrigger(const std::string&,bool){return false;}
    void setActiveTrigger(const std::string&,bool){}
    void copyStateToSystem(anim::IAnimationSystem*,bool){}
    void setTriggerPose(unsigned int){}
    void setTriggerMessage(unsigned int m){persistent|=m;events+="S"+std::to_string(m)+";";}
    void setTriggerOnceMessage(unsigned int m){order+='N';once|=m;events+="O"+std::to_string(m)+";";}
    void clearTriggerMessage(unsigned int m){persistent&=~m;events+="C"+std::to_string(m)+";";}
    void setTriggerMovement(float,const Vec3&,float,const Vec3&){}
    void getPlayingAnimationLength(float&,float&,bool){}
    void syncMovementAnimations(const anim::IAnimationSystem*){}
    void setForceTime(float){} float getForceTime(){return 0;}
};
struct BundleObserver : IAnimatedBundle {
    int selected=-1; anim::IAnimationSystem* systems[2];
    void addRef(){} int getRef(){return 1;} int release(){return 1;}
    void* queryInterface(unsigned int)const{return 0;}
    void setActiveAnimationSystem(int index){order+='S';selected=index;}
    void* getActiveAnimationSystem(){order+='G';return systems[selected];}
};
struct Flat { void** table; };
BundleObserver* alternateBundle;
void* __fastcall queryBundle(void*,void*,unsigned int iid)
{check(iid==50334,"alternate object queried for IAnimatedBundle");order+='Q';return alternateBundle;}
void __fastcall publish(void*,void*,const Mat4& transform)
{++geometryCalls;publishedTransform=&transform;}
int reloadState;
void __fastcall startReload(void*,void*){order+='A';}
ReloadState __fastcall getReload(void*,void*){order+='R';return static_cast<ReloadState>(reloadState);}
void __fastcall receiveFire(void*,void*,ReloadState s){check(static_cast<int>(s)==reloadState,"fire reload state");order+='F';}
void __fastcall receiveZoom(void*,void*,ReloadState s){check(static_cast<int>(s)==reloadState,"zoom reload state");order+='Z';}
void __fastcall receiveSound(void*,void*,ReloadState s){check(static_cast<int>(s)==reloadState,"sound reload state");order+='S';}
void __fastcall receiveTarget(void*,void*,ReloadState s){check(static_cast<int>(s)==reloadState,"target reload state");order+='T';}
}
namespace dice { namespace hfe {
// Diagnostic collaborator: collect unexpected error construction without invoking game logging.
Debug::Debug(DebugType type,const std::string& file,int line,const std::string& func)
    :m_type(type),m_file(file),m_func(func),m_line(line){++diagnostics;}
Debug::~Debug(){}
namespace WorldPref { int getTickCount(){return 100;} }
namespace world {
extern const unsigned int IID_IObjectTemplateComponent=50379;
extern const unsigned int IID_IObjectComponent=50043;
void Object::updateGeometry(){++baseGeometryCalls;}
// Required by the component vtable's unused simulation step. Never fabricate its vote.
bool GenericFireArm::busy(bool){std::fprintf(stderr,"Unexpected fixture busy() dependency\n");std::abort();}
}
} }
int main()
{
    alignas(GenericFireArm) unsigned char bytes[sizeof(GenericFireArm)]={};
    GenericFireArm* gun=reinterpret_cast<GenericFireArm*>(bytes);
    void* geomTable[0x54/4]={};geomTable[0x50/4]=reinterpret_cast<void*>(&publish);
    Flat geometry={geomTable};
    gun->GenericFireArm::updateGeometry();
    check(baseGeometryCalls==1 && geometryCalls==0,"null geometry delegates to Object");
    gun->m_geometry=&geometry;
    gun->GenericFireArm::updateGeometry();
    check(baseGeometryCalls==2 && geometryCalls==0,"normal display delegates to Object");
    gun->m_activeDisplayObject=reinterpret_cast<IObject*>(1);
    gun->m_flags=0x40000000u;
    gun->GenericFireArm::updateGeometry();
    check(baseGeometryCalls==2 && geometryCalls==1,"alternate display calls whole-geometry transform");
    check(publishedTransform==&Mat4::getZero(),"geometry keeps stable zero-matrix reference");
    check(gun->m_flags==0x40000000u,"alternate branch preserves dirty flags");
    for(float value:publishedTransform->m)check(value==0,"all sixteen suppression elements zero");
    gun->m_geometry=0;gun->GenericFireArm::updateGeometry();
    check(baseGeometryCalls==3,"alternate display without geometry still delegates");

    DefaultAnimationCompTemplate config;
    DefaultAnimationComp component(&config);
    SystemObserver first,third,alternate;
    gun->m_animationSystems[0]=&first;gun->m_animationSystems[1]=&third;
    gun->m_animationComp=&component;gun->m_defaultAnimationComp=&component;
    gun->m_activeDisplayObject=0;
    gun->GenericFireArm::setActiveAnimationSystem(0);
    check(gun->GenericFireArm::getActiveAnimationSystem()==&first,"first-person system selected");
    WeaponFireEvent event={};event.fireStrength=1;
    component.postFire(event);
    check(first.events=="O1;" && third.events.empty(),"shot routes to selected first-person system");
    gun->GenericFireArm::setActiveAnimationSystem(1);
    component.reload(static_cast<ReloadState>(3));
    check(third.events=="O2;" && first.events=="O1;","reload routes to selected third-person system");
    check(gun->m_activeAnimationSystemIndex==1,"selected index retained");
    BundleObserver bundle;bundle.systems[0]=&alternate;bundle.systems[1]=&third;
    alternateBundle=&bundle;
    void* objectTable[4]={};objectTable[3]=reinterpret_cast<void*>(&queryBundle);Flat object={objectTable};
    gun->m_activeDisplayObject=reinterpret_cast<IObject*>(1);
    gun->m_alternateDisplayObject=reinterpret_cast<IObject*>(&object);
    gun->m_defaultAnimationComp=0;
    order.clear();gun->GenericFireArm::setActiveAnimationSystem(0);
    check(order=="QSG" && bundle.selected==0,"alternate selection preserves query/set/get order");
    component.postFire(event);
    check(alternate.events=="O1;","alternate component receives alternate system");
    order.clear();check(gun->GenericFireArm::getActiveAnimationSystem()==&alternate,"alternate active-system getter");
    check(order=="QG","alternate getter queries then gets system");

    component.setActiveAnimationSystem(&first);first.clear();
    config.setLoopingFire(true);component.postFire(event);
    component.fireStop(true);
    check(first.events=="S1;","alternate-fire stop preserves looping shot");
    component.fireStop(false);check(first.events=="S1;C1;","primary stop clears looping fire");
    config.setLoopingFire(false);
    const char* reloadExpected[]={"C2;","","","O2;","S2;","","C2;"};
    for(int state=0;state!=7;++state){first.clear();component.reload(static_cast<ReloadState>(state));check(first.events==reloadExpected[state],"reload-state message mapping");}
    first.clear();component.updateOnZooming(1);component.updateOnZooming(0);
    check(first.events=="S4;C4;","zoom sets and clears message4");
    config.setAnimateOnZoom(false);first.clear();component.updateOnZooming(1);
    check(first.events.empty(),"authored animateOnZoom flag respected");
    component.disable();component.postFire(event);component.reload(static_cast<ReloadState>(3));
    check(first.events.empty(),"disabled component sends no trigger messages");

    void* ammoTable[0xC4/4]={};ammoTable[0x88/4]=reinterpret_cast<void*>(&startReload);ammoTable[0xC0/4]=reinterpret_cast<void*>(&getReload);
    void* fireTable[0x4C/4]={};fireTable[0x48/4]=reinterpret_cast<void*>(&receiveFire);
    void* zoomTable[0x4C/4]={};zoomTable[0x48/4]=reinterpret_cast<void*>(&receiveZoom);
    void* soundTable[0x4C/4]={};soundTable[0x48/4]=reinterpret_cast<void*>(&receiveSound);
    void* targetTable[0x4C/4]={};targetTable[0x48/4]=reinterpret_cast<void*>(&receiveTarget);
    Flat ammo={ammoTable},fire={fireTable},zoom={zoomTable},sound={soundTable},target={targetTable};
    gun->m_ammoComp=reinterpret_cast<IWeaponAmmoComp*>(&ammo);
    gun->m_fireComp=reinterpret_cast<IWeaponFireComp*>(&fire);
    gun->m_zoomComp=reinterpret_cast<IWeaponZoomComp*>(&zoom);
    gun->m_soundComp=reinterpret_cast<IWeaponSoundComp*>(&sound);
    gun->m_targetComp=reinterpret_cast<IWeaponTargetComp*>(&target);
    component.setActiveAnimationSystem(&first);first.clear();reloadState=3;order.clear();
    gun->GenericFireArm::doReload();
    check(order=="ARFZSNT" && first.events=="O2;","real doReload propagates ammo state in fire/zoom/sound/animation/target order");

    ThrownAnimationCompTemplate thrownConfig;ThrownAnimationComp thrown(&thrownConfig);
    thrown.setActiveAnimationSystem(&first);first.clear();
    thrown.updateOnFiring(static_cast<FireState>(0));
    thrown.updateOnFiring(static_cast<FireState>(2));
    thrown.updateOnFiring(static_cast<FireState>(2));
    check(first.events=="S8;O16;C8;C8;","throw preparation edge emits once and clears held message");
    const float strengths[]={0.0f,0.049f,0.05f,1.0f,std::numeric_limits<float>::quiet_NaN()};
    for(int i=0;i!=5;++i){first.clear();event.fireStrength=strengths[i];thrown.postFire(event);check(first.events==(i<2?"O4;":"O1;"),"throw strength ordered boundary and NaN");}
    first.clear();thrown.reload(static_cast<ReloadState>(0));check(first.events.empty(),"idle reload leaves thrown system alone");
    thrown.reload(static_cast<ReloadState>(3));check(first.events=="D0;O2;","thrown reload enables before reload message");
    first.clear();thrown.updateOnFiring(static_cast<FireState>(0));check(first.events=="S8;","reload resets throw-start latch");
    first.clear();thrown.disable();thrown.postFire(event);check(first.events=="D0;","disable forwards then clears active pointer");
    check(diagnostics==0,"valid events never enter unhandled-state diagnostic");
    std::printf("Weapon animation: %d checks, %d failures (strict x86 production bodies).\n",checks,failed);
    return failed?1:0;
}
