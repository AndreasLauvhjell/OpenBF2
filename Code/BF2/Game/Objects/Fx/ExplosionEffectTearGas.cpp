// [INFERRED] path: see header. These empty bodies are retail behavior, not scaffolding.
#include "ExplosionEffectTearGas.h"
namespace dice { namespace hfe { namespace world {
// ELF 812D0A0 / PE 621150: installs the vptr; neither argument is used.
CExplosionEffectTearGas::CExplosionEffectTearGas(const Vec3&, const IProjectileObject*) {}
// ELF destructor pair / PE shared deleting destructor 620750: base destruction only.
CExplosionEffectTearGas::~CExplosionEffectTearGas() {}
// ELF 812D110 / PE 621170: returns without reading either parameter.
void CExplosionEffectTearGas::handleImmediateEffect(IPlayer*, const IProjectileObject*) {}
// ELF 812D120 / PE 621180: returns without reading the parameter.
void CExplosionEffectTearGas::handlePersistentEffect(IPlayer*) {}
// ELF 812D130 / PE 621190: always false, so the manager deletes it on its next update.
bool CExplosionEffectTearGas::updateAndCull(float) { return false; }
} } }

