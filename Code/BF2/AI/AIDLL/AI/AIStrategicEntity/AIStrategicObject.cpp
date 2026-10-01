// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIStrategicObject operations.

#include "AIStrategicObject.h"

#include "../AIStrategic/SAI.h"          // EnemyStatistics - the four objects the constructor makes
#include "../Main/AIDllFrontier.h"
#include "../Main/AIMain.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes strategic-object state and its owned enemy-statistics records.

AIStrategicObject::AIStrategicObject(float radius, float f148)
    : m_position()      // the two-iteration zeroing loop at +132, in member-declaration position
{
    m_statistics[0] = new EnemyStatistics(6);
    m_statistics[1] = new EnemyStatistics(6);
    m_statistics[2] = new EnemyStatistics(2);
    m_statistics[3] = new EnemyStatistics(2);

    const std::set<int> empty;
    m_set20[0] = empty;
    m_set20[1] = empty;
    m_set44[0] = empty;
    m_set44[1] = empty;
    m_set68[0] = empty;
    m_set68[1] = empty;
    m_set92[0] = empty;
    m_set92[1] = empty;

    m_status[0] = StatusMap::Neutral;
    m_status[1] = StatusMap::Neutral;
    m_status[2] = StatusMap::Neutral;
    m_status[3] = StatusMap::Neutral;

    m_f148 = f148;

    m_size[0] = radius;
    m_size[1] = 0.0f;
    m_size[2] = 0.0f;

    m_f164[0] = radius;
    m_f164[1] = radius;
    m_f164[2] = radius;
    m_f164[3] = radius;

    for (int i = 0; i < 2; ++i)
    {
        m_needCurveInput[i] = 0.0f;
        m_friendlyRatio[i] = 0.0f;
        m_friendlyBattleStrength[i] = 0.0f;
        m_enemyBattleStrength[i] = 0.0f;
        m_unresolved212[i] = 0;
        m_resourceNeed[i] = 0;
        m_assignedSquadStrength[i] = 0;
    }

    m_attack[0] = true;
    m_attack[1] = true;

    // `m_index` (+240) is deliberately absent. The constructor does not touch it; see the header.
    m_i244    = -1;
    m_i248    = 0;
    m_category = -1;
}

// [FRONTIER] Destruction requires ordered statistics and container teardown.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when the destructor is recovered.
AIStrategicObject::~AIStrategicObject()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicObject::~AIStrategicObject");
}

// native set size, side is one-based.
// ELF getAssignedUnits(int) at returns the matching set; the two old STL layouts
// place their counts four bytes apart. SAI::updateStates tests the signed result against zero.
int AIStrategicObject::getNumberOfAssignedUnits(int side) const
{
    return static_cast<int>(m_set68[side - 1].size());
}

// byte at this + 0xEB + side, no bounds guard.
bool AIStrategicObject::isAttack(int side) const
{
    return m_attack[side - 1];
}

// load this + 4*side. The first
// statistics pointer is at +4, so the side is one-based. The caller passes this
// borrowed address to EnemyStatistics::add(EnemyStatistics&); pointer versus
// reference return spelling is [INFERRED]. No null or side guard is present.
EnemyStatistics& AIStrategicObject::getBattleStatistics(int side) const
{
    return *m_statistics[side - 1];
}

// Returns the signed count of released resources, or zero when a commander refuses.
// Re-queries the SAI after the commander callback and reads the object index afterward.
// The downstream SAI release operation remains a named frontier.
int AIStrategicObject::releaseAssignedResources(int side)
{
    if (IAIMain::m_instance->getSAI(side)->isPlayerCommander())
        return 0;

    ISAI* const sai = IAIMain::m_instance->getSAI(side);
    return sai->releaseSquadsFromObject(m_index, side);
}

// The ten accessors. Every one of them is between seven and thirty-three bytes in the image.

// Returns type code 3; AIStrategicArea returns 0.
// [UNRESOLVED] These codes are not established as StrategicObjectType values.
int AIStrategicObject::getType() const
{
    return 3;
}

// - `return (char *)this + 8 * side + 124;`, i.e. the 1-based fold. See the header.
const Vec2& AIStrategicObject::getPosition(int side) const
{
    return m_position[side - 1];
}

// - two dword stores, a memberwise Vec2 copy.
void AIStrategicObject::setPosition(int side, const Vec2& position)
{
    m_position[side - 1] = position;
}

// Returns bit 27. [UNRESOLVED] No named StrategicObjectType enumerator covers that bit.
int AIStrategicObject::getObjectTypeFlags() const
{
    return 0x8000000;
}

bool AIStrategicObject::hasObjectTypeFlag(StrategicObjectType) const
{
    return false;
}

bool AIStrategicObject::hasObjectTypeFlags(int) const
{
    return false;
}

float AIStrategicObject::gravity(const Vec3&, int)
{
    return 0.0f;
}

float AIStrategicObject::gravity(const Vec2&, int)
{
    return 0.0f;
}

// Returns twice the side-specific size for both axes; base strategic objects are square.
float AIStrategicObject::getXExtent(int side) const
{
    return m_size[side] + m_size[side];
}

float AIStrategicObject::getZExtent(int side) const
{
    return m_size[side] + m_size[side];
}

} // namespace ai
} // namespace hfe
} // namespace dice
