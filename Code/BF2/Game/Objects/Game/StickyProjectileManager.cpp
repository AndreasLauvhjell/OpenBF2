// Path: [INFERRED] Game/Objects/Game/StickyProjectileManager.cpp

#include "StickyProjectileManager.h"

namespace dice {
namespace hfe {
namespace world {

StickyProjectileManager* stickyProjectileManager = 0; // ELF ds:0x8AAFF4C

StickyProjectileManager::StickyProjectileManager()
{
    if (!stickyProjectileManager)
        stickyProjectileManager = this;
}

StickyProjectileManager::~StickyProjectileManager()
{
    if (stickyProjectileManager == this)
        stickyProjectileManager = 0;
}

// / 0x82882C0 / 0x8288330 / 0x8288340. These are exactly the four source-level
// std::set operations visible beneath the retail libstdc++ expansion: insert, erase-by-key,
// return-by-reference, and clear. There are no projectile/null guards or ownership side effects.
void StickyProjectileManager::addStickyProjectile(GenericProjectile* projectile)
{
    m_projectiles.insert(projectile);
}

void StickyProjectileManager::removeStickyProjectile(GenericProjectile* projectile)
{
    m_projectiles.erase(projectile);
}

const std::set<GenericProjectile*>& StickyProjectileManager::getAllStickyProjectiles() const
{
    return m_projectiles;
}

void StickyProjectileManager::reset()
{
    m_projectiles.clear();
}

} // namespace world
} // namespace hfe
} // namespace dice
