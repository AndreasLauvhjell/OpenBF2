// [TEMP] Adds real collection and small-family coverage to the existing class
// transport fixture. No production virtual or descriptor body is replaced.
#define main class_stream_regression_main
#include "bfmeme_class_stream_runtime.cpp"
#undef main
#include "../BF2/External/BFMeme/Event.h"
#include "../BF2/External/BFMeme/Action.h"
#include "../BF2/External/BFMeme/Function.h"
#include "../BF2/External/BFMeme/Node.h"
#include <new>

struct LinkFixture {
    Object* object;
    LinkFixture* next;
    explicit LinkFixture(Object* p=nullptr, LinkFixture* n=nullptr) : object(p), next(n) {}
};
static ObjectCollectionPosition* position(LinkFixture* p) {
    return reinterpret_cast<ObjectCollectionPosition*>(p);
}
static LinkFixture* linkAt(ObjectCollectionPosition* p) {
    return reinterpret_cast<LinkFixture*>(p);
}
struct CollectionFixture : ObjectCollection {
    LinkFixture* head=nullptr;
    int firstCalls=0, nextCalls=0, nullNextCalls=0, removals=0, appendCalls=0;
    bool appendResult=true;
    std::vector<Object*> accepted;
    ObjectCollectionPosition* first() override { ++firstCalls;return position(head); }
    ObjectCollectionPosition* next(ObjectCollectionPosition* p) override {
        ++nextCalls;if(!p) { ++nullNextCalls;return nullptr; } return position(linkAt(p)->next);
    }
    Object* getObject(ObjectCollectionPosition* p) override { return p ? linkAt(p)->object : nullptr; }
    void remove(System*,ObjectCollectionPosition* p) override {
        ++removals;LinkFixture** cursor=&head;
        while(*cursor && *cursor!=linkAt(p)) cursor=&(*cursor)->next;
        if(*cursor) *cursor=(*cursor)->next;
    }
    bool append(System*,Object* p) override { ++appendCalls;accepted.push_back(p);return appendResult; }
};
struct CollectionDispatch : ObjectCollection {
    System* seenSystem=nullptr;IStream* seenInput=nullptr;OStream* seenOutput=nullptr;int reads=0,writes=0;
    bool readObjects(System* s,IStream* in) override { ++reads;seenSystem=s;seenInput=in;return false; }
    bool writeObjects(System* s,OStream* out) override { ++writes;seenSystem=s;seenOutput=out;return false; }
};
struct RepeatedName : Object {
    int names=0;
    const char* getName() override { return ++names==1 ? "first" : "second"; }
};
struct IndexWriter : Stream {
    unsigned char index=0;bool result=true;int calls=0;std::string label;
    bool streamUByte(unsigned char* p,const char* name) override {
        ++calls;label=name;*p=index;return result;
    }
};
struct FamilyVisitor : Stream {
    TypeDescriptor* seen=nullptr;Object** address=nullptr;
    bool streamObject(System*,Object*& p,const char*,TypeDescriptor* type) override {
        seen=type;address=&p;return true;
    }
};
struct LoggingSystem : System {
    OutputLog log;Output output;int requests=0;
    LoggingSystem() : System(0,0),output(log) {}
    OStream* getOutputStream() override { ++requests;return &output; }
};
static void familyTests() {
    TypeDescriptor* types[]={Event::getClassType(),Action::getClassType(),Function::getClassType()};
    const char* names[]={"dice::meme::Event","dice::meme::Action","dice::meme::Function"};
    const unsigned sizes[]={8,4,8};
    for(int i=0;i<3;++i) {
        TypeDescriptor* type=types[i];const int baseline=type->getInstanceCount();
        CHECK(!std::strcmp(type->getName(),names[i]) && !type->getDescription());
        CHECK(type->getBaseType()==Object::getClassType() && type->getInstanceSize()==sizes[i]);
        CHECK(TypeDescriptor::find(names[i])==type && TypeDescriptor::find(type->getShortName())==type);
        RuntimeObject* raw=type->create();
        CHECK(raw && raw->getType()==type && raw->getObjectSize()==sizes[i]);
        CHECK(type->getInstanceCount()==baseline);
        delete raw;CHECK(type->getInstanceCount()==baseline);
        RuntimeObject* counted=nullptr;CHECK(createRuntimeObject(counted,type));
        CHECK(type->getInstanceCount()==baseline+1);
        CHECK(destroyRuntimeObject(counted) && !counted && type->getInstanceCount()==baseline);
        type->removeInstance();CHECK(type->getInstanceCount()==baseline-1);type->addInstance();
    }
    alignas(Event) unsigned char bytes[sizeof(Event)];std::memset(bytes,0xA5,sizeof(bytes));
    Event* predicate=new(bytes) Event;
    CHECK(bytes[4]==0 && bytes[5]==0xA5 && bytes[6]==0xA5 && bytes[7]==0xA5);
    CHECK(predicate->matches(nullptr,nullptr)); // Zero index short-circuits the missing null guard.
    IndexWriter writer;writer.index=255;
    CHECK(predicate->streamProperties(nullptr,&writer) && writer.calls==1);
    CHECK(writer.label=="Input index (0 == all indexes)");
    NodeInputEvent input={0,255,0,0,0};CHECK(predicate->matches(nullptr,&input));
    input.index=511;CHECK(!predicate->matches(nullptr,&input));
    input.index=-1;CHECK(!predicate->matches(nullptr,&input));
    writer.index=2;writer.result=false;CHECK(!predicate->streamProperties(nullptr,&writer));
    input.index=2;CHECK(predicate->matches(nullptr,&input)); // Failed visitor's mutation remains.
    predicate->~Event();
    Action action;CHECK(!action.execute(nullptr,nullptr,nullptr,nullptr));
    char name[]="alpha";Function function(name);name[0]='x';
    CHECK(!std::strcmp(function.getName(),"alpha"));
    LoggingSystem system;CHECK(function.call(&system) && system.requests==1);
    const std::string text(system.log.bytes.begin(),system.log.bytes.end());
    CHECK(text=="Meme: FunctionCall: alpha\r\n");
    CHECK(function.setName("") && function.getName() && *function.getName()==0);
    CHECK(function.call(&system) && system.requests==2);
    CHECK(function.setName(nullptr) && !function.getName() && function.call(nullptr));
    FamilyVisitor visitor;Event* event=nullptr;Action* act=nullptr;Function* fn=nullptr;
    CHECK(visitor.streamEvent(nullptr,event,"event",nullptr) && visitor.seen==types[0]);
    CHECK(visitor.streamAction(nullptr,act,"action",nullptr) && visitor.seen==types[1]);
    CHECK(visitor.streamFunction(nullptr,fn,"function",nullptr) && visitor.seen==types[2]);
    CHECK(visitor.streamEvent(nullptr,event,"explicit",types[2]) && visitor.seen==types[2]);
}
static void collectionTests() {
    ObjectCollection base;Object* nullObject=nullptr;
    CHECK(sizeof(ObjectCollection)==4 && !base.first() && !base.last() && base.size()==0);
    CHECK(!base.previous(nullptr) && base.countBefore(nullptr)==0 && base.countAfter(nullptr)==0);
    CHECK(base.indexOf(nullptr)==-1 && !base.positionAt(-1) && !base.positionAt(0));
    CHECK(!base.getObjectAt(0) && !base.find(nullptr));
    CHECK(base.getObjectType()==Object::getClassType());
    CHECK(!base.setObject(nullptr,nullptr) && !base.append(nullptr,nullptr) && !base.insert(nullptr,nullptr,nullptr));
    base.remove(nullptr,nullptr);CHECK(base.clear(nullptr));
    CHECK(!std::strcmp(base.getObjectName(nullptr,nullptr),"<empty>"));
    Data a("a"),b("b");LinkFixture three(&b),two(nullptr,&three),one(&a,&two),missing;
    CollectionFixture collection;collection.head=&one;
    CHECK(collection.size()==1 && collection.last()==position(&three));
    CHECK(collection.previous(position(&three))==position(&two));
    CHECK(!collection.previous(position(&one)) && !collection.previous(position(&missing)));
    CHECK(collection.countBefore(position(&three))==2 && collection.countAfter(position(&one))==0);
    CHECK(collection.countAfter(position(&two))==1 && collection.countAfter(position(&three))==0);
    CHECK(collection.indexOf(position(&three))==2 && collection.indexOf(position(&missing))==-1);
    CHECK(collection.positionAt(2)==position(&three) && !collection.positionAt(3) && !collection.positionAt(-1));
    CHECK(collection.getObjectAt(2)==&b && collection.find(nullptr)==position(&two));
    CHECK(!std::strcmp(collection.getObjectName(nullptr,position(&one)),"a"));
    RepeatedName named;LinkFixture label(&named);CHECK(!std::strcmp(collection.getObjectName(nullptr,position(&label)),"second") && named.names==2);
    Object anonymous;label.object=&anonymous;CHECK(!std::strcmp(collection.getObjectName(nullptr,position(&label)),"Object"));
    CollectionFixture empty;CHECK(!empty.last() && empty.nullNextCalls==1);
    // Remove a null-payload link and stop immediately, leaving the tail in place.
    CollectionFixture stop;stop.head=&two;
    CHECK(stop.clear(nullptr) && stop.removals==1 && stop.head==&three && stop.firstCalls==1);
    const int baseline=Data::getClassType()->getInstanceCount();RuntimeObject* allocated=nullptr;
    CHECK(createRuntimeObject(allocated,Data::getClassType()));
    LinkFixture owned(static_cast<Object*>(allocated));CollectionFixture clearing;clearing.head=&owned;
    CHECK(clearing.clear(nullptr) && !clearing.head && clearing.removals==1 && clearing.firstCalls==2);
    CHECK(Data::getClassType()->getInstanceCount()==baseline);
    // Real deletion's visitor failure occurs after removal and leaves the object alive.
    const int failedBaseline=FailingData::type()->getInstanceCount();
    RuntimeObject* failure=nullptr;CHECK(createRuntimeObject(failure,FailingData::type()));
    LinkFixture failureLink(static_cast<Object*>(failure),&three);CollectionFixture failing;failing.head=&failureLink;
    CHECK(!failing.clear(nullptr) && failing.head==&three && failing.removals==1 && failing.firstCalls==1);
    CHECK(FailingData::type()->getInstanceCount()==failedBaseline+1);
    CHECK(destroyRuntimeObject(failure) && FailingData::type()->getInstanceCount()==failedBaseline);
    // Real object reader, including ignored append failure and null terminator.
    SavingSystem system;InputLog encoded=frame("",Data::getClassType()->getName());
    InputLog second=frame("",Data::getClassType()->getName());InputLog terminator=frame("","");
    encoded.bytes.insert(encoded.bytes.end(),second.bytes.begin(),second.bytes.end());
    encoded.bytes.insert(encoded.bytes.end(),terminator.bytes.begin(),terminator.bytes.end());
    Input input(encoded);CollectionFixture read;read.appendResult=false;
    CHECK(read.read(&system,&input) && read.appendCalls==2 && read.accepted.size()==2);
    CHECK(Data::getClassType()->getInstanceCount()==baseline+2);
    for(Object*& value:read.accepted) CHECK(deleteObject(&system,value));
    CHECK(Data::getClassType()->getInstanceCount()==baseline);
    // A failed later frame keeps the already accepted objects; no rollback.
    InputLog partial=frame("",Data::getClassType()->getName());partial.bytes.push_back(1);
    Input partialInput(partial);CollectionFixture partialRead;
    CHECK(!partialRead.read(&system,&partialInput) && partialRead.appendCalls==1 && partialRead.accepted.size()==1);
    CHECK(Data::getClassType()->getInstanceCount()==baseline+1);
    CHECK(deleteObject(&system,partialRead.accepted[0]) && Data::getClassType()->getInstanceCount()==baseline);
    InputLog truncated;truncated.bytes={1,0};Input failedInput(truncated);CollectionFixture failedRead;
    CHECK(!failedRead.read(&system,&failedInput) && failedRead.appendCalls==0);
    // A null payload truncates output even when another position follows.
    Data data(nullptr);LinkFixture ignored(&a),hole(nullptr,&ignored),entry(&data,&hole);
    CollectionFixture write;write.head=&entry;OutputLog outputLog;Output output(outputLog);
    CHECK(write.write(&system,&output));
    std::vector<unsigned char> expected=frame("",Data::getClassType()->getName()).bytes;
    expected.insert(expected.end(),terminator.bytes.begin(),terminator.bytes.end());
    CHECK(outputLog.bytes==expected && write.nextCalls==1);
    OutputLog nullLog;Output nullOutput(nullLog);CHECK(base.write(&system,&nullOutput) && nullLog.bytes==terminator.bytes);
    OutputLog rejected;rejected.failCall=1;Output rejectedOutput(rejected);
    CHECK(!write.write(&system,&rejectedOutput) && write.nextCalls==1);
    // OStream/IStream virtual collection wrappers call the actual new helpers.
    OutputLog wrapperBytes;Output wrapperOutput(wrapperBytes);
    CHECK(wrapperOutput.streamCollection(&system,&base,"collection") && wrapperBytes.bytes==terminator.bytes);
    InputLog wrapperInput;wrapperInput.bytes=wrapperBytes.bytes;Input reader(wrapperInput);
    CHECK(reader.streamCollection(&system,&base,"collection"));
    CollectionDispatch dispatch;
    CHECK(!dispatch.read(&system,&reader) && dispatch.reads==1 && dispatch.seenSystem==&system && dispatch.seenInput==&reader);
    CHECK(!dispatch.write(&system,&wrapperOutput) && dispatch.writes==1 && dispatch.seenSystem==&system && dispatch.seenOutput==&wrapperOutput);
}
int main() {
    const int legacy=class_stream_regression_main();
    familyTests();collectionTests();
    std::printf("Stream dependency closure: %d/%d checks passed\n",checks-failures,checks);
    return legacy || failures ? 1 : 0;
}
