// [TEMP] Focused native x86 fixture for the recovered Bf2TransformNode::update.
// Production Bf2TransformNode, NodeState, Node traversal and button accessors run
// unchanged. Typed subclasses only observe callbacks and keep the unrelated
// Bf2ButtonNode::updateSelf frontier out of the exercised path.
#include "../BF2/Menu/Bf2TransformNode.h"
#include "../BF2/Menu/Bf2ButtonNode.h"
#include "../BF2/External/BFMeme/NodeState.h"
#include "../BF2/External/BFMeme/Style.h"
#include "../BF2/External/BFMeme/System.h"
#include <cmath>
#include <cstdio>
#include <vector>

using namespace dice::meme;

namespace {

unsigned int checks;
unsigned int failures;
std::vector<int> trace;

void check(bool condition, const char* description)
{
    ++checks;
    if (!condition) {
        ++failures;
        std::printf("FAIL: %s\n", description);
    }
}

bool closeFloat(float actual, float expected)
{
    return std::fabs(actual - expected) <= 0.00001f;
}

bool sameRect(const Rect& actual, const Rect& expected)
{
    return closeFloat(actual.x, expected.x)
        && closeFloat(actual.y, expected.y)
        && closeFloat(actual.width, expected.width)
        && closeFloat(actual.height, expected.height)
        && closeFloat(actual.z, expected.z);
}

struct Snapshot
{
    Snapshot() : called(false), system(nullptr), state(nullptr), rect(), delta(0.0f) {}
    bool called;
    System* system;
    const NodeState* state;
    Rect rect;
    float delta;
};

class FixtureSystem : public System
{
public:
    FixtureSystem() : System(800.0f, 600.0f) {}

    void setPointerForFixture(float x, float y)
    {
        m_pointerX = x;
        m_pointerY = y;
        m_previousPointerX = x;
        m_previousPointerY = y;
    }
};

class ObservedNode : public Node
{
public:
    explicit ObservedNode(int id)
        : m_id(id), m_result(true), m_getChildCalls(0), m_eventCount(0), m_lastEvent(0) {}

    Node*& getChild(int index) override
    {
        ++m_getChildCalls;
        return Node::getChild(index);
    }

    bool updateSelf(System* system, const NodeState* state,
                    const Rect* rect, float delta) override
    {
        trace.push_back(m_id);
        m_snapshot.called = true;
        m_snapshot.system = system;
        m_snapshot.state = state;
        m_snapshot.rect = *rect;
        m_snapshot.delta = delta;
        return m_result;
    }

    bool dispatchEvent(System*, const Rect*, const NodeState*,
                       const NodeInputEvent* event) override
    {
        trace.push_back(1000 + m_id);
        ++m_eventCount;
        m_lastEvent = event->type;
        return false;
    }

    int m_id;
    bool m_result;
    int m_getChildCalls;
    int m_eventCount;
    int m_lastEvent;
    Snapshot m_snapshot;
};

class ObservedButton : public Bf2ButtonNode
{
public:
    explicit ObservedButton(int id)
        : Bf2ButtonNode("ObservedButton", 0, 0, 1, 1),
          m_id(id), m_result(true), m_eventCount(0), m_lastEvent(0) {}

    bool updateSelf(System* system, const NodeState* state,
                    const Rect* rect, float delta) override
    {
        trace.push_back(m_id);
        m_snapshot.called = true;
        m_snapshot.system = system;
        m_snapshot.state = state;
        m_snapshot.rect = *rect;
        m_snapshot.delta = delta;
        return m_result;
    }

    bool dispatchEvent(System*, const Rect*, const NodeState*,
                       const NodeInputEvent* event) override
    {
        trace.push_back(1000 + m_id);
        ++m_eventCount;
        m_lastEvent = event->type;
        return false;
    }

    int m_id;
    bool m_result;
    int m_eventCount;
    int m_lastEvent;
    Snapshot m_snapshot;
};

class ObservedTransform : public Bf2TransformNode
{
public:
    ObservedTransform(int id, const char* name, int x, int y, int width, int height)
        : Bf2TransformNode(name, x, y, width, height),
          m_id(id), m_result(true), m_eventCount(0), m_lastEvent(0) {}

    bool updateSelf(System* system, const NodeState* state,
                    const Rect* rect, float delta) override
    {
        trace.push_back(m_id);
        m_snapshot.called = true;
        m_snapshot.system = system;
        m_snapshot.state = state;
        m_snapshot.rect = *rect;
        m_snapshot.delta = delta;
        return m_result;
    }

    bool dispatchEvent(System*, const Rect*, const NodeState*,
                       const NodeInputEvent* event) override
    {
        trace.push_back(1000 + m_id);
        ++m_eventCount;
        m_lastEvent = event->type;
        return false;
    }

    int m_id;
    bool m_result;
    int m_eventCount;
    int m_lastEvent;
    Snapshot m_snapshot;
};

void bindingAndOffsetCases()
{
    FixtureSystem system;
    system.setPointerForFixture(0.0f, 0.0f);
    const NodeState parentState = {1.0f, 1.0f};
    const Rect bounds = {-400.0f, -300.0f, 800.0f, 600.0f, 2.0f};

    ObservedTransform bound(10, "Bound", 5, -4, 20, 30);
    int liveX = -12;
    int liveY = 25;
    check(bound.setPositionBindingForHud(0, &liveX), "x live-int binding accepted");
    check(bound.setPositionBindingForHud(7, &liveY), "every nonzero binding index selects y");
    check(!bound.setPositionBindingForHud(0, nullptr), "null live-int binding refused");
    check(bound.setOffsetForHud(-7, 9), "signed offset setter accepted");
    bound.getState()->target = 0.75f;
    bound.getState()->current = 0.75f;

    trace.clear();
    check(bound.update(&system, &parentState, &bounds, 0.0f),
          "bound transform update succeeds");
    check(closeFloat(bound.getPosition().x, -19.0f)
              && closeFloat(bound.getPosition().y, 34.0f),
          "live signed ints are converted before signed offsets are added");

    liveX = 100;
    liveY = -60;
    trace.clear();
    check(bound.update(&system, &parentState, &bounds, 0.0f),
          "second bound transform update succeeds");
    check(closeFloat(bound.getPosition().x, 93.0f)
              && closeFloat(bound.getPosition().y, -51.0f),
          "live bindings reset both coordinates on every update");

    ObservedTransform accumulating(11, "Accumulating", 5, -4, 20, 30);
    accumulating.setOffsetForHud(-7, 9);
    accumulating.getState()->target = 0.75f;
    accumulating.getState()->current = 0.75f;
    check(accumulating.update(&system, &parentState, &bounds, 0.0f),
          "first unbound transform update succeeds");
    check(closeFloat(accumulating.getPosition().x, -2.0f)
              && closeFloat(accumulating.getPosition().y, 5.0f),
          "signed offsets mutate unbound stored position");
    check(accumulating.update(&system, &parentState, &bounds, 0.0f),
          "second unbound transform update succeeds");
    check(closeFloat(accumulating.getPosition().x, -9.0f)
              && closeFloat(accumulating.getPosition().y, 14.0f),
          "unbound signed offsets accumulate across updates");
}

void successorButtonAndOrderingCase()
{
    FixtureSystem system;
    system.setPointerForFixture(-1000.0f, -1000.0f);
    const NodeState parentState = {1.0f, 1.0f};
    const Rect bounds = {100.0f, 200.0f, 300.0f, 400.0f, 7.5f};

    ObservedTransform transform(3, "Root", 10, 20, 99, 88);
    transform.setOffsetForHud(-3, 4);
    transform.getState()->target = 0.0f;
    transform.getState()->current = 0.0f;

    ObservedNode chain(1);
    ObservedButton button(2);
    ObservedNode sibling(4);
    check(button.setMouseAreaForHud(5, -6, 40, 50),
          "button mouse-area geometry accepted");
    chain.setChild(0, &button);
    transform.setChild(1, &chain);
    transform.setChild(0, &sibling);

    trace.clear();
    check(transform.update(&system, &parentState, &bounds, 0.25f),
          "button-successor transform update succeeds");

    const std::vector<int> expectedTrace = {1003, 1001, 1002, 1004, 1, 2, 3, 4};
    check(trace == expectedTrace,
          "state event finishes before child, transform-self and successor updates");
    check(transform.m_eventCount == 1 && chain.m_eventCount == 1
              && button.m_eventCount == 1 && sibling.m_eventCount == 1
              && transform.m_lastEvent == 21 && chain.m_lastEvent == 21
              && button.m_lastEvent == 21 && sibling.m_lastEvent == 21,
          "updateNodeState event 21 traverses from the transform root");
    check(chain.m_getChildCalls == 2,
          "non-button successor step repeats typed getChild(0) check and fetch");

    const Rect adjusted = {112.0f, 218.0f, 40.0f, 50.0f, 7.5f};
    check(chain.m_snapshot.called && sameRect(chain.m_snapshot.rect, adjusted),
          "button position is added and button size replaces transform size");
    check(button.m_snapshot.called && sameRect(button.m_snapshot.rect, adjusted),
          "adjusted rectangle reaches the button successor unchanged");
    check(chain.m_snapshot.system == &system && button.m_snapshot.system == &system
              && chain.m_snapshot.state == transform.getState()
              && button.m_snapshot.state == transform.getState()
              && closeFloat(chain.m_snapshot.delta, 0.25f)
              && closeFloat(button.m_snapshot.delta, 0.25f),
          "child chain receives the exact system, local state and delta");
    check(closeFloat(chain.m_snapshot.state->target, 1.0f)
              && closeFloat(chain.m_snapshot.state->current, 0.0f),
          "child update observes NodeState after event-21 transition");
    check(transform.m_snapshot.state == &parentState
              && sibling.m_snapshot.state == &parentState
              && sameRect(transform.m_snapshot.rect, bounds)
              && sameRect(sibling.m_snapshot.rect, bounds),
          "Node fallback updates transform self and successor with parent geometry/state");

    chain.m_result = false;
    trace.clear();
    check(!transform.update(&system, &parentState, &bounds, 0.0f),
          "child failure propagates from transform update");
    check(trace == std::vector<int>({1}),
          "child failure stops its successor and the root fallback");
}

void noChildFallbackCases()
{
    FixtureSystem system;
    const NodeState parentState = {1.0f, 1.0f};
    const Rect bounds = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    ObservedTransform transform(30, "NoChild", 6, 7, 8, 9);
    ObservedNode sibling(31);
    transform.setChild(0, &sibling);
    transform.getState()->target = 0.75f;
    transform.getState()->current = 0.25f;

    trace.clear();
    check(transform.update(&system, &parentState, &bounds, 0.1f),
          "no-child fallback succeeds");
    check(trace == std::vector<int>({30, 31}),
          "no-child path falls back to Node self then successor");
    check(transform.m_snapshot.system == &system && sibling.m_snapshot.system == &system
              && transform.m_snapshot.state == &parentState
              && sibling.m_snapshot.state == &parentState
              && sameRect(transform.m_snapshot.rect, bounds)
              && sameRect(sibling.m_snapshot.rect, bounds),
          "no-child fallback forwards original arguments");
    check(closeFloat(transform.getState()->current, 0.55f),
          "no-child path still updates its local NodeState first");

    transform.m_result = false;
    trace.clear();
    check(!transform.update(&system, &parentState, &bounds, 0.0f),
          "no-child self failure propagates");
    check(trace == std::vector<int>({30}),
          "no-child self failure prevents successor update");
}

} // namespace

int main()
{
    static_assert(sizeof(void*) == 4, "fixture must validate the native x86 image");
    static_assert(sizeof(Bf2TransformNode) == 0x3C, "native Bf2TransformNode layout");
    static_assert(sizeof(Bf2ButtonNode) == 0xC0, "native Bf2ButtonNode layout");

    bindingAndOffsetCases();
    successorButtonAndOrderingCase();
    noChildFallbackCases();

    std::printf("Bf2TransformNode update: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
