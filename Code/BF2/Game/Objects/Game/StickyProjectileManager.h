// Path: [INFERRED] Game/Objects/Game/StickyProjectileManager.h
// Retail: ELF C1 0x8288150 / D1 0x8288220. The class owns a set of projectile pointers,
// self-registers in the world singleton slot, and exposes the four recovered set operations below.

#ifndef GAME_OBJECTS_GAME_STICKYPROJECTILEMANAGER_H
#define GAME_OBJECTS_GAME_STICKYPROJECTILEMANAGER_H

#include <set>

namespace dice {
namespace hfe {
namespace world {

class GenericProjectile;

class StickyProjectileManager
{
public:
    StickyProjectileManager();
    ~StickyProjectileManager();

    // with StickyCollisionComp, which is the only caller of the first two.
    // All four are plain set operations over m_projectiles in the ELF.
    void addStickyProjectile(GenericProjectile* projectile);
    void removeStickyProjectile(GenericProjectile* projectile);
    const std::set<GenericProjectile*>& getAllStickyProjectiles() const;
    void reset();

private:
    std::set<GenericProjectile*> m_projectiles;
};

extern StickyProjectileManager* stickyProjectileManager;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_STICKYPROJECTILEMANAGER_H
