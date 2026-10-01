// [TEMP] Real EventQueue and six real input visitors. Node callbacks record only
// event routing/traversal; the unpublished Node dispatch implementation is never
// called. Event5's unwritten index is seeded explicitly for the direct visitor.
#include "../BF2/External/BFMeme/EventQueue.h"
#include "../BF2/External/BFMeme/InputNodeVisitors.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/Style.h"
#include <cstdio>
#include <cstring>
#include <new>
#include <limits>
#include <vector>
using namespace dice::meme;
namespace {
int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL %d: %s\n", __LINE__, #x); } } while (0)
struct TestSystem : System {
    TestSystem() : System(800, 600) {}
    void lock(int n) { m_lockCount = n; }
};
struct Seen { int node; NodeInputEvent event; float target; };
std::vector<Seen> seen;
struct TestNode : Node {
    explicit TestNode(int id) : id(id), children(0), updates(0), handled(false),
        mutate(false), replacement(nullptr), append(nullptr) {}
    void visitChildren(System* s, NodeState* state, const Rect* rect, NodeVisitor* visitor) override
    {
        ++children;
        CHECK(rect == s->getBounds() && state == s->getNodeState());
        Node::visitChildren(s, state, rect, visitor);
        if (mutate) state->target = 2.0f;
        if (replacement) s->getRootNode() = replacement;
    }
    bool dispatchEvent(System* s, const Rect* rect, const NodeState* state, const NodeInputEvent* event) override
    {
        CHECK(rect == s->getBounds() && state == s->getNodeState());
        Seen value = {id, *event, state->target}; seen.push_back(value);
        return handled;
    }
    bool update(System* s, const NodeState* state, const Rect* rect, float delta) override
    {
        ++updates;
        CHECK(delta == 0.0f && state == s->getNodeState() && rect == s->getBounds());
        if (append) {
            EventQueue* q = append; append = nullptr;
            q->enqueue(s, QueuedEvent(4, -19, -8.75f, 9.75f, 0xFACE1234));
        }
        return false; // [TEMP] Recorded failure must not terminate queue processing.
    }
    int id, children, updates;
    bool handled, mutate;
    Node* replacement;
    EventQueue* append;
};
struct QueueView { QueuedEvent *free, *first, *last; };
QueueView view(EventQueue& q) { QueueView v; std::memcpy(&v, &q, sizeof v); return v; }
int length(QueuedEvent* e) { int n = 0; for (; e; e = e->next) { if (++n > 32) return -1; } return n; }
void state(TestSystem& s, float target = -3.0f, float current = -3.0f)
{ s.getNodeState()->target = target; s.getNodeState()->current = current; }
void reset(TestSystem& s, TestNode& n)
{ seen.clear(); state(s); s.getRootNode() = &n; s.lock(0); n.children = n.updates = 0; }
void queueOne(TestSystem& s, int type, int index, float first, float second)
{ EventQueue q(0); q.enqueue(&s, QueuedEvent(type,index,first,second,0x89ABCDEF)); CHECK(q.process(&s)); CHECK(!view(q).first && !view(q).last); }
}
int main()
{
    TestSystem s;
    TestNode a(1), b(2), c(3);
    CHECK(sizeof(EventQueue) == 12 && sizeof(QueuedEvent) == 24);
    CHECK(sizeof(Event1NodeVisitor) == 24 && sizeof(Event2NodeVisitor) == 16);
    CHECK(sizeof(Event4NodeVisitor) == 20 && sizeof(Event5NodeVisitor) == 16);
    CHECK(sizeof(PressStateNodeVisitor) == 8 && sizeof(ReleaseStateNodeVisitor) == 8);
    {
        EventQueue q(2); CHECK(length(view(q).free) == 3); CHECK(q.process(nullptr));
        CHECK(!view(q).first && !view(q).last && length(view(q).free) == 3);
        for (int i=0; i<5; ++i) q.enqueue(&s, QueuedEvent(i+20,i,1,2,0x12345678));
        CHECK(length(view(q).first) == 5 && !view(q).free);
        QueuedEvent* first = view(q).first;
        CHECK(first->type == 20 && first->next->type == 21 && view(q).last->type == 24);
        CHECK(first->field14 == 0x12345678);
        CHECK(q.process(&s)); CHECK(length(view(q).free) == 5 && !view(q).first && !view(q).last);
        QueuedEvent* recycled = view(q).free;
        q.enqueue(&s, QueuedEvent(3,0,0,0,0)); CHECK(view(q).first == recycled);
        CHECK(q.process(&s) && length(view(q).free) == 5);
    }
    reset(s,a); a.setChild(0,&b);
    Event1NodeVisitor first(-128, 0x10041, 7);
    first.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.size()==2 && seen[0].node==2 && seen[1].node==1);
    CHECK(seen[0].event.type==1 && seen[0].event.index==0x10041);
    CHECK(seen[0].event.first==-128 && seen[0].event.second==7 && !seen[0].event.field10);
    seen.clear(); b.handled=true; Event1NodeVisitor consume(2,3,4);
    consume.traverse0x08(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.size()==1 && seen[0].node==2);
    seen.clear(); consume.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.empty()); // handled flag persists; child traversal still occurs.
    Event4NodeVisitor all(2,-128,-3); all.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.size()==2 && seen[0].node==2 && seen[1].node==1);
    CHECK(seen[0].event.type==4 && seen[0].event.index==-128 && seen[0].event.second==-3);
    b.handled=false; a.setChild(0,nullptr);
    reset(s,a); a.mutate=true; Event2NodeVisitor afterChildren(9,10);
    afterChildren.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.size()==1 && seen[0].event.type==3 && seen[0].target==2.0f);
    a.mutate=false;
    reset(s,a); Event2NodeVisitor negative(9,10);
    negative.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.size()==1 && seen[0].event.type==2 && seen[0].event.second==0 && !seen[0].event.field10);
    state(s,std::numeric_limits<float>::quiet_NaN(),-3); seen.clear();
    negative.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.size()==1 && seen[0].event.type==3);
    const float blocked[] = {0.0f, -0.0f, 1.0f, std::numeric_limits<float>::quiet_NaN()};
    for(float current:blocked) {
        reset(s,a); state(s,-3,current);
        Event4NodeVisitor visitor(1,2,3); visitor.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
        CHECK(seen.empty() && a.children==0);
    }
    reset(s,a);
    alignas(Event5NodeVisitor) unsigned char storage[sizeof(Event5NodeVisitor)];
    std::memset(storage,0xA5,sizeof storage);
    int seededIndex=-128; std::memcpy(storage+12,&seededIndex,4);
    Event5NodeVisitor* fifth = new(storage) Event5NodeVisitor(0x10041);
    int retained=0; std::memcpy(&retained,storage+12,4); CHECK(retained==-128);
    fifth->traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.size()==1 && seen[0].event.type==5 && seen[0].event.index==-128 && seen[0].event.first==0x10041);
    seen.clear(); state(s,2,-3); fifth->traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
    CHECK(seen.size()==1 && seen[0].event.type==6); fifth->~Event5NodeVisitor();
    const float before[] = {2,3,-3,-4,1,-2,std::numeric_limits<float>::quiet_NaN()};
    const float pressed[] = {3,3,-4,-4,1,-2,std::numeric_limits<float>::quiet_NaN()};
    for(int i=0;i<7;++i) {
        reset(s,a); state(s,before[i],0); PressStateNodeVisitor press;
        press.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
        CHECK(i==6 ? s.getNodeState()->target!=s.getNodeState()->target : s.getNodeState()->target==pressed[i]);
        CHECK(a.children==(i<4 ? 1:0));
        a.children=0; ReleaseStateNodeVisitor release;
        release.traverse(&s,s.getRootNode(),s.getBounds(),s.getNodeState());
        CHECK(a.children==1);
        const float expected=i<2 ? 2.0f : i<4 ? -3.0f : before[i];
        CHECK(i==6 ? s.getNodeState()->target!=s.getNodeState()->target : s.getNodeState()->target==expected);
    }
    reset(s,a); queueOne(s,4,0x10041,-128.75f,7.75f);
    CHECK(seen.size()==1 && seen[0].event.first==-128 && seen[0].event.second==7 && seen[0].event.index==0x10041);
    CHECK(a.updates==1);
    reset(s,a); queueOne(s,1,9,1.75f,1.5f);
    CHECK(seen.size()==1 && seen[0].target==-4 && s.getNodeState()->target==-4);
    reset(s,a); s.lock(1); queueOne(s,1,9,1,1);
    CHECK(seen.size()==1 && seen[0].target==-3);
    reset(s,a); state(s,-4,-3); queueOne(s,2,0,1.5f,999);
    CHECK(seen.size()==1 && seen[0].event.type==2 && seen[0].target==-4 && s.getNodeState()->target==-3);
    reset(s,a); state(s,-4,-3); queueOne(s,2,0,0,0);
    CHECK(s.getNodeState()->target==-4);
    reset(s,a); state(s,3,-3); queueOne(s,2,0,1,0);
    CHECK(seen[0].event.type==3 && s.getNodeState()->target==2);
    reset(s,a); queueOne(s,3,0,1,1); CHECK(seen.empty() && !a.children && a.updates==1);
    reset(s,a); state(s,-3,0); queueOne(s,5,99,1,1); CHECK(seen.empty() && !a.children && a.updates==1);
    s.getRootNode()=nullptr; queueOne(s,5,99,1,1); // no indeterminate index read.
    reset(s,a); a.replacement=&c; c.updates=0;
    queueOne(s,4,3,4,5); CHECK(seen.size()==1 && seen[0].node==3 && c.updates==1 && !a.updates);
    a.replacement=nullptr;
    reset(s,a); {
        EventQueue q(0); a.append=&q;
        q.enqueue(&s,QueuedEvent(4,1,2,3,0)); CHECK(q.process(&s));
        CHECK(a.updates==2 && seen.size()==2 && seen[0].event.index==1 && seen[1].event.index==-19);
        CHECK(seen[1].event.first==-8 && seen[1].event.second==9);
        CHECK(!view(q).first && !view(q).last && length(view(q).free)==2);
    }
    std::printf("Event dispatch runtime: %d/%d checks passed\n",checks-failures,checks);
    return failures ? 1:0;
}
