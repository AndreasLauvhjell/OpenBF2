// [TEMP]
// Tests real list-box updates with typed System, input, selection, and Action observers.
// A child process verifies the missing inputDeviceManager null guard.
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <list>
#include <string>
#include <vector>

#define private public
#define protected public
#include "../BF2/Menu/BfNewListBoxNode.h"
#include "../BF2/Menu/BfListBoxData.h"
#undef protected
#undef private

#include "../BF2/External/BFMeme/Action.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/IO/Input/Input.h"

namespace dice { namespace hfe { namespace io {
IBase* inputDeviceManager = nullptr;
} } }

using namespace dice::meme;
namespace io = dice::hfe::io;

namespace {

unsigned int checks;
unsigned int failures;

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

void checkTrace(const std::vector<int>& actual, std::initializer_list<int> expected,
                const char* description)
{
    const std::vector<int> wanted(expected);
    check(actual == wanted, description);
    if (actual != wanted) {
        std::printf("  actual:");
        for (std::size_t i = 0; i != actual.size(); ++i)
            std::printf(" %d", actual[i]);
        std::printf("\n  wanted:");
        for (std::size_t i = 0; i != wanted.size(); ++i)
            std::printf(" %d", wanted[i]);
        std::printf("\n");
    }
}

class FixtureSystem : public System
{
public:
    FixtureSystem()
        : System(800.0f, 600.0f), trace(nullptr), ownerCalls(0), lastOwner(nullptr),
          soundCalls(0), lastSound(-1), lastVolume(0.0f) {}

    TypeDescriptor* getType() override { return Object::getClassType(); }

    void setPoint(float x, float y)
    {
        float* point = getPointerPosition();
        point[0] = x;
        point[1] = y;
    }

    bool playSound(int sound, float volume) override
    {
        ++soundCalls;
        lastSound = sound;
        lastVolume = volume;
        if (trace) trace->push_back(20);
        return true;
    }

    void setPointerOwner(Node* owner) override
    {
        ++ownerCalls;
        lastOwner = owner;
        m_field01c = owner;
    }

    std::vector<int>* trace;
    int ownerCalls;
    Node* lastOwner;
    int soundCalls;
    int lastSound;
    float lastVolume;
};

class FixtureData : public BfListBoxData
{
public:
    FixtureData()
        : BfListBoxData("FixtureData"), elementCount(0), countCalls(0),
          selectCalls(0), lastSelected(-999), lastNotify(false),
          firstRowAtSelection(-999), observedNode(nullptr), trace(nullptr) {}

    int getElementCount(System*) override
    {
        ++countCalls;
        if (trace) trace->push_back(10);
        return elementCount;
    }

    void selectRow(int row, bool notify) override
    {
        ++selectCalls;
        lastSelected = row;
        lastNotify = notify;
        firstRowAtSelection = observedNode ? observedNode->m_firstRow : -999;
        if (trace) trace->push_back(300 + row);
        BfListBoxData::selectRow(row, notify);
    }

    int elementCount;
    int countCalls;
    int selectCalls;
    int lastSelected;
    bool lastNotify;
    int firstRowAtSelection;
    BfNewListBoxNode* observedNode;
    std::vector<int>* trace;
};

class FixtureAction : public Action
{
public:
    FixtureAction()
        : runs(0), system(nullptr), bounds(nullptr), state(nullptr),
          hadEvent(false), event(), node(nullptr), firstRowAtRun(-999), trace(nullptr) {}

    bool execute(System* inSystem, const Rect* inBounds, const NodeState* inState,
                 const NodeInputEvent* inEvent) override
    {
        ++runs;
        system = inSystem;
        bounds = inBounds;
        state = inState;
        hadEvent = inEvent != nullptr;
        if (inEvent) event = *inEvent;
        firstRowAtRun = node ? node->m_firstRow : -999;
        if (trace) trace->push_back(40);
        return true;
    }

    int runs;
    System* system;
    const Rect* bounds;
    const NodeState* state;
    bool hadEvent;
    NodeInputEvent event;
    BfNewListBoxNode* node;
    int firstRowAtRun;
    std::vector<int>* trace;
};

class FixtureMouse : public io::IAdvancedInputDevice
{
public:
    FixtureMouse() : down(true), downCalls(0), lastButton(io::IDButton_31) {}

    void addRef() override {}
    int getRef() override { return 1; }
    int release() override { return 1; }
    void* queryInterface(unsigned int) const override { return nullptr; }
    io::InputDeviceTypes getType() const override { return io::IDTMouse; }
    bool setCooperativeMode(io::InputDeviceMode) override { return true; }
    io::InputDeviceMode getCooperativeMode() const override { return io::IDM_NONEXCLUSIVE; }
    bool activate() override { return true; }
    bool deActivate() override { return true; }
    void update(float) override {}
    bool isActive() const override { return true; }
    bool getNextInput(io::InputDeviceAxes&, io::InputDeviceButtons&,
                      io::InputDeviceKeys&, bool&) override { return false; }
    int getNumAxes() const override { return 0; }
    float readAxis(io::InputDeviceAxes, bool) const override { return 0.0f; }
    void peakXYAxis(float& x, float& y) override { x = 0.0f; y = 0.0f; }
    void setInvertAxis(io::InputDeviceAxes, bool) override {}
    bool getInvertAxis(io::InputDeviceAxes) const override { return false; }
    void setAxisScale(io::InputDeviceAxes, float) override {}
    float getAxisScale(io::InputDeviceAxes) const override { return 0.0f; }
    void setAxisAdd(io::InputDeviceAxes, float) override {}
    float getAxisAdd(io::InputDeviceAxes) const override { return 0.0f; }
    int getNumButtons() const override { return 1; }
    bool buttonDown(io::InputDeviceButtons button) const override
    {
        ++downCalls;
        lastButton = button;
        return down;
    }
    bool buttonPressed(io::InputDeviceButtons) const override { return false; }
    bool buttonReleased(io::InputDeviceButtons) const override { return false; }
    float buttonPressedSampleLatency(io::InputDeviceButtons) const override { return 0.0f; }

    bool down;
    mutable int downCalls;
    mutable io::InputDeviceButtons lastButton;
};

class FixtureInputManager : public io::IInputDeviceManager
{
public:
    FixtureInputManager() : answer(nullptr), getCalls(0), lastFlags(io::IDFNone) {}

    void addRef() override {}
    int getRef() override { return 1; }
    int release() override { return 1; }
    void* queryInterface(unsigned int) const override { return nullptr; }
    bool initDevices(unsigned int*) override { return true; }
    io::IInputDevice* getInputDevice(io::InputDeviceFlags flags) const override
    {
        ++getCalls;
        lastFlags = flags;
        return answer;
    }
    unsigned int getInputDevices() const override { return 0; }
    bool activateDevices(unsigned int) override { return true; }
    bool deActivateDevices(unsigned int) override { return true; }
    void update(float) override {}
    unsigned int getActiveInputDevices() const override { return 0; }
    void* getAllDevices() override { return nullptr; }

    io::IInputDevice* answer;
    mutable int getCalls;
    mutable io::InputDeviceFlags lastFlags;
};

void attach(BfNewListBoxNode& node, FixtureData& data)
{
    node.m_data = &data;
    data.m_node = &node;
    data.observedNode = &node;
}

void testNullDataEarlyReturn()
{
    BfNewListBoxNode node;
    const Rect bounds = {3.0f, 4.0f, 123.0f, 45.0f, 0.0f};
    node.m_field058 = 0.5f;
    node.m_field060 = 0.0f;
    node.m_field064 = 77;

    check(node.updateSelf(nullptr, nullptr, &bounds, 0.6f),
          "null data returns true without reading System");
    check(closeFloat(node.m_field058, 1.1f), "delta accumulates before null-data return");
    check(closeFloat(node.m_width, 123.0f), "bounds width stores before null-data return");
    check(node.m_field064 == -1, "stale field064 expires before null-data return");
}

void testAutoSelectionAndLiveEvent()
{
    FixtureSystem system;
    FixtureData data;
    FixtureAction action;
    BfNewListBoxNode node;
    const Rect bounds = {10.0f, 10.0f, 40.0f, 30.0f, 0.0f};
    const NodeState state = {1.0f, 1.0f};
    std::vector<int> trace;

    attach(node, data);
    data.elementCount = 3;
    data.trace = &trace;
    action.trace = &trace;
    action.node = &node;
    node.m_selectable = true;
    node.m_focusAction = &action;
    system.setPoint(0.0f, 0.0f); // outside: hover logic cannot add observations

    check(node.updateSelf(&system, &state, &bounds, 0.1f), "auto-selection update succeeds");
    check(data.m_row == 0 && data.selectCalls == 1 && data.lastNotify,
          "row -1 auto-selects row zero with notify=true");
    check(action.runs == 1, "selectRow-appended event 7 executes in the same list walk");
    checkTrace(trace, {10, 300, 40},
               "auto-count, selection callback, appended focus event preserve order");
    check(action.system == &system && action.bounds == &bounds && action.state == &state,
          "event 7 forwards System, bounds and NodeState");
    check(action.hadEvent && action.event.type == 1 && action.event.index == 0
          && closeFloat(action.event.first, 0.0f) && closeFloat(action.event.second, 0.0f)
          && action.event.field10 == 0,
          "event 7 builds the exact five-word focus payload");
    check(node.m_pendingEvents.empty(), "live pending-event list clears after appended event");
}

void testEventHelpersAndClamps()
{
    const Rect bounds = {10.0f, 10.0f, 40.0f, 30.0f, 0.0f};
    const NodeState state = {1.0f, 1.0f};

    {
        FixtureSystem system;
        FixtureData data;
        FixtureAction action;
        BfNewListBoxNode node;
        std::vector<int> trace;
        attach(node, data);
        data.m_row = 3;
        data.elementCount = 8;
        data.trace = &trace;
        action.node = &node;
        action.trace = &trace;
        node.m_firstRow = 3;
        node.m_selectable = true;
        node.m_focusAction = &action;
        node.enqueueEvent(1);
        system.setPoint(0.0f, 0.0f);

        check(node.updateSelf(&system, &state, &bounds, 0.0f), "previous-row event succeeds");
        check(data.m_row == 2 && node.m_firstRow == 2,
              "previous-row event selects and exposes the preceding row");
        check(data.firstRowAtSelection == 3 && action.firstRowAtRun == 2,
              "previous-row selection precedes first-row adjustment and appended focus");
        checkTrace(trace, {302, 40}, "previous-row callback precedes appended focus action");
    }

    {
        FixtureSystem system;
        FixtureData data;
        FixtureAction action;
        BfNewListBoxNode node;
        std::vector<int> trace;
        attach(node, data);
        data.m_row = 1;
        data.elementCount = 5;
        data.trace = &trace;
        action.node = &node;
        action.trace = &trace;
        node.m_firstRow = 0;
        node.m_rowHeight = 10.0f;
        node.m_spacing = 0.0f;
        node.m_sound068 = 73;
        node.m_selectable = true;
        node.m_focusAction = &action;
        node.enqueueEvent(2);
        system.trace = &trace;
        system.setPoint(0.0f, 0.0f);

        check(node.updateSelf(&system, &state, &bounds, 0.0f), "next-row event succeeds");
        check(data.m_row == 2 && node.m_firstRow == 1,
              "next-row event selects and advances the visible origin");
        check(system.soundCalls == 1 && system.lastSound == 73
              && closeFloat(system.lastVolume, 1.0f),
              "next-row event plays its sound at unit volume");
        check(data.firstRowAtSelection == 0 && action.firstRowAtRun == 1,
              "sound and selection precede next-row first-origin update and focus");
        checkTrace(trace, {10, 20, 302, 40},
                   "next-row count, sound, selection and focus preserve PE order");

        trace.clear();
        data.m_row = data.elementCount - 1;
        data.countCalls = 0;
        data.selectCalls = 0;
        system.soundCalls = 0;
        action.runs = 0;
        node.enqueueEvent(2);
        check(node.updateSelf(&system, &state, &bounds, 0.0f),
              "next-row event at final row succeeds");
        check(data.countCalls == 1 && data.selectCalls == 0
              && system.soundCalls == 0 && action.runs == 0,
              "final-row check returns before sound, selection and focus");
        checkTrace(trace, {10}, "final-row next event performs only the count query");
    }

    {
        FixtureSystem system;
        FixtureData data;
        BfNewListBoxNode node;
        attach(node, data);
        data.m_row = 0;
        data.elementCount = 8;
        node.m_rowCount = 5;
        node.m_visibleRows = 3;
        system.setPoint(0.0f, 0.0f);

        node.m_firstRow = 0;
        node.enqueueEvent(3);
        check(node.updateSelf(&system, &state, &bounds, 0.0f) && node.m_firstRow == 0,
              "single-row-up event clamps at zero");

        node.m_firstRow = 2;
        node.enqueueEvent(4);
        check(node.updateSelf(&system, &state, &bounds, 0.0f) && node.m_firstRow == 2,
              "single-row-down event clamps at rowCount-visibleRows");

        node.m_firstRow = 1;
        node.enqueueEvent(5);
        check(node.updateSelf(&system, &state, &bounds, 0.0f) && node.m_firstRow == 0,
              "page-up event clamps at zero");

        node.m_firstRow = 4;
        data.countCalls = 0;
        node.enqueueEvent(6);
        check(node.updateSelf(&system, &state, &bounds, 0.0f) && node.m_firstRow == 5,
              "page-down event clamps at elementCount-visibleRows");
        check(data.countCalls == 2,
              "clamped page-down arm repeats getElementCount exactly as retail");

        check(!node.m_field128, "event-8 flag starts clear");
        node.enqueueEvent(8);
        check(node.updateSelf(&system, &state, &bounds, 0.0f) && node.m_field128,
              "event 8 publishes its pending-first-row flag");
    }
}

void configureVertical(BfNewListBoxNode& node, FixtureData& data)
{
    attach(node, data);
    data.m_row = 0;
    data.elementCount = 20;
    data.m_field030 = 1.0f;
    node.m_field074 = true;
    node.m_field078 = 0.0f;
    node.m_rowCount = 10;
    node.m_visibleRows = 3;
    node.m_borderWidth = 0.0f;
    node.m_firstRow = 1;
    node.m_field028 = 0.0f;
}

void testVerticalDragAndPolling()
{
    const Rect bounds = {0.0f, 0.0f, 100.0f, 90.0f, 0.0f};
    const NodeState state = {1.0f, 1.0f};
    FixtureInputManager manager;
    FixtureMouse mouse;
    io::inputDeviceManager = &manager;

    {
        FixtureSystem system;
        FixtureData data;
        BfNewListBoxNode node;
        configureVertical(node, data);
        node.m_selectable = true;

        system.setPoint(0.0f, 9.0f); // exact step: strict greater-than refuses movement
        check(node.updateSelf(&system, &state, &bounds, 0.1f),
              "vertical exact-threshold update succeeds");
        check(node.m_firstRow == 1 && closeFloat(node.m_field028, 0.0f),
              "vertical drag requires distance strictly greater than one step");
        check(system.ownerCalls == 1 && system.lastOwner == &node,
              "selectable vertical drag claims pointer ownership before threshold test");

        system.setPoint(0.0f, 25.0f);
        check(node.updateSelf(&system, &state, &bounds, 0.1f),
              "vertical repeated-forward drag succeeds");
        check(node.m_firstRow == 3 && closeFloat(node.m_field028, 25.0f),
              "25-pixel drag repeats two nine-pixel row steps and stores pointer Y");

        node.m_field078 = 0.0f;
        system.setPoint(0.0f, 0.0f);
        check(node.updateSelf(&system, &state, &bounds, 0.1f),
              "vertical repeated-backward drag succeeds");
        check(node.m_firstRow == 1 && closeFloat(node.m_field028, 0.0f),
              "reverse drag repeats toward zero with lower clamp");
    }

    {
        FixtureSystem system;
        FixtureData data;
        BfNewListBoxNode node;
        configureVertical(node, data);
        node.m_selectable = true;
        node.m_field078 = 0.20f;
        manager.answer = nullptr;
        manager.getCalls = 0;
        system.setPoint(0.0f, 0.0f);

        check(node.updateSelf(&system, &state, &bounds, 0.06f),
              "vertical poll tolerates a null mouse device");
        check(manager.getCalls == 1 && manager.lastFlags == io::IDFMouse,
              "vertical poll asks the typed manager for IDFMouse");
        check(node.m_field074 && closeFloat(node.m_field078, 0.0f),
              "null mouse is guarded, leaves drag active, and resets poll timer");

        manager.answer = &mouse;
        mouse.down = false;
        mouse.downCalls = 0;
        node.m_field078 = 0.26f;
        node.m_field028 = 0.0f;
        system.setPoint(0.0f, 40.0f);
        check(node.updateSelf(&system, &state, &bounds, 0.0f),
              "released mouse poll returns true");
        check(!node.m_field074 && mouse.downCalls == 1
              && mouse.lastButton == io::IDButton_0,
              "released primary button ends vertical dragging");
        check(closeFloat(node.m_field028, 0.0f),
              "release returns before applying drag distance or updating pointer snapshot");

        node.m_field074 = true;
        node.m_field078 = 0.26f;
        mouse.down = true;
        system.setPoint(0.0f, 0.0f);
        check(node.updateSelf(&system, &state, &bounds, 0.0f) && node.m_field074,
              "held primary button keeps vertical dragging active");
    }

    io::inputDeviceManager = nullptr;
}

void prepareColumns(FixtureData& data)
{
    data.addColumn(20, true, false);
    data.addColumn(30, true, false);
    data.m_columns[1]->field01c = 5;
    data.m_columns[2]->field01c = 0;
}

void configureHorizontal(BfNewListBoxNode& node, FixtureData& data)
{
    attach(node, data);
    data.m_row = 0;
    data.elementCount = 4;
    node.m_field07c = true;
    node.m_field080 = 1;
    node.m_field028 = 20.0f;
    node.m_scrollWidth = 4.0f;
}

void testHorizontalResize()
{
    const Rect bounds = {0.0f, 0.0f, 100.0f, 50.0f, 0.0f};
    const NodeState state = {1.0f, 1.0f};
    FixtureSystem system;
    FixtureData data;
    BfNewListBoxNode node;
    prepareColumns(data);
    configureHorizontal(node, data);
    node.m_selectable = true;

    system.setPoint(25.0f, 0.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.0f), "horizontal grow succeeds");
    check(data.m_columns[1]->width == 25 && data.m_columns[2]->width == 35,
          "horizontal grow applies floored distance to every trailing column");
    check(closeFloat(node.m_field028, 25.0f), "horizontal grow stores pointer X");
    check(system.ownerCalls == 1 && system.lastOwner == &node,
          "selectable horizontal resize claims pointer ownership");

    data.m_columns[1]->width = 20;
    data.m_columns[2]->width = 30;
    node.m_field028 = 20.0f;
    system.setPoint(15.0f, 0.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.0f), "horizontal shrink succeeds");
    check(data.m_columns[1]->width == 15 && data.m_columns[2]->width == 25,
          "pointer-left-inside-bounds shrinks all trailing columns");

    data.m_columns[1]->width = 20;
    data.m_columns[2]->width = 30;
    node.m_field028 = 20.0f;
    system.setPoint(0.0f, 0.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.0f),
          "horizontal resize at left boundary succeeds");
    check(data.m_columns[1]->width == 40 && data.m_columns[2]->width == 50,
          "left boundary is not the shrink arm and therefore grows");

    data.m_columns[1]->width = 20;
    data.m_columns[2]->width = 30;
    ListBoxColumn* saved = data.m_columns[1];
    data.m_columns[1] = nullptr;
    node.m_field080 = 1;
    node.m_field028 = 10.0f;
    system.setPoint(15.0f, 0.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.0f),
          "horizontal resize tolerates a null in-range column");
    check(data.m_columns[2]->width == 35 && data.m_activeColumn == data.m_columns[2],
          "null column is skipped and later column remains active");
    data.m_columns[1] = saved;

    data.m_columns[1]->width = 20;
    data.m_columns[2]->width = 30;
    data.m_columnCount = 5; // indices 3 and 4 are beyond the vector
    node.m_field080 = 3;
    node.m_field028 = 10.0f;
    system.setPoint(14.0f, 0.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.0f),
          "horizontal resize tolerates selected indices beyond vector storage");
    check(data.m_columns[1]->width == 20 && data.m_columns[2]->width == 30
          && data.m_activeColumn == nullptr,
          "out-of-range selectors publish null and leave real columns unchanged");
    data.m_columnCount = 3;

    data.m_activeColumn = data.m_columns[1];
    node.m_field080 = 9; // loop is empty
    node.m_field028 = 10.0f;
    system.setPoint(12.0f, 0.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.0f),
          "horizontal empty-range update succeeds");
    check(data.m_activeColumn == data.m_columns[1],
          "empty horizontal range preserves the previously active column");

    node.m_field080 = 1;
    node.m_field028 = 10.0f;
    data.m_columns[1]->width = 20;
    data.m_columns[2]->width = 30;
    system.setPoint(10.9f, 0.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.0f),
          "subpixel horizontal update succeeds");
    check(data.m_columns[1]->width == 20 && data.m_columns[2]->width == 30
          && closeFloat(node.m_field028, 10.9f),
          "floor-to-zero distance skips widths but still stores pointer X");
}

void dwell(FixtureSystem& system, BfNewListBoxNode& node, FixtureData& data,
           const NodeState& state, const Rect& bounds, float x, float y)
{
    data.m_row = 0;
    system.setPoint(x, y);
    node.updateSelf(&system, &state, &bounds, 0.0f);  // movement resets timer
    node.updateSelf(&system, &state, &bounds, 1.0f);  // strict > 1: still not active
    check(!node.m_field108, "hover dwell at exactly one second remains inactive");
    node.updateSelf(&system, &state, &bounds, 0.01f);
}

void testStrictHoverAndDwell()
{
    const Rect bounds = {0.0f, 0.0f, 100.0f, 100.0f, 0.0f};
    const NodeState state = {1.0f, 1.0f};
    FixtureSystem system;
    FixtureData data;
    BfNewListBoxNode node;
    data.addColumn(20, true, false); // index 1 is the first searched column
    attach(node, data);
    data.m_row = 0;
    data.elementCount = 4;
    node.m_rowHeight = 10.0f;
    node.m_firstRow = 2;
    node.m_selectable = true;

    data.m_activeColumn = data.m_columns[1];
    node.m_field108 = true;
    system.setPoint(0.0f, 25.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.5f) && !node.m_field108,
          "hover rejects pointer exactly on the left edge");
    check(data.m_activeColumn == data.m_columns[1],
          "outside-hover return preserves active column");

    node.m_field108 = true;
    system.setPoint(100.0f, 25.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.5f) && !node.m_field108,
          "hover rejects pointer exactly on the right edge");

    dwell(system, node, data, state, bounds, 50.0f, 25.0f);
    check(node.m_field108, "stationary pointer becomes active only after one second");
    check(node.m_field10c[0] == 3,
          "hover row uses floor((absolute Y distance-rowHeight)/rowHeight)+firstRow");
    check(node.m_field10c[1] == 1,
          "invalid trailing column falls back to the node width and selects index-1");
    check(data.m_activeColumn == nullptr,
          "fallback is evaluated after the out-of-range selector publishes null");

    dwell(system, node, data, state, bounds, 15.0f, 25.0f);
    check(node.m_field108 && node.m_field10c[1] == 0
          && data.m_activeColumn == data.m_columns[1],
          "first searched column wins and remains the active column");
    system.setPoint(100.0f, 25.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.1f) && !node.m_field108
          && data.m_activeColumn == data.m_columns[1],
          "leaving bounds clears hover without clearing active column");

    data.m_columnCount = 0;
    data.m_activeColumn = data.m_columns[1];
    node.m_field10c[1] = 77;
    dwell(system, node, data, state, bounds, 16.0f, 25.0f);
    check(node.m_field108 && node.m_field10c[1] == 77
          && data.m_activeColumn == data.m_columns[1],
          "zero-column dwell publishes active hover while preserving column state");
    data.m_columnCount = 2;

    node.m_field108 = true;
    const float nan = std::numeric_limits<float>::quiet_NaN();
    system.setPoint(nan, 25.0f);
    check(node.updateSelf(&system, &state, &bounds, 0.1f) && !node.m_field108,
          "unordered pointer coordinate follows strict-outside hover arm");
    check(data.m_activeColumn == data.m_columns[1],
          "unordered outside arm also preserves active column");
}

int runNullManagerCase()
{
    const Rect bounds = {0.0f, 0.0f, 100.0f, 90.0f, 0.0f};
    const NodeState state = {1.0f, 1.0f};
    FixtureSystem system;
    FixtureData data;
    BfNewListBoxNode node;
    configureVertical(node, data);
    node.m_field078 = 0.26f;
    system.setPoint(0.0f, 0.0f);
    io::inputDeviceManager = nullptr;
    node.updateSelf(&system, &state, &bounds, 0.0f);
    std::fprintf(stderr, "FIXTURE_NULL_MANAGER_RETURNED\n");
    return 97;
}

} // namespace

int main(int argc, char** argv)
{
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    if (argc > 1 && std::strcmp(argv[1], "null-manager") == 0)
        return runNullManagerCase();

    testNullDataEarlyReturn();
    testAutoSelectionAndLiveEvent();
    testEventHelpersAndClamps();
    testVerticalDragAndPolling();
    testHorizontalResize();
    testStrictHoverAndDwell();

    std::printf("HUD new list-box update %u/%u checks passed\n",
                checks - failures, checks);
    return failures ? 1 : 0;
}
