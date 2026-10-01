// [TEMP] Typed callback observers; production SplitNode/Bf2SplitNode methods run.
// [RETIRE-WHEN: HUD-SPLIT-PREFIX-FIXTURE]
// Diagnostic /FORCE leaves unrelated Node suffix/stream dependencies uncalled.
#include "../BF2/Menu/Bf2SplitNode.h"
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include <cstdio>
#include <cstring>
#include <vector>
using namespace dice::meme;
namespace {
int checks, failures, destroyed;
std::vector<int> calls;
NodeState state = {1,1};
Rect bounds = {1,2,3,4,5}, clip = {6,7,8,9,10};
RenderContext context = {nullptr,nullptr,0,1,0,1,1};
void check(bool okay, const char* text)
{ ++checks; if (!okay) { ++failures; std::printf("FAIL: %s\n",text); } }
struct CallbackNode : Node {
    explicit CallbackNode(int i) : id(i), accept(true), owner(nullptr), replacement(nullptr) {}
    ~CallbackNode() override { ++destroyed; }
    void paintSelf(System* s,const NodeState* st,const Rect* r,const Rect* c,RenderContext* ctx) override
    {
        check(!s && st==&state && r==&bounds && c==&clip && ctx==&context,"paint argument forwarding");
        calls.push_back(id);
        if (owner) owner->setChild(0,replacement);
    }
    bool updateSelf(System* s,const NodeState* st,const Rect* r,float delta) override
    {
        check(!s && st==&state && r==&bounds && delta==-0.375f,"update argument forwarding");
        calls.push_back(id);
        if (owner) owner->setChild(0,replacement);
        return accept;
    }
    int id; bool accept; SplitNode* owner; Node* replacement;
};
struct ObservedSplit : SplitNode {
    bool accept = true;
    void paintSelf(System*,const NodeState*,const Rect*,const Rect*,RenderContext*) override { calls.push_back(2); }
    bool updateSelf(System*,const NodeState*,const Rect*,float) override { calls.push_back(2); return accept; }
};
struct Visitor : NodeVisitor {
    Visitor(SplitNode* p,Node* c,Node* n) : NodeVisitor(0),parent(p),child(c),next(n) {}
    void traverse0x08(System* s,Node*& n,const Rect* r,NodeState* st) override
    { check(!s && r==&bounds && st==&state && &n==&parent->getChild(1),"child visitor gets live child reference and original state"); calls.push_back(8); n=child; }
    void visit(System* s,Node*& n,const Rect* r,NodeState* st) override
    { check(!s && r==&bounds && st==&state && &n==&parent->getChild(0),"next visitor gets live next reference and original state"); calls.push_back(4); n=next; }
    SplitNode* parent; Node* child; Node* next;
};
struct PropertyStream : Stream {
    PropertyStream(SplitNode* p,Node* c,Node* n):parent(p),child(c),next(n),acceptNext(true),acceptChild(true) {}
    bool streamNode0x88(System* s,Node*& n,const char* label,TypeDescriptor* type) override
    { check(!s && !type && !std::strcmp(label,"Next node") && &n==&parent->getChild(0),"base stream label/type/live next"); calls.push_back(88); n=next; return acceptNext; }
    bool streamNode0x84(System* s,Node*& n,const char* label,TypeDescriptor* type) override
    { check(!s && !type && !std::strcmp(label,"Split node") && &n==&parent->getChild(1),"split stream label/type/live child"); calls.push_back(84); n=child; return acceptChild; }
    SplitNode* parent; Node* child; Node* next; bool acceptNext,acceptChild;
};
}
int main()
{
    // Look up the descriptor before any explicit derived getter/construction.
    TypeDescriptor* type = TypeDescriptor::find("dice::meme::Bf2SplitNode");
    check(type != nullptr, "derived descriptor is eagerly registered");
    check(TypeDescriptor::find("Bf2SplitNode") == type, "short descriptor lookup shares eager identity");
    check(!TypeDescriptor::find("bf2splitnode"), "descriptor lookup is case-sensitive");
    check(type == Bf2SplitNode::getClassType(), "getter returns registered descriptor");
    check(!std::strcmp(type->getName(), "dice::meme::Bf2SplitNode"), "full descriptor name");
    check(type->getBaseType() == SplitNode::getClassType(), "direct descriptor base is SplitNode");
    check(type->getBaseType()->getBaseType() == Node::getClassType(), "descriptor ancestry includes Node");
    check(type->isDerivedFrom(Object::getClassType()) && type->isDerivedFrom(RuntimeObject::getClassType()), "full object ancestry");
    check(type->getInstanceSize() == 16 && !type->getDescription(), "descriptor native size and null description");
    const int splitCount = SplitNode::getClassType()->getInstanceCount();
    const int nodeCount = Node::getClassType()->getInstanceCount();
    check(type->getInstanceCount() == 0, "derived count starts zero");
    type->removeInstance();
    check(type->getInstanceCount() == -1, "derived count decrement has no zero guard");
    type->addInstance(); type->addInstance();
    check(type->getInstanceCount() == 1, "derived count increments independently");
    check(SplitNode::getClassType()->getInstanceCount() == splitCount && Node::getClassType()->getInstanceCount() == nodeCount, "base counts remain independent");
    type->removeInstance();
    RuntimeObject* ordinary = type->create();
    check(ordinary && ordinary->getType() == type && ordinary->getObjectSize() == 16, "real descriptor factory publishes derived object");
    Bf2SplitNode* factoryNode = static_cast<Bf2SplitNode*>(ordinary);
    check(!factoryNode->getName() && !factoryNode->getChild(0) && !factoryNode->getChild(1), "factory constructs null name and both null links");
    check(factoryNode->slot_0x24() && type->getInstanceCount() == 0, "ordinary factory does not count named object");
    delete ordinary;
    check(type->getInstanceCount() == 0, "ordinary destruction does not count");
    RuntimeObject* managed = nullptr;
    check(createRuntimeObject(managed, type) && managed && managed->getType() == type, "count-managed helper uses actual factory");
    check(type->getInstanceCount() == 1, "managed creation increments derived count");
    check(destroyRuntimeObject(managed) && !managed && type->getInstanceCount() == 0, "managed destruction clears pointer and count");
    check(sizeof(SplitNode)==12 && sizeof(Bf2SplitNode)==16,"x86 native split layouts");
    SplitNode split;
    Node first,second;
    check(split.getObjectSize()==12 && split.getChildCount()==2,"base size and exact two-child count");
    check(!split.getChild(0) && !split.getChild(1),"constructor clears both links");
    check(!split.getName() && !split.slot_0x24(),"base inherits unnamed object defaults");
    split.setChild(0,&first); split.setChild(1,&second);
    split.setChild(-1,nullptr); split.setChild(2,nullptr);
    check(split.getChild(0)==&first && split.getChild(1)==&second,"invalid setters leave both links unchanged");
    check(&split.getChild(-1)==&split.getChild(0) && &split.getChild(2)==&split.getChild(0),"invalid getters select next");
    split.getChild(1)=nullptr;
    check(!split.getChild(1),"getChild returns writable child reference");
    split.setChild(0,nullptr);
    Visitor visitor(&split,&second,&first);
    calls.clear(); split.visitChildren(nullptr,&state,&bounds,&visitor);
    check(calls==std::vector<int>({8,4}) && split.getChild(0)==&first && split.getChild(1)==&second,"visitor order and mutation even for null links");
    PropertyStream stream(&split,nullptr,nullptr);
    calls.clear(); check(split.streamProperties(nullptr,&stream),"property streaming succeeds");
    check(calls==std::vector<int>({88,84}) && !split.getChild(0) && !split.getChild(1),"stream base-before-child order and live replacement");
    stream.acceptNext=false; stream.next=&first; stream.child=&second;
    calls.clear(); check(!split.streamProperties(nullptr,&stream),"base stream failure propagates");
    check(calls==std::vector<int>({88}) && split.getChild(0)==&first && !split.getChild(1),"base failure preserves side effect and stops child stream");
    stream.acceptNext=true; stream.acceptChild=false;
    calls.clear(); check(!split.streamProperties(nullptr,&stream),"child stream failure propagates");
    check(calls==std::vector<int>({88,84}) && split.getChild(1)==&second,"child side effect retained on failure");
    ObservedSplit parent;
    CallbackNode child(1),next(3),replacement(4);
    parent.setChild(1,&child); parent.setChild(0,&next);
    calls.clear(); parent.paint(nullptr,&state,&bounds,&clip,&context);
    check(calls==std::vector<int>({1,2,3}),"paint child then own paintSelf then next");
    calls.clear(); check(parent.update(nullptr,&state,&bounds,-0.375f),"complete update succeeds");
    check(calls==std::vector<int>({1,2,3}),"update child then own updateSelf then next");
    child.accept=false;
    calls.clear(); check(!parent.update(nullptr,&state,&bounds,-0.375f),"child false propagates");
    check(calls==std::vector<int>({1}),"child failure stops own/next update");
    child.accept=true; parent.accept=false;
    calls.clear(); check(!parent.update(nullptr,&state,&bounds,-0.375f),"own false propagates");
    check(calls==std::vector<int>({1,2}),"own failure stops next update");
    parent.accept=true; next.accept=false;
    calls.clear(); check(!parent.update(nullptr,&state,&bounds,-0.375f),"next false propagates");
    check(calls==std::vector<int>({1,2,3}),"next failure comes after child and self");
    next.accept=true; child.owner=&parent; child.replacement=&replacement;
    calls.clear(); parent.paint(nullptr,&state,&bounds,&clip,&context);
    check(calls==std::vector<int>({1,2,4}),"child paint may replace later next link");
    parent.setChild(0,&next);
    calls.clear(); check(parent.update(nullptr,&state,&bounds,-0.375f),"mutating child update succeeds");
    check(calls==std::vector<int>({1,2,4}),"child update may replace later next link");
    child.replacement=nullptr;
    calls.clear(); parent.paint(nullptr,&state,&bounds,&clip,&context);
    check(calls==std::vector<int>({1,2}),"child callback may clear next");

    char source[]="Original";
    Bf2SplitNode named(source);
    const char* copy=named.getName();
    source[0]='X';
    check(copy!=source && !std::strcmp(copy,"Original"),"constructor deep-copies input name");
    check(named.getObjectSize()==16 && named.getChildCount()==2 && named.slot_0x24(),"named subclass size/child count/slot24");
    check(!named.getChild(0) && !named.getChild(1),"named constructor retains cleared base links");
    check(named.setName(""),"empty name setter succeeds");
    check(named.getName() && !*named.getName(),"empty name is allocated rather than null");
    check(named.setName(nullptr) && !named.getName(),"null name setter frees and clears");
    const char embedded[]={'a','b',0,'c',0};
    check(named.setName(embedded) && !std::strcmp(named.getName(),"ab"),"name clone stops at first null");
    Bf2SplitNode unnamed(nullptr);
    check(!unnamed.getName(),"null constructor name remains null");
    check(named.getType() == type && unnamed.getType() == type, "named and null-name constructors share actual descriptor");
    PropertyStream namedStream(&named, &first, &second);
    calls.clear();
    check(named.streamProperties(nullptr, &namedStream), "named subclass uses inherited properties");
    check(calls == std::vector<int>({88,84}) && !std::strcmp(named.getName(), "ab"), "inherited properties preserve name and next-before-child order");
    check(named.getChild(0) == &second && named.getChild(1) == &first, "inherited properties mutate actual named links");
    for (int eventId : {7,9,13,14,17}) {
        NodeInputEvent event = {eventId, -3, 0.25f, -0.75f, 99};
        check(!named.dispatchEvent(nullptr, &bounds, &state, &event), "published default event returns real false");
        check(named.getChild(0) == &second && named.getChild(1) == &first, "default event preserves both named links");
    }
    int before=destroyed;
    {
        Bf2SplitNode owner("parent");
        owner.setChild(0,&child); owner.setChild(1,&next);
    }
    check(destroyed==before,"split destruction does not own either child link");
    check(type->getInstanceCount() == 0, "ordinary named operations leave derived count unchanged");
    std::printf("SplitNode/Bf2SplitNode: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
