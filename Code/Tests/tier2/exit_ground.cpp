// Test scaffolding. [CHOSEN, not recovered] Real stock-map objects and synthetic remote input, no clients.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <float.h>
#include <map>
#include <vector>
#include <string>
#define private public
#define protected public
#include "../../BF2/Game/Objects/Game/Soldier.h"
#include "../../BF2/Game/Objects/Game/PlayerControlObject.h"
#include "../../BF2/Game/Objects/Game/ForceObject.h"
#include "../../BF2/Game/Objects/ForceObjectTemplate.h"
#include "../../BF2/Game/Objects/PlayerControlObjectTemplate.h"
#include "../../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../../BF2/Game/GameLogic/Player.h"
#undef protected
#undef private
#include "../../BF2/Game/GameServer/GameServer.h"
#include "../../BF2/Game/GameLogic/ServerGameLogic.h"
#include "../../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Physics/IPhysicsNode.h"
#include "../../BF2/Physics/HeightmapCluster.h"
#include "../../BF2/Physics/PhysicsUtil.h"
#include "../../BF2/Game/Objects/GameLogicComp/ArmorGLComp.h"
#include "../../BF2/IO/Input/PlayerInput.h"
using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe { namespace world { extern IBase* heightmapCluster; } } }
static dice::hfe::world::IPlayer* player;
static IObject* vehicle;
static IObject* body;
static bool survived;
static bool exited, deployed, landed, finiteBody=true, finiteRoot=true, intact=true;
static int chuteFrames;
static float lastChuteSpeed;
static bool forceChecks;
static bool finite(const Vec3& v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
static IPhysicsNode* node(IObject* o) { return static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(o)); }
static void report(int frame) {
    const Vec3& p = body->getAbsolutePosition();
    const Vec3& v = node(body)->getPositionalSpeed();
    auto armor = static_cast<ArmorGLComp*>(object_detail::LayoutAccess::armorComponent(body));
    printf("[exit] f=%d alive=%d hp=%g body=%g/%g/%g speed=%g/%g/%g vehicle=%s\n",frame,player->getIsAlive(),armor?armor->getHitPoints():-1,p.x,p.y,p.z,v.x,v.y,v.z,player->getVehicle()?player->getVehicle()->getTemplate()->getName().c_str():"NULL");
    const Vec3& vp=vehicle->getAbsolutePosition(); const Vec3& vv=node(vehicle)->getPositionalSpeed();
    printf("[exit-detail] root=%g/%g/%g velocity=%g/%g/%g parent=%p air=%d wound=%d\n",vp.x,vp.y,vp.z,vv.x,vv.y,vv.z,body->getParent(),node(body)->getIsInAir(),player->getIsManDown());
    fflush(stdout);
}
void exitFrame(int frame) {
    if (frame == 30) {
        const char* name = std::getenv("BF2_EXIT_VEHICLE");
        if (!name) name = "usthe_uh60";
        auto templ = objectTemplateManager->getTemplate(name);
        printf("[exit] template %s = %p\n",name,templ); fflush(stdout);
        if (!templ) std::abort();
        player = static_cast<GameServer*>(game)->createPlayerWithValidName("ExitProbe",1);
        player->setIsRemote(true);
        player->setTeam(1,false);
        player->setKit(3);
        for(int k=0;k<7;++k) for(int s=0;s<2;++s) player->setChosenUnlockLevel(k,s,0);
        for (const auto& entry:objectManager->getAllRegisteredObjects()) {
            auto o=entry.second;
            if(o && !o->getParent() && o->getTemplate()->getName()==name &&
               object_detail::LayoutAccess::armorComponent(o)) { vehicle=o; break; }
        }
        if(!vehicle) std::abort();
        std::pair<Vec3,Vec3> at(vehicle->getAbsolutePosition(),Vec3::getZero());
        static_cast<ServerGameLogic*>(gameLogic)->spawnPlayer(player,&at);
        body = player->getVehicle();
        if (!game->enterVehicle(player,vehicle,false)) std::abort();
        report(frame);
    }
    if (frame == 30) {
        if(!std::getenv("BF2_EXIT_GROUND")) {
            Mat4 transform=vehicle->getAbsoluteTransformation();
            transform.m[13]+=120;
            vehicle->setAbsoluteTransformation(transform);
            node(vehicle)->setRotationalSpeed(Vec3::getZero());
        }
        float speed=20; if(std::getenv("BF2_EXIT_SPEED")) speed=static_cast<float>(atof(std::getenv("BF2_EXIT_SPEED")));
        node(vehicle)->setPositionalSpeed(Vec3{0,0,speed});
        static_cast<Player*>(player)->m_worldTime = -9999;
        static_cast<ServerGameLogic*>(gameLogic)->toggleEntryPoint(player,true);
        report(frame);
        if(std::getenv("BF2_EXIT_FPE")) { _clearfp(); unsigned int control; _controlfp_s(&control,0,_EM_INVALID|_EM_ZERODIVIDE|_EM_OVERFLOW); }
    }
    if (frame >= 30) {
        if(frame<40 || frame%60==0) report(frame);
        finiteBody &= finite(body->getAbsolutePosition()) && finite(node(body)->getPositionalSpeed());
        finiteRoot &= finite(vehicle->getAbsolutePosition()) && finite(node(vehicle)->getPositionalSpeed());
        auto armor=static_cast<ArmorGLComp*>(object_detail::LayoutAccess::armorComponent(body));
        intact &= player->getIsAlive() && !player->getIsManDown() && armor->getHitPoints()==100;
        if(frame==30) exited=player->getVehicle()==body && body->getParent()==0;
        if(frame==50) survived=exited && intact && finiteBody;
        IObject* current=player->getVehicle();
        if(current && current->getTemplate()->getName()=="parachute") {
            if(!deployed) {
                auto down=static_cast<const ForceObjectTemplate*>(objectTemplateManager->getTemplate("ParachuteForceDown"));
                auto fwd=static_cast<const ForceObjectTemplate*>(objectTemplateManager->getTemplate("parachuteForceFwd"));
                forceChecks=down && fwd && down->input==io::PIThrottle && down->force==10.0f &&
                    fwd->input==io::PIThrottle && fwd->force==5.0f &&
                    static_cast<PlayerControlObject*>(current)->m_activeAnimationSystem!=0;
                bool testedForce=false;
                for(IObject* child=current->getChild(); child; child=child->getSibling()) {
                    if(child->getTemplate()==down) {
                        testedForce=true;
                        auto force=static_cast<ForceObject*>(child);
                        const float saved=force->m_input;
                        io::PlayerInput input={}; input.setInputIsMapped(io::PIThrottle,true);
                        input.setInput(io::PIThrottle,0.25f);
                        force->handlePlayerInput(player,input,1.0f/30,0);
                        forceChecks &= force->m_input==0.25f;
                        input.setInputIsMapped(io::PIThrottle,false);
                        force->handlePlayerInput(player,input,1.0f/30,0);
                        forceChecks &= force->m_input==0.0f;
                        force->m_input=saved;
                    }
                }
                forceChecks &= testedForce;
                printf("[chute] authored force/input, seat animation, mapped/unmapped input checks=%d\n",forceChecks);
            }
            deployed=true; ++chuteFrames;
            finiteBody &= finite(current->getAbsolutePosition()) && finite(node(current)->getPositionalSpeed());
            lastChuteSpeed=node(current)->getPositionalSpeed().y;
            if(frame%60==0) printf("[chute] descent=%g\n",lastChuteSpeed);
        } else if(deployed && current==body && !body->getParent()) landed=true;
        if(!std::getenv("BF2_EXIT_GROUND") && !deployed && current==body && intact) {
            io::PlayerInput input={};
            input.setInputIsMapped(io::PIWeaponSelect9,true);
            input.setInput(io::PIWeaponSelect9,1.0f);
            static_cast<ServerGameLogic*>(gameLogic)->selectWeapon(player,body,input,false);
        }
    }
}
bool exitResults() {
    const bool chuteOK=std::getenv("BF2_EXIT_GROUND") || (deployed && landed && forceChecks && chuteFrames>60 && lastChuteSpeed>-10);
    const bool passed=survived && intact && finiteBody && finiteRoot && chuteOK;
    printf("[exit-result] passed=%d exited=%d survived=%d intact=%d finiteBody=%d finiteRoot=%d deployed=%d landed=%d chuteFrames=%d lastDescent=%g\n",passed,exited,survived,intact,finiteBody,finiteRoot,deployed,landed,chuteFrames,lastChuteSpeed);
    return passed;
}
