// [INFERRED] path: see header. These empty bodies are retail behavior, not scaffolding.
#include "ExplosionEffectMolotovCocktail.h"
namespace dice { namespace hfe { namespace world {
// ELF 812CE90 / PE 621200: installs the vptr; neither argument is used.
CExplosionEffectMolotovCocktail::CExplosionEffectMolotovCocktail(const Vec3&, const IProjectileObject*) {}
// ELF destructor pair / PE shared deleting destructor 620750: base destruction only.
CExplosionEffectMolotovCocktail::~CExplosionEffectMolotovCocktail() {}
// ELF 812CEE0 / PE 6211C0: returns without reading either parameter.
void CExplosionEffectMolotovCocktail::handleImmediateEffect(IPlayer*, const IProjectileObject*) {}
// ELF 812CEF0 / PE 6211D0: returns without reading the parameter.
void CExplosionEffectMolotovCocktail::handlePersistentEffect(IPlayer*) {}
// ELF 812CF00 / PE 6211E0: always false, so the manager deletes it on its next update.
bool CExplosionEffectMolotovCocktail::updateAndCull(float) { return false; }
} } }

