// Test scaffolding. [CHOSEN, not recovered] Finite-state observation; no sanitization, no client connection.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include "../../BF2/Game/GameServer/GameServer.h"
#include "../../BF2/Game/GameLogic/ServerGameLogic.h"
#include "../../BF2/Game/Objects/IPlayer.h"
#include "../../BF2/Game/Objects/IObject.h"
#include "../../BF2/Game/Objects/IObjectTemplate.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../BF2/Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../BF2/Physics/IPhysicsNode.h"
#include "../../BF2/IO/Network/INetworkable.h"
#include "../../BF2/IO/Network/INetworkableObject.h"
#include "../../BF2/IO/Network/NetworkManager.h"
#include "../../BF2/IO/BitStream.h"
#include "../../BF2/Core/Ids.h"
#include "../../BF2/Game/GameLogic/ActionBuffer.h"
#include "../../BF2/IO/Input/PlayerInput.h"
#include "../../BF2/Game/Objects/Weapons/GenericFireArm.h"
#define private public
#include "../../BF2/Game/Objects/Projectiles/GenericProjectile.h"
#undef private
#include <set>
using namespace dice::hfe;
using namespace dice::hfe::world;
static int failures=0, checks=0, lastStatus=0;
static IObject* vehicle=0;
static IPlayer* player=0;
static GenericFireArm* tow=0;
static int shots=0;
static std::set<unsigned> activeMissiles;
static IObject* wireObject=0;
static int wireFrame=0, wireChecks=0;
void floatPhysics(IPhysicsNode* n,const char* method,const char* phase) {
    if(!vehicle || n->getObject()!=vehicle)return;
    const Vec3 v[]={n->getPositionalSpeed(),n->getRotationalSpeed(),n->getPositionalAcceleration(),n->getRotationalAcceleration(),n->getPositionalFriction(),n->getRotationalFriction(),n->getLocalLinearSpeed(),n->getLocalAngularSpeed()};
    const char* fields[]={"velocity","angularVelocity","acceleration","angularAcceleration","friction","angularFriction","localVelocity","localAngularVelocity"};
    for(int i=0;i<8;++i)if(!std::isfinite(v[i].x)||!std::isfinite(v[i].y)||!std::isfinite(v[i].z)) {
        printf("[float-first] method=%s phase=%s field=%s value=%g/%g/%g mass=%g\n",method,phase,fields[i],v[i].x,v[i].y,v[i].z,n->getMass());fflush(stdout);std::abort();
    }
}
static void values(IObject* o, const char* field,const float* p,int n,int frame) {
    for(int i=0;i<n;++i) {
        ++checks;
        if(!std::isfinite(p[i])) {
            unsigned bits;std::memcpy(&bits,p+i,4);
            printf("[float] NONFINITE frame=%d object=%u template=%s field=%s[%d] bits=%08x value=%g flags=%08x\n",frame,o->getObjectId(),o->getTemplate()->getName().c_str(),field,i,bits,p[i],o->getFlags());
            if(++failures>30) {fflush(stdout);std::abort();}
        }
    }
}
void floatWire(const char* kind,const Vec3& v,const Vec3& ref,float scale,void* caller) {
    ++wireChecks;
    if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||
       !std::isfinite(ref.x)||!std::isfinite(ref.y)||!std::isfinite(ref.z)||!std::isfinite(scale)) {
        printf("[float-wire] NONFINITE frame=%d template=%s codec=%s value=%g/%g/%g reference=%g/%g/%g scale=%g caller=%p\n",wireFrame,wireObject?wireObject->getTemplate()->getName().c_str():"outside-snapshot",kind,v.x,v.y,v.z,ref.x,ref.y,ref.z,scale,caller);
        ++failures;fflush(stdout);
    }
}
static void snapshot(int frame) {
    wireFrame=frame;
    int members=0;
    for(const auto& entry:io::networkManager->getDescriptorMap()) {
        auto desc=entry.second;
        for(const auto& prop:desc->properties)for(auto net:prop.networkables) {
            auto no=static_cast<io::INetworkableObject*>(net->queryInterface(io::IID_INetworkableObject));
            auto o=no?no->m_object:0;
            if(!o || (o->getFlags()&1))continue;
            auto root=o;while(root->getParent())root=root->getParent();
            if(root!=vehicle && o->getTemplate()->getName()!="tow_missile")continue;
            wireObject=o;
            unsigned mask=0;net->updateStateMask(&mask);
            for(int precision=2;precision<=3;++precision) {
                io::BitStream stream(65536);
                // Root vehicle is SimpleObjectNetworkable: include the initialized physical
                // channels omitted by ordinary ghosts but needed for owner prediction.
                const unsigned sendMask=net->getGhostStateMask() | mask | (o==vehicle?0x436cu:0u);
                bool okay=net->getNetUpdate(stream,0,sendMask,precision);
                printf("[float-wire] frame=%d descriptor=%u template=%s precision=%d bits=%u okay=%d mask=%08x\n",frame,entry.first,o->getTemplate()->getName().c_str(),precision,stream.getBitPosition(),okay,sendMask);
                if(!okay)++failures;
            }
            ++members;
        }
    }
    wireObject=0;
    printf("[float-wire] frame=%d members=%d\n",frame,members);fflush(stdout);
}
static void scan(int frame) {
    for(const auto& entry:objectManager->getAllRegisteredObjects()) {
        IObject* o=entry.second;
        if(!o || (o->getFlags()&1))continue;
        values(o,"absolute",o->getAbsoluteTransformation().m,16,frame);
        values(o,"relative",o->getRelativeTransformation().m,16,frame);
        auto n=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(o));
        if(n && n->getIsMobile()) {
#define V(label, getter) values(o,label,&n->getter().x,3,frame)
            V("velocity",getPositionalSpeed);V("angularVelocity",getRotationalSpeed);
            V("acceleration",getPositionalAcceleration);V("angularAcceleration",getRotationalAcceleration);
            V("friction",getPositionalFriction);V("angularFriction",getRotationalFriction);
            V("localVelocity",getLocalLinearSpeed);V("localAngularVelocity",getLocalAngularSpeed);
#undef V
        }
    }
    fflush(stdout);
}
void floatFrame(int frame) {
    lastStatus=gameLogic->getGameStatus();
    if(frame<29)return;
    scan(frame);
    if(frame==30) {
        const char* name=std::getenv("BF2_FLOAT_VEHICLE");
        for(const auto& entry:objectManager->getAllRegisteredObjects()) {
            auto o=entry.second;
            if(o && !o->getParent() && !_stricmp(o->getTemplate()->getName().c_str(),name) && object_detail::LayoutAccess::armorComponent(o)) {vehicle=o;break;}
        }
        if(!vehicle) { printf("[float] no vehicle %s\n",name);fflush(stdout);std::abort(); }
        player=static_cast<GameServer*>(game)->createPlayerWithValidName("FloatProbe",1);
        player->setIsRemote(true);player->setTeam(1,false);player->setKit(3);
        for(int kit=0;kit<7;++kit)for(int slot=0;slot<2;++slot)player->setChosenUnlockLevel(kit,slot,0);
        std::pair<Vec3,Vec3> at(vehicle->getAbsolutePosition(),Vec3::getZero());
        static_cast<ServerGameLogic*>(gameLogic)->spawnPlayer(player,&at);
        scan(frame);
        bool entered=game->enterVehicle(player,vehicle,false);
        printf("[float] entry name=%s object=%u entered=%d controlled=%s\n",name,vehicle->getObjectId(),entered,player->getVehicle()?player->getVehicle()->getTemplate()->getName().c_str():"NULL");
        if(!entered)++failures;
        for(const auto& entry:objectManager->getAllRegisteredObjects()) {
            auto o=entry.second;
            if(o && !_stricmp(o->getTemplate()->getName().c_str(),"USAPC_LAV25_TowLauncher")) {
                auto root=o;while(root->getParent())root=root->getParent();
                if(root==vehicle)tow=static_cast<GenericFireArm*>(o);
            }
        }
        scan(frame);
    }
    if(player && frame>=30) {
        io::PlayerInput input={};
        input.mappingFlags=1ULL << io::PIThrottle;
        input.setInput(io::PIThrottle,frame>=90&&!tow?0.5f:0.0f);
        if(tow && (frame==60||frame==420||frame==780)) {
            input.mappingFlags |= 1ULL << tow->getFireInput();
            input.setInput(tow->getFireInput(),1.0f);
            printf("[float-tow] trigger frame=%d fireInput=%d ammo=%d\n",frame,tow->getFireInput(),tow->getAmmoInMag(0));
        }
        PlayerAction action;action.set(input);player->getActionBuffer()->add(action);
    }
    if(tow) {
        std::set<unsigned> now;
        for(const auto& entry:objectManager->getAllRegisteredObjects()) {
            auto o=entry.second;
            if(!o || o->getTemplate()->getName()!="tow_missile")continue;
            auto missile=static_cast<GenericProjectile*>(o);
            if(!missile->m_isActive)continue;
            now.insert(o->getObjectId());
            if(!activeMissiles.count(o->getObjectId())) {
                ++shots;
                printf("[float-tow] activated frame=%d shot=%d object=%u position=%g/%g/%g\n",frame,shots,o->getObjectId(),o->getAbsolutePosition().x,o->getAbsolutePosition().y,o->getAbsolutePosition().z);
                snapshot(frame);
            }
            values(o,"projectileVelocity",&missile->getVelocity()->x,3,frame);
        }
        activeMissiles=now;
    }
    if(vehicle && !std::getenv("BF2_FLOAT_SKIP_WIRE") && (frame==30||frame==31||frame==90||frame==180)) {snapshot(frame);scan(frame);}
}
bool floatResults(){
    const bool requiresTow = !_stricmp(std::getenv("BF2_FLOAT_VEHICLE"), "USAPC_LAV25");
    printf("[float] checks=%d wireChecks=%d failures=%d status=%d shots=%d\n",checks,wireChecks,failures,lastStatus,shots);
    return vehicle && checks && wireChecks && !failures && (!requiresTow || (tow && shots==3));
}
