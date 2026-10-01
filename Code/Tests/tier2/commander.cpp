// Test scaffolding. [CHOSEN, not recovered] stock-map commander diagnostic; no production overrides.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../../BF2/Game/Objects/Game/ArtilleryHandler.h"
#include "../../BF2/Game/Objects/Game/TurnableRemoteControlledObject.h"
#include "../../BF2/Game/Objects/Game/SupplyObject.h"
#include "../../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../../BF2/Game/Objects/Networkables/Networkables.h"
#undef protected
#undef private
#include "../../BF2/Game/Objects/RemoteControlledObjectTemplate.h"
#include "../../BF2/Game/Objects/Game/SupplyDropManager.h"
#include "../../BF2/Game/Objects/IObjectTemplate.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Game/IGameLogic.h"
#include "../../BF2/Game/IGame.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../BF2/Physics/IPhysicsNode.h"
#include "../../BF2/Physics/HeightmapCluster.h"
#include "../../BF2/IO/BitStream.h"
#include "../../BF2/IO/Console/Console.h"
#include "../../BF2/Core/System.h"
#include "../../BF2/Core/Ids.h"

using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe { namespace world { extern IBase* heightmapCluster; } } }
namespace {
unsigned crateId;
int maxShells;
bool landed;
bool replicatedLanding;
bool wirePass = true;
bool landingEdge;
bool upright = true;
bool requested;
int team;
Vec2 target;
ArtilleryHandler* handler;
}
bool commanderSetup()
{
    IObjectTemplate* saved = objectTemplateManager->getActiveObjectTemplate();
    objectTemplateManager->setActiveObjectTemplate(objectTemplateManager->getTemplate("supply_ParachuteForceDown"));
    const char* names[] = {"constantForce", "useAbsoluteDirection", "onlyWhenOccupied", "startDelay"};
    const char* values[] = {"5", "1", "0", "1"};
    bool ok = true;
    for (int i=0; i<4; ++i)
    {
        std::string reply;
        io::mainConsole->executeLine(std::string("ObjectTemplate.")+names[i], reply, false,false,false,false,false);
        std::printf("[commander] binding %s=%s expected=%s\n",names[i],reply.c_str(),values[i]);
        if (reply != values[i]) ok = false;
    }
    objectTemplateManager->setActiveObjectTemplate(saved);
    return ok;
}
void commanderSnapshot(int frame)
{
    const int tick = WorldPref::getTickCount();
    if (frame == 30)
    {
        artilleryManager->refresh();
        for (team=1; team<=2; ++team)
        {
            handler = artilleryManager->getHandler(static_cast<unsigned char>(team));
            if (handler && !handler->m_artillery.empty()) break;
        }
        if (team>2) { std::printf("[commander] NO ARTILLERY\n"); return; }
        const Vec3 pos = handler->m_artillery.front().artillery->getAbsolutePosition();
        target = Vec2{pos.x + 200.0f, pos.z + 200.0f};
        // Pick a flat, dry patch near the battery, keeping the drop away from the mountain face.
        Vec2 drop{pos.x + 40.0f, pos.z + 40.0f};
        IHeightmapCluster* terrain = static_cast<IHeightmapCluster*>(heightmapCluster);
        float bestSlope = -1.0f;
        for (int dx=-100; dx<=100; dx+=20)
            for (int dz=-100; dz<=100; dz+=20)
            {
                Vec3 normal; unsigned char material;
                const float x=pos.x+dx, z=pos.z+dz;
                const float height = terrain->getHeightAndNormalInWorldCoords(x,z,normal,material);
                if (height > terrain->getWaterLevelInWorldCoords(x,z)+2 && normal.y > bestSlope)
                { bestSlope=normal.y; drop=Vec2{x,z}; }
            }
        std::printf("[commander] drop=%g/%g normalY=%g\n",drop.x,drop.y,bestSlope);
        supplyDropManager->spawnSupplyDrop(static_cast<unsigned char>(team), drop);
        for (const auto& entry : objectManager->getAllRegisteredObjects())
            if (entry.second->getTemplate() && entry.second->getTemplate()->getName() == "supply_crate")
                crateId = entry.first;
        std::printf("[commander] setup team=%d pieces=%u minTicks=%d tick=%d crate=%u\n",
            team, unsigned(handler->m_artillery.size()), handler->m_minIntervalTicks, tick, crateId);
    }
    if (handler && !requested && tick > handler->m_minIntervalTicks + 30)
    {
        requested = true;
        const bool accepted = handler->setTarget(target);
        if (accepted) handler->requestFire();
        std::printf("[commander] request tick=%d accepted=%d pending=%d active=%d\n",
            tick, accepted, handler->m_firePending, handler->m_missionActive);
    }
    int shells = 0;
    for (const auto& entry : objectManager->getAllRegisteredObjects())
    {
        IObject* obj = entry.second;
        if (obj->getTemplate() && obj->getTemplate()->getClassID() == CID_GenericProjectileTemplate
            && !(obj->getFlags() & 1u)) ++shells;
    }
    if (shells > maxShells) { maxShells = shells; std::printf("[commander] shells=%d tick=%d\n", shells, tick); }
    if (frame % 30 != 0) return;
    if (crateId)
    {
        auto found = objectManager->getAllRegisteredObjects().find(crateId);
        if (found != objectManager->getAllRegisteredObjects().end())
        {
            SupplyObject* crate = static_cast<SupplyObject*>(found->second);
            IObject* chute = 0;
            std::vector<IObject*> children;
            getChilds(crate, children, 0, 0);
            for (IObject* child : children)
                if (child->getTemplate() && child->getTemplate()->getName() == "supply_parachute") chute = child;
            auto net = static_cast<SimpleObjectNetworkable*>(crate->getNetworkable());
            // No clients are connected, so the fixture explicitly samples the real replication
            // state machine that a connection's ghost pass would normally request.
            unsigned int mask = 0;
            if (net) net->updateStateMask(&mask);
            const int wire = net ? net->m_records[net->m_head].supplyB0 : -1;
            if (net)
            {
                unsigned char buffer[16] = {};
                io::BitStream stream(buffer, sizeof(buffer));
                net->getNetUpdate(stream, 0, 0x8000, 0);
                unsigned int encodedMask=0, encodedLanded=0;
                stream.readBits(&encodedMask,19);
                stream.readBits(&encodedLanded,1);
                if (encodedMask != 0x8000 || encodedLanded != unsigned(crate->m_suppliesLanded)
                    || stream.getBitPosition() != 20) wirePass=false;
                if ((mask & 0x8000) && crate->m_suppliesLanded) landingEdge=true;
            }
            const Vec3 p = crate->getAbsolutePosition();
            const Mat4 m = crate->getAbsoluteTransformation();
            IPhysicsNode* node=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(crate));
            if (!crate->m_suppliesLanded && node->getIsInAir() && frame > 60 && m.m[5] < 0.0f) upright = false;
            if (crate->m_suppliesLanded && chute && (chute->getFlags() & 1u)) landed = true;
            if (landed && wire == 1) replicatedLanding = true;
            std::printf("[commander] crate frame=%d pos=%g/%g/%g upY=%g landed=%d wire=%d chuteFlags=%x latchOffset=%u\n",
                frame, p.x,p.y,p.z,m.m[5],crate->m_suppliesLanded,wire,chute?chute->getFlags():0,
                unsigned(reinterpret_cast<char*>(&crate->m_suppliesLanded)-reinterpret_cast<char*>(crate)));
        }
    }
    if (handler && requested)
    {
        std::printf("[commander] mission frame=%d tick=%d pending=%d active=%d fireAt=%d\n",
            frame,tick,handler->m_firePending,handler->m_missionActive,handler->m_fireAtTick);
        for (const auto& piece : handler->m_artillery)
        {
            auto h = static_cast<TurnableRemoteControlledObject*>(piece.horizontalTurnObject);
            auto v = static_cast<TurnableRemoteControlledObject*>(piece.verticalTurnObject);
            auto weapon = static_cast<GenericFireArm*>(piece.artillery->m_weapon);
            std::printf("[commander] piece failed=%d h=%d angle=%g/%g v=%d angle=%g/%g burst=%d enabled=%d busy=%d rcid=%d id=%u registered=%p\n",
                piece.missionFailed,h->m_status,h->m_previousAngle,h->m_targetAngle,
                v->m_status,v->m_previousAngle,v->m_targetAngle,weapon->getBurstSize(),weapon->m_itemEnabled,weapon->busy(false),
                piece.artillery->m_objectId,piece.artillery->getObjectId(),gameLogic->getRemoteControlledObject(piece.artillery->m_objectId));
        }
    }
    std::fflush(stdout);
}
bool commanderResult()
{
    std::printf("[commander] RESULT upright=%d landed=%d replicated=%d wire=%d edge=%d requested=%d maxShells=%d\n",
        upright,landed,replicatedLanding,wirePass,landingEdge,requested,maxShells);
    return upright && landed && replicatedLanding && wirePass && landingEdge && requested && maxShells >= 5;
}
