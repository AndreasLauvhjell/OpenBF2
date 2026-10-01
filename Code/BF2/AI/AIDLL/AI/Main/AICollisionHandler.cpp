// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AICollisionHandler operations.

#include "AICollisionHandler.h"

#include <string>

#include "../../../../Core/ClassManager.h"
#include "CommonSingletonServer.h"
#include "AIDllFrontier.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// IAICollisionHandler
IAICollisionHandler::IAICollisionHandler()
{
    // the guard reads the roster static, not the registry. Reproduced.
    if (!CommonSingletonServer::m_iAICollisionHandler)
        classManager->registerSimpleSingleton("IAICollisionHandler", this);
}

IAICollisionHandler::~IAICollisionHandler()
{
    if (CommonSingletonServer::m_iAICollisionHandler == this)
        classManager->unregisterSimpleSingleton("IAICollisionHandler");
}

// Stateless collision adapter; construction requires only the base-class initialization.
AICollisionHandler::AICollisionHandler()
{
}

// The vptr reset and the base call, which again is what C++ emits.
AICollisionHandler::~AICollisionHandler()
{
}

void AICollisionHandler::handleProjectileFire(const Vec3&, world::IObject *, const Mat4&, int, IAIObject *, int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AICollisionHandler::handleProjectileFire");
}

int AICollisionHandler::handleIndirectProjectileHit(int, IAIObject *, int, const Vec3&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AICollisionHandler::handleIndirectProjectileHit");
}

int AICollisionHandler::handleIndirectProjectileHit(int, IAIObject *, const Vec3&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AICollisionHandler::handleIndirectProjectileHit");
}

int AICollisionHandler::handleProjectileDamage(IAIObject *, IAIObject *, const int&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AICollisionHandler::handleProjectileDamage");
}

int AICollisionHandler::handleProjectileMiss(IAIObject *, int, int, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AICollisionHandler::handleProjectileMiss");
}

int AICollisionHandler::victimUpdate(int, int, const Vec3&, IAIObject *, IAIObject *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AICollisionHandler::victimUpdate");
}

int AICollisionHandler::shooterUpdate(int, int, world::IObject *, IAIObject *, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AICollisionHandler::shooterUpdate");
}

int AICollisionHandler::handleProjectileTimeout(int, int, const Vec3&, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AICollisionHandler::handleProjectileTimeout");
}

} // namespace ai
} // namespace hfe
} // namespace dice
