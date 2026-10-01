// [TEMP] Real Pointer/EventQueue tests. Callback subclasses only observe or
// provide a specific callback result; no production method under test is replaced.
#include "../BF2/External/BFMeme/EventQueue.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#if !defined(BFMEME_QUEUE_ONLY)
#include "../BF2/External/BFMeme/Pointer.h"
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/Stream.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#endif
using namespace dice::meme;
namespace {
int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line%d: %s\n", __LINE__, #x); } } while(0)
// Allocation observer uses fixed storage and the real CRT heap. It never changes
// allocation success or returned data. Tracking is enabled only around queue tests.
struct Allocation { void* address; bool freed; } allocations[64];
void* releaseOrder[64];
bool tracking = false;
int allocated = 0, released = 0, duplicateRelease = 0;
void startTracking() { allocated = released = duplicateRelease = 0; tracking = true; }
void recordRelease(void* address)
{
    if (tracking && address) {
        for (int i = 0; i < allocated; ++i) if (allocations[i].address == address) {
            if (allocations[i].freed) ++duplicateRelease;
            allocations[i].freed = true; releaseOrder[released++] = address; return;
        }
    }
}
// Read-only fixture snapshot of the evidenced three-pointer queue layout.
// No private access macro, production accessor or intrusive test friend is added.
struct QueueView { QueuedEvent *free, *first, *last; };
QueueView view(const EventQueue& queue)
{ QueueView result; std::memcpy(&result, &queue, sizeof(result)); return result; }
int chainLength(QueuedEvent* node)
{ int count = 0; for (; node && count < 64; node = node->next) ++count; return count; }
void queueTests()
{
    CHECK(sizeof(EventQueue) == 12 && sizeof(QueuedEvent) == 24);
    startTracking();
    {
        EventQueue queue(0);
        const QueueView q = view(queue);
        CHECK(allocated == 1 && q.free && !q.first && !q.last && !q.free->next);
        CHECK(q.free->type == 0 && q.free->index == 0 && q.free->first == 0 &&
              q.free->second == 0 && q.free->field14 == 0);
    }
    CHECK(released == 1 && duplicateRelease == 0); tracking = false;
    startTracking();
    {
        EventQueue queue(3);
        QueueView q = view(queue);
        CHECK(allocated == 4 && chainLength(q.free) == 4 && !q.first && !q.last);
        QueuedEvent* pool[4];
        QueuedEvent* cursor = q.free;
        for (int i=0; i<4; ++i) { pool[i] = cursor; cursor = cursor->next; }
        for (int i=0; i<5; ++i) {
            QueuedEvent event(i+1, -17-i, -0.0f, 19.25f+i, 0x89ABCDEFu-i);
            queue.enqueue(nullptr, event);
            q = view(queue);
            CHECK(q.first == pool[0] && q.last && !q.last->next);
            CHECK(std::memcmp(q.last, &event, sizeof(event)) == 0);
            CHECK(allocated == (i<4 ? 4 : 5));
            if (i<4) CHECK(q.last == pool[i]);
        }
        CHECK(chainLength(q.first) == 5 && !q.free);
    }
    CHECK(released == 5 && duplicateRelease == 0); tracking = false;
    QueuedEvent* expectedRelease[4];
    startTracking();
    {
        EventQueue queue(3);
        QueuedEvent event(21, 7, 2.5f, -9.0f, 0xDEADBEEF);
        queue.enqueue(nullptr, event); queue.enqueue(nullptr, event);
        const QueueView q = view(queue);
        CHECK(chainLength(q.free) == 2 && chainLength(q.first) == 2 && allocated == 4);
        expectedRelease[0]=q.free; expectedRelease[1]=q.free->next;
        expectedRelease[2]=q.first; expectedRelease[3]=q.first->next;
    }
    CHECK(released == 4 && duplicateRelease == 0); tracking = false;
    for (int i=0; i<4; ++i) CHECK(releaseOrder[i]==expectedRelease[i]);
    // Deliberately non-null source link: retail copies all six words after
    // clearing last->next, so destruction follows this independently allocated node.
    startTracking();
    {
        EventQueue queue(0);
        QueuedEvent* extra = new QueuedEvent(6, 8, 1, 2, 3);
        QueuedEvent event(99, -100, -0.0f, 0.5f, 0x10203040);
        event.next = extra;
        queue.enqueue(nullptr, event);
        const QueueView q = view(queue);
        CHECK(q.first == q.last && q.last->next == extra);
        CHECK(std::memcmp(q.last, &event, sizeof(event)) == 0 && chainLength(q.first) == 2);
    }
    CHECK(allocated == 2 && released == 2 && duplicateRelease == 0); tracking = false;
}
}
void* operator new(std::size_t size)
{
    void* result = std::malloc(size ? size : 1);
    if (!result) throw std::bad_alloc();
    if (tracking && allocated < 64) allocations[allocated++] = {result, false};
    return result;
}
void operator delete(void* address) noexcept { recordRelease(address); std::free(address); }
void operator delete(void* address, std::size_t) noexcept { recordRelease(address); std::free(address); }

#if !defined(BFMEME_QUEUE_ONLY)
namespace {
class CursorSystem : public System
{
public:
    CursorSystem() : System(800, 600), callbackCount(0), seenX(0), seenY(0), seenStoredX(0), seenStoredY(0) {}
    bool pointerPositionChanged(float* x, float* y) override
    {
        ++callbackCount; seenX = *x; seenY = *y;
        seenStoredX = m_pointerX; seenStoredY = m_pointerY;
        *x = -999; *y = -888; return false;
    }
    void coordinates(float x, float y, float previousX, float previousY)
    { m_pointerX=x; m_pointerY=y; m_previousPointerX=previousX; m_previousPointerY=previousY; }
    bool coordinatesAre(float x, float y, float px, float py) const
    { return m_pointerX==x && m_pointerY==y && m_previousPointerX==px && m_previousPointerY==py; }
    void installQueue(EventQueue* queue) { m_eventQueue = queue; }
    void installPointer(Object* pointer) { m_field020 = pointer; }
    void installRegistry(NameTreeNode* registry) { m_registry = registry; }
    bool closed() const { return !m_field020 && !m_field018 && !m_registry && !m_eventQueue && !m_field024; }
    int callbackCount;
    float seenX, seenY, seenStoredX, seenStoredY;
};
class CursorPointer : public Pointer
{
public:
    void enable(bool value) { m_enabled = value; }
    void dimensions(float width, float height) { m_width = width; m_height = height; }
    bool initial() const { return m_width==20 && m_height==20 && !m_graphic && !m_enabled; }
    Node* graphic() const { return m_graphic; }
};
class Graphic : public Node
{
public:
    Graphic() : paints(0), updates(0), delta(0), accept(false), sawClip(false), seenSystem(nullptr) {}
    ~Graphic() override { ++destroyed; }
    const char* getName() override { return "fixture named graphic"; }
    void paintSelf(System* system, const NodeState* input, const Rect* area, const Rect* clip, RenderContext* render) override
    { ++paints; state=*input; bounds=*area; context=*render; sawClip=clip!=nullptr; seenSystem=system; }
    bool updateSelf(System* system, const NodeState* input, const Rect* area, float d) override
    { ++updates; state=*input; bounds=*area; delta=d; seenSystem=system; return accept; }
    int paints, updates;
    float delta;
    bool accept, sawClip;
    System* seenSystem;
    NodeState state;
    Rect bounds;
    RenderContext context;
    static int destroyed;
};
int Graphic::destroyed=0;
void pointerTests()
{
    CHECK(sizeof(Pointer)==20);
    alignas(CursorPointer) unsigned char storage[sizeof(CursorPointer)];
    std::memset(storage, 0xA5, sizeof(storage));
    CursorPointer* constructed = new(storage) CursorPointer;
    CHECK(constructed->initial() && constructed->getObjectSize()==20);
    CHECK(storage[17]==0xA5 && storage[18]==0xA5 && storage[19]==0xA5);
    constructed->~CursorPointer();
    CHECK(storage[17]==0xA5 && storage[18]==0xA5 && storage[19]==0xA5);

    CursorSystem system;
    CursorPointer pointer;
    Graphic graphic, replacement;
    system.coordinates(15, -30, 100, 200);
    CHECK(paintPointer(nullptr, nullptr) && updatePointer(nullptr, nullptr, 1));
    CHECK(positionPointer(nullptr, nullptr, 1, 2) && pointerEvent1(nullptr, nullptr, 1, 2, 3));
    CHECK(pointerEvent2(nullptr, nullptr, 1, 2));
    CHECK(updatePointer(&system, &pointer, 3.0f) && system.coordinatesAre(15,-30,100,200));
    CHECK(pointer.update(&system, 3.0f) && system.coordinatesAre(15,-30,15,-30));
    CHECK(pointer.paint(nullptr) && pointer.setPosition(&system, 2, 3));
    CHECK(system.callbackCount==0 && system.coordinatesAre(15,-30,15,-30));
    CHECK(pointer.event1(nullptr, 1, 2, 3) && pointer.event2(nullptr, 1, 2));
    CHECK(pointer.event4(nullptr, 1, 2, 3) && pointer.event5(nullptr, 1, 2));
    pointer.enable(true);
    CHECK(pointer.setPosition(&system, 81.5f, -25.25f));
    CHECK(system.callbackCount==1 && system.seenX==81.5f && system.seenY==-25.25f);
    CHECK(system.seenStoredX==81.5f && system.seenStoredY==-25.25f);
    CHECK(system.coordinatesAre(81.5f,-25.25f,15,-30)); // callback edits locals only
    system.installPointer(&pointer);
    CHECK(system.setPointerPosition(20, 40) && system.callbackCount==2);
    CHECK(system.coordinatesAre(20,40,15,-30));
    system.installPointer(nullptr);
    CHECK(system.setPointerPosition(1,2) && system.coordinatesAre(20,40,15,-30));
    pointer.setGraphic(&system, &graphic);
    pointer.dimensions(30, 50);
    CHECK(paintPointer(&system, &pointer) && graphic.paints==1 && graphic.seenSystem==&system);
    CHECK(graphic.bounds.x==5 && graphic.bounds.y==15 && graphic.bounds.width==30 && graphic.bounds.height==50 && graphic.bounds.z==0);
    CHECK(graphic.state.target==1 && graphic.state.current==1 && !graphic.sawClip);
    CHECK(!graphic.context.parent && !graphic.context.effect && graphic.context.field08==0 && graphic.context.field0c==1 &&
          graphic.context.field10==0 && graphic.context.field14==1 && graphic.context.field18==1);
    CHECK(!updatePointer(&system, &pointer, -0.25f));
    CHECK(graphic.updates==1 && graphic.delta==-0.25f && system.coordinatesAre(20,40,20,40));
    graphic.accept=true;
    CHECK(pointer.update(&system, 2.5f) && graphic.updates==2 && graphic.delta==2.5f);
    pointer.enable(false); system.coordinates(3,4,7,8);
    CHECK(updatePointer(&system,&pointer,1) && graphic.updates==2 && system.coordinatesAre(3,4,7,8));
    CHECK(pointer.update(&system,1) && graphic.updates==2 && system.coordinatesAre(3,4,3,4));
    CHECK(pointer.paint(&system) && graphic.paints==1);
    pointer.setGraphic(&system,&replacement); // named old graphic is borrowed by deleteObject
    CHECK(pointer.graphic()==&replacement && graphic.getChildCount()==1);
    const int previouslyDestroyed=Graphic::destroyed;
    Graphic* externalGraphic=new Graphic;
    { CursorPointer local; local.setGraphic(&system,externalGraphic); }
    CHECK(Graphic::destroyed==previouslyDestroyed);
    if (Graphic::destroyed==previouslyDestroyed) delete externalGraphic;
    CHECK(Graphic::destroyed==previouslyDestroyed+1);

    EventQueue queue(0); system.installQueue(&queue); pointer.enable(true);
    CHECK(pointerEvent1(&system,&pointer,-5,77,9));
    CHECK(pointerEvent2(&system,&pointer,15,-3));
    CHECK(pointer.event4(&system,-7,88,-2));
    CHECK(pointer.event5(&system,13,-11));
    QueuedEvent* event=view(queue).first;
    CHECK(event && event->type==1 && event->index==77 && event->first==-5 && event->second==9 && event->field14==0);
    event=event->next;
    CHECK(event && event->type==2 && event->index==-3 && event->first==15 && event->second==0 && event->field14==0);
    event=event->next;
    CHECK(event && event->type==4 && event->index==88 && event->first==-7 && event->second==-2 && event->field14==0);
    event=event->next;
    CHECK(event && event->type==5 && event->index==-11 && event->first==13 && event->second==0 && event->field14==0 && !event->next);
    system.installQueue(nullptr); // stack queue belongs to this fixture

    TypeDescriptor* type=Pointer::getClassType();
    const int count=type->getInstanceCount();
    CHECK(pointer.getType()==type && type->getBaseType()==Object::getClassType());
    CHECK(type->getInstanceSize()==20 && !std::strcmp(type->getName(),"dice::meme::Pointer"));
    Object* runtime=nullptr;
    CHECK(createObject(&system,runtime,type) && runtime && type->getInstanceCount()==count+1);
    CHECK(deleteObject(&system,runtime) && !runtime && type->getInstanceCount()==count);
    CHECK(createObject(&system,runtime,type) && type->getInstanceCount()==count+1);
    system.installPointer(runtime);
    system.installQueue(new EventQueue(2));
    system.installRegistry(new NameTreeRoot);
    CHECK(system.close() && system.closed() && type->getInstanceCount()==count);
    CHECK(system.close() && system.closed() && type->getInstanceCount()==count);
}
}
#endif
int main()
{
    queueTests();
#if defined(BFMEME_QUEUE_ONLY)
    std::printf("BFMeme EventQueue strict: %d/%d checks passed.\n",checks-failures,checks);
#else
    pointerTests();
    std::printf("BFMeme Pointer/EventQueue: %d/%d checks passed (/FORCE uncalled UI frontiers).\n",checks-failures,checks);
#endif
    return failures ? 1 : 0;
}
