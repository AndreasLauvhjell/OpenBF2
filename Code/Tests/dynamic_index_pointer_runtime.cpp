// [TEMP] Native tests of the real complete DynamicIndexPointer family. Access
// subclasses only arrange evidenced fields; callback subclasses record calls.
#include "../BF2/External/BFMeme/DynamicIndexPointer.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/EventQueue.h"
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include <cstdio>
#include <cstring>
#include <limits>
#include <cmath>
#include <new>
using namespace dice::meme;
namespace {
int checks=0, failures=0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line%d: %s\n",__LINE__,#x); } } while(0)
class InputSystem : public System
{
public:
    InputSystem() : System(800,600) {}
    void queue(EventQueue* q) { m_eventQueue=q; }
    void position(float x,float y,float px,float py)
    { m_pointerX=x; m_pointerY=y; m_previousPointerX=px; m_previousPointerY=py; }
    bool previous(float x,float y) const { return m_previousPointerX==x && m_previousPointerY==y; }
};
class InspectPointer : public DynamicIndexPointer
{
public:
    void enabled(bool v) { m_enabled=v; }
    void locked(bool v) { m_locked=v; }
    bool locked() const { return m_locked; }
    void order(int index,int value) { m_indexOrder[index]=value; }
    int order(int index) const { return m_indexOrder[index]; }
    bool orderIs(int a,int b,int c,int d) const
    { return m_indexOrder[0]==a && m_indexOrder[1]==b && m_indexOrder[2]==c && m_indexOrder[3]==d; }
    int count() const { return m_assignedCount; }
    void count(int value) { m_assignedCount=value; }
    int last() const { return m_lastInputIndex; }
    float timer() const { return m_timer; }
    void timer(float v) { m_timer=v; }
    bool zeroWords() const { return !m_field030 && !m_field034; }
};
struct QueueView { QueuedEvent *free,*first,*last; };
QueueView view(EventQueue& q) { QueueView v; std::memcpy(&v,&q,sizeof(v)); return v; }
void checkEvent(EventQueue& q,int type,int index,float first,float second)
{
    QueuedEvent* e=view(q).last;
    CHECK(e && e->type==type && e->index==index && e->first==first && e->second==second && !e->field14 && !e->next);
}
struct PropertyStream : Stream
{
    PropertyStream() : calls(0),failAt(-1),write(false),target(nullptr) {}
    bool streamInt(int* value,const char* label) override
    {
        CHECK(target && calls<4 && value==reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(target)+24)+calls);
        CHECK(std::strcmp(label,"Index order")==0);
        if(write) *value=10+calls;
        return calls++!=failAt;
    }
    bool streamBool(bool* value,const char* label) override
    {
        CHECK(calls==4 && std::strcmp(label,"Is locked")==0);
        CHECK(value==reinterpret_cast<bool*>(reinterpret_cast<unsigned char*>(target)+20));
        if(write) *value=true;
        return calls++!=failAt;
    }
    int calls,failAt;
    bool write;
    DynamicIndexPointer* target;
};
struct GatePointer : InspectPointer
{
    GatePointer() : permit(false),calls(0),seen(nullptr) {}
    bool streamProperties(System* s,Stream*) override { ++calls; seen=s; return permit; }
    bool permit; int calls; System* seen;
};
struct Graphic : Node
{
    explicit Graphic(InspectPointer* p) : pointer(p),calls(0),seenTimer(0),seenDelta(0) {}
    bool updateSelf(System*,const NodeState* state,const Rect*,float delta) override
    { ++calls; seenTimer=pointer->timer(); seenDelta=delta; CHECK(state->target==1 && state->current==1); return false; }
    InspectPointer* pointer; int calls; float seenTimer,seenDelta;
};
}
int main()
{
    CHECK(sizeof(DynamicIndexPointer)==60);
    alignas(InspectPointer) unsigned char storage[sizeof(InspectPointer)];
    std::memset(storage,0xA5,sizeof(storage));
    InspectPointer* initial=new(storage) InspectPointer;
    CHECK(!initial->isEnabled() && !initial->locked() && initial->orderIs(100,100,100,100));
    CHECK(initial->count()==0 && initial->last()==0 && initial->zeroWords() && initial->timer()==0);
    CHECK(storage[17]==0xA5 && storage[18]==0xA5 && storage[19]==0xA5);
    CHECK(storage[21]==0xA5 && storage[22]==0xA5 && storage[23]==0xA5);
    initial->~InspectPointer();
    CHECK(storage[21]==0xA5 && storage[22]==0xA5 && storage[23]==0xA5);

    InputSystem system; EventQueue queue(10); system.queue(&queue);
    InspectPointer pointer;
    CHECK(pointer.getObjectSize()==60);
    // Direct derived events have no enabled check, unlike the guarded wrappers.
    CHECK(pointerEvent1(&system,&pointer,1,3,2) && !view(queue).first && pointer.last()==0);
    CHECK(pointer.event1(&system,-9,3,7)); checkEvent(queue,1,1,-9,7);
    CHECK(pointer.orderIs(100,100,1,100) && pointer.last()==2 && pointer.count()==0);
    QueuedEvent* previous=view(queue).last;
    CHECK(pointerEvent2(&system,&pointer,99,3) && view(queue).last==previous);
    CHECK(pointer.event2(&system,99,3)); checkEvent(queue,2,1,99,0);
    pointer.locked(true);
    // An existing unlocked mapping does not consume a count when already <=1.
    CHECK(pointer.event1(&system,0,3,0) && pointer.count()==0); checkEvent(queue,1,1,0,0);
    pointer.order(0,100); pointer.order(1,100); pointer.order(2,100); pointer.order(3,100);
    const int inputs[]={3,1,3,4,2}; const int mapped[]={1,2,1,3,4};
    const int counts[]={1,2,2,3,4};
    for(int i=0;i<5;++i) {
        CHECK(pointer.event1(&system,-20-i,inputs[i],50+i));
        checkEvent(queue,1,mapped[i],float(-20-i),float(50+i));
        CHECK(pointer.count()==counts[i] && pointer.last()==inputs[i]-1);
    }
    CHECK(pointer.orderIs(2,4,1,3));
    for(int i=1;i<=4;++i) {
        CHECK(pointer.event2(&system,25,i)); checkEvent(queue,2,pointer.order(i-1),25,0);
    }
    CHECK(pointer.event2(&system,8,0)); checkEvent(queue,2,1,8,0);
    CHECK(pointer.event2(&system,-8,-7)); checkEvent(queue,2,1,-8,0);
    CHECK(pointer.count()==4 && pointer.last()==1); // event2 changes neither field
    CHECK(pointer.event1(&system,3,0,4)); checkEvent(queue,1,1,3,4);
    CHECK(pointer.orderIs(100,100,100,100) && pointer.count()==4 && pointer.last()==0);
    CHECK(pointer.event1(&system,6,2,8)); checkEvent(queue,1,5,6,8);
    CHECK(pointer.count()==5 && pointer.order(1)==5);
    // Signed comparison: a negative serialized order is kept and forwarded.
    pointer.order(0,-3);
    CHECK(pointer.event1(&system,7,1,9) && pointer.count()==5); checkEvent(queue,1,-3,7,9);
    pointer.locked(false);
    CHECK(pointer.event1(&system,1,-2,2) && pointer.last()==-3); checkEvent(queue,1,1,1,2);
    CHECK(pointer.orderIs(100,100,100,100) && pointer.count()==5);
    CHECK(pointer.event1(&system,1,5,2) && pointer.last()==4 && pointer.orderIs(100,100,100,100));
    pointer.enabled(true);
    CHECK(pointerEvent1(&system,&pointer,4,4,5)); checkEvent(queue,1,1,4,5);
    CHECK(pointer.orderIs(100,100,100,1));
    CHECK(pointer.event4(&system,6,17,7)); checkEvent(queue,4,17,6,7);
    CHECK(pointer.event5(&system,8,19)); checkEvent(queue,5,19,8,0);

    PropertyStream properties; properties.target=&pointer; properties.write=true;
    CHECK(pointer.visit0x3c(&system,&properties) && properties.calls==5);
    CHECK(pointer.orderIs(10,11,12,13) && pointer.locked());
    CHECK(pointer.count()==5 && pointer.zeroWords()); // only the five properties are visited
    for(int failure=0;failure<5;++failure) {
        PropertyStream stopped; stopped.target=&pointer; stopped.failAt=failure;
        CHECK(!pointer.visit0x3c(&system,&stopped) && stopped.calls==failure+1);
    }
    GatePointer gate; PropertyStream gated; gated.target=&gate;
    CHECK(!gate.visit0x3c(&system,&gated) && gate.calls==1 && gate.seen==&system && gated.calls==0);
    gate.permit=true;
    CHECK(gate.visit0x3c(&system,&gated) && gate.calls==2 && gated.calls==5);

    pointer.enabled(false); pointer.timer(0.125f); system.position(3,4,8,9);
    CHECK(updatePointer(&system,&pointer,0.5f) && pointer.timer()==0.125f && system.previous(8,9));
    CHECK(pointer.update(&system,0.5f) && pointer.timer()==-0.375f && system.previous(3,4));
    CHECK(pointer.update(&system,2) && pointer.timer()==-0.375f);
    pointer.timer(1);
    CHECK(pointer.update(&system,-0.5f) && pointer.timer()==1.5f);
    pointer.timer(0);
    CHECK(pointer.update(&system,-0.5f) && pointer.timer()==0);
    const float nan=std::numeric_limits<float>::quiet_NaN(); pointer.timer(nan);
    CHECK(pointer.update(&system,1) && std::isnan(pointer.timer()));
    pointer.timer(1);
    CHECK(pointer.update(&system,nan) && std::isnan(pointer.timer()));
    pointer.timer(1); pointer.enabled(true);
    Graphic graphic(&pointer); pointer.setGraphic(&system,&graphic);
    CHECK(!pointer.update(&system,0.25f));
    CHECK(graphic.calls==1 && graphic.seenTimer==0.75f && graphic.seenDelta==0.25f);

    TypeDescriptor* type=DynamicIndexPointer::getClassType();
    const int dynamicCount=type->getInstanceCount();
    const int baseCount=Pointer::getClassType()->getInstanceCount();
    CHECK(pointer.getType()==type && type->getInstanceSize()==60);
    CHECK(type->getBaseType()==Pointer::getClassType() && type->isDerivedFrom(Object::getClassType()));
    CHECK(std::strcmp(type->getName(),"dice::meme::DynamicIndexPointer")==0);
    CHECK(TypeDescriptor::find("DynamicIndexPointer")==type);
    Object* object=nullptr;
    CHECK(createObject(&system,object,type) && object && object->getObjectSize()==60);
    CHECK(type->getInstanceCount()==dynamicCount+1 && Pointer::getClassType()->getInstanceCount()==baseCount);
    CHECK(createObject(&system,object,type) && type->getInstanceCount()==dynamicCount+1);
    CHECK(deleteObject(&system,object) && !object && type->getInstanceCount()==dynamicCount);
    CHECK(Pointer::getClassType()->getInstanceCount()==baseCount);
    system.queue(nullptr);
    std::printf("DynamicIndexPointer: %d/%d checks passed (/FORCE uncalled surrounding UI frontiers).\n",checks-failures,checks);
    return failures ? 1 : 0;
}
