// [TEMP] Byte transport, filesystem-open response and serializable-object fixtures.
// Actual file/class streams, System and registry bodies run. No stream descriptor
// or production font/sound/class-reader body is substituted.
#include "../BF2/External/BFMeme/FileOStream.h"
#include "../BF2/External/BFMeme/ClassOStream.h"
#include "../BF2/External/BFMeme/ClassIStream.h"
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/IO/File/FileManager.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
using namespace dice::meme;
namespace io = dice::hfe::io;
namespace dice { namespace hfe { namespace io { FileManager* fileManager; } } }
static int checks, failures;
#define CHECK(expr) do { ++checks; if (!(expr)) { ++failures; std::printf("FAIL %d: %s\n",__LINE__,#expr); } } while (0)
struct Transport : io::INonlinearStream {
    int releases=0, flushes=0, position=0, seekReturn=0, writes=0, origin=-1;
    unsigned int limit=1000;
    std::vector<unsigned char> bytes;
    void addRef() override {}
    int getRef() override { return 1; }
    int release() override { ++releases; return 0; }
    void* queryInterface(unsigned int) const override { return nullptr; }
    unsigned int read(void*,unsigned int) override { return 0; }
    unsigned int write(const void* p,unsigned int n) override {
        ++writes; n=(std::min)(n,limit); const auto* b=static_cast<const unsigned char*>(p);
        bytes.insert(bytes.end(),b,b+n); position+=n; return n;
    }
    int skip(unsigned int) override { return 0; }
    int getAvailableBytes() override { return 0; }
    io::IStream* clone() override { return nullptr; }
    bool canRead() override { return false; }
    bool canWrite() override { return true; }
    void flushWrite() override { ++flushes; }
    int seek(io::SeekOrigin o,int p) override { origin=o;position=p;return seekReturn; }
    int getPosition() override { return position; }
    int getSize() override { return static_cast<int>(bytes.size()); }
};
struct Files : io::FileManager {
    Transport* result;
    int opens=0, mode=-1, access=-1; unsigned int flags=1; std::string name;
    io::IStream* open(const std::string& n,FileMode m,FileAccess a,unsigned int f) override {
        ++opens; name=n;mode=m;access=a;flags=f;return result;
    }
};
struct OutputLog {
    std::vector<unsigned char> bytes;
    int destroyed=0, flushes=0, calls=0, failCall=-1;
};
struct Output : OStream {
    OutputLog& log; int position=0;
    explicit Output(OutputLog& l) : log(l) {}
    ~Output() override { ++log.destroyed; }
    int write(const void* p,int n) override {
        if (++log.calls == log.failCall) return 0;
        if (position+n>static_cast<int>(log.bytes.size())) log.bytes.resize(position+n);
        if(n) std::memcpy(log.bytes.data()+position,p,n);position+=n;return n;
    }
    int getPosition() override { return position; }
    bool setPosition(int p) override { position=p;return true; }
    void flush() override { ++log.flushes; }
};
struct InputLog { std::vector<unsigned char> bytes; int destroyed=0, position=0, skips=0, lastSkip=0; };
struct Input : IStream {
    InputLog& log;
    explicit Input(InputLog& l) : IStream(nullptr),log(l) {}
    ~Input() override { ++log.destroyed; }
    int read(void* p,int n) override {
        n=(std::min)(n,static_cast<int>(log.bytes.size())-log.position);
        if(n) std::memcpy(p,log.bytes.data()+log.position,n);log.position+=n;return n;
    }
    int getAvailableBytes() override { return static_cast<int>(log.bytes.size())-log.position; }
    int skip(int n) override { ++log.skips;log.lastSkip=n;log.position+=n;return n; }
    int getPosition() override { return log.position; }
    bool setPosition(int p) override { log.position=p;return true; }
};
static void literal(std::vector<unsigned char>& out,const char* value) {
    const unsigned char length=static_cast<unsigned char>(std::strlen(value));
    out.push_back(length);out.insert(out.end(),value,value+length);
}
static void fileTests()
{
    // FileManager's destructor is an unrelated frontier. As in the existing
    // font fixture, keep this constructor-only provider for the process lifetime.
    Transport device; Files& files=*new Files;files.result=&device;io::fileManager=&files;
    {
        FileOStream output("menu/test");
        CHECK(sizeof(FileOStream)==8 && output.getObjectSize()==4);
        CHECK(files.opens==1 && files.name=="menu/test" && files.mode==3 && files.access==2 && files.flags==0);
        device.limit=3;CHECK(output.write("ABCDE",5)==3 && device.bytes==std::vector<unsigned char>({'A','B','C'}));
        CHECK(output.getPosition()==3);
        device.seekReturn=256;CHECK(!output.setPosition(8) && device.origin==0 && device.position==8);
        device.seekReturn=257;CHECK(output.setPosition(9) && device.position==9);
        output.flush();CHECK(device.flushes==0); // Actual inherited empty OStream flush.
    }
    CHECK(device.releases==1);
    files.result=nullptr;
    { FileOStream failed("unavailable"); CHECK(failed.getObjectSize()==4 && files.opens==2); }
    CHECK(device.releases==1);io::fileManager=nullptr;
}
static void classTests()
{
    MemoryOStream memory(4);
    CHECK(sizeof(MemoryOStream)==16 && memory.getObjectSize()==4 && memory.getPosition()==0);
    CHECK(memory.write("abcdef",6)==4 && memory.getPosition()==4);
    CHECK(!memory.setPosition(4) && memory.getPosition()==4);
    CHECK(!memory.setPosition(-1) && memory.getPosition()==4);
    CHECK(memory.setPosition(2) && memory.write("XYZ",3)==2 && memory.getPosition()==4);
    CHECK(memory.write("x",1)==0 && memory.getPosition()==4);
    OutputLog forwarded;
    { ForwardingOStream output(new Output(forwarded));
      int value=0x12345678;CHECK(output.streamInt(&value,"int"));output.flush();
      CHECK(output.getPosition()==4 && forwarded.flushes==1);
      CHECK(output.setPosition(1) && output.getPosition()==1);
    }
    CHECK(forwarded.destroyed==1 && forwarded.bytes==std::vector<unsigned char>({0x78,0x56,0x34,0x12}));
    OutputLog encoded;
    {
        ClassOStream output(new Output(encoded));
        CHECK(sizeof(ClassOStream)==0x81C && output.getObjectSize()==4);
        CHECK(output.writeShortString(nullptr,nullptr,""));CHECK(output.writeShortString(nullptr,"one",""));
        CHECK(output.writeShortString(nullptr,"one",""));CHECK(output.writeShortString(nullptr,"two",""));
        CHECK(encoded.bytes.empty() && output.getPosition()==8);
        CHECK(output.finish(nullptr));
        std::vector<unsigned char> expected;literal(expected,"MemeFile 2.0");literal(expected,"one");literal(expected,"two");
        expected.push_back(0);const unsigned char ids[]={0,0,1,0,1,0,2,0};expected.insert(expected.end(),ids,ids+8);
        CHECK(encoded.bytes==expected);
        CHECK(output.getPosition()==static_cast<int>(expected.size()));
        unsigned char tail=0xEF;CHECK(output.streamUByte(&tail,"tail") && encoded.bytes.back()==tail);
    }
    CHECK(encoded.destroyed==1);
    OutputLog failed;failed.failCall=1;
    { ClassOStream output(new Output(failed));CHECK(output.writeShortString(nullptr,"retained",""));
      CHECK(!output.finish(nullptr) && failed.bytes.empty() && output.getPosition()==0);
    }
    CHECK(failed.destroyed==1);
    OutputLog wrapped;
    { ClassOStream output(new Output(wrapped));std::string longName(256,'x');
      CHECK(output.writeShortString(nullptr,longName.c_str(),""));CHECK(output.finish(nullptr));
      std::vector<unsigned char> expected;literal(expected,"MemeFile 2.0");
      expected.insert(expected.end(),{0,0,1,0});CHECK(wrapped.bytes==expected);
    }
}
static TypeDescriptor saveType("fixture::SavingObject");
struct SavingObject : Object {
    bool enabled=true, named=true, result=true;int visits=0;
    TypeDescriptor* getType() override { return &saveType; }
    const char* getName() override { return named ? "entry" : nullptr; }
    bool slot_0x20() override { return enabled; }
    bool streamProperties(System*,Stream* output) override {
        ++visits;int value=42;return result && output->streamInt(&value,"Value");
    }
};
struct SavingSystem : System {
    NameTreeRoot registry;OutputLog log;int opens=0;bool failOpen=false;std::string path;
    SavingSystem() : System(800,600) { m_registry=&registry; }
    OStream* createOutputStream(const char* name) override {
        ++opens;path=name;return failOpen ? nullptr : new Output(log);
    }
};
static void saveTests()
{
    SavingSystem system;SavingObject object;Object* value=&object;
    CHECK(system.registry.insert(&system,value,"Top"));
    CHECK(saveNamedObject(&system,value));CHECK(system.opens==1 && system.path=="Top/entry" && object.visits==1);
    std::vector<unsigned char> expected;literal(expected,"MemeFile 2.0");literal(expected,"fixture::SavingObject");
    expected.insert(expected.end(),{0,1,0,42,0,0,0});
    CHECK(system.log.bytes==expected && system.log.destroyed==1);
    object.enabled=false;CHECK(saveNamedObject(&system,value) && system.opens==1);
    object.enabled=true;object.named=false;CHECK(saveNamedObject(&system,value) && system.opens==1);object.named=true;
    system.failOpen=true;CHECK(!saveNamedObject(&system,value) && system.opens==2 && object.visits==1);
    system.failOpen=false;object.result=false;system.log.bytes.clear();
    CHECK(!saveNamedObject(&system,value) && object.visits==2 && system.log.bytes.empty() && system.log.destroyed==2);
    value=nullptr;CHECK(saveNamedObject(&system,value) && system.opens==3);
}
static void inputTests()
{
    System system(800,600);
    OutputLog bytes;
    { ClassOStream output(new Output(bytes));
      CHECK(output.writeShortString(&system,"one","name"));
      int number=0x12345678;CHECK(output.streamInt(&number,"number"));
      CHECK(output.writeShortString(&system,"one","name"));
      CHECK(output.writeShortString(&system,nullptr,"empty"));CHECK(output.finish(&system));
    }
    InputLog log;log.bytes=bytes.bytes;
    { ClassIStream input(new Input(log));
      CHECK(sizeof(ClassIStream)==0x814 && input.getObjectSize()==0x814);
      CHECK(input.init(&system) && input.getPosition()==0 && input.getAvailableBytes()==10);
      char name[256];CHECK(input.readShortString(&system,name,1,"name") && !std::strcmp(name,"one"));
      // The physical destination is large; supplied capacity1 is deliberately ignored by ClassIStream.
      int number=0;CHECK(input.streamInt(&number,"number") && number==0x12345678 && input.getPosition()==6);
      CHECK(input.readShortString(&system,name,256,"name") && !std::strcmp(name,"one"));
      CHECK(input.readShortString(&system,name,256,"empty") && name[0]==0);
      name[0]='x';CHECK(!input.readShortString(&system,name,256,"eof") && name[0]==0);
      CHECK(input.setPosition(2) && input.getPosition()==2);
      number=0;CHECK(input.streamInt(&number,"number") && number==0x12345678);
    }
    CHECK(log.destroyed==1);
    InputLog owned;owned.bytes={1,2,3};
    { ForwardingIStream input(new Input(owned),true);unsigned char b=0;
      CHECK(input.streamUByte(&b,"") && b==1 && input.getAvailableBytes()==2);
      CHECK(input.skip(1)==1 && input.getPosition()==2);
    }
    CHECK(owned.destroyed==1);
    InputLog borrowed;borrowed.bytes={9};Input child(borrowed);IStream* wrapperAddress;
    { ForwardingIStream input(&child,false);wrapperAddress=&input;CHECK(child.getSource()==&input); }
    IStream* retainedSource;std::memcpy(&retainedSource,reinterpret_cast<const char*>(&child)+4,4);
    CHECK(borrowed.destroyed==0 && retainedSource==wrapperAddress); // Retail dangling source link; do not dereference.
    InputLog wrong;literal(wrong.bytes,"wrong version");
    { ClassIStream input(new Input(wrong));CHECK(!input.init(&system)); }
    CHECK(wrong.destroyed==1);
    InputLog noDictionary;literal(noDictionary.bytes,"MemeFile 2.0");
    { ClassIStream input(new Input(noDictionary));CHECK(!input.init(&system)); }
    CHECK(noDictionary.destroyed==1); // Failure before first dictionary allocation is safe to destroy.
    OutputLog sound;Output output(sound);int handle=7;
    CHECK(output.streamSound(&system,&handle,"sound") && sound.bytes==std::vector<unsigned char>({0}));
    InputLog soundInput;soundInput.bytes={0};Input input(soundInput);
    CHECK(input.streamSound(&system,&handle,"sound") && handle==0);
    handle=7;CHECK(!input.streamSound(&system,&handle,"eof") && handle==0);
}
// [TEMP] A complete Data-derived class injects the property-failure condition;
// production readObject, descriptor counting and registry methods are unchanged.
struct FailingData : Data {
    static TypeDescriptor* type();
    FailingData() : Data(nullptr) {}
    TypeDescriptor* getType() override { return type(); }
    bool streamProperties(System*,Stream*) override { return false; }
};
TypeDescriptor* FailingData::type() {
    static ClassTypeDescriptor<FailingData,Data> descriptor("fixture::FailingData");return &descriptor;
}
static InputLog frame(const char* name,const char* type,int payload=0) {
    InputLog log;log.bytes.resize(4);literal(log.bytes,name);literal(log.bytes,type);
    log.bytes.resize(log.bytes.size()+payload,0xA5);
    unsigned long size=static_cast<unsigned long>(log.bytes.size());std::memcpy(log.bytes.data(),&size,4);return log;
}
static void objectInputTests()
{
    SavingSystem system;Object* value=nullptr;TypeDescriptor* dataType=Data::getClassType();
    const int baseline=dataType->getInstanceCount();
    InputLog empty=frame("","",3);Input emptyInput(empty);
    CHECK(readObject(&system,value,&emptyInput) && !value && empty.lastSkip==3 && empty.position==9);
    InputLog unknown=frame("","fixture::unknown",2);Input unknownInput(unknown);
    CHECK(readObject(&system,value,&unknownInput) && !value && unknown.lastSkip==2);
    InputLog anonymous=frame("",dataType->getName(),2);Input anonymousInput(anonymous);
    CHECK(readObject(&system,value,&anonymousInput) && value && !value->getName());
    CHECK(dataType->getInstanceCount()==baseline+1 && anonymous.lastSkip==2);
    CHECK(deleteObject(&system,value) && !value && dataType->getInstanceCount()==baseline);
    InputLog named=frame("Top/entry",dataType->getName());Input namedInput(named);
    CHECK(readObject(&system,value,&namedInput) && value && !std::strcmp(value->getName(),"entry"));
    CHECK(system.findNamedObject("Top/entry")==value && dataType->getInstanceCount()==baseline+1);
    Object* existing=value;value=nullptr;InputLog reused=frame("Top/entry","fixture::unknown",2);Input reusedInput(reused);
    CHECK(readObject(&system,value,&reusedInput) && value==existing && reused.lastSkip==2);
    CHECK(!loadNamedObject(&system,value,nullptr) && value==existing);
    value=nullptr;CHECK(loadNamedObject(&system,value,"Top/entry") && value==existing);
    CHECK(system.registry.eraseObject(&system,value));RuntimeObject* runtime=value;
    CHECK(destroyRuntimeObject(runtime) && !runtime && dataType->getInstanceCount()==baseline);value=nullptr;
    TypeDescriptor* failureType=FailingData::type();const int failureBaseline=failureType->getInstanceCount();
    InputLog anonFailure=frame("",failureType->getName(),3);Input anonFailureInput(anonFailure);
    CHECK(!readObject(&system,value,&anonFailureInput) && value && anonFailure.skips==0);
    CHECK(failureType->getInstanceCount()==failureBaseline);delete value;value=nullptr;
    InputLog namedFailure=frame("Failed/item",failureType->getName(),3);Input namedFailureInput(namedFailure);
    CHECK(readObject(&system,value,&namedFailureInput) && value && namedFailure.lastSkip==3);
    CHECK(system.findNamedObject("Failed/item")==value && failureType->getInstanceCount()==failureBaseline+1);
    CHECK(system.registry.eraseObject(&system,value));runtime=value;
    CHECK(destroyRuntimeObject(runtime) && failureType->getInstanceCount()==failureBaseline);value=nullptr;
    InputLog direct;literal(direct.bytes,dataType->getName());Input directInput(direct);
    CHECK(readNamedObject(&system,value,&directInput,"Direct/entry") && value);
    CHECK(system.findNamedObject("Direct/entry")==value && dataType->getInstanceCount()==baseline+1);
    CHECK(system.registry.eraseObject(&system,value));runtime=value;
    CHECK(destroyRuntimeObject(runtime) && dataType->getInstanceCount()==baseline);value=nullptr;
    // Object calls cross the ForwardingIStream back-reference and must still
    // reach ClassIStream's dictionary string reader rather than raw byte strings.
    OutputLog roundTrip;Data anonymousData(nullptr);Object* saved=&anonymousData;
    { ClassOStream output(new Output(roundTrip));
      CHECK(output.streamObject(&system,saved,"Object",nullptr) && output.finish(&system));
    }
    InputLog roundTripInput;roundTripInput.bytes=roundTrip.bytes;
    { ClassIStream input(new Input(roundTripInput));
      CHECK(input.init(&system) && input.streamObject(&system,value,"Object",nullptr));
      CHECK(value && value->getType()==dataType && !value->getName());
      CHECK(deleteObject(&system,value) && dataType->getInstanceCount()==baseline);
    }
    CHECK(roundTripInput.destroyed==1);
    InputLog truncated;truncated.bytes={1,0};Input truncatedInput(truncated);
    CHECK(!readObject(&system,value,&truncatedInput) && !value && truncated.skips==0);
}
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    fileTests();classTests();saveTests();inputTests();objectInputTests();
    std::printf("BFMeme class/file streams: %d/%d checks passed\n",checks-failures,checks);
    return failures ? 1 : 0;
}
