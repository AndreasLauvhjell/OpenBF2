// [TEMP] [CHOSEN, not recovered] Native lifetimes and input data for two production scans.
// No retail constructor/destructor, full strategic-area update, or retail STL layout is tested.
#include "strategic_neighbours_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

__declspec(noreturn) static void unexpected(const char* method)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", method);
    std::fflush(stderr);
    std::abort();
}

#include "strategic_neighbours_mocks.inl"

namespace dice { namespace hfe { namespace ai {

// Chosen native lifetimes initialize only what the scans read. All STL members have their
// genuine constructors/destructors. No raw storage, layout writes or production-header macros.
AIStrategicObject::AIStrategicObject(float, float)
{
    for (auto& status : m_status) status = StatusMap::Unknown;
}
AIStrategicObject::~AIStrategicObject() {}
AIStrategicArea::AIStrategicArea(const std::string& name, unsigned int, float radius)
    : AIStrategicObject(radius, 0.0f), m_name(name)
{
}
AIStrategicArea::~AIStrategicArea() {}

} } }

using namespace dice::hfe::ai;
static int checks;

static void check(bool value, bool expected, const char* expression)
{
    ++checks;
    if (value != expected) {
        std::fprintf(stderr, "FAIL %s expected=%d actual=%d\n", expression, expected, value);
        std::exit(2);
    }
}

static void both(const AIStrategicObject& subject, StatusMap::Status status, int side,
                 bool expected, const char* label)
{
    check(subject.hasNeighbourWithStatus(status, side), expected, label);
    check(subject.hasNeighbouringObjectWithStatus(status, side), expected, label);
}

int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "The fixture must be MSVC x86");
    if (argc != 2) return 3;
    AIStrategicArea subject("subject", 0, 1.0f);
    AIStrategicArea first("first", 1, 1.0f);
    AIStrategicArea later("later", 2, 1.0f);
    first.m_status[0] = StatusMap::Owned;
    first.m_status[1] = StatusMap::Hostile;
    later.m_status[0] = StatusMap::Neutral;
    later.m_status[1] = StatusMap::Owned;
    const AIStrategicObject& view = subject;
    const char* mode = argv[1];

    if (!std::strcmp(mode, "empty")) {
        for (int side = 1; side <= 2; ++side)
            for (int status = 0; status <= 3; ++status)
                both(view, static_cast<StatusMap::Status>(status), side, false, "empty");
    } else if (!std::strcmp(mode, "side_selection")) {
        subject.m_neighbours = { &first };
        subject.m_neighbouringObjects = { &first };
        both(view, StatusMap::Owned, 1, true, "owned side 1");
        both(view, StatusMap::Owned, 2, false, "owned side 2");
        both(view, StatusMap::Hostile, 1, false, "hostile side 1");
        both(view, StatusMap::Hostile, 2, true, "hostile side 2");
        // Observe fresh field reads on a second call; a cached result would be wrong.
        first.m_status[1] = StatusMap::Owned;
        both(view, StatusMap::Owned, 2, true, "changed side 2");
        both(view, StatusMap::Hostile, 2, false, "old status absent");
    } else if (!std::strcmp(mode, "first")) {
        subject.m_neighbours = { &first, &later };
        subject.m_neighbouringObjects = { &first, &later };
        both(view, StatusMap::Owned, 1, true, "first owned");
        both(view, StatusMap::Hostile, 2, true, "first hostile");
    } else if (!std::strcmp(mode, "later")) {
        subject.m_neighbours = { &first, &later };
        subject.m_neighbouringObjects = { &first, &later };
        both(view, StatusMap::Neutral, 1, true, "later neutral");
        both(view, StatusMap::Owned, 2, true, "later owned");
    } else if (!std::strcmp(mode, "miss")) {
        subject.m_neighbours = { &first, &later, &first };
        subject.m_neighbouringObjects = { &first, &later, &first };
        both(view, StatusMap::Hostile, 1, false, "side 1 miss");
        both(view, StatusMap::Neutral, 2, false, "side 2 miss");
        both(view, StatusMap::Unknown, 1, false, "unknown miss");
    } else if (!std::strcmp(mode, "distinct_vectors")) {
        subject.m_neighbours = { &first };
        subject.m_neighbouringObjects = { &later };
        check(view.hasNeighbourWithStatus(StatusMap::Owned, 1), true, "neighbour owned");
        check(view.hasNeighbouringObjectWithStatus(StatusMap::Owned, 1), false, "object not owned");
        check(view.hasNeighbourWithStatus(StatusMap::Neutral, 1), false, "neighbour not neutral");
        check(view.hasNeighbouringObjectWithStatus(StatusMap::Neutral, 1), true, "object neutral");
        subject.m_neighbours.clear();
        check(view.hasNeighbourWithStatus(StatusMap::Neutral, 1), false, "first vector cleared");
        check(view.hasNeighbouringObjectWithStatus(StatusMap::Neutral, 1), true, "second retained");
    } else if (!std::strcmp(mode, "unknown")) {
        first.m_status[0] = StatusMap::Unknown;
        subject.m_neighbours = { &first };
        subject.m_neighbouringObjects = { &first };
        both(view, StatusMap::Unknown, 1, true, "unknown matches equality");
        both(view, StatusMap::Unknown, 2, false, "unknown other side");
    } else if (!std::strcmp(mode, "early_first")) {
        // The null suffix is legal pointer data and MUST remain unvisited after the match.
        // This tests early return; it does not assert any null-entry guard on a visited element.
        subject.m_neighbours = { &first, nullptr };
        subject.m_neighbouringObjects = { &first, nullptr };
        both(view, StatusMap::Owned, 1, true, "first match before null suffix");
    } else if (!std::strcmp(mode, "early_later")) {
        subject.m_neighbours = { &first, &later, nullptr };
        subject.m_neighbouringObjects = { &first, &later, nullptr };
        both(view, StatusMap::Neutral, 1, true, "later match before null suffix");
    } else {
        return 3;
    }
    std::printf("PASS %s checks=%d\n", mode, checks);
    return 0;
}
