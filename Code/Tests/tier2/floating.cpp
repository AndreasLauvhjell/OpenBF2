#include <cstdlib>
// Test scaffolding. [CHOSEN, not recovered] Observe actual stock objects after production startup.
#include <cstdio>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include "../../BF2/IO/Console/Console.h"
#include "../../BF2/IO/Input/PlayerInput.h"
#include "../../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../BF2/Game/Objects/Game/Engine.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Game/Objects/IObject.h"
#include "../../BF2/Game/Objects/IObjectTemplate.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Game/Objects/RotationalBundleLeaves.h"
#include "../../BF2/Game/Objects/GameLogicComp/IArmorGLComp.h"
#include "../../BF2/Physics/IPhysicsNode.h"
#include "../../BF2/Physics/ICollisionMesh.h"
#include "../../BF2/Physics/BasicPhysicsSystem.h"
#include "../../BF2/Physics/HeightmapCluster.h"
#include "../../BF2/Core/Ids.h"
using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe { namespace world { extern IBase* heightmapCluster; extern IBase* basicPhysicsSystem; } } }
namespace {
int checks=0, failures=0, idleChecks=0;
IObject* driven=0;
std::vector<IObject*> controls;
Vec3 driveStart, turnStart;
void check(bool pass,const char* name) { ++checks; if(!pass) ++failures; std::printf("[boat] CHECK %s %s\n",name,pass?"PASS":"FAIL"); }
void bindings()
{
    auto saved=objectTemplateManager->getActiveObjectTemplate();
    std::ifstream in(std::getenv("BF2_TIER2_EXPECTATIONS"));
    std::string name,prop,expected;
    while(in>>name>>prop>>expected)
    {
        auto t=objectTemplateManager->getTemplate(name);
        if(!t) continue;
        objectTemplateManager->setActiveObjectTemplate(t);
        std::string reply;
        io::mainConsole->executeLine("ObjectTemplate."+prop,reply,false,false,false,false,false);
        float value=0; std::istringstream parsed(reply);
        check(bool(parsed>>value)&&std::fabs(value-std::stof(expected))<0.00001f,(name+"."+prop).c_str());
    }
    objectTemplateManager->setActiveObjectTemplate(saved);
    check(static_cast<IHeightmapCluster*>(heightmapCluster)->getSeaWaterLevel()==83.0f,"stock Wake sea level");
}
}
void boatSnapshot(int frame)
{
    for (const auto& entry : objectManager->getAllRegisteredObjects())
    {
        IObject* root = entry.second;
        if (!root || root->getParent() || !root->getTemplate()
            || root->getTemplate()->getName() != "boat_rib") continue;
        auto armor = static_cast<IArmorGLComp*>(root->queryComponent(IID_IArmorGLComp, IID_IArmorGLComp));
        if (!armor) continue;
        auto node = static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(root));
        const Vec3 p = root->getAbsolutePosition(), v = node->getPositionalSpeed();
        auto terrain=static_cast<IHeightmapCluster*>(heightmapCluster);
        auto mesh=static_cast<ICollisionMesh*>(object_detail::LayoutAccess::collisionMesh(root));
        std::printf("[boat] frame=%d id=%u pos=%g/%g/%g v=%g/%g/%g hp=%g sleep=%d\n", frame,
            entry.first,p.x,p.y,p.z,v.x,v.y,v.z,armor->getHitPoints(),node->getIsSleeping());
        std::printf("[boat] surface water=%g ground=%g lod1=%d enabled=%d gravity=%g\n",
            terrain->getWaterLevelInWorldCoords(p.x,p.z),terrain->getHeightInWorldCoords(p.x,p.z),mesh?mesh->hasLod(1):-1,
            node->getEnabled(),static_cast<IPhysicsSystem*>(basicPhysicsSystem)->getGravity());
        if (frame==300 || (frame==900 && root!=driven))
        {
            ++idleChecks;
            check(std::isfinite(p.y)&&std::fabs(p.y-83.0f)<0.3f&&std::fabs(v.y)<0.01f
                  &&armor->getHitPoints()==750.0f,"healthy boat floats at stock waterline");
        }
        if (frame != 0 && frame != 300) continue;
        std::vector<IObject*> pending(1,root);
        while (!pending.empty())
        {
            IObject* object = pending.back(); pending.pop_back();
            for (IObject* c=object->getChild(); c; c=object_detail::LayoutAccess::sibling(c)) pending.push_back(c);
            if (!object->getTemplate() || object->getTemplate()->getClassID()!=CID_FloatingBundleTemplate) continue;
            auto physics=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(object));
            auto t=static_cast<const FloatingBundleTemplate*>(object->getTemplate());
            Vec3 q=object->getAbsolutePosition();
            std::printf("[boat] float=%s node=%p mobile=%d sleeping=%d flags=%x y=%g lift=%g/%g hull=%g\n",
                t->getName().c_str(),physics,physics?physics->getIsMobile():-1,physics?physics->getIsSleeping():-1,
                object_detail::LayoutAccess::flags(object),q.y,t->m_floatMinLift,t->m_floatMaxLift,t->m_hullHeight);
        }
    }
    std::fflush(stdout);
}
void boatFrame(int frame)
{
    if(frame==0) bindings();
    if(frame%150==0) boatSnapshot(frame);
    if(frame==300)
    {
        for(const auto& entry:objectManager->getAllRegisteredObjects())
        {
            auto o=entry.second;
            if(o&&!o->getParent()&&o->getTemplate()&&o->getTemplate()->getName()=="boat_rib"
                &&o->queryComponent(IID_IArmorGLComp,IID_IArmorGLComp)) { driven=o; break; }
        }
        if(!driven) { check(false,"drive boat exists"); return; }
        // Test scaffolding. Move one already spawned stock boat to unobstructed open water for the control probe.
        Mat4 transform=Mat4::getIdentity(); transform.m[12]=-500; transform.m[13]=82.876f; transform.m[14]=-500;
        driven->setAbsoluteTransformation(transform);
        auto node=static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(driven));
        node->setPositionalSpeed(Vec3::getZero()); node->setRotationalSpeed(Vec3::getZero()); node->setIsAwake();
        driveStart=driven->getAbsolutePosition();
        std::vector<IObject*> pending(1,driven);
        while(!pending.empty())
        {
            auto o=pending.back(); pending.pop_back();
            for(auto c=o->getChild();c;c=object_detail::LayoutAccess::sibling(c)) pending.push_back(c);
            if(o->getTemplate()->getClassID()==CID_EngineTemplate || o->getTemplate()->getName()=="boat_rib_enginebase")
            {
                controls.push_back(o);
                if(o->getTemplate()->getClassID()==CID_EngineTemplate) o->handleMessage(static_cast<TemplateMessage>(4),0,false);
            }
        }
        check(controls.size()==2,"stock engine and steering mount found");
    }
    if(!driven) return;
    if(frame==450)
    {
        const auto p=driven->getAbsolutePosition();
        std::printf("[boat] DRIVE dz=%g\n",p.z-driveStart.z);
        check(p.z-driveStart.z>3.0f,"throttle propels boat through water");
        const auto& m=driven->getAbsoluteTransformation(); turnStart={m.m[8],m.m[9],m.m[10]};
    }
    if(frame==600)
    {
        const auto& m=driven->getAbsoluteTransformation();
        const float dot=m.m[8]*turnStart.x+m.m[9]*turnStart.y+m.m[10]*turnStart.z;
        std::printf("[boat] STEER forwardDot=%g\n",dot);
        check(std::isfinite(dot)&&dot<0.98f,"yaw input turns boat");
    }
    io::PlayerInput input={};
    input.setInputIsMapped(io::PIThrottle,true); input.setInputIsMapped(io::PIYaw,true);
    input.setInput(io::PIThrottle,frame<600?1.0f:0.0f);
    input.setInput(io::PIYaw,(frame>=450&&frame<600)?1.0f:0.0f);
    for(auto c:controls) c->handlePlayerInput(0,input,1.0f/30.0f,0);
}
bool boatResults()
{
    check(idleChecks==11,"six initial and five final idle boats observed");
    std::printf("[boat] RESULT checks=%d failures=%d\n",checks,failures);
    return checks>20 && failures==0;
}
