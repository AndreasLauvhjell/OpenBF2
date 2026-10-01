// [TEMP] Typed byte providers and external source-open responses. All named-input
// decorators, System selection, registry and object readers are production bodies.
#include "../BF2/External/BFMeme/NamedInputStream.h"
#include "../BF2/External/BFMeme/ClassOStream.h"
#include "../BF2/External/BFMeme/OStream.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/EventQueue.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
#include <functional>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
using namespace dice::meme;
static int checks, failures;
#define CHECK(e) do { ++checks; if (!(e)) { ++failures; std::printf("FAIL %d: %s\n",__LINE__,#e); } } while(0)
struct InputLog {
    std::vector<unsigned char> bytes;
    int position=0, destroyed=0, reads=0, skips=0, seeks=0, lastSkip=0;
    std::function<void()> onDestroy;
};
struct Input : IStream {
    InputLog& log;
    explicit Input(InputLog& l) : IStream(nullptr),log(l) {}
    ~Input() override { ++log.destroyed;if(log.onDestroy)log.onDestroy(); }
    int read(void* p,int n) override {
        ++log.reads;n=(std::min)(n,static_cast<int>(log.bytes.size())-log.position);
        if (n) std::memcpy(p,log.bytes.data()+log.position,n);
        log.position+=n;return n;
    }
    int skip(int n) override { ++log.skips;log.lastSkip=n;log.position+=n;return n; }
    int getPosition() override { return log.position; }
    bool setPosition(int n) override { ++log.seeks;log.position=n;return true; }
    int getAvailableBytes() override { return static_cast<int>(log.bytes.size())-log.position; }
};
struct OutputLog {
    std::vector<unsigned char> bytes;
    int writes=0, destroyed=0, position=0, lastCount=-1, writeResult=-1;
    std::function<void()> onDestroy;
};
struct Output : OStream {
    OutputLog& log;
    explicit Output(OutputLog& l) : log(l) {}
    ~Output() override { ++log.destroyed;if(log.onDestroy)log.onDestroy(); }
    int write(const void* p,int n) override {
        ++log.writes;log.lastCount=n;
        if (log.position+n>static_cast<int>(log.bytes.size())) log.bytes.resize(log.position+n);
        if (n) std::memcpy(log.bytes.data()+log.position,p,n);
        log.position+=n;return log.writeResult<0 ? n : log.writeResult;
    }
    int getPosition() override { return log.position; }
    bool setPosition(int n) override { log.position=n;return true; }
};
struct TestSystem : System {
    NameTreeRoot registry;
    InputLog* nextInput=nullptr;
    OStream* replaceOutput=nullptr;
    bool changeOutput=false, rewindOnOpen=false;
    int opens=0;std::string path;
    TestSystem() : System(800,600) { m_registry=&registry; }
    void cache(IStream* input,OStream* output) { m_field04c=input;m_field050=output; }
    IStream* createInputStream(const char* name) override {
        ++opens;path=name;if(changeOutput)m_field050=replaceOutput;
        if(nextInput && rewindOnOpen)nextInput->position=0;
        return nextInput ? new Input(*nextInput) : nullptr;
    }
};
static void literal(std::vector<unsigned char>& out,const char* text) {
    const unsigned char n=static_cast<unsigned char>(std::strlen(text));
    out.push_back(n);out.insert(out.end(),text,text+n);
}
static void teeTests()
{
    InputLog source;source.bytes={1,2,3};OutputLog target;target.writeResult=0;Output output(target);
    { TeeIStream input(new Input(source),&output);char bytes[8]={};
      CHECK(sizeof(TeeIStream)==20 && input.getObjectSize()==8);
      CHECK(input.read(bytes,8)==3 && !std::memcmp(bytes,"\1\2\3",3));
      CHECK(target.bytes==source.bytes && target.writes==1 && target.lastCount==3);
      CHECK(input.read(bytes,1)==0 && target.writes==2 && target.lastCount==0);
      CHECK(input.getPosition()==3 && input.setPosition(1) && source.position==1);
      unsigned char b=0;CHECK(input.streamUByte(&b,"") && b==2 && target.lastCount==1);
    }
    CHECK(source.destroyed==1 && target.destroyed==0);
    InputLog empty;OutputLog emptyTarget;Output emptyOutput(emptyTarget);
    { TeeIStream input(new Input(empty),&emptyOutput);int value=7;
      CHECK(!input.streamInt(&value,"") && value==7 && emptyTarget.lastCount==0);
    }
}
static void relativeTests()
{
    InputLog log;log.bytes={0,1,2,3,4,5,6,7,8,9};log.position=5;Input source(log);
    { RelativeIStream input(&source);
      CHECK(sizeof(RelativeIStream)==24 && input.getObjectSize()==8);
      CHECK(input.getPosition()==0 && input.getAvailableBytes()==5);
      CHECK(input.skip(5)==5 && log.skips==0 && log.position==5 && input.getPosition()==5);
      unsigned char bytes[3]={};CHECK(input.read(bytes,3)==3 && bytes[0]==5 && bytes[2]==7);
      CHECK(input.getPosition()==8 && log.position==8);
      CHECK(input.skip(-7)==-7 && input.getPosition()==1 && log.skips==0);
      CHECK(input.setPosition(2) && log.seeks==1 && log.position==2 && input.getPosition()==-5);
      {
          ForwardingIStream other(&source,false);CHECK(source.getSource()==&other);
          CHECK(input.read(bytes,0)==0 && source.getSource()==&input);
      }
      // The sibling's borrowed destructor leaves its own pointer in the child;
      // getPosition must restore this wrapper before any source-chain access.
      CHECK(input.getPosition()==-5 && source.getSource()==&input);
      { ForwardingIStream other(&source,false);CHECK(input.skip(0)==0 && source.getSource()==&input); }
      input.getPosition();
    }
    CHECK(log.destroyed==0); // Borrowed source survives; its stale source link is not traversed.
}
static void selectionTests()
{
    TestSystem system;InputLog direct;direct.bytes={9};system.nextInput=&direct;
    IStream* plain=system.openNamedInputStream("plain");
    CHECK(plain && system.opens==1 && system.path=="plain");delete plain;CHECK(direct.destroyed==1);
    system.nextInput=nullptr;CHECK(system.openNamedInputStream("missing")==nullptr && system.opens==2);
    InputLog cache;cache.bytes={3,4};Input cacheInput(cache);system.cache(&cacheInput,nullptr);
    IStream* replay=system.openNamedInputStream("ignored");unsigned char b=0;
    CHECK(replay && system.opens==2 && replay->streamUByte(&b,"") && b==3);
    delete replay;CHECK(cache.destroyed==0);
    OutputLog firstLog,secondLog;Output first(firstLog),second(secondLog);
    InputLog original;original.bytes={0xA1};system.nextInput=&original;
    system.cache(&cacheInput,&first);system.changeOutput=true;system.replaceOutput=&second;
    IStream* recording=system.openNamedInputStream("recorded");
    CHECK(recording && system.opens==3 && system.path=="recorded");
    CHECK(recording->streamUByte(&b,"") && b==0xA1 && secondLog.bytes==original.bytes && firstLog.bytes.empty());
    delete recording;CHECK(original.destroyed==1 && cache.destroyed==0 && secondLog.destroyed==0);
    system.changeOutput=false;system.nextInput=nullptr;system.cache(nullptr,&first);
    IStream* missing=system.openNamedInputStream("null-underlying");
    CHECK(missing!=nullptr && system.opens==4);delete missing; // Construction accepts null; never call read.
}
static void frameRecordingTests()
{
    TestSystem system;TypeDescriptor* type=Data::getClassType();const int count=type->getInstanceCount();
    InputLog original;original.bytes.resize(4);literal(original.bytes,"");literal(original.bytes,type->getName());
    const int consumed=static_cast<int>(original.bytes.size());original.bytes.insert(original.bytes.end(),{0xCC,0xDD,0xEE});
    unsigned long size=static_cast<unsigned long>(original.bytes.size());std::memcpy(original.bytes.data(),&size,4);
    OutputLog recorded;Output output(recorded);system.nextInput=&original;system.cache(nullptr,&output);
    Object* object=nullptr;
    { IStream* input=system.openNamedInputStream("framed");
      CHECK(input->streamObject(&system,object,"Object",nullptr) && object && object->getType()==type);
      CHECK(input->getPosition()==static_cast<int>(size) && original.skips==1 && original.lastSkip==3);
      delete input;
    }
    CHECK(recorded.bytes.size()==static_cast<unsigned int>(consumed) && original.destroyed==1);
    CHECK(deleteObject(&system,object) && type->getInstanceCount()==count);
    InputLog compact;compact.bytes=recorded.bytes;Input compactInput(compact);system.cache(&compactInput,nullptr);
    { IStream* input=system.openNamedInputStream("same-frame");
      CHECK(input->streamObject(&system,object,"Object",nullptr) && object && object->getType()==type);
      CHECK(input->getPosition()==static_cast<int>(size) && compact.position==consumed && compact.skips==0);
      delete input;
    }
    CHECK(deleteObject(&system,object) && type->getInstanceCount()==count && compact.destroyed==0);
}
static void namedLoadTests()
{
    TestSystem system;TypeDescriptor* type=Data::getClassType();const int count=type->getInstanceCount();
    OutputLog serialized;
    { ClassOStream output(new Output(serialized));
      CHECK(output.writeShortString(&system,type->getName(),"Class name") && output.finish(&system));
    }
    InputLog original;original.bytes=serialized.bytes;OutputLog cache;Output cacheOutput(cache);
    system.nextInput=&original;system.cache(nullptr,&cacheOutput);Object* value=nullptr;
    CHECK(loadNamedObject(&system,value,"Record/item") && value && system.findNamedObject("Record/item")==value);
    CHECK(cache.bytes==serialized.bytes && original.destroyed==1 && type->getInstanceCount()==count+1);
    CHECK(system.registry.eraseObject(&system,value));RuntimeObject* runtime=value;
    CHECK(destroyRuntimeObject(runtime) && type->getInstanceCount()==count);value=nullptr;
    InputLog replay;replay.bytes=cache.bytes;replay.bytes.insert(replay.bytes.end(),cache.bytes.begin(),cache.bytes.end());
    Input replayInput(replay);system.cache(&replayInput,nullptr);const int opens=system.opens;
    CHECK(loadNamedObject(&system,value,"Replay/first") && value);
    CHECK(replay.position==static_cast<int>(cache.bytes.size()) && system.opens==opens);
    CHECK(system.registry.eraseObject(&system,value));runtime=value;CHECK(destroyRuntimeObject(runtime));value=nullptr;
    CHECK(loadNamedObject(&system,value,"Replay/second") && value && replay.position==static_cast<int>(replay.bytes.size()));
    CHECK(system.registry.eraseObject(&system,value));runtime=value;
    CHECK(destroyRuntimeObject(runtime) && type->getInstanceCount()==count && replay.destroyed==0);
    // An old-format file is reopened after version rejection. Recording keeps
    // both reads; replay consumes the corresponding two cache spans in order.
    InputLog legacy;literal(legacy.bytes,type->getName());OutputLog legacyCache;Output legacyOutput(legacyCache);
    system.nextInput=&legacy;system.rewindOnOpen=true;system.cache(nullptr,&legacyOutput);value=nullptr;
    const int legacyOpens=system.opens;
    CHECK(loadNamedObject(&system,value,"Legacy/record") && value);
    CHECK(system.opens==legacyOpens+2 && legacy.destroyed==2);
    std::vector<unsigned char> twice=legacy.bytes;twice.insert(twice.end(),legacy.bytes.begin(),legacy.bytes.end());
    CHECK(legacyCache.bytes==twice);
    CHECK(system.registry.eraseObject(&system,value));runtime=value;
    CHECK(destroyRuntimeObject(runtime) && type->getInstanceCount()==count);value=nullptr;
    InputLog legacyReplay;legacyReplay.bytes=legacyCache.bytes;Input legacyInput(legacyReplay);
    system.cache(&legacyInput,nullptr);const int beforeReplay=system.opens;
    CHECK(loadNamedObject(&system,value,"Legacy/replay") && value);
    CHECK(system.opens==beforeReplay && legacyReplay.position==static_cast<int>(legacyReplay.bytes.size()));
    CHECK(system.registry.eraseObject(&system,value));runtime=value;
    CHECK(destroyRuntimeObject(runtime) && type->getInstanceCount()==count && legacyReplay.destroyed==0);
}
// [TEMP] Only the node's updateSelf hook supplies the requested failure condition.
// Node construction, Node::update, System::update, NodeState and empty queue
// processing are real. There is no Node descriptor override or factory.
struct MenuNode : Node {
    bool result=true;int calls=0;float delta=123.0f;std::vector<std::string>* trace=nullptr;
    bool updateSelf(System*,const NodeState*,const Rect*,float d) override {
        ++calls;delta=d;if(trace)trace->push_back("update");return result;
    }
};
struct MenuSystem : TestSystem {
    MenuNode node;EventQueue queue;
    InputLog cacheLog,mainLog;OutputLog outputLog;
    bool cacheReadable=false,cacheWritable=false,mainReadable=true;
    int cacheOpens=0,mainOpens=0,outputOpens=0,inputDeletes=0,outputDeletes=0;
    IStream* openedCache=nullptr;OStream* openedOutput=nullptr;
    std::vector<std::string> trace;
    MenuSystem() : queue(0) {
        m_field018=&node;m_eventQueue=&queue;node.trace=&trace;
        cacheLog.onDestroy=[this]{trace.push_back("delete-cache-input");};
        mainLog.onDestroy=[this]{trace.push_back("delete-main-input");};
        outputLog.onDestroy=[this]{trace.push_back("delete-cache-output");};
    }
    IStream* createInputStream(const char* name) override {
        trace.push_back(std::string("open-input:")+name);
        if(!std::strcmp(name,"cache")) {
            ++cacheOpens;if(!cacheReadable)return nullptr;
            openedCache=new Input(cacheLog);return openedCache;
        }
        ++mainOpens;return mainReadable ? new Input(mainLog) : nullptr;
    }
    OStream* createOutputStream(const char* name) override {
        ++outputOpens;trace.push_back(std::string("open-output:")+name);
        openedOutput=cacheWritable ? new Output(outputLog) : nullptr;return openedOutput;
    }
    void deleteInputStream(IStream* input) override { ++inputDeletes;System::deleteInputStream(input); }
    void deleteOutputStream(OStream* output) override { ++outputDeletes;System::deleteOutputStream(output); }
    IStream* cacheInput() const { return m_field04c; }
    OStream* cacheOutput() const { return m_field050; }
    Node* root() const { return m_field018; }
    void replaceCacheOutput(OStream* output) { m_field050=output; }
};
static void removeLoaded(MenuSystem& system,const char* path,TypeDescriptor* type,int count)
{
    Object* value=system.findNamedObject(path);CHECK(value && value->getType()==type);
    CHECK(system.registry.eraseObject(&system,value));RuntimeObject* runtime=value;
    CHECK(destroyRuntimeObject(runtime) && type->getInstanceCount()==count);
}
static void menuLoadTests()
{
    TypeDescriptor* type=Data::getClassType();const int count=type->getInstanceCount();
    OutputLog encoded;
    { ClassOStream output(new Output(encoded));
      CHECK(output.writeShortString(nullptr,type->getName(),"Class name") && output.finish(nullptr));
    }
    {
        MenuSystem system;system.cacheReadable=true;system.cacheLog.bytes=encoded.bytes;
        CHECK(system.loadMenu("Menu/replay","cache"));
        CHECK(system.cacheOpens==1 && system.mainOpens==0 && system.outputOpens==0);
        CHECK(system.node.calls==1 && system.node.delta==0.0f && system.root()==&system.node);
        CHECK(system.cacheLog.destroyed==1 && system.cacheInput()==system.openedCache && !system.cacheOutput());
        CHECK(system.inputDeletes==0 && system.outputDeletes==0);
        CHECK(system.trace==std::vector<std::string>({"open-input:cache","update","delete-cache-input"}));
        removeLoaded(system,"Menu/replay",type,count);
    }
    {
        MenuSystem system;system.cacheWritable=true;system.mainLog.bytes=encoded.bytes;
        CHECK(system.loadMenu("Menu/record","cache"));
        CHECK(system.cacheOpens==1 && system.outputOpens==1 && system.mainOpens==1);
        CHECK(system.outputLog.bytes==encoded.bytes && system.outputLog.destroyed==1 && system.mainLog.destroyed==1);
        CHECK(!system.cacheInput() && system.cacheOutput()==system.openedOutput);
        CHECK(system.node.calls==1 && system.node.delta==0.0f && system.inputDeletes==0 && system.outputDeletes==0);
        CHECK(system.trace==std::vector<std::string>({"open-input:cache","open-output:cache","open-input:Menu/record","delete-main-input","update","delete-cache-output"}));
        removeLoaded(system,"Menu/record",type,count);
    }
    {
        MenuSystem system;system.mainLog.bytes=encoded.bytes;
        CHECK(system.loadMenu("Menu/fallback","cache"));
        CHECK(system.cacheOpens==1 && system.outputOpens==1 && system.mainOpens==1);
        CHECK(!system.cacheInput() && !system.cacheOutput() && system.node.calls==1 && system.node.delta==0.0f);
        CHECK(system.mainLog.destroyed==1 && system.outputLog.destroyed==0);
        removeLoaded(system,"Menu/fallback",type,count);
    }
    {
        MenuSystem system;system.mainLog.bytes=encoded.bytes;
        CHECK(system.loadMenu("Menu/plain",nullptr));
        CHECK(system.cacheOpens==0 && system.outputOpens==0 && system.mainOpens==1 && system.node.calls==1);
        CHECK(system.mainLog.destroyed==1 && system.root()==&system.node);
        removeLoaded(system,"Menu/plain",type,count);
    }
    {
        MenuSystem system;system.cacheWritable=true;system.mainLog.bytes=encoded.bytes;system.node.result=false;
        CHECK(!system.loadMenu("Menu/update-failure","cache"));
        CHECK(system.node.calls==1 && system.node.delta==0.0f && system.outputLog.destroyed==1 && system.mainLog.destroyed==1);
        CHECK(type->getInstanceCount()==count+1); // Failed update does not roll back the loaded object.
        removeLoaded(system,"Menu/update-failure",type,count);
    }
    {
        MenuSystem system;system.mainReadable=false;
        CHECK(!system.loadMenu("Menu/read-failure",nullptr));
        CHECK(system.mainOpens==1 && system.node.calls==1 && system.node.delta==0.0f);
        CHECK(system.root()==&system.node && type->getInstanceCount()==count);
    }
    {
        MenuSystem system;system.cacheReadable=true;system.cacheLog.bytes=encoded.bytes;
        CHECK(!system.loadMenu(nullptr,"cache")); // Main-path failure still updates and cleans cache input.
        CHECK(system.mainOpens==0 && system.node.calls==1 && system.cacheLog.destroyed==1);
        CHECK(system.cacheInput()==system.openedCache && system.inputDeletes==0);
    }
    {
        MenuSystem system;Data existing("item");CHECK(system.registry.insert(&system,&existing,"Existing"));
        InputLog inputLog;OutputLog oldLog,replacementLog;
        Input* input=new Input(inputLog);Output* oldOutput=new Output(oldLog);Output* replacement=new Output(replacementLog);
        inputLog.onDestroy=[&]{system.trace.push_back("delete-retained-input");system.replaceCacheOutput(replacement);};
        replacementLog.onDestroy=[&]{system.trace.push_back("delete-replacement-output");};
        system.cache(input,oldOutput);
        CHECK(system.loadMenu("Existing/item",nullptr));
        CHECK(system.cacheOpens==0 && system.mainOpens==0 && system.outputOpens==0);
        CHECK(inputLog.destroyed==1 && oldLog.destroyed==0 && replacementLog.destroyed==1);
        CHECK(system.cacheInput()==input && system.cacheOutput()==replacement);
        CHECK(system.inputDeletes==0 && system.outputDeletes==0);
        CHECK(system.trace==std::vector<std::string>({"update","delete-retained-input","delete-replacement-output"}));
        delete oldOutput;CHECK(oldLog.destroyed==1); // It was replaced during cleanup and never deleted by loadMenu.
        CHECK(system.registry.eraseObject(&system,&existing));
    }
}
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    teeTests();relativeTests();selectionTests();frameRecordingTests();namedLoadTests();menuLoadTests();
    std::printf("BFMeme named input: %d/%d checks passed\n",checks-failures,checks);
    return failures ? 1 : 0;
}
