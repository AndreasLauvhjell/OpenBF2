// [TEMP] Stock-content observer. No product method is overridden; boot uses BF2::init.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include "../../BF2/Core/Ids.h"
#include "../../BF2/Core/System.h"
#include "../../BF2/Game/IGameLogic.h"
#include "../../BF2/Game/Objects/IObject.h"
#include "../../BF2/Game/Objects/IObjectTemplate.h"
#include "../../BF2/Game/Objects/HudGLCompTemplates.h"
#include "../../BF2/Game/Objects/StrategicObjectCompTemplate.h"
#include "../../BF2/Game/Objects/StrategicObjectComp.h"
#include "../../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../BF2/IO/Console/Console.h"
#include "../../BF2/IO/Network/INetworkable.h"
using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe { extern IGameLogic* gameLogic; } }
namespace {
bool passed = true;
int snapshots;
unsigned priorityCalls, missingPriorities;
unsigned previousTick;
bool sawTickReset;
void check(bool value, const char* message)
{
    if (!value) passed=false;
    std::printf("[tier3-content] %s %s\n",value?"PASS":"FAIL",message);
    std::fflush(stdout);
}
bool close(const Vec3& a,const Vec3& b)
{
    return std::fabs(a.x-b.x)<0.0001f && std::fabs(a.y-b.y)<0.0001f && std::fabs(a.z-b.z)<0.0001f;
}
void snapshot()
{
    ++snapshots;
    unsigned ikObjects=0, strategicObjects=0, destroyables=0;
    auto holders=gameLogic->getStrategicObjectMap();
    std::set<StrategicObjectHolder*> seen;
    const Vec3* sharedResult=nullptr;
    for (const auto& entry : objectManager->getAllRegisteredObjects())
    {
        IObject* object=entry.second;
        if (!object->getTemplate()) continue;
        if (object->getTemplate()->getComponentFromClassID(CID_IkTemplateComponent)) ++ikObjects;
        auto strategic=static_cast<IStrategicObjectComp*>(
            object->queryComponent(IID_IStrategicObjectComp,IID_IStrategicObjectComp));
        if (strategic)
        {
            ++strategicObjects;
            const std::string key=strategic->getStrategicObjectName()+static_cast<char>('0'+strategic->getIndex());
            const auto holder=holders->find(key);
            if (holder==holders->end()) { check(false,"runtime strategic object has a holder"); continue; }
            if (seen.insert(holder->second).second)
            {
                std::printf("[tier3-holder] key=%s template=%s index=%d destroyed=%d\n",key.c_str(),
                    object->getTemplate()->getName().c_str(),strategic->getIndex(),holder->second->getNrDestroyedObjects());
                check(close(strategic->getHolderPosition(),holder->second->getPosition()),"holder position matches live-piece average");
                if (sharedResult) check(sharedResult==&strategic->getHolderPosition(),"holder result reference shared across components");
                sharedResult=&strategic->getHolderPosition();
            }
        }
        if (object->getTemplate()->getClassID()==CID_DestroyableObjectTemplate)
        {
            ++destroyables;
            if (!object->getNetworkable() || object->getNetworkable()->m_basePriority!=-1.0f)
                check(false,"destroyable networkable exists with base priority -1");
        }
    }
    std::printf("[tier3-census] snapshot=%d ikObjects=%u strategicObjects=%u holders=%u destroyables=%u\n",
        snapshots,ikObjects,strategicObjects,unsigned(holders->size()),destroyables);
    check(ikObjects>0 && destroyables>0,"stock objects exercise IK and RW-2");
    check(strategicObjects==0 && holders->empty(),"dedicated startup omits retail HudCompModule");
}
}
bool contentSetup()
{
    std::ifstream input(std::getenv("BF2_TIER3_EXPECTATIONS"));
    check(bool(input),"stock expectations opened");
    std::string line;
    unsigned ikValues=0, strategicValues=0;
    while (std::getline(input,line))
    {
        std::istringstream fields(line);
        std::string kind,name,property,value;
        std::getline(fields,kind,'\t'); std::getline(fields,name,'\t');
        std::getline(fields,property,'\t'); std::getline(fields,value);
        IObjectTemplate* objectTemplate=objectTemplateManager->getTemplate(name);
        if (!objectTemplate) continue; // Unused templates may have been pruned after load.
        if (kind=="ik")
        {
            auto ik=static_cast<IIkTemplateComponent*>(objectTemplate->queryComponent(IID_IIkTemplateComponent));
            if (!ik) { check(false,(name+" has authored IK component").c_str()); continue; }
            Vec3 expected; std::istringstream(value)>>expected;
            bool ok=false;
            if (property=="positionLeft") ok=close(ik->getPositionLeft(),expected);
            if (property=="positionRight") ok=close(ik->getPositionRight(),expected);
            if (property=="poleVectorLeft") ok=close(ik->getPoleVectorLeft(),expected);
            if (property=="poleVectorRight") ok=close(ik->getPoleVectorRight(),expected);
            if (property=="rotationLeft" || property=="rotationRight")
            {
                Mat4 matrix=Mat4::getIdentity(); setRotationZXY(matrix,-expected.z,-expected.y,-expected.x);
                const Mat4& actual=property=="rotationLeft"?ik->getRotationLeftMat():ik->getRotationRightMat();
                ok=true; for (int i=0;i<16;++i) ok &= std::fabs(matrix.m[i]-actual.m[i])<0.0001f;
            }
            check(ok,(name+" ik."+property).c_str()); ++ikValues;
        }
        else
        {
            auto strategic=static_cast<StrategicObjectCompTemplate*>(objectTemplate->queryComponent(IID_IStrategicObjectCompTemplate));
            // Both retail dedicated initModules bodies omit HudCompModule. Its
            // absence here is expected; the strict fixture tests its recovered code.
            if (!strategic) { ++strategicValues; continue; }
            check(false,"unexpected dedicated HUD component publication");
            // Read back through the production console registry as well as the typed component.
            auto saved=objectTemplateManager->getActiveObjectTemplate();
            objectTemplateManager->setActiveObjectTemplate(objectTemplate);
            std::string reply;
            io::mainConsole->executeLine("ObjectTemplate.strategicObject."+property,reply,false,false,false,false,false);
            objectTemplateManager->setActiveObjectTemplate(saved);
            check(reply==value,(name+" strategicObject."+property+" = "+reply).c_str()); ++strategicValues;
        }
    }
    std::printf("[tier3-properties] ik=%u omittedHud=%u\n",ikValues,strategicValues);
    check(ikValues>=6 && strategicValues>0,"IK values retained; dedicated HUD omission observed");
    snapshot();
    return passed;
}
void contentFrame(int frame)
{
    const unsigned tick=WorldPref::getTickCount();
    if (frame>360 && tick<previousTick) sawTickReset=true;
    previousTick=tick;
}
void contentPriority(IObject* object, io::INetworkable* networkable)
{
    ++priorityCalls;
    if (!networkable) ++missingPriorities;
    if (priorityCalls==1 || !networkable)
        std::printf("[tier3-priority] call=%u object=%s networkable=%p\n",priorityCalls,
            object->getTemplate()->getName().c_str(),networkable);
}
bool contentResult()
{
    snapshot();
    std::printf("[tier3-priority] calls=%u missing=%u\n",priorityCalls,missingPriorities);
    check(priorityCalls>0 && missingPriorities==0,"every reached destroyable priority store has a networkable");
    if (std::getenv("BF2_TIER3_RELOAD"))
        check(sawTickReset && priorityCalls>4 && gameLogic->isLevelLoaded()
              && gameLogic->getGameStatus()==1,"requested same-map reload completed and resumed play ticks");
    return passed;
}
