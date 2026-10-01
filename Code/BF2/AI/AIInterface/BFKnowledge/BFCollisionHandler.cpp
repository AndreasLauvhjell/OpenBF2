// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFCollisionHandler operations.

#include "BFCollisionHandler.h"

#include "AIFrontier.h"
#include "../../../Game/Objects/IObject.h"       // getAbsolutePosition, the +0x6C / +0x68 slot

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"
#include "../SingletonServer.h"   // BFSingletonServer::updateStaticMethods
#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../AIDLL/AI/Main/AISettingsInterfaces.h"   // IAISettings::getMaxNBots - slot +0

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IBFCollisionHandler::IBFCollisionHandler()
{
    if (!CommonSingletonServer::m_iBFCollisionHandler)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->registerSimpleSingleton(std::string("IBFCollisionHandler"), this);
        BFSingletonServer::updateStaticMethods();
    }
}

IBFCollisionHandler::~IBFCollisionHandler()
{
    if (CommonSingletonServer::m_iBFCollisionHandler == this)
    {
        // The roster refresh. Seven interface pairs do this and nine do not - see BFTerrain.cpp.
        classManager->unregisterSimpleSingleton(std::string("IBFCollisionHandler"));
        BFSingletonServer::updateStaticMethods();
    }
}

// Initializes and registers the collision adapter used by the AI.
BFCollisionHandler::BFCollisionHandler()
{
    // A diagnostic, not a guard - see the block above.
    IAISettings* const settings =
        checkPointer(static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings));

    // the bot count sizes the whole class. See the block above.
    m_count = settings->getMaxNBots();

    if (m_count == -1)
    {
        m_projectiles = 0;
    }
    else
    {
        m_projectiles = checkPointer(new BotProjectile*[m_count]);

        for (int i = 0; i < m_count; ++i)
            m_projectiles[i] = checkPointer(new BotProjectile());
    }
}

// +0, +56 and +60 are deliberately untouched - see the note in BFCollisionHandler.h. Retail
// clears +4..+55 as thirteen dword stores; the two flags, their padding and the four Vec3s.
BFCollisionHandler::BotProjectile::BotProjectile()
{
    m_active = false;
    m_passed = false;
    m_prePoint.x = m_prePoint.y = m_prePoint.z = 0.0f;
    m_postPoint.x = m_postPoint.y = m_postPoint.z = 0.0f;
    m_startPoint.x = m_startPoint.y = m_startPoint.z = 0.0f;
    m_targetPoint.x = m_targetPoint.y = m_targetPoint.z = 0.0f;
}

// [INFERRED] local helper name. The pass test compares distXZSqr's unrounded x87 result with the
// stored float: neither image stores it first. Under SSE2 a C++ call would round it to float on
// return, so the distXZSqr arithmetic is repeated here in double - equal to the PE's 53-bit x87.
// [PLATFORM] the ELF keeps a 64-bit mantissa. Strictly short of the target; equality and NaN
// count as passed.
static bool botProjectileShortOfTarget(const Vec3& start, const Vec3& position, float passDistanceSq)
{
    const double dx = static_cast<double>(start.x) - position.x;
    const double dz = static_cast<double>(start.z) - position.z;
    return dx * dx + dz * dz < passDistanceSq;
}

BFCollisionHandler::~BFCollisionHandler()
{
}

void BFCollisionHandler::removeAllProjectiles()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::removeAllProjectiles");
}

void BFCollisionHandler::handleCollision(world::IObject* a, world::IObject* b, const Vec3& at)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleCollision");
}

int BFCollisionHandler::handleProjectileTimeout(world::IProjectileObject* projectile, world::IObject* owner)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleProjectileTimeout");
}

int BFCollisionHandler::handleProjectileFire(const Vec3& from, world::IProjectileObject* projectile, world::IObject* weapon, world::IPlayer* player)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleProjectileFire");
}

void BFCollisionHandler::handleProjectileFire(world::IProjectileObject* projectile, world::IObject* weapon)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleProjectileFire");
}

void BFCollisionHandler::handleProjectileDamage(world::IObject* target, float amount, int type, unsigned int id, Vec3 at)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleProjectileDamage");
}

void BFCollisionHandler::handleExplosionDamage(world::IObject* target, float amount, int type, unsigned int id, Vec3 at)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleExplosionDamage");
}

int BFCollisionHandler::handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, const Vec3& at, int a)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleIndirectProjectileHit");
}

int BFCollisionHandler::handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, int a, const Vec3& at, int b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleIndirectProjectileHit");
}

int BFCollisionHandler::handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, world::IProjectileObject* projectile, world::IObject* weapon, int a)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleIndirectProjectileHit");
}

int BFCollisionHandler::handleProjectileMiss(int a, world::IObject* object, int b, const Vec3& at)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::handleProjectileMiss");
}

void BFCollisionHandler::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFCollisionHandler::reset");
}

// Every live, unpassed round is sampled once per AI tick: its highest Y is kept, and it is
// either still short of the target distance (the sample becomes the pre-point) or it has passed
// (flag set, the sample becomes the post-point). Records are visited by id; the count is read
// live. No null test on the record or its object. The max-height store happens on both arms;
// an unordered height keeps the old value (ELF fcmovbe / PE ordered-less test).
void BFCollisionHandler::updateBotProjectiles()
{
    for (int id = 0; id < m_count; ++id)
    {
        BotProjectile* const record = m_projectiles[id];
        if (!record->m_active || record->m_passed)
            continue;

        const Vec3& position = record->m_object->getAbsolutePosition();
        const bool shortOfTarget =
            botProjectileShortOfTarget(record->m_startPoint, position, record->m_passDistanceSq);
        record->m_maxHeight = record->m_maxHeight < position.y ? position.y : record->m_maxHeight;
        if (shortOfTarget)
            record->m_prePoint = position;
        else
        {
            record->m_passed = true;
            record->m_postPoint = position;
        }
    }
}

// [INFERRED] void: the two
// images leak different registers. No bounds or null test. The object's position is asked
// twice - once for the start point, once for the pre-point - and the maximum height is seeded
// from the second answer's Y. The pass distance is narrowed to float on the store.
void BFCollisionHandler::addBotProjectile(int id, world::IObject* object, const Vec3& at)
{
    BotProjectile* const record = m_projectiles[id];
    record->m_active = true;
    record->m_passed = false;
    record->m_object = object;
    record->m_targetPoint = at;
    record->m_startPoint = object->getAbsolutePosition();
    record->m_prePoint = object->getAbsolutePosition();
    record->m_passDistanceSq = distXZSqr(record->m_startPoint, record->m_targetPoint);
    record->m_maxHeight = record->m_prePoint.y;
}

// Both flags cleared, the array re-read between
// them; the object pointer and all points stay as they were.
void BFCollisionHandler::removeBotProjectile(int id)
{
    m_projectiles[id]->m_active = false;
    m_projectiles[id]->m_passed = false;
}

// No bounds or active test.
bool BFCollisionHandler::botProjectilePassedPoint(int id) const
{
    return m_projectiles[id]->m_passed;
}

// Pre-point copied before post-point; the array is
// re-read for the second copy in both images.
void BFCollisionHandler::botProjectilePrePostPoints(int id, Vec3& pre, Vec3& post)
{
    pre = m_projectiles[id]->m_prePoint;
    post = m_projectiles[id]->m_postPoint;
}

// the record's address + 8.
const Vec3& BFCollisionHandler::botProjectilePrePoint(int id)
{
    return m_projectiles[id]->m_prePoint;
}

// the record's address + 0x14.
const Vec3& BFCollisionHandler::botProjectilePostPoint(int id)
{
    return m_projectiles[id]->m_postPoint;
}

// the record's address + 0x2C.
const Vec3& BFCollisionHandler::botProjectileTargetPoint(int id)
{
    return m_projectiles[id]->m_targetPoint;
}

// the float at + 0x3C, returned in st0.
float BFCollisionHandler::botProjectileMaxHeight(int id)
{
    return m_projectiles[id]->m_maxHeight;
}

} // namespace ai
} // namespace hfe
} // namespace dice
