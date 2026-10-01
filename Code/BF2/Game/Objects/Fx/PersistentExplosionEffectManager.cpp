// Persistent explosion-effect lifecycle and per-player updates.

#include "PersistentExplosionEffectManager.h"

#include "IExplosionEffect.h"
#include "ExplosionEffectFlashBang.h"
#include "ExplosionEffectTearGas.h"
#include "ExplosionEffectMolotovCocktail.h"
#include "../../../Core/Ids.h"          // IID_IProjectileObject
#include "../IObject.h"                 // queryInterface (IBase +0x0C)
#include "../Projectiles/GenericProjectile.h"
#if !DEDICATED
#include "../../HudInformationLayer.h"  // the client constructor's registration arm
#include "../../Main/BF2ClientSeams.h"  // g_hudInformationLayer, ds:0x98734C
#endif

namespace dice {
namespace hfe {
namespace world {

namespace {

    // ELF 812D682 dispatches +B4 to the -316 thunk 82874C0 -> getExplosionType(8285180).
    // PE 563161 dispatches +B0 to 5B16C0 on the +154 projectile face; it reads face+EC.
    // The existing concrete method preserves virtual dispatch after native base adjustment.
    int projectileObjectGetExplosionType(const IProjectileObject* projectile)
    {
        return static_cast<const GenericProjectile*>(projectile)->getExplosionType();
    }

} // namespace

// ELF 812D820/812D890/812D900 / PE 563320/563390/563400, full paired reads.
// Ordinary source-level new plus list::insert at begin. Only FlashBang is 16 bytes
// on retail x86; TearGas/Molotov are four-byte vptr-only objects. Both arguments
// are present in ALL three instruction streams, even when constructors ignore them.
template<class Effect>
IExplosionEffect* CPersistentExplosionEffectManager::initExplosionEffect(
        const Vec3& position, const IProjectileObject* projectile)
{
    IExplosionEffect* const effect = new Effect(position, projectile);
    m_effects.insert(m_effects.begin(), effect);
    return effect;
}

// Constructs the effect list, then registers client HUD alpha before visibility when the layer exists.
// Backing variables have file scope; dedicated builds omit their registration.
bool  blindingLightDamageShow  = false;   // ds:0x9FDC3C
float blindingLightDamageAlpha = 0.0f;    // ds:0x9FDC40

CPersistentExplosionEffectManager::CPersistentExplosionEffectManager()
    : m_effects()                       // allocate(12), header->next = header->prev = header
{
#if !DEDICATED
    if (g_hudInformationLayer) {
        HudInformationLayer* const layer =
            static_cast<HudInformationLayer*>(g_hudInformationLayer);
        layer->registerFloat(std::string("BlindingLightDamageAlpha"), &blindingLightDamageAlpha);
        layer->registerBool(std::string("BlindingLightDamageShow"), &blindingLightDamageShow);
    }
#endif
}

// (D1), 46 B. `_List_base<IExplosionEffect*>::__clear` then the header node's deallocate -
// i.e. the compiler destroying the one member.
// IT does NOT delete THE effects. The list nodes go; anything still live leaks. killAllEffects is
// the call that actually destroys them, and nothing in the recovered tree invokes it before this.
// Reproduced as found - the leak is dice's, and it only matters at shutdown.
CPersistentExplosionEffectManager::~CPersistentExplosionEffectManager()
{
}

// . The damage side's entry point: `object` is the projectile that just went off.
// TWO silent refusals - a null object, and an object that does not answer IID_IProjectileObject.
// Neither is reported. A third sits inside createExplosionEffect (an explosion type outside 1..3).
void CPersistentExplosionEffectManager::handleExplosionEffect(IPlayer* causedBy,
                                                              const Vec3& position,
                                                              const IObject* projectile)
{
    if (!projectile)
        return;

    const IProjectileObject* projectileObject = static_cast<const IProjectileObject*>(
        projectile->queryInterface(IID_IProjectileObject));            // IBase +0x0C
    if (!projectileObject)
        return;

    IExplosionEffect* effect = createExplosionEffect(position, projectileObject);
    if (effect)
        effect->handleImmediateEffect(causedBy, projectileObject);     // IExplosionEffect +0x00
}

// . THE first statement OF every player'S tick - Player::update hands the player
// here before it does anything else, on every fixed step, for every player in the registry.
// It is a plain walk: each live effect gets a look at the player. With no effects in flight the
// whole body is a load and a compare, which is why a NULL manager survived every drive up to the
// one that first had a player in the registry.
void CPersistentExplosionEffectManager::handleObject(IPlayer* player)
{
    for (std::list<IExplosionEffect*>::iterator it = m_effects.begin(); it != m_effects.end(); ++it)
        (*it)->handlePersistentEffect(player);                         // IExplosionEffect +0x04
}

// . The per-frame ageing pass, driven only by the local human player's update.
// updateAndCull's false is what kills the effect - the effect reports, the manager destroys. The
// destruction is `delete` through the interface's deleting destructor (+0x10), not a release().
// THE ELF's erase idiom IS "STEP BACK, THEN ERASE": it saves the node, moves the cursor to the
// node's prev, erases the saved one, and lets the loop's `++` walk forward again - which lands on
// the erased node's successor because the list is circular. Written below as the erase-returns-next
// form: same sequence of visits, same deletions, and no decrement of begin().
void CPersistentExplosionEffectManager::updateExplosionEffects(float deltaTime)
{
    for (std::list<IExplosionEffect*>::iterator it = m_effects.begin(); it != m_effects.end(); )
    {
        if (!(*it)->updateAndCull(deltaTime))                          // IExplosionEffect +0x08
        {
            delete *it;                                                // +0x10
            it = m_effects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

// . updateExplosionEffects without the test - destroy and erase everything.
void CPersistentExplosionEffectManager::killAllEffects()
{
    for (std::list<IExplosionEffect*>::iterator it = m_effects.begin(); it != m_effects.end(); )
    {
        delete *it;
        it = m_effects.erase(it);
    }
}

// . THE factory, and the mapping is the recovered fact worth keeping:
// explosion type 1  ->  CExplosionEffectFlashBang
// 2  ->  CExplosionEffectMolotovCocktail
// 3  ->  CExplosionEffectTearGas
// else  ->  null, silently
// THE compiled shape IS A compare chain, NOT A switch: `== 1`, then `> 1`, then `== 2`, `!= 3`.
// Three arms and a default - too few for a jump table.
IExplosionEffect* CPersistentExplosionEffectManager::createExplosionEffect(
        const Vec3& position, const IProjectileObject* projectile)
{
    switch (projectileObjectGetExplosionType(projectile))              // IProjectileObject +0xB4
    {
    case 1:  return initExplosionEffect<CExplosionEffectFlashBang>(position, projectile);
    case 2:  return initExplosionEffect<CExplosionEffectMolotovCocktail>(position, projectile);
    case 3:  return initExplosionEffect<CExplosionEffectTearGas>(position, projectile);
    default: return 0;
    }
}

// `xor eax,eax; retn`. The parameter is not touched. Reproduced as found - see the
// header. No recovered caller.
bool CPersistentExplosionEffectManager::isObjectAffected(const IPlayer* /*player*/) const
{
    return false;
}

} // namespace world
} // namespace hfe
} // namespace dice
