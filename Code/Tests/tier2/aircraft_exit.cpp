// Test scaffolding. Stock objects; synthetic remote pilot; no connected clients.
// Routine dedicated-server momentum and copilot camera observation.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <map>
#include <vector>
#include <string>
#define private public
#define protected public
#include "../../BF2/Game/GameLogic/Player.h"
#include "../../BF2/Game/Objects/PlayerControlObjectTemplate.h"
#include "../../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../../BF2/Game/Objects/Weapons/ZoomComponents/ToggleCameraComp.h"
#undef protected
#undef private
#include "../../BF2/Game/GameServer/GameServer.h"
#include "../../BF2/Game/GameLogic/ServerGameLogic.h"
#include "../../BF2/Game/GameLogic/ActionBuffer.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Physics/IPhysicsNode.h"
#include "../../BF2/Game/Objects/GameLogicComp/ArmorGLComp.h"
#include "../../BF2/IO/Input/PlayerInput.h"
#include "../../BF2/Game/Objects/ICameraObject.h"
#include "../../BF2/Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../BF2/Core/Ids.h"
using namespace dice::hfe;
using namespace dice::hfe::world;
static dice::hfe::world::IPlayer* player;
static IObject* vehicle;
static IObject* body;
static bool exited, retained, carried, finiteState = true;
static IObject* copilotSeat;
static bool cockpitBlocked, cameraSelected, launched, projectileSeen;
static int initialAmmo;
static bool copilotMode() { return std::getenv("BF2_EXIT_COPILOT") != 0; }
static void copilotFrame(int frame) {
    auto pco=static_cast<IPlayerControlObject*>(copilotSeat->queryInterface(IID_IPlayerControlObject));
    auto weapon=static_cast<GenericFireArm*>(pco->getActiveWeapon(0));
    auto zoom=static_cast<ToggleCameraComp*>(weapon->m_zoomComp);
    if(frame==30) initialAmmo=weapon->getAmmoInMag(0);
    if(frame==55) cockpitBlocked=weapon->getAmmoInMag(0)==initialAmmo && !zoom->fireAllowed();
    if(frame==80) cameraSelected=pco->getActiveCamera()->getCameraId()==11 && zoom->fireAllowed();
    if(frame>=100) launched |= weapon->getAmmoInMag(0)<initialAmmo;
    if(frame>=90 && zoom->m_projectile.get() && !projectileSeen) {
        projectileSeen=true;
        printf("[air-exit-projectile] f=%d postFire projectile=%p\n",frame,zoom->m_projectile.get());
    }
    if(frame==30||frame%10==0) {
        printf("[air-exit-copilot] f=%d camera=%d weapon=%s ammo=%d zoomFireAllowed=%d active=%d\n",
               frame,pco->getActiveCamera()->getCameraId(),weapon->getTemplate()->getName().c_str(),
               weapon->getAmmoInMag(0),zoom->fireAllowed(),player->getVehicle()==copilotSeat);
        fflush(stdout);
    }
    io::PlayerInput input={};
    input.setInputIsMapped(io::PIFire,true); input.setInputIsMapped(io::PIAltFire,true);
    input.setInput(io::PIFire,((frame>=35&&frame<=45)||(frame>=90&&frame<=100))?1.0f:0.0f);
    input.setInput(io::PIAltFire,frame>=65&&frame<=68?1.0f:0.0f);
    PlayerAction action; action.set(input); player->getActionBuffer()->add(action);
}
static IPhysicsNode* node(IObject* o) { return static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(o)); }
static float speed(const Vec3& v) { return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z); }
static bool finite(const Vec3& v) { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
static void report(int frame, const char* phase) {
    const Vec3 v = node(vehicle)->getPositionalSpeed();
    const Vec3 b = node(body)->getPositionalSpeed();
    const Vec3 p = vehicle->getAbsolutePosition();
    auto armor = static_cast<ArmorGLComp*>(object_detail::LayoutAccess::armorComponent(vehicle));
    printf("[air-exit] f=%d %s plane=%g velocity=%g/%g/%g pos=%g/%g/%g sleeping=%d hp=%g body=%g velocity=%g/%g/%g seated=%d\n",
           frame, phase, speed(v), v.x,v.y,v.z,p.x,p.y,p.z,node(vehicle)->getIsSleeping(),
           armor->getHitPoints(),speed(b),b.x,b.y,b.z,player->getVehicle()==vehicle);
    fflush(stdout);
}
void exitFrame(int frame) {
    if (frame == 30) {
        const int port = static_cast<GameServer*>(game)->m_port;
        const int expectedPort = std::atoi(std::getenv("BF2_PORT"));
        printf("[air-exit-port] GameServer=%d expected=%d\n",port,expectedPort);
        fflush(stdout);
        if (port != expectedPort) std::abort();
        const char* name = std::getenv("BF2_EXIT_VEHICLE");
        for (const auto& entry : objectManager->getAllRegisteredObjects()) {
            IObject* o = entry.second;
            if (o && !o->getParent() && o->getTemplate()->getName()==name &&
                object_detail::LayoutAccess::armorComponent(o)) { vehicle=o; break; }
        }
        if (!vehicle) { printf("[air-exit] no live %s\n",name); fflush(stdout); std::abort(); }
        player = static_cast<GameServer*>(game)->createPlayerWithValidName("AircraftExitProbe",1);
        player->setIsRemote(true);
        player->setTeam(1,false);
        player->setKit(3);
        for (int k=0;k<7;++k) for (int s=0;s<2;++s) player->setChosenUnlockLevel(k,s,0);
        std::pair<Vec3,Vec3> at(vehicle->getAbsolutePosition(),Vec3::getZero());
        static_cast<ServerGameLogic*>(gameLogic)->spawnPlayer(player,&at);
        body = player->getVehicle();
        if (copilotMode()) {
            for (IObject* child=vehicle->getChild();child;child=child->getSibling())
                if(child->getTemplate()->getName()=="ruair_su34_CoPilot") copilotSeat=child;
            if(!copilotSeat) std::abort();
        }
        if (!game->enterVehicle(player,copilotMode()?copilotSeat:vehicle,false)) std::abort();
        Mat4 transform=vehicle->getAbsoluteTransformation();
        transform.m[12]=0; transform.m[13]=300; transform.m[14]=0;
        vehicle->setAbsoluteTransformation(transform);
        node(vehicle)->setRotationalSpeed(Vec3::getZero());
        node(vehicle)->setPositionalSpeed(Vec3{transform.m[8]*100,transform.m[9]*100,transform.m[10]*100});
        report(frame,"setup");
    }
    if (copilotMode()) { if(frame>=30) copilotFrame(frame); return; }
    if (frame >= 30 && frame < 60) {
        io::PlayerInput input={};
        input.setInputIsMapped(io::PIThrottle,true);
        input.setInput(io::PIThrottle,1.0f);
        PlayerAction action; action.set(input); player->getActionBuffer()->add(action);
    }
    if (frame == 60) {
        report(frame,"before");
        const Vec3 before = node(vehicle)->getPositionalSpeed();
        static_cast<Player*>(player)->m_worldTime = -9999;
        static_cast<ServerGameLogic*>(gameLogic)->toggleEntryPoint(player,true);
        const Vec3 after = node(vehicle)->getPositionalSpeed();
        const Vec3 soldier = node(body)->getPositionalSpeed();
        const float modifier = static_cast<const PlayerControlObjectTemplate*>(vehicle->getTemplate())->m_positionalSpeedModifier;
        retained = before.x==after.x && before.y==after.y && before.z==after.z;
        carried = std::fabs(soldier.x-modifier*after.x)<0.0001f &&
                  std::fabs(soldier.y-modifier*after.y)<0.0001f &&
                  std::fabs(soldier.z-modifier*after.z)<0.0001f;
        exited = player->getVehicle()==body && !body->getParent();
        printf("[air-exit-check] retained=%d carried=%d modifier=%g exited=%d\n",retained,carried,modifier,exited);
        report(frame,"after");
    }
    if (frame>=30) {
        finiteState &= finite(node(vehicle)->getPositionalSpeed()) && finite(vehicle->getAbsolutePosition());
        if ((frame>=55 && frame<70) || frame%15==0) report(frame,"tick");
    }
}
bool exitResults() {
    if(copilotMode()) {
        printf("[air-exit-copilot-result] cockpitBlocked=%d cameraSelected=%d launched=%d projectile=%d\n",cockpitBlocked,cameraSelected,launched,projectileSeen);
        return cockpitBlocked && cameraSelected && launched && projectileSeen;
    }
    printf("[air-exit-result] retained=%d carried=%d exited=%d finite=%d\n",retained,carried,exited,finiteState);
    return retained && carried && exited && finiteState;
}
