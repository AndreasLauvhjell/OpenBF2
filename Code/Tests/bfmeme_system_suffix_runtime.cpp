// [TEMP] Real System suffix/setSize/paint with callback observers. /FORCE is
// limited to uncalled surrounding UI work; no production method is shadowed.
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Pointer.h"
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/Data.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/Menu/Ref2System.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <windows.h>
using namespace dice::meme;
namespace {
int checks=0, failures=0, order[32], used=0,lastLine=0;
#define CHECK(x) do { lastLine=__LINE__; ++checks; if (!(x)) { ++failures; std::printf("FAIL line%d: %s\n",__LINE__,#x); } } while(0)
void record(int item) { order[used++]=item; }
void clearOrder() { used=0; }
bool sequence(int a,int b=0,int c=0,int d=0,int e=0)
{
    const int expected[5]={a,b,c,d,e}; int count=0;
    while(count<5 && expected[count]) ++count;
    return used==count && std::memcmp(order,expected,count*sizeof(int))==0;
}
class Surface : public System
{
public:
    Surface() : System(800,600), sizeCalls(0), sameAddress(false), editSize(false),
        inject(false), injected(nullptr), seenWidth(0), seenHeight(0), seenX(0), seenY(0) {}
    void beginPaint() override { record(10); if(inject) m_field018=injected; }
    void endPaint() override { record(40); }
    void sizeChanged(const float* before,const float* after) override
    {
        ++sizeCalls; oldSize[0]=before[0];oldSize[1]=before[1];
        newSize[0]=after[0];newSize[1]=after[1];
        sameAddress=after==&m_width && before!=after;
        seenWidth=m_width;seenHeight=m_height;seenX=m_originX;seenY=m_originY;
        if(editSize) { const_cast<float*>(after)[0]=111;const_cast<float*>(after)[1]=222; }
    }
    void install(Node* node,Pointer* pointer) { m_field018=node;m_field020=pointer; }
    void pointer(Pointer* value) { m_field020=value; }
    void registry(NameTreeNode* value) { m_registry=value; }
    int texture()const { return m_field054; }
    bool geometry(float x,float y,float w,float h)const
    { return m_originX==x && m_originY==y && m_width==w && m_height==h; }
    const Rect* boundsAddress()const { return reinterpret_cast<const Rect*>(&m_originX); }
    const NodeState* stateAddress()const { return reinterpret_cast<const NodeState*>(&m_field028); }
    void interaction(float target,float current) { m_field028=target;m_field02c=current; }
    int sizeCalls;bool sameAddress,editSize,inject;Node* injected;
    float oldSize[2],newSize[2],seenWidth,seenHeight,seenX,seenY;
};
class Graphic : public Node
{
public:
    explicit Graphic(int value):id(value),calls(0),replacePointer(false),replacement(nullptr),
        seen(nullptr),statePtr(nullptr),boundsPtr(nullptr),clipPtr(nullptr) {}
    void paintSelf(System* system,const NodeState* s,const Rect* b,const Rect* clip,RenderContext* c) override
    {
        record(id);++calls;seen=system;statePtr=s;boundsPtr=b;clipPtr=clip;
        state=*s;bounds=*b;context=*c;
        if(replacePointer) static_cast<Surface*>(system)->pointer(replacement);
    }
    int id,calls;bool replacePointer;Pointer* replacement;System* seen;
    const NodeState* statePtr;const Rect* boundsPtr;const Rect* clipPtr;
    NodeState state;Rect bounds;RenderContext context;
};
class Cursor : public Pointer
{
public:
    void configure(Node* graphic,bool enabled) { m_graphic=graphic;m_enabled=enabled; }
};
class RejectingCursor : public Cursor
{
public:
    RejectingCursor():calls(0){}
    bool paint(System*) override { ++calls;record(30);return false; }
    int calls;
};
void suffixTests()
{
    System base(20,30);Node first,second;Surface observed;
    CHECK(sizeof(System)==88);
    CHECK(base.getPointerOwner()==nullptr);
    base.setPointerOwner(&first);CHECK(base.getPointerOwner()==&first);
    base.setPointerOwner(&second);CHECK(base.getPointerOwner()==&second);
    base.setPointerOwner(nullptr);CHECK(base.getPointerOwner()==nullptr);
    observed.setDefaultTexture(0x12345678);CHECK(observed.texture()==0x12345678);
    observed.setDefaultTexture(-17);CHECK(observed.texture()==-17);
    observed.setDefaultTexture(0);CHECK(observed.texture()==0);
    CHECK(!base.playSound(17,1));CHECK(!base.playSound(-17,-1));
    CHECK(!base.playSound(0,std::numeric_limits<float>::quiet_NaN()));
    CHECK(base.loadSound(nullptr)==0 && base.loadSound("Step sound")==0);
    CHECK(base.getSoundName(-1)==nullptr && base.getSoundName(42)==nullptr);
    CHECK(base.preUpdate(-1) && base.postUpdate(-1));
    CHECK(base.preUpdate(std::numeric_limits<float>::quiet_NaN()));
    CHECK(base.postUpdate(std::numeric_limits<float>::infinity()));
    base.beginPaint();base.endPaint();base.sizeChanged(nullptr,nullptr);
    CHECK(base.paint()); // Real empty tree/pointer and real empty render hooks.
}
void sizeTests()
{
    Surface system;system.setSize(640,480);
    CHECK(system.sizeCalls==1 && system.sameAddress);
    CHECK(system.oldSize[0]==800 && system.oldSize[1]==600);
    CHECK(system.newSize[0]==640 && system.newSize[1]==480);
    CHECK(system.seenWidth==640 && system.seenHeight==480 && system.seenX==-320 && system.seenY==-240);
    CHECK(system.geometry(-320,-240,640,480));
    system.setSize(640,480);CHECK(system.sizeCalls==2); // No equality guard.
    system.editSize=true;system.setSize(-20,10);
    CHECK(system.oldSize[0]==640 && system.oldSize[1]==480);
    CHECK(system.newSize[0]==-20 && system.newSize[1]==10);
    CHECK(system.geometry(10,-5,111,222)); // Actual member address reaches hook.
    system.editSize=false;system.setSize(0,-0.0f);
    CHECK(system.oldSize[0]==111 && system.oldSize[1]==222);
    CHECK(std::signbit(system.seenX) && !std::signbit(system.seenY));
    system.setSize(std::numeric_limits<float>::quiet_NaN(),-8);
    CHECK(std::isnan(system.seenX) && std::isnan(system.seenWidth));
    CHECK(system.seenY==4 && system.seenHeight==-8);
}
void paintTests()
{
    Surface system;Graphic root(20),next(21),cursorGraphic(30);Cursor cursor;RejectingCursor rejects;
    system.interaction(-7,3.5f);
    clearOrder();CHECK(system.paint());CHECK(sequence(10,40));
    system.install(&root,nullptr);clearOrder();CHECK(system.paint());CHECK(sequence(10,20,40));
    CHECK(root.seen==&system && root.statePtr==system.stateAddress() && root.boundsPtr==system.boundsAddress());
    CHECK(root.clipPtr==nullptr && root.state.target==-7 && root.state.current==3.5f);
    CHECK(root.bounds.x==-400 && root.bounds.y==-300 && root.bounds.width==800 && root.bounds.height==600 && root.bounds.z==0);
    CHECK(!root.context.parent && !root.context.effect && root.context.field08==0 && root.context.field0c==1);
    CHECK(root.context.field10==0 && root.context.field14==1 && root.context.field18==1);
    root.setChild(0,&next);clearOrder();CHECK(system.paint());CHECK(sequence(10,20,21,40));
    CHECK(next.calls==1 && next.statePtr==root.statePtr && next.boundsPtr==root.boundsPtr);
    root.setChild(0,nullptr);
    cursor.configure(&cursorGraphic,true);system.install(&root,&cursor);
    clearOrder();CHECK(system.paint());CHECK(sequence(10,20,30,40));
    CHECK(cursorGraphic.seen==&system && cursorGraphic.state.target==1 && cursorGraphic.state.current==1);
    CHECK(cursorGraphic.bounds.x==-10 && cursorGraphic.bounds.y==-10 && cursorGraphic.bounds.width==20 && cursorGraphic.bounds.height==20);
    cursor.configure(&cursorGraphic,false);clearOrder();CHECK(system.paint());CHECK(sequence(10,20,40));
    CHECK(cursorGraphic.calls==1);
    rejects.configure(nullptr,true);system.install(&root,&rejects);clearOrder();CHECK(!system.paint());
    CHECK(sequence(10,20,30) && rejects.calls==1); // Failed paint skips endPaint.
    rejects.configure(nullptr,false);clearOrder();CHECK(system.paint());CHECK(sequence(10,20,40));
    CHECK(rejects.calls==1);
    system.install(nullptr,nullptr);system.inject=true;system.injected=&root;
    clearOrder();CHECK(system.paint());CHECK(sequence(10,20,40)); // beginPaint runs before root reload.
    system.inject=false;rejects.configure(nullptr,true);
    root.replacePointer=true;root.replacement=&rejects;system.install(&root,nullptr);
    clearOrder();CHECK(!system.paint());CHECK(sequence(10,20,30)); // Pointer reloaded after Node callback.
    CHECK(rejects.calls==2);
    root.replacePointer=false;system.install(nullptr,nullptr);
}
class RefusingNameData : public Data
{
public:
    RefusingNameData():Data("Retained"),calls(0){}
    bool setName(const char*) override { ++calls;return false; }
    int calls;
};
void namedHelperTests()
{
    Surface system;NameTreeRoot tree;Data first,second("Before"),blocker("Block");
    Object immutable;system.registry(&tree);
    const int count=Data::getClassType()->getInstanceCount();
    CHECK(!system.assignObjectPath(nullptr,"unused"));
    CHECK(!system.assignObjectPath(&immutable,"unused")); // Real Object rejects naming.
    CHECK(system.findNamedObject("Missing")==nullptr);
    CHECK(system.assignObjectPath(&first,"First"));
    CHECK(std::strcmp(first.getName(),"First")==0 && system.findNamedObject("First")==&first);
    CHECK(system.assignObjectPath(&first,"First")); // Existing identical object succeeds.
    CHECK(!system.assignObjectPath(&second,"First"));
    CHECK(std::strcmp(second.getName(),"Before")==0);
    CHECK(system.assignObjectPath(&first,"A/B\\Leaf"));
    CHECK(std::strcmp(first.getName(),"Leaf")==0);
    CHECK(system.findNamedObject("First")==nullptr && system.findNamedObject("A/B/Leaf")==&first);
    CHECK(system.findNamedObject("A\\B\\Leaf")==&first);
    CHECK(system.assignObjectPath(&first,"C\\D/Last")); // Latest slash of either kind.
    CHECK(system.findNamedObject("A/B/Leaf")==nullptr && system.findNamedObject("C/D/Last")==&first);
    // Isolate the collision: a preceding lexicographically later container makes
    // retail createContainer insert before it, without ever reaching object entries.
    NameTreeRoot fallbackTree;system.registry(&fallbackTree);
    CHECK(system.assignObjectPath(&blocker,"Block"));
    CHECK(system.assignObjectPath(&second,"Block/Fallback")); // Existing object blocks container creation.
    CHECK(system.findNamedObject("Fallback")==&second);
    CHECK(fallbackTree.findChild("Block")->getObject()==&blocker);
    CHECK(system.findNamedObject("Block")==&blocker);
    CHECK(Data::getClassType()->getInstanceCount()==count); // Assignment does not register/count.
    RefusingNameData refuses;
    CHECK(system.assignObjectPath(&refuses,"Discarded")); // Failed erase/setName results ignored.
    CHECK(refuses.calls==1 && system.findNamedObject("Retained")==&refuses);
    CHECK(system.findNamedObject("Discarded")==nullptr);
    Data empty("Prior");CHECK(system.assignObjectPath(&empty,"Trailing/"));
    CHECK(empty.getName() && !*empty.getName());
    NameTreeNode inert;system.registry(&inert);Data lost;
    CHECK(system.assignObjectPath(&lost,"NoContainer/Accepted")); // Real base registry rejects insert.
    CHECK(std::strcmp(lost.getName(),"Accepted")==0 && system.findNamedObject("Accepted")==nullptr);
    system.registry(nullptr); // Destructors do not close the borrowed fixture trees/objects.
}
}
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* info)->LONG {
        std::fprintf(stderr,"Fixture exception %08lx at %p after line%d\n",
            info->ExceptionRecord->ExceptionCode,info->ExceptionRecord->ExceptionAddress,lastLine);
        std::fflush(stderr);return EXCEPTION_EXECUTE_HANDLER;
    });
    suffixTests();sizeTests();paintTests();namedHelperTests();
    std::printf("System suffix/setSize/paint/named helpers: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
