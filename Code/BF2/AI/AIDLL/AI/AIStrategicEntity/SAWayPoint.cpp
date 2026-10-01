// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SAWayPoint operations.

#include "SAWayPoint.h"

#include "../DrawPrimitives/DPManager.h"
#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/IAIPathfinding.h"

namespace dice {
namespace hfe {
namespace ai {

// Five stores and no allocation.
SAWayPoint::SAWayPoint(const Vec3& position, int map)
    : m_position(position)
    , m_island(0xFFFFFFFFu)          // 1, written as the dword the constructor stores
    , m_polygon(0xFFFFFFFFu)
    , m_map(map)
    , m_debugPrimitive(0)
{
}

// Removes the waypoint's debug primitive; the waypoint owns no other heap resources.
SAWayPoint::~SAWayPoint()
{
    undraw();
}

// Resolves the waypoint polygon with search mode 3 on the trace map and 0 otherwise.
// Requires the pathfinder singleton and preserves its separate reads for query and search.
void SAWayPoint::init()
{
    IAIPathfinding* const pathfinding = IAIPathfinding::instance;

    const int traceMap = pathfinding->getTraceMapIndex();

    pathfinding->getPolygon(m_position, m_map, m_polygon, m_island,
                            3 * (traceMap == m_map));
}

// Deletes the debug circle when present and clears its handle unconditionally.
// Safe to repeat after an earlier undraw call.
void SAWayPoint::undraw() const
{
    if (m_debugPrimitive)
    {
        static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager)
            ->deleteCircle(m_debugPrimitive);
    }

    m_debugPrimitive = 0;
}

// [FRONTIER] Waypoint drawing requires the remaining primitive-manager interfaces.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding SAWayPoint operation.
void SAWayPoint::draw() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAWayPoint::draw");
}

} // namespace ai
} // namespace hfe
} // namespace dice
