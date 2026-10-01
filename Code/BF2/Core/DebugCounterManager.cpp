// Debug-counter registration and graph display.
// Preserve retail quirks: remove and two lookups treat a nonzero name comparison as a match;
// remove leaves holes; the text array uses scalar deletion; graph teardown guards m_graph before
// calling m_graphRenderer. Those behaviors are marked locally where they affect the contract.

#include "DebugCounterManager.h"

#include "ClassManager.h"       // ClassManager::getSingleton (vtable +0x4C)
#include "Debug.h"              // the constructor's failure diagnostic
#include "EventManager.h"       // eventManager, registerEvent / registerEventHandler
#include "Ids.h"                // IID_IBase, IID_IDebugCounterManager - the two ids queryInterface
                                // answers, and both are `const unsigned int` globals in the ELF
                                // rather than immediates (0x83C3116 / 0x83C3123 load them)
#include "StringUtil.h"         // compareIgnoreCase, formatString

#include <cmath>                // sqrt

namespace dice {
namespace hfe {

// Engine-owned class registry.
extern ClassManager* classManager;

// Counters are class-shared; formatted rows and draw timing have translation-unit storage.
std::vector<IDebugCounter*> DebugCounterManager::m_counterVec;

namespace {

// One formatted row per counter plus a header row, rebuilt and emptied every draw().
// [CHOSEN] name.
std::vector<std::string> s_textLines;

// Bumped by update() - which is all update() does - zeroed by reset(), and compared
// against s_drawResetLimit at the end of draw(). [CHOSEN] name.
int s_updateCount = 0;

// Zero-initialized draw reset limit with no observed writer. [UNRESOLVED] whether a writer was intended.
// Keep it as storage; the local name is [CHOSEN].
int s_drawResetLimit = 0;

// [CHOSEN] Raw virtual calls for collaborators whose types are [UNRESOLVED].
// Replace with typed interfaces after establishing their ABI; preserve the observed slot contracts.
#if defined(_MSC_VER)
#  define BF2_VCALL __thiscall     // an MSVC virtual member call; `this` in ecx, rest on the stack
#else
#  define BF2_VCALL                // the ELF's convention: everything on the stack, `this` first
#endif

template <class Fn>
inline Fn vslot(void* object, unsigned int byteOffset)
{
    return reinterpret_cast<Fn>((*reinterpret_cast<void***>(object))[byteOffset / 4]);
}

typedef void        (BF2_VCALL* VFn_v)(void*);
typedef const char* (BF2_VCALL* VFn_name)(void*);
typedef void        (BF2_VCALL* VFn_format)(void*, char*);
typedef float       (BF2_VCALL* VFn_sample)(void*, int);
typedef int         (BF2_VCALL* VFn_count)(void*);
typedef void        (BF2_VCALL* VFn_rect)(void*, const float*);
typedef void        (BF2_VCALL* VFn_range)(void*, float, float);
typedef void        (BF2_VCALL* VFn_int)(void*, int);
typedef void        (BF2_VCALL* VFn_float)(void*, float);
typedef void        (BF2_VCALL* VFn_rgb)(void*, int, int, int);
typedef void*       (BF2_VCALL* VFn_make)(void*);
typedef void        (BF2_VCALL* VFn_ptr)(void*, void*);
typedef void        (BF2_VCALL* VFn_text)(void*, int, int, std::vector<std::string>*, int, int, int);

} // anonymous namespace

// Starts with one reference and registers the core draw event/listener.
// Allocate the text buffer only after registration and its failure diagnostic.
DebugCounterManager::DebugCounterManager()
  : m_refCount(1),
    m_enabled(false),
    m_color(0),
    m_posX(0),
    m_posY(0),
    m_ySpacing(0),
    m_graphCounterName(""),      // built from the empty literal at 0x88E24BB
    m_graph(0),
    m_graphRenderer(0),
    m_graphFunction("0"),        // the literal string "0" (0x88E409A), not a number
    m_graphYOffset(0),
    m_graphPosX(0),
    m_graphPosY(0),
    m_graphWidth(400),           // 0x190
    m_graphHeight(200),          // 0xC8
    m_graphMin(-100),
    m_graphMax(100),
    m_textBuffer(0)              // 0x83C1856; allocated at the bottom of this body, not here
{
    // Register the core draw-debug-counters event. The event ID remains local until a shared declaration is available.
    eventManager->registerEvent(ECCore, 5, "ECCoreCEDrawDebugCounters");

    // Register the IEventListener subobject through its implicit base conversion.
    if (!eventManager->registerEventHandler(ECCore, this, 0))
    {
        // Failure to register the event handler is fatal.
        Debug(DEBUGTYPE_6, "Core/DebugCounterManager.cpp", 45, "")
            << "Failed to register DebugCounterManager as event handler for ECCore.";
    }

    // operator new[](0x400). One formatted counter row; the counters write into it
    // through their vtable +0x10. See defect 2: the destructor frees this with scalar delete.
    m_textBuffer = new char[1024];
}

// Unregisters event handling and deletes the shared counter list. Only one manager should own this lifecycle.
DebugCounterManager::~DebugCounterManager()
{
    // EventManager vtable +0x38 then +0x30, the exact inverse of the ctor.
    eventManager->unregisterEvent(ECCore, 5);
    eventManager->unregisterEventHandler(ECCore, this);

    // shipped defect 3, verbatim: the guard is on m_graph, the call is on m_graphRenderer.
    if (m_graph)
        vslot<VFn_ptr>(m_graphRenderer, 0x14)(m_graphRenderer, m_graph);   // remove the graph

    // shipped defect 2, verbatim: allocated with new[], released with scalar delete (__ZdlPv).
    delete m_textBuffer;

    const int n = static_cast<int>(m_counterVec.size());
    for (int i = 0; i < n; ++i)
    {
        // Raw operator delete, no destructor call - which is how we know IDebugCounter has no
        // virtual destructor. `delete c` on a polymorphic type would have gone through a vtable slot.
        if (m_counterVec[i])
            ::operator delete(m_counterVec[i]);
    }

    // The two std::strings are released by the compiler after this; the ELF's trailing
    // _M_destroy pairs (0x83C1D1E, 0x83C1D3B) are that, not source.
}

// Takes ownership of counter and returns the live entry. A duplicate name deletes the supplied
// counter and returns the existing one; otherwise reuses an empty slot or appends.
IDebugCounter* DebugCounterManager::add(IDebugCounter* counter)
{
    const int n = static_cast<int>(m_counterVec.size());

    for (int i = 0; i < n; ++i)
    {
        IDebugCounter* existing = m_counterVec[i];
        if (existing)
        {
            // Fetch both names on each comparison; name callbacks may have observable side effects.
            const char* existingName = vslot<VFn_name>(existing, 0x00)(existing);
            const char* addedName    = vslot<VFn_name>(counter,  0x00)(counter);

            if (compareIgnoreCase(addedName, existingName) == 0)
            {
                ::operator delete(counter);
                return m_counterVec[i];
            }
        }
    }

    // 0x83C1FDE-0x83C1FFF. First null slot wins - the holes `remove` leaves behind.
    for (int i = 0; i < n; ++i)
    {
        if (!m_counterVec[i])
        {
            m_counterVec[i] = counter;
            return counter;
        }
    }

    m_counterVec.push_back(counter);
    return counter;
}

// Removes the first counter whose name differs case-insensitively, preserving the shipped inverted test.
// The slot is nulled rather than erased.
bool DebugCounterManager::remove(const char* name)
{
    const int n = static_cast<int>(m_counterVec.size());

    for (int i = 0; i < n; ++i)
    {
        IDebugCounter* c = m_counterVec[i];
        if (c)
        {
            const char* counterName = vslot<VFn_name>(c, 0x00)(c);
            if (compareIgnoreCase(name, counterName))          // inverted - see above
            {
                ::operator delete(m_counterVec[i]);
                m_counterVec[i] = 0;
                return true;
            }
        }
    }
    return false;
}

// No bounds check and no null filtering - the raw slot, holes included.
IDebugCounter* DebugCounterManager::get(int index)
{
    return m_counterVec[index];
}

// Virtual (+0x10) but touches no member - everything it works on is static.
void DebugCounterManager::reset()
{
    s_updateCount = 0;

    const int n = static_cast<int>(m_counterVec.size());
    for (int i = 0; i < n; ++i)
    {
        IDebugCounter* c = m_counterVec[i];
        if (c)
        {
            vslot<VFn_v>(c, 0x04)(c);
            // Re-read the vector after the callback because it may reallocate the container.
            vslot<VFn_v>(m_counterVec[i], 0x08)(m_counterVec[i]);
        }
    }
}

// Draws enabled counters and their selected graph, then performs the end-of-frame reset.
// Graph drawing and resetting do not require the text writer.
void DebugCounterManager::draw()
{
    if (!m_enabled)
        return;

    // (a) the text block
    IBase* writer = classManager->getSingleton(std::string("DebugTextWriterFixed"));
    if (writer)
    {
        const int n = static_cast<int>(m_counterVec.size());

        // Reserved for the counters only - the header row pushed next is the overflow.
        s_textLines.reserve(n);

        // Three column headings; "Tag" is the string at 0x889D326.
        s_textLines.push_back(formatString("%-30s %10s %10s", "Tag", "This frame", "Average"));

        for (int i = 0; i < n; ++i)
        {
            IDebugCounter* c = m_counterVec[i];
            if (c)
            {
                vslot<VFn_v>(c, 0x0C)(c);                                   // settle the sample
                vslot<VFn_format>(c, 0x10)(c, m_textBuffer);                // format one row
                s_textLines.push_back(std::string(m_textBuffer));
            }
        }

        vslot<VFn_int>(writer, 0x20)(writer, m_color);
        vslot<VFn_text>(writer, 0x28)(writer, m_posX, m_posY,
                                      &s_textLines, m_ySpacing, 0, 0);

        // 0x83C23DE-0x83C2413: `if (size) erase(begin, end) else _M_fill_insert(end, 0, string())`
        // is libstdc++ 3.x's std::vector::resize(0, T()) inlined, not two different operations.
        s_textLines.resize(0);
    }

    // (b) the line graph
    if (m_graphCounterName.compare("") != 0)
    {
        if (m_graphRenderer)
        {
            if (m_graph)
            {
                // vtable +0x40, the same slot graphEnable primes with 0.0f.
                vslot<VFn_float>(m_graph, 0x40)(m_graph, getValueFromFunction());

                // The graph sits below the text block, so its Y depends on how many rows were
                // printed. The +1 is the header row; the 12 is a per-row constant with no name in
                // the binary. Re-laid out only when the number changes.
                const int offset = (m_ySpacing + 12)
                                 * (static_cast<int>(m_counterVec.size()) + 1);
                if (offset != m_graphYOffset)
                {
                    m_graphYOffset = offset;
                    const float rect[4] = { static_cast<float>(m_graphPosX),
                                            static_cast<float>(m_graphPosY + offset),
                                            static_cast<float>(m_graphWidth),
                                            static_cast<float>(m_graphHeight) };
                    vslot<VFn_rect>(m_graph, 0x10)(m_graph, rect);
                }
            }
        }
    }

    // (c) the end-of-frame sweep
    // s_drawResetLimit is never written anywhere in the binary - see its declaration above.
    if (s_updateCount > s_drawResetLimit)
    {
        const int n = static_cast<int>(m_counterVec.size());
        for (int i = 0; i < n; ++i)
        {
            IDebugCounter* c = m_counterVec[i];
            if (c)
                vslot<VFn_v>(c, 0x04)(c);
        }
    }
}

// Packs truncated color components into byte positions without scaling or clamping.
// The graph receives the resulting color for all three channels.
void DebugCounterManager::setColor(const Vec3& color)
{
    m_color = (static_cast<int>(color.x) << 16)
            + (static_cast<int>(color.y) << 8)
            +  static_cast<int>(color.z);

    if (m_graph)
    {
        const int c = m_color - 0x1000000;
        vslot<VFn_rgb>(m_graph, 0x44)(m_graph, c, c, c);
    }
}

// The IEventListener half; reached through the +4 thunk at 0x83C2DB0.
// Both literals are 5: category ECCore, id CEDrawDebugCounters. Nothing else is handled.
void DebugCounterManager::handleEvent(EventCategory category, unsigned int id,
                                      EventNode* /*node*/, void* /*userData*/)
{
    if (category == ECCore && id == 5)
        draw();                                                 // +0x18
}

// Selects a counter to plot and creates the graph on first use.
// Clears the selected name if DebugRenderer is unavailable.
void DebugCounterManager::graphEnable(const std::string& counterName)
{
    m_graphCounterName = counterName;

    if (!m_graph)
    {
        IBase* renderer = classManager->getSingleton(std::string("DebugRenderer"));
        if (renderer)
            m_graph = vslot<VFn_make>(renderer, 0x20)(renderer);
    }

    if (!m_graph)
    {
        m_graphCounterName = "";                                // the rollback
        return;
    }

    const float rect[4] = { static_cast<float>(m_graphPosX),
                            static_cast<float>(m_graphPosY),
                            static_cast<float>(m_graphWidth),
                            static_cast<float>(m_graphHeight) };
    vslot<VFn_rect>(m_graph, 0x10)(m_graph, rect);

    vslot<VFn_int>(m_graph, 0x3C)(m_graph, m_graphWidth);       // one sample per pixel of width
    vslot<VFn_float>(m_graph, 0x40)(m_graph, 0.0f);             // prime the series
    vslot<VFn_v>(m_graph, 0x50)(m_graph);

    vslot<VFn_range>(m_graph, 0x1C)(m_graph, static_cast<float>(m_graphMin),
                                             static_cast<float>(m_graphMax));

    const int c = m_color - 0x1000000;
    vslot<VFn_rgb>(m_graph, 0x44)(m_graph, c, c, c);
    vslot<VFn_int>(m_graph, 0x40)(m_graph, 0);

    if (!m_graphRenderer)
        m_graphRenderer = classManager->getSingleton(std::string("DebugLineGraphRenderer"));

    if (m_graphRenderer)
        vslot<VFn_ptr>(m_graphRenderer, 0x10)(m_graphRenderer, m_graph);
}


// shipped defect 3, verbatim: the guard tests m_graph, the call goes through m_graphRenderer.
// Note also that m_graph itself is not cleared - the graph object survives, it is only unregistered,
// which is what lets graphEnable's `if (!m_graph)` skip re-creating it.
void DebugCounterManager::graphDisable()
{
    m_graphCounterName = "";

    if (m_graph)
        vslot<VFn_ptr>(m_graphRenderer, 0x14)(m_graphRenderer, m_graph);
}

// Position changes; the size half of the rect is re-sent unchanged.
void DebugCounterManager::graphPos(int x, int y)
{
    m_graphPosX = x;
    m_graphPosY = y;

    if (m_graph)
    {
        const float rect[4] = { static_cast<float>(x), static_cast<float>(y),
                                static_cast<float>(m_graphWidth),
                                static_cast<float>(m_graphHeight) };
        vslot<VFn_rect>(m_graph, 0x10)(m_graph, rect);
    }
}

// The mirror of graphPos - same +0x10 slot, same four-float rect.
void DebugCounterManager::graphSize(int width, int height)
{
    m_graphWidth  = width;
    m_graphHeight = height;

    if (m_graph)
    {
        const float rect[4] = { static_cast<float>(m_graphPosX),
                                static_cast<float>(m_graphPosY),
                                static_cast<float>(width), static_cast<float>(height) };
        vslot<VFn_rect>(m_graph, 0x10)(m_graph, rect);
    }
}


// the members are stored in the opposite order to the arguments (0x83C2A55 writes +0x48 from the
// second argument before 0x83C2A5A writes +0x44 from the first). Immaterial, and preserved as read.
void DebugCounterManager::graphMinMax(int min, int max)
{
    m_graphMax = max;
    m_graphMin = min;

    if (m_graph)
        vslot<VFn_range>(m_graph, 0x1C)(m_graph, static_cast<float>(m_graphMin),
                                                 static_cast<float>(max));
}


// Returns the first counter whose name differs case-insensitively, or null when none does.
// This inverted comparison is existing behavior.
IDebugCounter* DebugCounterManager::getCounterByName(const char* name)
{
    const int n = static_cast<int>(m_counterVec.size());

    for (int i = 0; i < n; ++i)
    {
        IDebugCounter* c = m_counterVec[i];
        if (c)
        {
            const char* counterName = vslot<VFn_name>(c, 0x00)(c);
            if (compareIgnoreCase(name, counterName))           // inverted - see above
                return m_counterVec[i];
        }
    }
    return 0;
}

// Reduces a named counter to a graph value; missing counters return zero.
// The comparison is intentionally inverted: every mode except magnitude uses the norm;
// magnitude takes the digit path and returns zero.
float DebugCounterManager::getValueFromFunction()
{
    IDebugCounter* counter = getCounterByName(m_graphCounterName.c_str());
    if (!counter)
        return 0.0f;

    if (compareIgnoreCase(m_graphFunction.c_str(), "magnitude"))     // inverted - see above
    {
        float sumOfSquares = 0.0f;
        const int count = vslot<VFn_count>(counter, 0x18)(counter);
        for (int i = 0; i < count; ++i)
        {
            const float v = vslot<VFn_sample>(counter, 0x14)(counter, i);
            sumOfSquares = v * v + sumOfSquares;
        }
        // 0x83C2BF7 calls the double `sqrt` and narrows on return, not sqrtf.
        return static_cast<float>(sqrt(static_cast<double>(sumOfSquares)));
    }

    // Single ASCII digit as a sample index. 0x83C2C86 `cmp dl, 9` after the `- 48`, unsigned - so
    // anything outside '0'..'9', including the empty string's terminator, yields 0.
    const char c = *m_graphFunction.c_str();
    if (static_cast<unsigned int>(c - '0') <= 9u)
        return vslot<VFn_sample>(counter, 0x14)(counter, c - '0');
    return 0.0f;
}

// The IBase quartet. Emitted last in the TU, after the vector template instances.

// Returns the new count in eax; the interface declares void, so it is dropped.
void DebugCounterManager::addRef()
{
    ++m_refCount;
}


int DebugCounterManager::getRef()
{
    return m_refCount;
}


// Releases one reference and deletes the manager at zero. Returns the remaining count.
int DebugCounterManager::release()
{
    const int count = --m_refCount;
    if (count == 0 && this)
        delete this;
    return count;
}

// Two ids only. Both are loaded from memory (ds:IID_IBase, ds:IID_...), which is
// what Core/Ids.h's `const unsigned int` globals compile to - the ids are not immediates here.
// const. _ZNK... - see the census note in Core/IBase.h.
void* DebugCounterManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IDebugCounterManager)
        return const_cast<DebugCounterManager*>(this);
    return 0;
}

// Advances the shared frame counter used by draw().
void DebugCounterManager::update()
{
    ++s_updateCount;
}

// A byte store; draw()'s only gate.
void DebugCounterManager::enable(bool on)
{
    m_enabled = on;
}


void DebugCounterManager::setPosition(int x, int y)
{
    m_posX = x;
    m_posY = y;
}


void DebugCounterManager::setYSpacing(int spacing)
{
    m_ySpacing = spacing;
}

// std::string::assign, not a re-construction.
void DebugCounterManager::setGraphFunction(const std::string& fn)
{
    m_graphFunction = fn;
}

// `mov eax, [ecx+2Ch]; ret`. The one member of this class whose return value is
// genuinely materialised - a reference to the member, not a copy.
const std::string& DebugCounterManager::getGraphFunction() const
{
    return m_graphFunction;
}

// Creates a one-reference counter manager; rejects aggregation and releases on IID mismatch.
// Release unwinds event registration through the destructor.
IBase* DebugCounterManager::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    DebugCounterManager* instance = new DebugCounterManager();

    // void* -> IBase*: the ELF returns the queryInterface result unchanged (0x84FA4EB `mov eax,
    // esi`). IDebugCounterManager derives from IBase at offset 0, so this is the same address.
    IBase* result = static_cast<IBase*>(instance->queryInterface(iid));
    if (!result)
        instance->release();

    return result;
}

} // namespace hfe
} // namespace dice

// Use the complete DebugCounterManager declaration for construction and dispatch.
// Keep global declaration types consistent across translation units; they affect MSVC linkage.
