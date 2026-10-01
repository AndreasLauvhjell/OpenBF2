// [TEMP] Bounded Node traversal tests. Observer subclasses model callback
// responses; the production Node constructor, storage, property visitor,
// paint/update traversal, typed event dispatch and NodeVisitor wrappers are exercised.
// /FORCE declarations are never reached; this is not Node publication proof.
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/Stream.h"
#include <cstdio>
#include <cstring>
#include <climits>
#include <vector>
using namespace dice::meme;
namespace {
int checks = 0, failures = 0, destroyed = 0;
void check(bool value, const char* label)
{
    ++checks;
    if (!value) { ++failures; std::printf("FAIL: %s\n", label); }
}
std::vector<int> order;
const NodeState* expectedState;
const Rect* expectedBounds;
const Rect* expectedClip;
RenderContext* expectedContext;
float expectedDelta;

struct ObservedNode : Node
{
    explicit ObservedNode(int value) : id(value), accept(true), changeNext(false), replacement(nullptr) {}
    ~ObservedNode() override { ++destroyed; }
    void paintSelf(System* system, const NodeState* state, const Rect* bounds,
                   const Rect* clip, RenderContext* context) override
    {
        check(!system && state == expectedState && bounds == expectedBounds &&
              clip == expectedClip && context == expectedContext, "paint forwards five typed arguments");
        order.push_back(id);
        if (changeNext) setChild(0, replacement);
    }
    bool updateSelf(System* system, const NodeState* state, const Rect* bounds, float delta) override
    {
        check(!system && state == expectedState && bounds == expectedBounds &&
              delta == expectedDelta, "update forwards state, bounds and float delta");
        order.push_back(id);
        if (changeNext) setChild(0, replacement);
        return accept;
    }
    int id;
    bool accept, changeNext;
    Node* replacement;
};
struct ObservedVisitor : NodeVisitor
{
    ObservedVisitor() : NodeVisitor(-17), calls(0), seen(nullptr), replacement(nullptr) {}
    void visit(System* system, Node*& node, const Rect* rect, NodeState* state) override
    {
        check(!system && rect == expectedBounds && state == expectedState, "visitor uses Rect before NodeState");
        ++calls;
        seen = &node;
        node = replacement;
    }
    int mode() const { return m_mode; }
    int calls;
    Node** seen;
    Node* replacement;
};
struct ObservedStream : Stream
{
    ObservedStream() : accept(true), calls(0), seen(nullptr), replacement(nullptr) {}
    bool streamNode0x88(System* system, Node*& value, const char* label, TypeDescriptor* type) override
    {
        ++calls;
        check(!system && !type && std::strcmp(label, "Next node") == 0, "properties retain exact label and null requested type");
        seen = &value;
        value = replacement;
        return accept;
    }
    bool accept;
    int calls;
    Node** seen;
    Node* replacement;
};
struct EventNode : Node
{
    EventNode() : calls(0), hook(-1), first(0), index(0), second(0),
                  accept(false), seenEvent(nullptr) {}
    bool record(int id, System* system, const Rect* rect, const NodeState* state,
                int firstValue, int indexValue, int secondValue)
    {
        ++calls;
        check(!system && rect == expectedBounds && state == expectedState,
              "dispatch retains System, Rect, NodeState order");
        hook = id; first = firstValue; index = indexValue; second = secondValue;
        return accept;
    }
    bool onEvent(System* system, const NodeInputEvent* event) override
    {
        ++calls; hook = 0; seenEvent = event;
        check(!system, "generic dispatch forwards System");
        return accept;
    }
    bool onEvent1(System* s, const Rect* r, const NodeState* n, int a, int b, int c) override
    { return record(1, s, r, n, a, b, c); }
    bool onEvent2(System* s, const Rect* r, const NodeState* n, int a, int b) override
    { return record(2, s, r, n, a, b, 0); }
    bool onEvent3(System* s, const Rect* r, const NodeState* n, int a, int b) override
    { return record(3, s, r, n, a, b, 0); }
    bool onEvent4(System* s, const Rect* r, const NodeState* n, int a, int b, int c) override
    { return record(4, s, r, n, a, b, c); }
    int calls, hook, first, index, second;
    bool accept;
    const NodeInputEvent* seenEvent;
};
}
int main()
{
    check(sizeof(Node) == 8, "Node x86 size");
    check(sizeof(NodeVisitor) == 8, "three-slot visitor x86 size");
    check(sizeof(NodeState) == 8, "interaction state x86 size");
    check(sizeof(NodeInputEvent) == 20, "plain event payload x86 size");
    check(sizeof(RenderContext) == 28, "render context x86 size");
    Node node, tail;
    check(node.getObjectSize() == 8, "actual Node object size hook");
    check(node.getChildCount() == 1, "signed child count");
    check(!node.getChild(0), "constructor clears successor");
    check(&node.getChild(-12) == &node.getChild(1234), "base ignores all child indices");
    node.setChild(-7, &tail);
    check(node.getChild(14) == &tail, "setChild ignores index");
    node.getChild(0) = nullptr;
    check(!node.getChild(0), "child getter returns writable live reference");
    check(!node.getState(), "base has no own interaction state");

    NodeState state = {-3.5f, 2.25f};
    Rect bounds = {1, 2, 3, 4, 5}, clip = {6, 7, 8, 9, 10};
    RenderContext context = {nullptr, nullptr, 0, 1, 0, 1, 1};
    expectedState = &state; expectedBounds = &bounds; expectedClip = &clip;
    expectedContext = &context; expectedDelta = -0.375f;
    check(node.update(nullptr, &state, &bounds, expectedDelta), "base update with null successor");
    node.paint(nullptr, &state, &bounds, &clip, &context);
    check(!node.getChild(0) && context.field0c == 1, "base paint leaves storage/context untouched");

    ObservedNode first(1), second(2), third(3);
    first.setChild(0, &second); second.setChild(0, &third);
    order.clear(); first.paint(nullptr, &state, &bounds, &clip, &context);
    check(order == std::vector<int>({1,2,3}), "paint visits self then successor chain");
    order.clear(); check(first.update(nullptr, &state, &bounds, expectedDelta), "update accepts complete chain");
    check(order == std::vector<int>({1,2,3}), "update visits self before successor");
    second.accept = false;
    order.clear(); check(!first.update(nullptr, &state, &bounds, expectedDelta), "update propagates later false");
    check(order == std::vector<int>({1,2}), "later false stops successor traversal");
    first.accept = false;
    order.clear(); check(!first.update(nullptr, &state, &bounds, expectedDelta), "own false propagates");
    check(order == std::vector<int>({1}), "own false stops before successor");
    first.accept = true; second.accept = true;
    first.changeNext = true; first.replacement = &third;
    order.clear(); first.paint(nullptr, &state, &bounds, &clip, &context);
    check(order == std::vector<int>({1,3}), "paint re-reads successor after self callback");
    first.setChild(0, &second);
    order.clear(); check(first.update(nullptr, &state, &bounds, expectedDelta), "mutating update succeeds");
    check(order == std::vector<int>({1,3}), "update re-reads successor after self callback");
    first.replacement = nullptr;
    order.clear(); check(first.update(nullptr, &state, &bounds, expectedDelta), "callback may clear successor");
    check(order == std::vector<int>({1}), "cleared successor is not visited");

    ObservedVisitor visitor;
    check(visitor.mode() == -17, "visitor constructor stores complete mode word");
    visitor.replacement = &tail;
    node.visitChildren(nullptr, &state, &bounds, &visitor);
    check(visitor.calls == 1 && visitor.seen == &node.getChild(0), "visitChildren forwards even null link by reference");
    check(node.getChild(0) == &tail, "visitor can replace live child");
    Node* entry = nullptr;
    visitor.traverse(nullptr, entry, &bounds, &state);
    check(visitor.calls == 2 && entry == &tail, "visitor slot00 calls virtual slot04");
    visitor.traverse0x08(nullptr, entry, &bounds, &state);
    check(visitor.calls == 3, "visitor slot08 calls slot00 then slot04");
    NodeVisitor baseVisitor(4);
    baseVisitor.traverse0x08(nullptr, entry, &bounds, &state);
    check(entry == &tail, "actual base visitor hook is empty");

    ObservedStream stream;
    stream.replacement = nullptr;
    check(node.streamProperties(nullptr, &stream), "actual Node property visitor accepts stream result");
    check(stream.calls == 1 && stream.seen == &node.getChild(0) && !node.getChild(0), "properties pass live successor address");
    stream.accept = false; stream.replacement = &tail;
    check(!node.streamProperties(nullptr, &stream), "actual Node properties propagate false");
    check(node.getChild(0) == &tail, "stream side effect retained on false");

    NodeInputEvent event = {4, -99, 1.5f, -2.5f, 0x11223344};
    check(!node.onEvent(nullptr, &event), "default generic event is false");
    check(!node.onEvent1(nullptr, &bounds, &state, -4, 123, 9), "default six-argument event1 is false");
    check(!node.onEvent2(nullptr, &bounds, &state, -4, 123), "default five-argument event2 is false");
    check(!node.onEvent3(nullptr, &bounds, &state, -4, 123), "default five-argument event3 is false");
    check(!node.onEvent4(nullptr, &bounds, &state, -4, 123, 9), "default six-argument event4 is false");

    EventNode receiver, ignoredSuccessor;
    receiver.setChild(0, &ignoredSuccessor);
    Node* dispatch = &receiver;
    for (int type = 1; type <= 4; ++type)
    {
        for (int accepting = 0; accepting != 2; ++accepting)
        {
            receiver.accept = accepting != 0;
            event = {type, accepting ? INT_MAX : INT_MIN,
                     accepting ? -19.875f : 19.875f,
                     accepting ? 43.75f : -43.75f, 0xFEDCBA98};
            const int beforeCalls = receiver.calls;
            check(dispatch->dispatchEvent(nullptr, &bounds, &state, &event) == receiver.accept,
                  "actual dispatch preserves specialized hook bool result");
            check(receiver.calls == beforeCalls + 1 && receiver.hook == type,
                  "actual dispatch selects one specialized hook without generic fallback");
            check(receiver.first == (accepting ? -19 : 19) && receiver.index == event.index,
                  "dispatch truncates first float toward zero and retains signed index");
            check(receiver.second == ((type == 1 || type == 4) ? (accepting ? 43 : -43) : 0),
                  "only event1 and event4 forward truncated second float");
        }
    }
    const int genericTypes[] = {INT_MIN, -1, 0, 5, 6, 19, 22, INT_MAX};
    for (int type : genericTypes)
    {
        receiver.accept = !receiver.accept;
        event = {type, -99, 1.5f, -2.5f, 0x11223344};
        const int beforeCalls = receiver.calls;
        check(dispatch->dispatchEvent(nullptr, &bounds, &state, &event) == receiver.accept,
              "actual generic dispatch preserves hook bool result");
        check(receiver.calls == beforeCalls + 1 && receiver.hook == 0 && receiver.seenEvent == &event,
              "default switch forwards the original complete event pointer once");
    }
    check(ignoredSuccessor.calls == 0, "event dispatch never traverses successor even on false");
    for (int type = 1; type <= 6; ++type)
    {
        event.type = type;
        check(!node.dispatchEvent(nullptr, &bounds, &state, &event),
              "real base dispatch calls recovered false defaults");
    }
    const int before = destroyed;
    { Node parent; parent.setChild(0, &first); }
    check(destroyed == before, "Node destructor does not own successor");
    std::printf("BFMeme Node prefix: %d/%d checks passed (/FORCE uncalled descriptors/reset/suffix events).\n", checks-failures, checks);
    return failures ? 1 : 0;
}
