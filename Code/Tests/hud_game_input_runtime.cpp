// [TEMP] Actual HUD input, Ref2 snapshot copy and Pointer movement bodies.
// Only typed collaborator answers/callbacks are observed. Graphic+258 is a
// non-dereferenced presence word; no picture/Node construction is fabricated.
// [RETIRE-WHEN: HUD-GAME-INPUT-FIXTURE] Real HUD/picture/device lifetime covers this path.
#include <deque>
#include <functional>
#include <cmath>
#include <cfloat>
#include <climits>
#include <cstdlib>
#include <xmmintrin.h>
#define main ref2InputRegressionMain
#include "ref2_input_runtime.cpp"
#undef main
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#undef protected
#undef private
namespace dice { namespace hfe {
hud::BF2HudManager* hudManager=nullptr;
namespace io { IBase* inputDeviceManager=nullptr; }
} }
using hud::BF2HudManager;
static std::string inputTrace;
[[noreturn]] static void unsupportedInput() {
    std::fprintf(stderr,"[FRONTIER] HUD input fixture unexpected collaborator call\n");
    std::fflush(stderr);std::abort();
}
struct InputDevice : io::IInputDevice {
    bool active=false;
    std::function<void()> callback;
    void addRef() override {unsupportedInput();}
    int getRef() override {unsupportedInput();}
    int release() override {unsupportedInput();}
    void* queryInterface(unsigned) const override {unsupportedInput();}
    io::InputDeviceTypes getType() const override {unsupportedInput();}
    bool setCooperativeMode(io::InputDeviceMode) override {unsupportedInput();}
    io::InputDeviceMode getCooperativeMode() const override {unsupportedInput();}
    bool activate() override {unsupportedInput();}
    bool deActivate() override {unsupportedInput();}
    void update(float) override {unsupportedInput();}
    bool isActive() const override {inputTrace+='A';if(callback)callback();return active;}
    bool getNextInput(io::InputDeviceAxes&,io::InputDeviceButtons&,io::InputDeviceKeys&,bool&) override {unsupportedInput();}
};
struct Devices : io::IInputDeviceManager {
    mutable unsigned calls=0;
    InputDevice* first=nullptr;
    InputDevice* second=nullptr;
    std::function<void(unsigned)> callback;
    void addRef() override {unsupportedInput();}
    int getRef() override {unsupportedInput();}
    int release() override {unsupportedInput();}
    void* queryInterface(unsigned) const override {unsupportedInput();}
    bool initDevices(unsigned*) override {unsupportedInput();}
    io::IInputDevice* getInputDevice(io::InputDeviceFlags flags) const override {
        inputTrace+='D';check(flags==io::IDF_FALCON,"exact Falcon device flag");
        const unsigned index=++calls;if(callback)callback(index);
        return index==1?first:second;
    }
    unsigned getInputDevices() const override {unsupportedInput();}
    bool activateDevices(unsigned) override {unsupportedInput();}
    bool deActivateDevices(unsigned) override {unsupportedInput();}
    void update(float) override {unsupportedInput();}
    unsigned getActiveInputDevices() const override {unsupportedInput();}
    void* getAllDevices() override {unsupportedInput();}
};
struct InputHud : BF2HudManager {
    std::deque<int> modes;
    unsigned modeCalls=0;
    int fallback=0;
    std::function<void(unsigned)> callback;
    int getMode() override {
        inputTrace+='M';++modeCalls;if(callback)callback(modeCalls);
        if(modes.empty())return fallback;
        const int result=modes.front();modes.pop_front();return result;
    }
};
struct InputSystem : Ref2System {
    InputSystem(float width,float height):Ref2System(width,height){}
    unsigned changes=0;
    std::function<void()> callback;
    bool pointerPositionChanged(float* x,float* y) override {
        inputTrace+='C';++changes;
        check((*x==m_pointerX||std::isnan(*x))&&(*y==m_pointerY||std::isnan(*y)),"real Pointer stores before callback");
        if(callback)callback();*x=9999;*y=-9999;return false;
    }
};
static unsigned short inputControl() {
    unsigned short controlWord;
    __asm { fnstcw controlWord }
    return controlWord;
}
static void inputControl(unsigned short controlWord) { __asm { fldcw controlWord } }
static bool sameInputFloat(float a,float b) {
    return (std::isnan(a)&&std::isnan(b))||std::memcmp(&a,&b,4)==0;
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);setvbuf(stdout,nullptr,_IONBF,0);
    if(ref2InputRegressionMain())return 1;
    const unsigned regressionChecks=checks;
    InputSystem ui(800,600),replacement(1000,700);
    Pointer pointer,secondPointer;pointer.setEnabled(true);secondPointer.setEnabled(true);
    ui.getPointerObject()=&pointer;replacement.getPointerObject()=&secondPointer;
    InputHud* h=new InputHud;
    hudManager=h;
    InputDevice device,otherDevice;Devices devices,otherDevices;
    io::inputDeviceManager=&devices;
    io::GameInput input;
    std::memset(&input,0xA5,sizeof(input));input.mappingFlags=0;
    std::memset(input.inputs,0,sizeof(input.inputs));
    auto reset=[&]() {
        h->m_field0f4=&ui;h->m_field258=reinterpret_cast<dice::meme::Bf2PictureNode*>(1);h->m_field180=40;h->m_field17c=20;
        h->m_field200=200;h->m_field204=100;h->m_field208=450;h->m_field20c=320;
        h->m_field218=300;h->m_field21c=150;h->m_field220=350;h->m_field224=270;
        h->modeCalls=0;h->fallback=0;h->modes.clear();h->callback=nullptr;
        input.mappingFlags=(1ULL<<20)|(1ULL<<21);input.inputs[20]=0.5f;input.inputs[21]=-0.25f;
        ui.m_pointerX=0;ui.m_pointerY=0;ui.changes=0;ui.callback=nullptr;ui.getPointerObject()=&pointer;
        replacement.m_pointerX=0;replacement.m_pointerY=0;replacement.changes=0;replacement.callback=nullptr;
        devices.calls=0;devices.first=nullptr;devices.second=nullptr;devices.callback=nullptr;
        otherDevices.calls=0;otherDevices.first=&otherDevice;otherDevices.second=&otherDevice;otherDevices.callback=nullptr;
        device.active=false;device.callback=nullptr;otherDevice.active=true;otherDevice.callback=nullptr;
        pointer.setEnabled(true);io::inputDeviceManager=&devices;inputTrace.clear();
    };
    reset();h->m_field0f4=nullptr;h->handleGameInput(1,input);
    check(inputTrace.empty(),"no UI returns before mode or device reads");
    reset();h->m_field258=0;h->handleGameInput(1,input);
    check(inputTrace.empty()&&std::memcmp(&input,&ui.m_inputs[0],320)==0,"snapshot copied in full before absent-graphic guard");
    io::GameInput banks[4];std::memcpy(banks,ui.m_inputs,sizeof(banks));
    ui.setInput(input,-1);ui.setInput(input,4);ui.setInput(input,INT_MIN);ui.setInput(input,INT_MAX);
    check(std::memcmp(banks,ui.m_inputs,sizeof(banks))==0,"snapshot user guard rejects both signed extremes");
    for(int user=0;user<4;++user){
        input.dword0=user+33;ui.setInput(input,user);
        check(std::memcmp(&input,&ui.m_inputs[user],320)==0,"each native 320-byte snapshot bank copies padding");
    }
    reset();h->handleGameInput(std::numeric_limits<float>::quiet_NaN(),input);
    check(ui.m_pointerX==20&&ui.m_pointerY==-10&&inputTrace=="MMMMMMMMDC","relative movement ignores delta and uses default signed sensitivity");
    check(ui.changes==1&&std::memcmp(&input,&ui.m_inputs[0],320)==0,"real Pointer callback rejection and local edits ignored");
    reset();h->m_field180=-40;h->handleGameInput(0,input);
    check(ui.m_pointerX==-20&&ui.m_pointerY==10,"negative sensitivity reverses relative input");
    reset();h->m_field180=0;ui.m_pointerX=23;ui.m_pointerY=-17;h->handleGameInput(0,input);
    check(ui.m_pointerX==23&&ui.m_pointerY==-17,"zero sensitivity retains the previous position");
    for(unsigned mask=0;mask<4;++mask){
        reset();input.mappingFlags=(mask&1?1ULL<<20:0)|(mask&2?1ULL<<21:0)|(1ULL<<63);
        h->handleGameInput(0,input);
        check(ui.m_pointerX==(mask&1?20.0f:0.0f)&&ui.m_pointerY==(mask&2?-10.0f:0.0f),"each axis obeys its low mapping bit independently");
    }
    const int restricted[]={3,4,5,7,8,30,31,6};
    for(unsigned i=0;i<8;++i){
        reset();h->fallback=restricted[i];h->handleGameInput(100,input);
        check(h->modeCalls==i+1,"mode chain preserves comparison order and short circuit");
        check(ui.m_pointerX==(i==7?10.0f:50.0f)&&ui.m_pointerY==(i==7?-5.0f:20.0f),"mode-specific origins extents and sensitivity");
    }
    reset();h->modes={0,4};h->callback=[&](unsigned n){if(n==1){check(std::memcmp(&input,&ui.m_inputs[0],320)==0,"snapshot precedes first mode callback");input.inputs[20]=1000;}};
    h->handleGameInput(0,input);
    check(h->modeCalls==2&&ui.m_pointerX==50,"changing mode answers and pre-callback axis snapshot retained");
    reset();devices.first=&device;devices.second=&otherDevice;h->handleGameInput(0,input);
    check(inputTrace=="MMMMMMMMDDAC","second device lookup supplies isActive receiver");
    check(ui.m_pointerX==200&&ui.m_pointerY==-75,"active Falcon uses absolute half extents");
    reset();devices.first=&device;devices.second=&device;h->handleGameInput(0,input);
    check(ui.m_pointerX==20&&ui.m_pointerY==-10&&devices.calls==2,"inactive Falcon retains relative movement");
    reset();devices.first=&device;devices.callback=[&](unsigned){io::inputDeviceManager=&otherDevices;};h->handleGameInput(0,input);
    check(devices.calls==1&&otherDevices.calls==1&&ui.m_pointerX==200,"device manager global reloads between lookups");
    reset();devices.first=&device;devices.second=&device;device.callback=[&](){h->m_field0f4=&replacement;};
    h->handleGameInput(0,input);
    check(ui.m_pointerX==20&&ui.m_pointerY==-10&&replacement.m_pointerX==20&&replacement.m_pointerY==-10,"captured position receives movement then current UI receives Pointer dispatch");
    check(ui.changes==0&&replacement.changes==1,"last UI and Pointer are reloaded after device callbacks");
    reset();pointer.setEnabled(false);h->handleGameInput(0,input);
    check(ui.m_pointerX==20&&ui.m_pointerY==-10&&ui.changes==0,"disabled Pointer still receives direct coordinate movement but no callback");
    reset();ui.getPointerObject()=nullptr;h->handleGameInput(0,input);
    check(ui.m_pointerX==20&&ui.changes==0,"null Pointer uses real wrapper guard after movement");
    reset();input.inputs[20]=100;input.inputs[21]=-100;h->handleGameInput(0,input);
    check(ui.m_pointerX==400&&ui.m_pointerY==-300,"default rectangle clamps upper X and lower Y");
    reset();h->fallback=3;h->m_field200=-100;h->m_field204=-100;ui.m_pointerX=80;ui.m_pointerY=0;
    input.mappingFlags=0;h->handleGameInput(0,input);
    check(ui.m_pointerX==-50&&ui.m_pointerY==20,"negative extents preserve lower-first asymmetric clipping");
    reset();input.inputs[20]=std::numeric_limits<float>::quiet_NaN();input.inputs[21]=std::numeric_limits<float>::infinity();h->handleGameInput(0,input);
    check(std::isnan(ui.m_pointerX)&&ui.m_pointerY==300,"unordered coordinate survives while infinity clamps");
    reset();h->fallback=3;h->m_field208=std::numeric_limits<float>::quiet_NaN();h->m_field204=std::numeric_limits<float>::quiet_NaN();
    ui.m_pointerY=40;input.mappingFlags=0;h->handleGameInput(0,input);
    check(ui.m_pointerX==0&&ui.m_pointerY==40,"NaN origin or upper extent leaves ordered coordinates unchanged");
    const unsigned short savedControl=inputControl();const unsigned savedSse=_mm_getcsr();
    for(unsigned rounding=0;rounding<4;++rounding){
        reset();h->fallback=3;h->m_field17c=16777217;h->m_field200=h->m_field204=2.0e9f;
        h->m_field208=h->m_field20c=-1.0e9f;ui.m_pointerX=-16777216.0f;ui.m_pointerY=16777216.0f;
        input.inputs[20]=1;input.inputs[21]=-1;
        inputControl(static_cast<unsigned short>((savedControl&~0xF00u)|0x300u|(rounding<<10)));
        h->handleGameInput(0,input);
        check(ui.m_pointerX==1&&ui.m_pointerY==-1,"signed sensitivity is not rounded to float before cancellation");
        check(inputControl()==static_cast<unsigned short>((savedControl&~0xF00u)|0x300u|(rounding<<10))&&(_mm_getcsr()&~63u)==(savedSse&~63u),"input arithmetic preserves caller control modes");
    }
    inputControl(savedControl);_mm_setcsr(savedSse);
    // HUD destruction still requires unrelated owners. Release only native
    // constructor-owned containers and blink state, matching the existing fixtures.
    delete h->m_field2dc;
    using WS=std::wstring;using S=std::string;using IL=std::list<int>;using SL=std::list<std::string>;using IV=std::vector<std::uint32_t>;
    h->m_field010.~WS();h->m_field038.~WS();h->m_field060.~WS();h->m_field08c.~S();h->m_field0c0.~S();h->m_field304.~S();
    h->m_field0e8.~IL();h->m_field2e0.~SL();h->m_field33c.~SL();h->m_field32c.~IV();
    h->modes.~deque();h->callback.~function();::operator delete(h);
    hudManager=nullptr;io::inputDeviceManager=nullptr;ui.getPointerObject()=nullptr;replacement.getPointerObject()=nullptr;
    std::printf("HUD game input: %u/%u checks passed; Ref2 regression %u checks\n",checks-regressionChecks-failures,checks-regressionChecks,regressionChecks);
    return failures?1:0;
}
