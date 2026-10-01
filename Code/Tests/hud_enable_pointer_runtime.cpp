// [TEMP][CHOSEN, not recovered] Actual enablePointer with native HUD/Ref2/Pointer.
// Ammo uses its actual constructor/destructor. Picture dispatch observes an
// undereferenced identity token; no picture or MovePointer descriptor is supplied.
// [RETIRE-WHEN: HUD-ENABLE-POINTER-FIXTURE] Replace storage/dispatch observers when
// the complete picture, MovePointer and HUD lifetimes cover these branches.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <vector>
#include <list>
#include <map>
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudAmmo.h"
#include "../BF2/Menu/Ref2System.h"
#include "../BF2/Menu/Bf2PictureNode.h"
#include "../BF2/External/BFMeme/Pointer.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#undef protected
#undef private
#include <cmath>
#include <cfloat>
#include <climits>
#include "../BF2/Core/EventManager.h"
using namespace dice::hfe;
using namespace dice::hfe::hud;
namespace meme=dice::meme;
using meme::Pointer;
namespace dice { namespace hfe {
class ClassManager;class System;
ClassManager* classManager=nullptr;System* g_system=nullptr;
EventManager* eventManager=nullptr;
hud::BF2HudManager* hudManager=nullptr;
} }
static unsigned checks,failures;
static std::vector<std::string> trace;
static void check(bool pass,const char* label) {
    ++checks;if(!pass){++failures;std::printf("FAIL %s\n",label);}
}
static unsigned textureCalls;
static meme::Bf2PictureNode* expectedGraphic;
static int expectedTexture;
static std::function<void()> onTexture;
namespace dice { namespace meme {
bool dispatchPictureTexture(Bf2PictureNode* picture,int texture) {
    ++textureCalls;trace.push_back("texture");
    check(picture==expectedGraphic&&texture==expectedTexture,"texture seam forwards exact live identity and signed handle");
    if(onTexture)onTexture();
    return false; // Verify the actual caller ignores the real hook's result.
}
} }
struct ObservedPointer : Pointer {
    std::function<void()> onType;
    unsigned types=0;
    meme::TypeDescriptor* getType() override {
        ++types;trace.push_back("type");if(onType)onType();
        return Pointer::getClassType();
    }
    float width() const {return m_width;}
    float height() const {return m_height;}
};
static bool samePointerFloat(float lhs,float rhs) {
    return (std::isnan(lhs)&&std::isnan(rhs))||std::memcmp(&lhs,&rhs,4)==0;
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);setvbuf(stdout,nullptr,_IONBF,0);
    static_assert(sizeof(void*)==4&&sizeof(BF2HudManager)==824,"native x86 HUD");
    static_assert(sizeof(meme::PictureNode)==12&&sizeof(meme::Bf2PictureNode)==96,"native unpublished picture storage");
    EventManager events;eventManager=&events;
    BF2HudManager* hud=new BF2HudManager;hudManager=hud;
    auto* ammo=new HudAmmo(nullptr);hud->m_field12c=ammo;
    menu::Ref2System first(800,600),second(1000,700);
    ObservedPointer pointer,replacement;
    first.getPointerObject()=&pointer;second.getPointerObject()=&replacement;
    int token1=0,token2=0;
    auto* graphic1=reinterpret_cast<meme::Bf2PictureNode*>(&token1);
    auto* graphic2=reinterpret_cast<meme::Bf2PictureNode*>(&token2);
    hud->m_field0f4=nullptr;hud->m_field15c=false;
    hud->enablePointer(INT_MIN,28);
    check(hud->m_field15c&&textureCalls==0,"null System still enables HUD latch and reaches no descriptor");
    hud->m_field0f4=&first;hud->m_field258=graphic1;
    hud->m_field154=12.5f;hud->m_field158=-4.25f;
    hud->m_field1f8=470;hud->m_field1fc=330;
    hud->m_field210=525;hud->m_field214=380;
    expectedGraphic=graphic1;expectedTexture=INT_MIN;
    first.getPointerPosition()[0]=91;first.getPointerPosition()[1]=92;
    trace.clear();hud->enablePointer(expectedTexture,0);
    check(trace==std::vector<std::string>{"type","texture"},"descriptor precedes texture and no replacement occurs for exact Pointer identity");
    check(pointer.isEnabled()&&pointer.width()==12.5f&&pointer.height()==-4.25f,"false texture result still enables pointer and copies default dimensions");
    check(first.getPointerPosition()[0]==91&&first.getPointerPosition()[1]==92,"ordinary mode leaves position unchanged");
    check(hud->m_field258==graphic1&&first.getPointerObject()==&pointer,"existing ordinary pointer and graphic identities preserved");
    ammo->m_field0a4=true;ammo->m_field0a0=-17;
    for(int mode:{0,10,3,6,4,5,7,8,30,31,2,27,29,INT_MIN,INT_MAX}) {
        first.getPointerPosition()[0]=91;first.getPointerPosition()[1]=92;
        hud->enablePointer(expectedTexture,mode);
        const bool sized=mode==0||mode==10;
        check(pointer.width()==(sized?-17.0f:12.5f)&&pointer.height()==(sized?-17.0f:-4.25f),"Ammo size applies only for modes0/10; no negative clamp");
        float x=91,y=92;
        if(mode==4||mode==5||mode==3||mode==7||mode==8||mode==30||mode==31){x=70;y=30;}
        else if(mode==6){x=125;y=80;}
        check(first.getPointerPosition()[0]==x&&first.getPointerPosition()[1]==y,"exact recenter mode set and coordinate source");
    }
    ammo->m_field0a0=INT_MAX;hud->enablePointer(expectedTexture,10);
    check(pointer.width()==2147483648.0f&&pointer.height()==2147483648.0f,"signed Ammo integer converts once to equal rounded floats");
    ammo->m_field0a4=false;
    hud->m_field154=-0.0f;hud->m_field158=std::numeric_limits<float>::quiet_NaN();
    hud->enablePointer(expectedTexture,0);
    check(samePointerFloat(pointer.width(),-0.0f)&&std::isnan(pointer.height()),"default dimension copies retain signed zero and NaN");
    // Recenter subtraction retains half of a subnormal until its final store.
    unsigned previousControl,ignoredControl;
    _controlfp_s(&previousControl,0,0);_controlfp_s(&ignoredControl,_PC_64|_RC_NEAR,_MCW_PC|_MCW_RC);
    const float subnormal=std::numeric_limits<float>::denorm_min();
    first.m_width=first.m_height=subnormal;hud->m_field1f8=hud->m_field1fc=subnormal;
    hud->enablePointer(expectedTexture,3);
    check(first.getPointerPosition()[0]==0&&first.getPointerPosition()[1]==0,"half-subnormal is retained before subtraction and round-to-even store");
    _controlfp_s(&ignoredControl,previousControl,_MCW_PC|_MCW_RC);
    first.m_width=800;first.m_height=600;hud->m_field1f8=470;hud->m_field1fc=330;
    // The type callback changes the live System and graphic before later reads.
    pointer.onType=[&]{hud->m_field0f4=&second;hud->m_field258=graphic2;};
    expectedGraphic=graphic2;expectedTexture=INT_MAX;
    hud->m_field154=22;hud->m_field158=23;
    onTexture=[&]{hud->m_field15c=false;hud->m_field154=24;hud->m_field158=25;};
    trace.clear();hud->enablePointer(expectedTexture,4);
    check(trace==std::vector<std::string>{"type","texture"},"type callback changes live receivers without extra type dispatch");
    check(!replacement.isEnabled()&&replacement.width()==24&&replacement.height()==25,"texture callback changes enabled latch and sizes before reload");
    check(second.getPointerPosition()[0]==-30&&second.getPointerPosition()[1]==-20,"recenter reads replacement System dimensions and position storage");
    pointer.onType=nullptr;onTexture=nullptr;
    hud->m_field258=nullptr;expectedGraphic=nullptr;
    hud->enablePointer(expectedTexture,0);
    check(textureCalls>0&&replacement.isEnabled(),"caller adds no graphic-null guard before the explicit dispatch frontier");
    std::printf("HUD enablePointer: %u/%u checks passed\n",checks-failures,checks);
    hud->m_field12c=nullptr;
    delete ammo; // Actual destructor removes both subscriptions before EventManager dies.
    // HUD remains a pre-init fixture owner. Ref2/Pointer lifetimes end normally;
    // picture replacement and mode28 are deliberately uncalled.
    return failures?1:0;
}
