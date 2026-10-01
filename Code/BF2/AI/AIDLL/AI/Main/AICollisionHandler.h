// Handles collisions in the AI module.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_MAIN_AICOLLISIONHANDLER_H
#define AI_AIDLL_AI_MAIN_AICOLLISIONHANDLER_H

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include "AIDllTypes.h"

namespace dice {
namespace hfe {
namespace ai {

// Collision-handler interface. Registration guards the roster static, which may remain
// stale until updateStaticMethods refreshes it.
class IAICollisionHandler
{
public:
    IAICollisionHandler();
    virtual ~IAICollisionHandler();
    virtual void handleProjectileFire(const Vec3&, world::IObject *, const Mat4&, int, IAIObject *, int, int) = 0;
    virtual int handleIndirectProjectileHit(int, IAIObject *, int, const Vec3&, int) = 0;
    virtual int handleIndirectProjectileHit(int, IAIObject *, const Vec3&, int) = 0;
    virtual int handleProjectileDamage(IAIObject *, IAIObject *, const int&, int) = 0;
    virtual int handleProjectileMiss(IAIObject *, int, int, const Vec3&) = 0;
    virtual int victimUpdate(int, int, const Vec3&, IAIObject *, IAIObject *) = 0;
    virtual int shooterUpdate(int, int, world::IObject *, IAIObject *, const Vec3&) = 0;
    virtual int handleProjectileTimeout(int, int, const Vec3&, bool) = 0;
};

// AICollisionHandler - sizeof == 4, from BF2AI::init step 10.
class AICollisionHandler : public IAICollisionHandler
{
public:
    AICollisionHandler();
    virtual ~AICollisionHandler();

    // IAICollisionHandler
    virtual void handleProjectileFire(const Vec3&, world::IObject *, const Mat4&, int, IAIObject *, int, int);
    virtual int handleIndirectProjectileHit(int, IAIObject *, int, const Vec3&, int);
    virtual int handleIndirectProjectileHit(int, IAIObject *, const Vec3&, int);
    virtual int handleProjectileDamage(IAIObject *, IAIObject *, const int&, int);
    virtual int handleProjectileMiss(IAIObject *, int, int, const Vec3&);
    virtual int victimUpdate(int, int, const Vec3&, IAIObject *, IAIObject *);
    virtual int shooterUpdate(int, int, world::IObject *, IAIObject *, const Vec3&);
    virtual int handleProjectileTimeout(int, int, const Vec3&, bool);

};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_AICOLLISIONHANDLER_H
