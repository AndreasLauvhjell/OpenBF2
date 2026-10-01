// Test scaffolding. [CHOSEN, not recovered] Native stock-object collision regression.
#include <cstdio>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include "../../BF2/Game/GameLogic/GameLogicCollision.h"
#include "../../BF2/Game/Objects/GameLogicComp/ArmorGLComp.h"
#include "../../BF2/Game/Objects/GameLogicComp/ArmorGLCompTemplate.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Game/Objects/IObject.h"
#include "../../BF2/Game/Objects/IObjectTemplate.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Physics/MaterialManager.h"
#include "../../BF2/Physics/ResponsePhysics.h"
#include "../../BF2/Physics/IPhysicsNode.h"
#include "../../BF2/Physics/ICollisionMesh.h"
#include "../../BF2/Math/LocalBoundingBox.h"
#include "../../BF2/Game/Utility/Vars.h"
#include "../../BF2/Core/Ids.h"
using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe { namespace world { extern IBase* materialManager; } } }
namespace {
int checks=0, failures=0;
IObject* pair[2]={0,0};
Mat4 savedTransform[2];
float savedGravity[2], savedHealth[2];
int physicalContacts=0;
void check(bool ok, const char* name) {
    ++checks; if(!ok) ++failures;
    std::printf("[collision] %s %s\n",ok?"PASS":"FAIL",name);
}
bool near(float a,float b) { return std::isfinite(a) && std::fabs(a-b)<0.003f; }
class PushProbe : public ResponsePhysics {
public:
    void adjust(float x) { m_positionalAdjusts=Vec3{x,0,0}; }
    float boost() const { return m_hardPush; }
};
void exercise() {
    IObject* self=0; IObject* other=0;
    std::map<const IObjectTemplate*,IObject*> candidates;
    for(const auto& entry:objectManager->getAllRegisteredObjects()) {
        IObject* o=entry.second;
        if(!o || o->getParent() || !o->getTemplate()) continue;
        std::string name=o->getTemplate()->getName();
        std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
        if(name!="usjep_hmmwv" && name.find("jep_")!=0 && name.find("jeep_")!=0) continue;
        if(!o->queryComponent(IID_IArmorGLComp,IID_IArmorGLComp)) continue;
        auto previous=candidates.find(o->getTemplate());
        if(previous!=candidates.end()) { self=previous->second; other=o; break; }
        candidates[o->getTemplate()]=o;
    }
    check(self && other,"two live stock jeeps of identical mass");
    if(!self || !other) return;
    pair[0]=self; pair[1]=other;
    auto armor=static_cast<ArmorGLComp*>(object_detail::LayoutAccess::armorComponent(self));
    auto t=armor->getTemplateForSpawner();
    auto node=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(self));
    const float hp=armor->getHitPoints();
    const float speedMod=t->getSpeedMod();
    const int own=armor->getDefaultMaterial();
    const int surface=4;
    const float material=static_cast<MaterialManager*>(materialManager)->getDamageMod(surface,own);
    const float mass=getMassMod(other,self);
    const Vec3 zero={0,0,0}, head={0,0,1};
    std::printf("[collision] template=%s HP=%g own=%d other=%d mat=%g mass=%g speedMod=%g\n",
                self->getTemplate()->getName().c_str(),hp,own,surface,material,mass,speedMod);
    check(near(mass,1.0f),"equal vehicle masses give unit mass multiplier");
    check(!armor->isInvunerable(),"spawn protection expired");
    check(material>0,"stock material pair damages light vehicle armor");
    check(Vars::queryFloat("collision-impact-speed-limit")==4.0f,"retail 4 m/s threshold");
    check(Vars::queryInt("collision-use-cubic-speedmod")==0,"quadratic stock default");
    check(speedMod==1.0f,"stock armor speed modifier is one");
    struct Case { const char* name; float speed; Vec3 normal; float mod; int cubic; float golden; };
    const Case cases[]={
        {"3 m/s bump",3,head,1,0,0}, {"4 m/s threshold",4,head,1,0,0},
        {"6 m/s head-on",6,head,1,0,4}, {"8 m/s head-on",8,head,1,0,16},
        {"8 m/s oblique cosine .8",8,{0.6f,0,0.8f},1,0,10.24f},
        {"8 m/s tangential",8,{1,0,0},1,0,0},
        {"quarter speed modifier",8,head,0.25f,0,4},
        {"zero speed modifier",8,head,0,0,0},
        {"double speed modifier",8,head,2,0,32},
        {"cubic excess speed",8,head,1,1,64},
        {"cubic oblique quarter modifier",8,{0.6f,0,0.8f},0.25f,1,10.24f}
    };
    for(const Case& c:cases) {
        armor->setHitPoints(hp);
        t->setSpeedMod(c.mod);
        *Vars::getInt("collision-use-cubic-speedmod",0)=c.cubic;
        const Vec3 speed={0,0,c.speed};
        GameLogicCollision::serverHandleCollisionObjectVsObject(other,self,speed,c.normal,zero,surface,own);
        const float lost=hp-armor->getHitPoints(), expected=c.golden*material;
        std::printf("[collision] case=%s expected=%g actual=%g\n",c.name,expected,lost);
        check(near(lost,expected),c.name);
    }
    *Vars::getInt("collision-use-cubic-speedmod",0)=0;
    t->setSpeedMod(speedMod);
    armor->setHitPoints(hp);
    // Real SimpleObject -> ArmorGLComp -> ServerGameLogic -> health application.
    const Vec3 speed={0,0,8};
    self->handleCollision(other,speed,head,zero,own,surface,0,0);
    const float first=armor->getHitPoints();
    check(near(hp-first,16*material),"first contact traverses native damage pipeline");
    for(int i=0;i<60;++i) self->handleCollision(other,speed,head,zero,own,surface,0,0);
    check(armor->getHitPoints()==first,"60 duplicate contacts do not multiply damage");
    armor->handleUpdate(0.5f);
    self->handleCollision(other,speed,head,zero,own,surface,0,0);
    check(armor->getHitPoints()==first,"same pair blocked after half a second");
    armor->handleUpdate(0.5f);
    self->handleCollision(other,speed,head,zero,own,surface,0,0);
    check(armor->getHitPoints()==first,"same pair blocked at exactly one second");
    armor->handleUpdate(0.001f);
    self->handleCollision(other,speed,head,zero,own,surface,0,0);
    check(near(first-armor->getHitPoints(),16*material),"same pair reopens strictly after one second");
    check(armor->collision(own+1000,surface,other),"different own material has independent contact key");
    check(armor->collision(own,surface+1000,other),"different surface material has independent contact key");
    check(armor->collision(own,surface,self),"different other object has independent contact key");
    armor->setHitPoints(hp);
    const Vec3 position=self->getAbsolutePosition();
    {
        PushProbe response;
        response.setObject(self);
        for(int i=0;i<8;++i) { response.adjust(0.4f); response.solveImpulse(node); }
        check(response.boost()==0,"sub-half-metre depenetration does not amplify speed");
        response.adjust(0.6f); response.solveImpulse(node);
        check(response.boost()==0,"first deep push has no speed amplification");
        response.adjust(0.6f); response.solveImpulse(node);
        check(response.boost()==0,"second deep push has no speed amplification");
        response.adjust(0.6f); response.solveImpulse(node);
        check(near(response.boost(),10),"third .6m push adds retail 10x collision-speed boost");
        response.solveImpulse(node);
        check(response.boost()==0,"boost clears on an unpushed step");
    }
    self->setAbsolutePosition(position);
    armor->setHitPoints(hp);
}
void startBump() {
    if(!pair[0] || !pair[1]) return;
    auto mesh=static_cast<ICollisionMesh*>(object_detail::LayoutAccess::collisionMesh(pair[0]));
    const LocalBoundingBox* bounds=mesh->getBoundingBox(1);
    const float gap=bounds->m_max.z-bounds->m_min.z+0.1f;
    std::printf("[collision] physical hull minZ=%g maxZ=%g initial separation=%g speed=2\n",
                bounds->m_min.z,bounds->m_max.z,gap);
    for(int i=0;i<2;++i) {
        auto node=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(pair[i]));
        auto armor=static_cast<ArmorGLComp*>(object_detail::LayoutAccess::armorComponent(pair[i]));
        savedTransform[i]=pair[i]->getAbsoluteTransformation();
        savedGravity[i]=node->getGravityModifier();
        savedHealth[i]=armor->getHitPoints();
        Mat4 transform=Mat4::getIdentity();
        transform.m[12]=500; transform.m[13]=500; transform.m[14]=500+(i?gap:0);
        pair[i]->setAbsoluteTransformation(transform);
        node->setGravityModifier(0);
        node->setRotationalSpeed(Vec3{0,0,0});
        node->setPositionalSpeed(Vec3{0,0,i?0.0f:2.0f});
        node->setIsAwake();
    }
}
void observeBump(int frame) {
    if(!pair[0] || !pair[1]) return;
    for(int i=0;i<2;++i) {
        auto node=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(pair[i]));
        auto armor=static_cast<ArmorGLComp*>(object_detail::LayoutAccess::armorComponent(pair[i]));
        const Vec3 pos=pair[i]->getAbsolutePosition(), speed=node->getPositionalSpeed();
        if(frame%10==0) std::printf("[collision] physical frame=%d vehicle=%d HP=%g z=%g v=%g/%g/%g\n",
                                   frame,i,armor->getHitPoints(),pos.z,speed.x,speed.y,speed.z);
        if(frame==400) {
            check(near(armor->getHitPoints(),savedHealth[i]),"physical 2 m/s vehicle bump preserves health");
            pair[i]->setAbsoluteTransformation(savedTransform[i]);
            node->setGravityModifier(savedGravity[i]);
            node->setPositionalSpeed(Vec3{0,0,0});
            node->setRotationalSpeed(Vec3{0,0,0});
        }
    }
    if(frame==400) check(physicalContacts>0,"native physics reports contacts between the two vehicles");
}
}
void collisionContact(IObject* self,IObject* other,const Vec3& speed) {
    if(!pair[0] || !pair[1] || !other) return;
    IObject* a=getRootParent(self); IObject* b=getRootParent(other);
    if((a==pair[0] && b==pair[1]) || (a==pair[1] && b==pair[0])) {
        ++physicalContacts;
        if(physicalContacts<=12) std::printf("[collision] native contact speed=%g/%g/%g\n",speed.x,speed.y,speed.z);
    }
}
void collisionFrame(int frame) {
    if(frame==300) exercise();
    if(frame==301) { physicalContacts=0; startBump(); }
    if(frame>=302 && frame<=400) observeBump(frame);
}
bool collisionResults() {
    std::printf("[collision] RESULT checks=%d failures=%d\n",checks,failures);
    return checks>20 && failures==0;
}
