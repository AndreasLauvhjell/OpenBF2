// [TEMP] [CHOSEN, not recovered] Dedicated x86 contract fixture for triggerable products.
// Real factories, console bindings, objects, manager and bitstream. No sockets or game client.
// Empty world and player registries deliberately isolate authored motion and wire behavior.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/ConsoleObjectMacro.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Core/Modules.h"
#include "../BF2/Game/Objects/Game/Triggerable.h"
#include "../BF2/Game/Objects/Game/TriggerableManager.h"
#include "../BF2/Game/Objects/TemplateLeaves.h"
#include "../BF2/Game/Objects/Fx/EffectBundleTemplate.h"
#include "../BF2/Game/Objects/Networkables/Networkables.h"
#include "../BF2/IO/BitStream.h"
#include "../BF2/IO/Console/Console.h"
#include "../BF2/IO/Stream/MemStream.h"
#include "../BF2/Physics/ResponsePhysicsManager.h"
#include "../BF2/Scene/Object/BinTreeCuller.h"
#include "../BF2/Scene/Object/QuadTreeCuller.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
namespace dice { namespace hfe { extern ClassManager* classManager;
namespace world { extern ResponsePhysicsManager* responsePhysicsManager; } } }
using namespace dice::hfe;
using namespace dice::hfe::world;
namespace {
int checks=0, failures=0;
void check(bool ok,const char* label)
{
    ++checks; if (!ok) ++failures;
    std::printf("%s %s\n",ok?"PASS":"FAIL",label); std::fflush(stdout);
}
LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process=GetCurrentProcess(); SymInitialize(process,0,TRUE);
    std::fprintf(stderr,"Fixture exception %08lx at %p\n",fault->ExceptionRecord->ExceptionCode,
                 fault->ExceptionRecord->ExceptionAddress);
    CONTEXT context=*fault->ContextRecord; STACKFRAME64 frame={};
    frame.AddrPC.Offset=context.Eip; frame.AddrFrame.Offset=context.Ebp; frame.AddrStack.Offset=context.Esp;
    frame.AddrPC.Mode=frame.AddrFrame.Mode=frame.AddrStack.Mode=AddrModeFlat;
    for (int i=0;i<14;++i)
    {
        char storage[sizeof(SYMBOL_INFO)+MAX_SYM_NAME]={};
        SYMBOL_INFO* symbol=reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct=sizeof(SYMBOL_INFO); symbol->MaxNameLen=MAX_SYM_NAME;
        DWORD64 displacement=0;
        if (SymFromAddr(process,frame.AddrPC.Offset,&displacement,symbol))
            std::fprintf(stderr,"  %08llx %s+%llu\n",frame.AddrPC.Offset,symbol->Name,displacement);
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386,process,GetCurrentThread(),&frame,&context,
                         0,SymFunctionTableAccess64,SymGetModuleBase64,0)) break;
    }
    std::fflush(stderr); return EXCEPTION_EXECUTE_HANDLER;
}
class FixtureClasses : public ClassManager
{
public:
    std::vector<ClassRegInfo> entries;
    FixtureClasses()
    {
        GameLogicObjTemplBFModule().getClassList(entries);
        GameLogicObjTemplBaseModule().getClassList(entries);
        ObjTemplBaseModule().getClassList(entries);
    }
    IBase* createInstance(unsigned int cid,unsigned int iid,IBase* outer) override
    {
        if (cid==CID_BinTreeCuller) return BinTreeCuller::create(iid,outer);
        if (cid==CID_QuadTreeCuller) return QuadTreeCuller::create(iid,outer);
        for (const auto& entry:entries) if (entry.classId==cid) return entry.create(iid,outer);
        return 0; // [TEMP] Models absent unrelated classes only.
    }
    bool getClassName(unsigned int cid,std::string& name) override
    {
        for (const auto& entry:entries) if (entry.classId==cid) { name=entry.name;return true; }
        return false;
    }
};
ConsoleObject* binding(const char* name)
{
    for (ConsoleObject* object:consoleBindingList("world.objTemplGame"))
        if (std::strcmp(object->getFunctionName(),name)==0 && object->isActive()) return object;
    return 0;
}
std::string set(const char* name,const char* value)
{
    ConsoleObject* object=binding(name);
    if (!object) { check(false,name); return "missing"; }
    std::string argument(value); return object->execute(&argument,1);
}
std::string get(const char* name)
{
    ConsoleObject* object=binding(name); return object?object->execute(0,0):"missing";
}
bool vectorIs(const Vec3& v,float x,float y,float z)
{ return std::fabs(v.x-x)<0.002f && std::fabs(v.y-y)<0.002f && std::fabs(v.z-z)<0.002f; }
struct Property { const char* name; const char* type; const char* initial; const char* authored; };
class ScriptStream : public io::MemStream
{
public:
    std::vector<unsigned int> writes;
    bool shortWrite=false;
    ScriptStream() { setSize(8192); }
    unsigned int write(const void* data,unsigned int size) override
    {
        writes.push_back(size);
        if (shortWrite && size) return size-1; // [TEMP] Deliberate short-write failure condition.
        return MemStream::write(data,size);
    }
    std::string text() { return std::string(static_cast<const char*>(getPtr()),getPosition()); }
    io::IStream* stream() { return static_cast<io::IMemStream*>(this); }
};
void author(const Property* rows,unsigned int count)
{
    for (unsigned int i=0;i<count;++i)
    {
        const Property& row=rows[i]; ConsoleObject* object=binding(row.name);
        check(object && object->getType()==1 && object->getMinNoArgs()==0 && object->getMaxNoArgs()==1
              && std::strcmp(object->getReturnType(),row.type)==0 && !object->hasRange(),row.name);
        check(get(row.name)==row.initial,"retail property default");
        check(set(row.name,row.authored).empty() && get(row.name)==row.authored,"authored property roundtrip");
    }
}
}
int main()
{
    static_assert(sizeof(void*)==4,"Dedicated x86 required");
    static_assert(sizeof(TriggerableTargetNetworkable::Record)==40,"Four-record wire history stride");
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(reportFault); _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    eventManager=new EventManager(); responsePhysicsManager=new ResponsePhysicsManager();
    classManager=new FixtureClasses(); io::mainConsole=new io::OldConsole();
    objectTemplateManager=new ObjectTemplateManager(); objectManager=new ObjectManager();
    playerManager=new PlayerManager(); triggerableManager=new TriggerableManager();
    auto* triggerTemplate=static_cast<TriggerableTemplate*>(
        objectTemplateManager->createTemplate(CID_TriggerableTemplate,"trigger_fixture"));
    auto* targetTemplate=static_cast<TriggerableTargetTemplate*>(
        objectTemplateManager->createTemplate(CID_TriggerableTargetTemplate,"target_fixture"));
    check(triggerTemplate && targetTemplate,"both real template factories create their concrete products");
    if (!triggerTemplate || !targetTemplate) return 1;
    check(targetTemplate->getId()==triggerTemplate->getId()+1,"template IDs advance once per real factory product");
    check(targetTemplate->getNetworkableInfo()=="TriggerableTargetInfo"
          && triggerTemplate->getNetworkableInfo()=="TriggerableInfo","retail default networkable info names");
    const Property targetProperties[]={
        {"movementSpeed","float","1","2"},{"movementType","int","0","0"},
        {"clockwise","bool","0","1"},{"type","int","0","7"},
        {"delayBeforeStart","float","0","1"},{"delayBeforeCycle","float","0","0.5"},
        {"delayBeforeEnd","float","0","100"},{"cycles","int","1","2"}};
    author(targetProperties,sizeof(targetProperties)/sizeof(*targetProperties));
    objectTemplateManager->setActiveObjectTemplate(triggerTemplate);
    const Property triggerProperties[]={
        {"radius","float","5","8"},{"triggerId","int","-1","55"},
        {"events","int","3","1"},{"chance","float","100","0.25"},
        {"averageTimeBetweenTests","float","10","4"},{"minimumTimeBetweenTriggering","float","1","2"},
        {"triggerForEachActivatingPlayer","bool","0","1"},{"notCycledIfOccupied","bool","0","1"},
        {"hasPartner","bool","0","1"}};
    author(triggerProperties,sizeof(triggerProperties)/sizeof(*triggerProperties));
    set("chance","-2");check(get("chance")=="0","chance clamps its lower bound");
    set("chance","2");check(get("chance")=="1","chance clamps its upper bound");
    ConsoleObject* add=binding("addTriggerableTarget");
    check(add && add->getType()==0 && add->getMinNoArgs()==1 && add->getMaxNoArgs()==1
          && std::strcmp(add->getReturnType(),"void")==0,"target-name authoring is a one-string method");
    set("addTriggerableTarget","target_fixture");
    set("addTriggerableTarget","absent_target");
    check(triggerTemplate->m_targetNames.size()==2 && triggerTemplate->m_targetNames.front()=="target_fixture",
          "target authoring preserves insertion order and unresolved names");
    Triggerable* trigger=static_cast<Triggerable*>(triggerTemplate->createObject());
    TriggerableTarget* target=static_cast<TriggerableTarget*>(targetTemplate->createObject());
    check(trigger && target && target->getName()=="target_fixture","real runtime factories preserve template identity and target name");
    trigger->init(); target->init();
    check(triggerableManager->getTriggerableTargetByName("target_fixture")==target,"target init publishes the real object in the manager");
    check(!trigger->isOccupied() && !trigger->checkTriggerableArea(),"empty player registry cannot activate or occupy the trigger");
    check(trigger->isInArea(Vec3{7,0,0}) && !trigger->isInArea(Vec3{8,0,0}),"trigger radius boundary is strictly excluded");
    check(trigger->getTargetMovementType()==0,"trigger resolves its first target's authored movement type");
    target->setAbsolutePosition(Vec3{0,0,0}); target->setAbsolutePositionSecondary(Vec3{4,0,0});
    check(!trigger->handleTrigger(0.25f) && !trigger->allTargetsStopped(),"first trigger update starts the authored cycle");
    trigger->handleTrigger(0.5f);check(vectorIs(target->getAbsolutePosition(),0,0,0),"start delay prevents early motion");
    trigger->handleTrigger(0.5f); trigger->handleTrigger(0.5f);
    check(vectorIs(target->getAbsolutePosition(),1,0,0),"translation uses authored speed after the start delay");
    trigger->handleTrigger(10);check(vectorIs(target->getAbsolutePosition(),4,0,0),"large movement step clamps to the first endpoint");
    check(!trigger->handleTrigger(0.5f),"remaining cycle waits through its transition");
    trigger->handleTrigger(0.5f);trigger->handleTrigger(1);
    check(vectorIs(target->getAbsolutePosition(),2,0,0),"second cycle reverses direction");
    trigger->handleTrigger(10);check(vectorIs(target->getAbsolutePosition(),0,0,0),"second cycle returns exactly to its start");
    check(trigger->handleTrigger(0.5f) && trigger->allTargetsStopped(),"retail completion does not wait out delayBeforeEnd");
    trigger->refresh();check(vectorIs(target->getAbsolutePosition(),0,0,0)
        && vectorIs(target->getAbsolutePositionSecondary(),4,0,0),"refresh restores authored transform endpoints");
    check(!trigger->handleTrigger(0.1f),"refreshed target starts another cycle");
    trigger->resetTriggerTargets();check(trigger->allTargetsStopped(),"reset marks targets stopped before the next update");
    trigger->handleTrigger(0.1f);trigger->handleTrigger(0.1f);

    auto* writer=static_cast<TriggerableTargetNetworkable*>(
        static_cast<io::INetworkable*>(TriggerableTargetNetworkable::create(io::IID_INetworkable,0)));
    check(writer && writer->getRef()==1 && writer->queryInterface(io::IID_INetworkableObject)
          && !writer->queryInterface(0xffffffffu),"networkable factory exposes both real interfaces with one reference");
    check(!TriggerableTargetNetworkable::create(0xffffffffu,0)
          && !TriggerableTargetNetworkable::create(io::IID_INetworkable,static_cast<io::INetworkable*>(writer)),"factory rejects unknown interfaces and aggregation");
    writer->init(target);
    unsigned int mask=0; writer->updateStateMask(&mask);
    check(mask==15 && writer->getGhostStateMask()==15 && writer->getMinimumBitSize()==100,
          "first snapshot forces the four retail fields and retains the shipped minimum estimate");
    mask=0;writer->updateStateMask(&mask);check(mask==0,"unchanged target produces no dirty fields");
    target->setPos(Vec3{1.25f,-2,3.5f}); mask=0;writer->updateStateMask(&mask);
    check(mask==1,"position change dirties only the position field");
    unsigned char bytes[128]={}; io::BitStream stream(bytes,sizeof(bytes));
    check(writer->getNetUpdate(stream,0,15,0) && stream.getBitPosition()==214,
          "uncompressed full update is exactly 4+98+98+7+7 bits");
    stream.setReadMode();unsigned int wireMask=0,selector=1,state=0,cycles=0;
    stream.readBits(&wireMask,4);stream.readBits(&selector,2);float x=0,y=0,z=0;
    stream.readBits(&x,32);stream.readBits(&y,32);stream.readBits(&z,32);
    check(wireMask==15 && selector==0 && vectorIs(Vec3{x,y,z},1.25f,-2,3.5f),"wire header and raw XYZ follow retail field order");
    stream.readBits(&selector,2);stream.readBits(&x,32);stream.readBits(&y,32);stream.readBits(&z,32);
    stream.readBits(&state,7);stream.readBits(&cycles,7);
    check(state==3 && cycles==3 && !stream.hasError(),"wire state and cycles use the seven-bit plus-one encoding");
    TriggerableTarget* replica=static_cast<TriggerableTarget*>(targetTemplate->createObject());
    TriggerableTargetNetworkable* reader=new TriggerableTargetNetworkable();reader->init(replica);
    io::BitStream incoming(bytes,sizeof(bytes));incoming.setBitSize(214);incoming.setReadMode();
    reader->setNetUpdate(incoming,12.5f,true,0,0);
    check(!incoming.hasError() && vectorIs(replica->getAbsolutePosition(),1.25f,-2,3.5f),"received full update applies the actual target transform");
    check(reader->getIsUpdateValid(0),"received first baseline becomes valid");
    stream.clear();stream.setWriteMode();stream.setCompressionVector(Vec3{0,0,0});
    check(writer->getNetUpdate(stream,0,15,0) && stream.getBitPosition()<214,"compression-reference branch emits a smaller valid update");
    stream.setReadMode();reader->setNetUpdate(stream,13,false,0,0);
    check(!stream.hasError() && vectorIs(replica->getAbsolutePosition(),1.25f,-2,3.5f),"compressed update roundtrips through the real decoder");
    // External 18-bit payload: mask12, encoded state0 and cycles0 are both the -1 sentinel.
    unsigned char sentinelBytes[3]={0x0c,0,0};io::BitStream sentinels(sentinelBytes,sizeof(sentinelBytes));
    sentinels.setBitSize(18);sentinels.setReadMode();reader->setNetUpdate(sentinels,14,false,0,0);
    mask=0;reader->updateStateMask(&mask);
    check(mask==12,"sentinel baseline differs from the preserved live state and cycle count");
    stream.clear();stream.setWriteMode();reader->getNetUpdate(stream,0,12,0);stream.setReadMode();
    wireMask=state=cycles=0;stream.readBits(&wireMask,4);stream.readBits(&state,7);stream.readBits(&cycles,7);
    check(wireMask==12 && state==3 && cycles==3,"incoming -1 sentinels do not overwrite live target fields");
    check(writer->release()==0 && reader->release()==0,"networkable release destroys the allocated history records");

    trigger->refresh();targetTemplate->m_movementType=1;targetTemplate->m_movementSpeed=45;
    targetTemplate->m_cycles=1;targetTemplate->m_delayBeforeStart=0;targetTemplate->m_delayBeforeCycle=0;
    target->setRot(Vec3{0,0,0});target->setRotationSecondary(Vec3{90,0,0});
    trigger->handleTrigger(0);trigger->handleTrigger(0);trigger->handleTrigger(0.5f);
    float rz=0,rx=0,ry=0;getRotationZXY(target->getAbsoluteTransformation(),rz,rx,ry);
    check(vectorIs(Vec3{-ry,-rx,-rz},22.5f,0,0),"rotation follows authored degrees-per-second and reversed ZXY storage");
    trigger->handleTrigger(0.5f);getRotationZXY(target->getAbsoluteTransformation(),rz,rx,ry);
    check(vectorIs(Vec3{-ry,-rx,-rz},45,0,0),"rotation continues through the intermediate orientation");
    trigger->handleTrigger(0.5f);getRotationZXY(target->getAbsoluteTransformation(),rz,rx,ry);
    check(vectorIs(Vec3{-ry,-rx,-rz},90,0,0),"retail rotation snaps when post-step distance is within one further step");
    check(trigger->handleTrigger(0),"rotation cycle reaches completion through the normal target manager path");
    trigger->refresh();getRotationZXY(target->getAbsoluteTransformation(),rz,rx,ry);
    check(vectorIs(Vec3{-ry,-rx,-rz},0,0,0) && vectorIs(target->getRotationSecondary(),90,0,0),
          "rotation refresh restores both saved orientations");

    BundleTemplate::TemplateInfo child("target_fixture");
    targetTemplate->updateFlags(0,0xffffffffu);
    ScriptStream rejected;
    check(!triggerTemplate->makeMemberScript(&child,rejected.stream()) && rejected.writes.empty(),
          "bundle member writer rejects a template without any save flag");
    targetTemplate->updateFlags(0x80000000u,0);
    check(!triggerTemplate->makeMemberScript(&child,rejected.stream()),"member save gate tests bit7, not signed32 flags");
    targetTemplate->updateFlags(0x80u,0xffffffffu^0x80u);
    ScriptStream plain;
    check(triggerTemplate->makeMemberScript(&child,plain.stream())
          && plain.text()=="ObjectTemplate.addTemplate target_fixture\n" && plain.writes.size()==1,
          "identity member emits one exact addTemplate line in one write");
    child.m_transformation.m[12]=0.001f;child.m_transformation.m[13]=-0.001f;
    ScriptStream threshold;triggerTemplate->makeMemberScript(&child,threshold.stream());
    check(threshold.text()==plain.text(),"position values at the signed epsilon thresholds are omitted");
    child.m_transformation.m[12]=1.25f;child.m_transformation.m[13]=-2;child.m_transformation.m[14]=3.5f;
    setRotationZXY(child.m_transformation,0,0,-30);
    ScriptStream transformed;
    check(triggerTemplate->makeMemberScript(&child,transformed.stream()),"position and rotation member script succeeds");
    check(transformed.text().find("ObjectTemplate.setPosition 1.25/-2/3.5\n")!=std::string::npos
          && transformed.text().find("ObjectTemplate.setRotation 30/")!=std::string::npos,
          "member script preserves slash XYZ and the reversed-negated rotation convention");
    ScriptStream shortOutput;shortOutput.shortWrite=true;
    check(!triggerTemplate->makeMemberScript(&child,shortOutput.stream()),"member writer reports short writes as false");
    BundleTemplate::TemplateInfo absent("missing_script_child");ScriptStream missing;
    check(!triggerTemplate->makeMemberScript(&absent,missing.stream()) && missing.writes.empty(),
          "unresolved child name returns false without a stream write");
    EffectBundleTemplate* effectTemplate=new EffectBundleTemplate();ScriptStream effectOutput;
    check(effectTemplate->makeMemberScript(&child,effectOutput.stream()) && effectOutput.writes.size()==2
          && effectOutput.writes[1]==0 && effectOutput.text()==transformed.text(),
          "dedicated effect override preserves the successful base output and trailing zero-byte write");
    ScriptStream effectFailure;effectFailure.shortWrite=true;
    check(!effectTemplate->makeMemberScript(&child,effectFailure.stream()) && effectFailure.writes.size()==1,
          "effect override propagates base failure without a trailing write");
    std::printf("triggerable fixture: %d checks, %d failures\n",checks,failures);std::fflush(stdout);
    // World objects survive until process exit; portal teardown and a populated player world are separate gates.
    return failures?1:0;
}
