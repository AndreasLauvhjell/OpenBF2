// [INFERRED] path: beside PersistentExplosionEffectManager, no __FILE__/anon-ns witness.
// Complete paired ELF/PE lifecycle and virtual methods read.
// Retail allocates only one vptr (4 bytes on x86).
#ifndef GAME_OBJECTS_FX_EXPLOSIONEFFECTMOLOTOVCOCKTAIL_H
#define GAME_OBJECTS_FX_EXPLOSIONEFFECTMOLOTOVCOCKTAIL_H
#include "IExplosionEffect.h"
#include "../../../Math/Vec3.h"
namespace dice { namespace hfe { namespace world {
class CExplosionEffectMolotovCocktail : public IExplosionEffect
{
public:
    CExplosionEffectMolotovCocktail(const Vec3&, const IProjectileObject*);
    virtual void handleImmediateEffect(IPlayer*, const IProjectileObject*);
    virtual void handlePersistentEffect(IPlayer*);
    virtual bool updateAndCull(float);
    virtual ~CExplosionEffectMolotovCocktail();
};
#if defined(_M_IX86)
static_assert(sizeof(CExplosionEffectMolotovCocktail) == 4, "retail effect allocation");
#endif
} } }
#endif
