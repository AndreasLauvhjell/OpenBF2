// [TEMP] Tests only the verbatim recovered deny sweep, with real declarations.
// SAI and interface lifetimes below are chosen native C++ fixture lifetimes.
// They do not exercise retail construction, destruction, request creation or
// engine commander bodies. Never run the nonempty mode-true retail UAF branch.
#include "sai_deny_runtime.h"
#include <climits>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <new>

using namespace dice::hfe;
using namespace dice::hfe::ai;

static_assert(sizeof(void*) == 4, "This fixture validates MSVC x86 only");
static_assert(sizeof(SAI::DenyRequest) == 24, "MSVC x86 native request size");
static_assert(offsetof(SAI::DenyRequest, m_deadline) == 8, "MSVC x86 deadline alignment");
static_assert(offsetof(SAI::DenyRequest, m_playerId) == 16, "MSVC x86 player field");

[[noreturn]] static void unexpected(const char* message)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", message);
    std::fflush(stderr);
    std::abort();
}

static void require(bool condition, const char* message)
{
    if (!condition) unexpected(message);
}

// Fixed storage keeps allocation instrumentation independent of STL allocation.
struct Event { char kind; int identity, tag, side, player; };
struct Allocation { void* pointer; unsigned deletes; };
static Event events[64];
static unsigned eventCount;
static Allocation allocations[16];
static unsigned allocationCount;

static void trace(char kind, int identity = 0, int tag = 0, int side = 0, int player = 0)
{
    require(eventCount < 64, "event storage capacity");
    events[eventCount++] = {kind, identity, tag, side, player};
}

// [CHOSEN, not recovered] Matching malloc/free-backed allocation replacement.
// Only registered request allocations are observed; native list nodes remain real.
void* operator new(std::size_t size)
{
    if (void* memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept
{
    if (!memory) return;
    for (unsigned i = 0; i < allocationCount; ++i)
    {
        if (allocations[i].pointer == memory && allocations[i].deletes == 0)
        {
            ++allocations[i].deletes;
            trace('D', static_cast<int>(i + 1));
            break;
        }
    }
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept { ::operator delete(memory); }

static bool cancelMode, replacePayload;
static double clockValue = 10.0;
static unsigned clockReads, commandCalls;
static std::list<SAI::DenyRequest*>* activeQueue;
static IBFCommander* nextCommander;
static SAI::DenyRequest* replacedOriginal;
static SAI::DenyRequest* replacement;

static double observeTime()
{
    ++clockReads;
    trace('T');
    return clockValue;
}

static int observeCommand(int identity, int tag, int side, int player)
{
    require(clockReads == 1, "one clock sample precedes callbacks");
    require(!activeQueue->empty(), "request node remains present during callback");
    require(activeQueue->front()->m_playerId == player, "callback precedes current-node erase");
    trace('C', identity, tag, side, player);
    ++commandCalls;
    if (replacePayload)
    {
        require(activeQueue->front() == replacedOriginal, "replacement targets original payload");
        activeQueue->front() = replacement;
        replacePayload = false;
    }
    if (nextCommander)
    {
        CommonSingletonServer::m_iBFCommander = nextCommander;
        nextCommander = nullptr;
    }
    return -731; // Chosen observer result: the recovered caller must discard it.
}

#include "sai_deny_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// [CHOSEN, not recovered] Native object lifetimes, limited to fields under test.
void* CommonSingletonServer::m_iBFTimer = nullptr;
void* CommonSingletonServer::m_iBFCommander = nullptr;
IBFTimer::IBFTimer() {}
IBFTimer::~IBFTimer() {}
IBFCommander::IBFCommander() {}
IBFCommander::~IBFCommander() {}
ISAI::~ISAI() {}
SAI::SAI(int side, int, int, Vec2, Vec2) : m_side(side), m_flag1D1(cancelMode)
{
    activeQueue = &m_denyRequests;
}
SAI::~SAI()
{
    // Safe fixture cleanup for retained requests; this is not the retail destructor.
    for (auto request : m_denyRequests) ::operator delete(request);
    m_denyRequests.clear();
    activeQueue = nullptr;
}
} } }

static SAI::DenyRequest* request(int tag, double deadline, int player, bool enqueue = true)
{
    require(allocationCount < 16, "request allocation capacity");
    auto result = new SAI::DenyRequest{tag, deadline, player};
    allocations[allocationCount++] = {result, 0};
    if (enqueue) activeQueue->push_back(result);
    return result;
}

static void expect(std::initializer_list<Event> wanted)
{
    require(eventCount == wanted.size(), "exact event count");
    unsigned index = 0;
    for (const auto& expected : wanted)
    {
        const auto& actual = events[index++];
        require(actual.kind == expected.kind && actual.identity == expected.identity &&
                actual.tag == expected.tag && actual.side == expected.side &&
                actual.player == expected.player, "exact timer/callback/deletion order and arguments");
    }
}

static void expectRetained(std::initializer_list<SAI::DenyRequest*> wanted)
{
    require(activeQueue->size() == wanted.size(), "retained request count");
    auto actual = activeQueue->begin();
    for (auto expected : wanted)
        require(*actual++ == expected, "retained request ordering");
}

int main(int argc, char** argv)
{
    require(argc == 2, "one named mode required");
    const char* mode = argv[1];
    cancelMode = std::strcmp(mode, "empty_true") == 0;
    ObservedTimer timer;
    ObservedCommander first, second;
    first.identity = 1;
    second.identity = 2;
    CommonSingletonServer::m_iBFTimer = cancelMode ? nullptr : &timer;
    CommonSingletonServer::m_iBFCommander = &first;
    SAI sai(37, 0, 0, Vec2(), Vec2());

    if (std::strcmp(mode, "empty_false") == 0 || cancelMode)
    {
        sai.SAI::updateRequestDenyTimers();
        require(activeQueue->empty() && commandCalls == 0, "empty mode has no requests or callbacks");
        require(clockReads == (cancelMode ? 0u : 1u), "empty mode clock contract");
        if (cancelMode) expect({}); else expect({{'T'}});
    }
    else if (std::strcmp(mode, "expiry_boundaries") == 0)
    {
        auto equal = request(0, 10.0, 101);
        auto future = request(1, 11.0, 102);
        auto nan = request(2, std::numeric_limits<double>::quiet_NaN(), 103);
        request(-17, 9.0, 104);
        CommonSingletonServer::m_iBFCommander = nullptr;
        sai.SAI::updateRequestDenyTimers();
        expectRetained({equal, future, nan});
        expect({{'T'}, {'D', 4}});
        require(commandCalls == 0 && clockReads == 1, "strict ordered expiration only");
        require(allocations[0].deletes == 0 && allocations[1].deletes == 0 &&
                allocations[2].deletes == 0 && allocations[3].deletes == 1, "only past request freed");
    }
    else if (std::strcmp(mode, "nan_clock") == 0)
    {
        clockValue = std::numeric_limits<double>::quiet_NaN();
        auto past = request(0, -1.0, 101);
        auto finite = request(1, 10.0, 102);
        auto nan = request(2, clockValue, 103);
        CommonSingletonServer::m_iBFCommander = nullptr;
        sai.SAI::updateRequestDenyTimers();
        expectRetained({past, finite, nan});
        expect({{'T'}});
        require(clockReads == 1 && commandCalls == 0, "NaN clock retains every request");
    }
    else if (std::strcmp(mode, "all_tags") == 0)
    {
        for (int tag = 0; tag < 5; ++tag) request(tag, 1.0, 101 + tag);
        sai.SAI::updateRequestDenyTimers();
        expectRetained({});
        expect({{'T'}, {'C', 1, 0, 37, 101}, {'D', 1}, {'C', 1, 1, 37, 102}, {'D', 2},
                {'C', 1, 2, 37, 103}, {'D', 3}, {'C', 1, 3, 37, 104}, {'D', 4},
                {'C', 1, 4, 37, 105}, {'D', 5}});
        require(clockReads == 1 && commandCalls == 5, "all five distinct typed callbacks");
    }
    else if (std::strcmp(mode, "unknown_tags") == 0)
    {
        request(-1, 1.0, 101);
        request(5, 1.0, 102);
        request(INT_MIN, 1.0, 103);
        request(INT_MAX, 1.0, 104);
        CommonSingletonServer::m_iBFCommander = nullptr;
        sai.SAI::updateRequestDenyTimers();
        expectRetained({});
        expect({{'T'}, {'D', 1}, {'D', 2}, {'D', 3}, {'D', 4}});
        require(clockReads == 1 && commandCalls == 0, "unknown tags never access commander");
    }
    else if (std::strcmp(mode, "fresh_commander") == 0)
    {
        request(0, 1.0, 101);
        request(1, 1.0, 102);
        nextCommander = &second;
        sai.SAI::updateRequestDenyTimers();
        expectRetained({});
        expect({{'T'}, {'C', 1, 0, 37, 101}, {'D', 1}, {'C', 2, 1, 37, 102}, {'D', 2}});
        require(clockReads == 1 && commandCalls == 2, "fresh commander singleton per due entry");
    }
    else if (std::strcmp(mode, "payload_reload") == 0)
    {
        replacedOriginal = request(3, 1.0, 101);
        replacement = request(4, 1000.0, 999, false);
        replacePayload = true;
        sai.SAI::updateRequestDenyTimers();
        expectRetained({});
        expect({{'T'}, {'C', 1, 3, 37, 101}, {'D', 2}});
        require(clockReads == 1 && commandCalls == 1, "one callback before payload replacement");
        require(allocations[0].deletes == 0 && allocations[1].deletes == 1,
                "replacement freed after callback while original allocation remains alive");
        ::operator delete(replacedOriginal); // Explicit fixture cleanup after the oracle.
    }
    else unexpected("unknown mode");

    std::printf("PASS %s\n", mode);
    return 0;
}
