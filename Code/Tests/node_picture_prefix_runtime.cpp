// [TEMP][CHOSEN, not recovered][RETIRE-WHEN: NODE-PICTURE-PREFIX-FIXTURE]
// Actual prefix methods with real PictureNode/TransformNode construction and typed callbacks.
// Node suffix/reset are uncalled.
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <float.h>
#include <limits>
#include <functional>
#include <vector>
#include <string>
#define protected public
#include "../BF2/External/BFMeme/PictureNode.h"
#include "../BF2/External/BFMeme/TransformNode.h"
#include "../BF2/External/BFMeme/NodeState.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/Effect.h"
#include "../BF2/External/BFMeme/Quad.h"
#undef protected
using namespace dice::meme;
static unsigned checks,failures;
static std::vector<std::string> trace;
static void check(bool value,const char* name){++checks;if(!value){++failures;std::printf("FAIL %s\n",name);}}
static bool rectEqual(const Rect& a,const Rect& b){return a.x==b.x&&a.y==b.y&&a.width==b.width&&a.height==b.height&&a.z==b.z;}
struct SpyNode : Node {
    explicit SpyNode(const char* name):name(name){}
    std::string name;Rect bounds{};RenderContext context{};const NodeState* state=nullptr;
    const Rect* originalBounds=nullptr;const Rect* clip=nullptr;RenderContext* originalContext=nullptr;
    std::function<void()> callback;bool result=true;int events=0;
    void paint(System*,const NodeState* s,const Rect* b,const Rect* c,RenderContext* r) override {
        trace.push_back(name+".paint");state=s;bounds=*b;clip=c;context=*r;originalContext=r;
        if(callback)callback();
    }
    bool update(System*,const NodeState* s,const Rect* b,float dt) override {
        trace.push_back(name+".update");state=s;bounds=*b;originalBounds=b;check(dt==0.0f,"zero update forwarded");
        if(callback)callback();return result;
    }
    bool dispatchEvent(System*,const Rect*,const NodeState*,const NodeInputEvent*) override {++events;return false;}
};
struct Transform : TransformNode {
    Rect selfBounds{};const NodeState* selfState=nullptr;RenderContext* selfContext=nullptr;
    const Rect* updateBounds=nullptr;bool result=true;std::function<void()> paintCallback,updateCallback,eventCallback;
    int events=0;
    void paintSelf(System*,const NodeState* s,const Rect* b,const Rect*,RenderContext* r) override {
        trace.push_back("self.paint");selfBounds=*b;selfState=s;selfContext=r;if(paintCallback)paintCallback();
    }
    bool updateSelf(System*,const NodeState*,const Rect* b,float) override {
        trace.push_back("self.update");updateBounds=b;if(updateCallback)updateCallback();return result;
    }
    bool dispatchEvent(System*,const Rect*,const NodeState*,const NodeInputEvent*) override {
        ++events;if(eventCallback)eventCallback();return false;
    }
};
struct Visitor : NodeVisitor {
    Visitor():NodeVisitor(0){}
    Node** first=nullptr;Node** second=nullptr;NodeState* own=nullptr;NodeState* parent=nullptr;
    Rect transformed{};const Rect* original=nullptr;std::function<void()> callback;
    void traverse0x08(System*,Node*& n,const Rect* b,NodeState* s) override {
        trace.push_back("child.visit");first=&n;transformed=*b;own=s;if(callback)callback();
    }
    void visit(System*,Node*& n,const Rect* b,NodeState* s) override {
        trace.push_back("next.visit");second=&n;original=b;parent=s;
    }
};
struct StreamSpy : Stream {
    int failAt=-1,calls=0;std::vector<std::string> labels;std::vector<void*> addresses;
    bool accept(void* p,const char* label){addresses.push_back(p);labels.push_back(label);return calls++!=failAt;}
    bool streamNode0x88(System*,Node*& n,const char* label,TypeDescriptor* type) override {
        check(!type,"next stream null requested descriptor");return accept(&n,label);
    }
    bool streamNode0x84(System*,Node*& n,const char* label,TypeDescriptor* type) override {
        check(!type,"child stream null requested descriptor");return accept(&n,label);
    }
    bool streamFloat(float* p,const char* label) override{return accept(p,label);}
    bool streamTexture(System*,int* p,const char* label) override{return accept(p,label);}
    void clear(){calls=0;labels.clear();addresses.clear();}
};
struct DrawSystem : System {
    DrawSystem():System(800,600){}
    int texture=0,draws=0;Quad copy{&initial,nullptr};static Rect initial;
    bool drawTexture(int t,Quad* q) override {texture=t;copy=*q;++draws;return false;}
};
Rect DrawSystem::initial{0,0,0,0,0};
int main(){
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);setvbuf(stdout,nullptr,_IONBF,0);
    static_assert(sizeof(void*)==4,"x86");static_assert(sizeof(TransformNode)==36,"native transform");
    static_assert(sizeof(PictureNode)==12,"native picture");
    DrawSystem system;Transform t;PictureNode picture(-37);SpyNode child("child"),next("next"),replacement("replacement");
    check(t.getObjectSize()==36&&picture.getObjectSize()==12,"actual native size methods");check(!t.getName(),"actual null name default");
    check(t.getChildCount()==2,"actual two-child count");
    t.setChild(0,&next);t.setChild(1,&child);
    for(int index:{-1,2,0x7fffffff}){t.setChild(index,&replacement);check(t.m_next==&next&&t.m_child==&child,"invalid setter is ignored");check(&t.getChild(index)==&t.m_next,"invalid getter returns next reference");}
    check(&t.getChild(1)==&t.m_child&&&t.getChild(0)==&t.m_next,"both child live references");
    t.getChild(1)=&replacement;check(t.m_child==&replacement,"child reference writes actual member");t.m_child=&child;
    NodeState parent{1,1};Rect bounds{10,20,100,200,7},clip{1,2,3,4,5};RenderContext context{nullptr,nullptr,2,3,4,5,6};
    t.m_position={3,4};t.m_size={30,40};Visitor visitor;
    visitor.callback=[&]{t.m_next=&replacement;};trace.clear();t.visitChildren(&system,&parent,&bounds,&visitor);
    check(trace==std::vector<std::string>{"child.visit","next.visit"},"specialized child visitor before direct next visitor");
    check(visitor.first==&t.m_child&&visitor.second==&t.m_next&&*visitor.second==&replacement,"visitor aliases live links after mutation");
    check(visitor.own==&t.m_state&&visitor.parent==&parent&&visitor.original==&bounds,"visitor state/bounds identities");
    check(rectEqual(visitor.transformed,Rect{13,24,30,40,7}),"visitor offsets replace dimensions and preserve depth");
    t.m_child=nullptr;t.m_next=nullptr;trace.clear();t.visitChildren(&system,&parent,&bounds,&visitor);
    check(trace.size()==2,"both null children still visited");visitor.callback=nullptr;
    StreamSpy stream;
    for(int failed=-1;failed<6;++failed){stream.clear();stream.failAt=failed;check(t.streamProperties(&system,&stream)==(failed<0),"transform property short-circuit result");check(stream.calls==(failed<0?6:failed+1),"transform property short-circuit count");}
    stream.clear();stream.failAt=-1;t.streamProperties(&system,&stream);
    check(stream.labels==std::vector<std::string>{"Next node","X","Y","Width","Height","Transformed node"},"exact transform property order");
    check(stream.addresses==std::vector<void*>{&t.m_next,&t.m_position.x,&t.m_position.y,&t.m_size.x,&t.m_size.y,&t.m_child},"native property addresses");
    for(int failed=-1;failed<2;++failed){stream.clear();stream.failAt=failed;check(picture.streamProperties(&system,&stream)==(failed<0),"picture property result");check(stream.calls==(failed<0?2:failed+1),"picture property base guard");}
    check(stream.labels==std::vector<std::string>{"Next node","Picture"},"picture exact property label");
    picture.paintSelf(&system,nullptr,&bounds,&clip,&context);check(system.draws==1&&system.texture==-37,"picture forwards signed texture and ignores draw failure");
    check(system.copy.clip==&clip&&system.copy.corners[0].x==10&&system.copy.corners[2].y==220,"picture real quad uses bounds and clip");
    t.m_child=&child;t.m_next=&next;t.m_state={1,1};trace.clear();t.paint(&system,&parent,&bounds,&clip,&context);
    check(trace==std::vector<std::string>{"self.paint","child.paint","next.paint"},"transform paint order");
    check(t.selfState==&parent&&t.selfContext==&context&&child.state==&t.m_state&&next.state==&parent,"paint state identity transition");
    check(rectEqual(t.selfBounds,Rect{13,24,30,40,7})&&rectEqual(child.bounds,t.selfBounds)&&rectEqual(next.bounds,bounds),"paint transformed child and original next bounds");
    check(child.originalContext==&context&&next.originalContext==&context,"no effect reuses original context");
    Effect effect;context.effect=&effect;t.m_state={1,-3};trace.clear();t.paint(&system,&parent,&bounds,&clip,&context);
    check(child.context.parent==&context&&!child.context.effect&&child.context.field08==2&&child.context.field0c==3,"effect creates actual parent context chain");
    check(child.context.field10==-3&&child.context.field14==2&&child.context.field18==1,"child context decoded state and falling rate");
    t.m_state={-4,-2};t.paint(&system,&parent,&bounds,&clip,&context);check(child.context.field10==3&&child.context.field14==1,"rising rate compares decoded target and current");
    const float nan=std::numeric_limits<float>::quiet_NaN();t.m_state={nan,nan};t.paint(&system,&parent,&bounds,&clip,&context);
    check(child.context.field10==0&&std::isnan(child.context.field14),"unordered state has zero rate and preserves decoded NaN");
    t.paintCallback=[&]{t.m_child=&replacement;t.m_position={1000,1000};};replacement.callback=[&]{t.m_next=&child;};trace.clear();
    t.paint(&system,&parent,&bounds,&clip,&context);
    check(trace==std::vector<std::string>{"self.paint","replacement.paint","child.paint"},"paint reloads child and successor after callbacks");
    check(replacement.bounds.x==13&&replacement.bounds.y==24,"paint callback leaves captured transformed bounds");
    t.paintCallback=nullptr;replacement.callback=nullptr;t.m_position={3,4};t.m_state={1,1};t.m_child=&child;t.m_next=&next;context.effect=nullptr;
    trace.clear();check(t.update(&system,&parent,&bounds,0),"actual transform update succeeds");
    check(trace==std::vector<std::string>{"child.update","self.update","next.update"},"update child precedes base self and next");
    check(t.updateBounds==&bounds&&child.state==&t.m_state&&rectEqual(child.bounds,Rect{13,24,30,40,7}),"update forwards original parent and transformed child context");
    child.result=false;trace.clear();check(!t.update(&system,&parent,&bounds,0)&&trace==std::vector<std::string>{"child.update"},"child false suppresses base update");child.result=true;
    t.result=false;trace.clear();check(!t.update(&system,&parent,&bounds,0)&&trace==std::vector<std::string>{"child.update","self.update"},"base self false suppresses successor");t.result=true;
    next.result=false;check(!t.update(&system,&parent,&bounds,0),"successor false propagates");next.result=true;
    struct Inside {float x,y,width,height;bool expected;};
    const Inside cases[]={{13,24,30,40,true},{43,64,30,40,true},{12,24,30,40,false},{13,65,30,40,false},{nan,24,30,40,true},{13,nan,30,40,true},{13,24,nan,40,true},{13,24,-1,40,false}};
    parent.target=-2;t.m_child=nullptr;t.m_next=nullptr;
    for(const auto& item:cases){t.m_size={item.width,item.height};t.m_state={1,1};system.m_pointerX=item.x;system.m_pointerY=item.y;
        check(t.update(&system,&parent,&bounds,0),"actual state-update branch returns");check((t.m_state.target<0)==item.expected,"inclusive/unordered pointer bounds semantics");}
    parent.target=nan;t.m_state={0,0};t.update(&system,&parent,&bounds,0);check(t.m_state.target==1,"unordered parent is enabled");
    parent.target=0;t.m_state={1,1};t.update(&system,&parent,&bounds,0);check(t.m_state.target==0,"zero parent disables child state");
    parent.target=-2;t.m_state={1,1};t.m_position={0,0};t.m_size={FLT_MAX,1};Rect huge{FLT_MAX,0,1,1,0};
    system.m_pointerX=std::numeric_limits<float>::infinity();system.m_pointerY=0;
    t.update(&system,&parent,&huge,0);check(t.m_state.target==1,"unspilled finite sum remains less than infinite pointer");
    t.m_state={1,1};t.m_size={1,1};t.m_child=nullptr;t.m_next=&next;system.m_pointerX=0;system.m_pointerY=0;
    Rect origin{0,0,1,1,0};t.eventCallback=[&]{t.m_child=&replacement;};
    replacement.callback=[&]{t.m_next=&child;};trace.clear();t.update(&system,&parent,&origin,0);
    check(trace==std::vector<std::string>{"replacement.update","self.update","child.update"},"state event replaces child before update and child callback replaces next");
    t.eventCallback=nullptr;replacement.callback=nullptr;t.m_child=nullptr;t.m_next=nullptr;
    parent.target=-2;t.m_state={1,1};t.m_position={0,0};t.m_size={1,1};Rect precision{16777216,0,1,1,0};system.m_pointerX=16777218;system.m_pointerY=0;
    unsigned oldControl=0;_controlfp_s(&oldControl,0,0);unsigned ignored;_controlfp_s(&ignored,_PC_24,_MCW_PC);
    t.update(&system,&parent,&precision,0);check(t.m_state.target==1,"x87 24-bit sum bound rejects rounded-down right edge");
    _controlfp_s(&ignored,oldControl,_MCW_PC);
    std::printf("Node/Picture prefix: %u/%u checks passed\n",checks-failures,checks);return failures?1:0;
}
