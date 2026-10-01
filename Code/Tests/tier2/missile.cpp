// Test scaffolding. [CHOSEN, not recovered] Stock missile diagnostic inputs and observations.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <map>
#include <list>
#include <vector>
#include <string>
#include <sstream>
#define private public
#define protected public
#include "../../BF2/Game/Objects/Projectiles/GenericProjectile.h"
#include "../../BF2/Game/Objects/Projectiles/SeekComponents/SeekClosestTargetComp.h"
#include "../../BF2/Game/Objects/Projectiles/FollowComponents/DefaultFollowComp.h"
#include "../../BF2/Game/Objects/Projectiles/ProjectileCompTemplates.h"
#include "../../BF2/Game/Objects/Game/TargetObject.h"
#include "../../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../../BF2/Game/Objects/Weapons/TargetComponents/MultiFireArmTargetComp.h"
#include "../../BF2/Game/Objects/Game/PlayerControlObject.h"
#undef protected
#undef private
#include "../../BF2/Game/IGame.h"
#include "../../BF2/Game/IGameLogic.h"
#include "../../BF2/Core/System.h"
#include "../../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../BF2/Game/Objects/IObjectTemplate.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Physics/IPhysicsNode.h"
#include "../../BF2/Game/GameServer/GameServer.h"
#include "../../BF2/Game/GameLogic/ServerGameLogic.h"
#include "../../BF2/Game/Objects/IPlayer.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Scene/Object/ObjectUtil.h"
#include "../../BF2/Core/Ids.h"
using namespace dice::hfe;
using namespace dice::hfe::world;
static GenericProjectile* missiles[2] = {};
static TargetObject* target = 0;
static PlayerControlObject* planes[2]={};
static GenericFireArm* launcher=0;
static WeakPtr<IObject> fired;
static bool observedTurn=false;
static bool completed=false;
static bool negativeCase=false;
static bool reverseCase=false;
static void requireMissile(bool okay,const char* message)
{
    if(!okay){std::fprintf(stderr,"[missile-test] FAIL %s\n",message);std::fflush(stderr);std::abort();}
}
static void planeFrame(int f)
{
    if(f==30)
    {
        negativeCase=std::getenv("BF2_MISSILE_FRIENDLY")!=0;
        reverseCase=std::getenv("BF2_MISSILE_REVERSE")!=0;
        for(int i=0;i<2;++i)
        {
            auto player=static_cast<GameServer*>(game)->createPlayerWithValidName(i?"MissileTarget":"MissileSource",i+1);
            player->setIsRemote(true);player->setTeam(negativeCase?1:i+1,false);player->setKit(3);
            for(int kit=0;kit<7;++kit)for(int slot=0;slot<2;++slot)player->setChosenUnlockLevel(kit,slot,0);
            std::pair<Vec3,Vec3> at(Vec3{100,500,100},Vec3::getZero());
            static_cast<ServerGameLogic*>(gameLogic)->spawnPlayer(player,&at);
            planes[i]=static_cast<PlayerControlObject*>(game->createObjectOnAllClients(objectTemplateManager->getTemplate((i!=reverseCase)?"air_j10":"air_f35b"),at.first,at.second,true,0,0));
            printf("[plane] enter i=%d result=%d\n",i,planes[i]->enter(player));fflush(stdout);
        }
        launcher=static_cast<GenericFireArm*>(planes[0]->getActiveWeapon(1));
        printf("[plane] launcher=%s\n",launcher->getTemplate()->getName().c_str());
    }
    if(f>=30)
    {
        for(int i=0;i<2;++i)
        {
            // Test scaffolding. Repeatable flight inputs: hold the launch plane in place and move the
            // occupied target laterally after launch. Real map objects, components and ticks.
            Mat4 at=Mat4::getIdentity();at.m[12]=i?(reverseCase?70:130)+(f>100?(reverseCase?-1:1)*(f-100)*0.7f:0):100;at.m[13]=i?505:500;at.m[14]=i?300:100;
            planes[i]->setAbsoluteTransformation(at);
            static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(planes[i]))->setPositionalSpeed(Vec3{i&&f>100?(reverseCase?-21.0f:21.0f):0,0,100});
        }
        auto tc=static_cast<MultiFireArmTargetComp*>(launcher->m_targetComp);
        if(f%10==0)printf("[plane] f=%d target=%p lock=%d allow=%d player=%p\n",f,launcher->getTargetObject(),launcher->hasTargetLock(),tc->m_allowTargetSeek,launcher->getPlayer());
        if(f==60)
        {
            auto m=launcher->getAbsoluteTransformation();
            printf("[plane] templ system=%d type=%d dist=%g angle=%g delay=%d forward=%g/%g/%g item=%d flags=%x\n",tc->getTargetSystem(),tc->getTargetType(),tc->getMaxTargetDistance(),tc->m_multiTemplate->getLockAngle(),tc->m_multiTemplate->getLockDelayTicks(),m.m[8],m.m[9],m.m[10],launcher->m_itemEnabled,launcher->getFlags());
            for(auto& entry:objectManager->getAllRegisteredObjects())
            {
                auto o=entry.second;if(o->getTemplate()->getClassID()!=CID_TargetObjectTemplate || getRootParent(o)!=planes[1])continue;
                auto t=static_cast<TargetObject*>(o);auto pos=t->getAbsolutePosition();
                printf("[plane] heat=%p active=%d latch=%d registered=%d type=%d pos=%g/%g/%g rootflags=%x teams=%d/%d\n",t,t->getTargetActive(),t->m_targetActive,t->m_registeredWithGameLogic,t->getTargetType(),pos.x,pos.y,pos.z,planes[1]->getFlags(),planes[0]->getTeam(),planes[1]->getTeam());
            }
            for(int bits=0;bits<8;++bits)
            {
                auto found=gameLogic->findBestTargetObject(launcher->getAbsolutePosition(),Vec3{0,0,1},static_cast<TargetType>(2),375,0.261799f,planes[0],launcher,0,launcher,bits&1,bits&2,bits&4);
                printf("[plane] search terrain=%d objects=%d friendly=%d found=%p\n",!!(bits&1),!!(bits&2),!!(bits&4),found);
            }
        }
        if(f==100)
        {
            requireMissile(launcher->hasTargetLock()!=negativeCase,"enemy lock / friendly exclusion");
            requireMissile(tc->getTargetType()==2,"stock heat target type");
            requireMissile(std::fabs(launcher->getMaxTargetAngle()-45)<0.001f,"stock seeker angle");
            requireMissile(static_cast<GenericFireArm*>(planes[0]->getActiveWeapon(0))->getMaxTargetAngle()==0,"unguided cannon has no seeker");
            // Both authored limits must participate: range is the smaller launcher/seeker value.
            auto st=static_cast<SeekClosestTargetCompTemplate*>(launcher->getProjectileTemplate()->getComponentFromClassID(CID_SeekClosestTargetCompTemplate));
            const float authored=st->getMaxTargetDistance();
            st->setMaxTargetDistance(200);requireMissile(tc->getMaxTargetDistance()==200,"seeker bounds range");
            st->setMaxTargetDistance(500);requireMissile(tc->getMaxTargetDistance()==375,"launcher bounds range");
            st->setMaxTargetDistance(authored);
            launcher->doFire(static_cast<FireState>(1),false);
            printf("[plane] fired last=%d\n",launcher->getLastFiredProjectileId());
        }
        if(f>=100 && f<125)
            for(auto& entry:objectManager->getAllRegisteredObjects())
            {
                auto o=entry.second;
                if(o->getTemplate()!=launcher->getProjectileTemplate())continue;
                auto p=static_cast<GenericProjectile*>(o);if(!p->m_isActive)continue;
                fired=p->getWeakPtr();
                auto follow=static_cast<DefaultFollowComp*>(p->m_followComp);
                auto v=*p->getVelocity();auto pos=p->getAbsolutePosition();
                if(f==100)requireMissile(p->getTargetObject()==launcher->getTargetObject(),"launch inherits weapon lock");
                if(f>=101 && f<=110)
                {
                    requireMissile(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z),"finite missile velocity");
                    if(negativeCase)requireMissile(!p->getTargetObject()&&std::fabs(v.x)<0.001f,"friendly missile stays unguided");
                    else
                    {
                        requireMissile(p->getTargetObject()!=0,"missile retains target");
                        if((reverseCase?-v.x:v.x)>10.0f)observedTurn=true;
                    }
                }
                printf("[plane] f=%d missile=%u target=%p follow=%p terminal=%d kill=%d pos=%g/%g/%g speed=%g/%g/%g\n",f,p->getObjectId(),p->getTargetObject(),follow->m_targetObject.get(),follow->m_terminal,follow->m_killDistanceReached,pos.x,pos.y,pos.z,v.x,v.y,v.z);
            }
        if(f==124)
        {
            completed=true;
            requireMissile(fired.get()!=0,"projectile launched");
            requireMissile(negativeCase||observedTurn,"guided missile turns toward moving target");
            printf("[missile-test] PASS %s %s: lookup, range, lock, launch, guidance\n",reverseCase?"Archer":"Sidewinder",negativeCase?"friendly exclusion":"enemy tracking");
        }
        fflush(stdout);
    }
}
void missileFrame(int f)
{
    if(std::getenv("BF2_MISSILE_PLANE")){planeFrame(f);return;}
    if (f == 30)
    {
        target=static_cast<TargetObject*>(game->createObjectOnAllClients(objectTemplateManager->getTemplate("HeatObject"),Vec3{130,505,300},Vec3::getZero(),true,0,0));
        target->setTargetActive(true,false);
        const char* names[]={"aim9m_sidewinder","aa11_archer"};
        for(int i=0;i<2;++i)
        {
            missiles[i]=static_cast<GenericProjectile*>(game->createObjectOnAllClients(objectTemplateManager->getTemplate(names[i]),Vec3{100,500,100},Vec3::getZero(),true,0,0));
            auto p=missiles[i];
            p->activate();
            auto node=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(p));
            node->setPositionalSpeed(Vec3{0,0,100});
            p->setTargetObject(target);
            auto s=static_cast<SeekClosestTargetComp*>(p->m_seekComp);
            auto follow=static_cast<DefaultFollowComp*>(p->m_followComp);
            printf("[missile] %s launch tick=%u flags=%x seek=%p follow=%p type=%d delay=%u distance=%g angle=%g yaw=%g pitch=%g activeTarget=%d\n",names[i],p->m_dword1BC,p->getFlags(),s,follow,s->getTargetType(),s->getTrackingDelayTick(),s->getMaxTargetDistance(),s->getMaxTargetAngle(),follow->m_template->getMaxYaw(),follow->m_template->getMaxPitch(),target->getTargetActive());
        }
    }
    if(f>=30 && f<55)
    {
        for(int i=0;i<2;++i)
        {
            auto p=missiles[i];
            auto s=static_cast<SeekClosestTargetComp*>(p->m_seekComp);
            auto follow=static_cast<DefaultFollowComp*>(p->m_followComp);
            auto node=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(p));
            auto v=node->getPositionalSpeed();auto pos=p->getAbsolutePosition();auto mat=p->getAbsoluteTransformation();
            printf("[missile] f=%d tick=%d i=%d active=%d target=%p seek=%p follow=%p delay=%u turns=%g/%g terminal=%d kill=%d pos=%g/%g/%g vel=%g/%g/%g forward=%g/%g/%g\n",f,WorldPref::getTickCount(),i,p->m_isActive,target,s->getTargetObject(),follow->m_targetObject.get(),follow->m_delayTick,follow->m_yawTurn,follow->m_pitchTurn,follow->m_terminal,follow->m_killDistanceReached,pos.x,pos.y,pos.z,v.x,v.y,v.z,mat.m[8],mat.m[9],mat.m[10]);
        }
        fflush(stdout);
    }
}

bool missileResults() { return completed; }
