// Exposes engine collisions to the AI.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_BFCOLLISIONHANDLER_H
#define AI_AIINTERFACE_BFKNOWLEDGE_BFCOLLISIONHANDLER_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "AIAdapterTypes.h"

namespace dice {
namespace hfe {

class IBase;

namespace ai {

class IBFCollisionHandler
{
public:
    IBFCollisionHandler();
    virtual ~IBFCollisionHandler();

    virtual void removeAllProjectiles() = 0;
    virtual void handleCollision(world::IObject* a, world::IObject* b, const Vec3& at) = 0;
    virtual int handleProjectileTimeout(world::IProjectileObject* projectile, world::IObject* owner) = 0;
    virtual int handleProjectileFire(const Vec3& from, world::IProjectileObject* projectile, world::IObject* weapon, world::IPlayer* player) = 0;
    virtual void handleProjectileFire(world::IProjectileObject* projectile, world::IObject* weapon) = 0;
    virtual void handleProjectileDamage(world::IObject* target, float amount, int type, unsigned int id, Vec3 at) = 0;
    virtual void handleExplosionDamage(world::IObject* target, float amount, int type, unsigned int id, Vec3 at) = 0;
    virtual int handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, const Vec3& at, int a) = 0;
    virtual int handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, int a, const Vec3& at, int b) = 0;
    virtual int handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, world::IProjectileObject* projectile, world::IObject* weapon, int a) = 0;
    virtual int handleProjectileMiss(int a, world::IObject* object, int b, const Vec3& at) = 0;
    virtual void reset() = 0;
    virtual void updateBotProjectiles() = 0;
    // Return types below
    // different register on each image (void); the three point getters return the address of a
    // record Vec3 (reference spelling [INFERRED]). No caller in this tree used the old guesses.
    virtual void addBotProjectile(int id, world::IObject* object, const Vec3& at) = 0;
    virtual void removeBotProjectile(int id) = 0;
    virtual bool botProjectilePassedPoint(int id) const = 0;
    virtual void botProjectilePrePostPoints(int id, Vec3& pre, Vec3& post) = 0;
    virtual const Vec3& botProjectilePrePoint(int id) = 0;
    virtual const Vec3& botProjectilePostPoint(int id) = 0;
    virtual const Vec3& botProjectileTargetPoint(int id) = 0;
    virtual float botProjectileMaxHeight(int id) = 0;
};

class BFCollisionHandler : public IBFCollisionHandler
{
public:
    // Per-bot projectile state retained while a fired round approaches and passes its target.
    struct BotProjectile
    {
        BotProjectile();     // inlined into BFCollisionHandler's constructor

        /*  0 */ world::IObject* m_object;  // the round; not initialised
        /*  4 */ bool m_active;             // zeroed; set by add, cleared by remove
        /*  5 */ bool m_passed;             // zeroed; botProjectilePassedPoint
        /*  8 */ Vec3 m_prePoint;           // zeroed; last position short of the target distance
        /* 20 */ Vec3 m_postPoint;          // zeroed; first position at/after it
        /* 32 */ Vec3 m_startPoint;         // zeroed; the round's position when added
        /* 44 */ Vec3 m_targetPoint;        // zeroed; the aimed point
        /* 56 */ float m_passDistanceSq;    // not initialised; distXZSqr(start, target), stored float
        /* 60 */ float m_maxHeight;         // not initialised; highest Y seen, seeded from the start
    };

    BFCollisionHandler();
    virtual ~BFCollisionHandler();

    virtual void removeAllProjectiles();
    virtual void handleCollision(world::IObject* a, world::IObject* b, const Vec3& at);
    virtual int handleProjectileTimeout(world::IProjectileObject* projectile, world::IObject* owner);
    virtual int handleProjectileFire(const Vec3& from, world::IProjectileObject* projectile, world::IObject* weapon, world::IPlayer* player);
    virtual void handleProjectileFire(world::IProjectileObject* projectile, world::IObject* weapon);
    virtual void handleProjectileDamage(world::IObject* target, float amount, int type, unsigned int id, Vec3 at);
    virtual void handleExplosionDamage(world::IObject* target, float amount, int type, unsigned int id, Vec3 at);
    virtual int handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, const Vec3& at, int a);
    virtual int handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, int a, const Vec3& at, int b);
    virtual int handleIndirectProjectileHit(world::IPlayer* player, IAIObject* object, world::IProjectileObject* projectile, world::IObject* weapon, int a);
    virtual int handleProjectileMiss(int a, world::IObject* object, int b, const Vec3& at);
    virtual void reset();
    virtual void updateBotProjectiles();
    virtual void addBotProjectile(int id, world::IObject* object, const Vec3& at);
    virtual void removeBotProjectile(int id);
    virtual bool botProjectilePassedPoint(int id) const;
    virtual void botProjectilePrePostPoints(int id, Vec3& pre, Vec3& post);
    virtual const Vec3& botProjectilePrePoint(int id);
    virtual const Vec3& botProjectilePostPoint(int id);
    virtual const Vec3& botProjectileTargetPoint(int id);
    virtual float botProjectileMaxHeight(int id);

private:
    // `operator new(0xC)` in `BFConsole::aiMainInit`.
    /* +0x00 */ // vptr
    // sized by the bot count. `IAISettings::getMaxNBots`, and the same -1 sentinel that
    // gates `BotManager`'s pool: -1 means no bots, and then there is no array at all.
    /* +0x04 */ int m_count;
    /* +0x08 */ BotProjectile** m_projectiles;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_BFCOLLISIONHANDLER_H
