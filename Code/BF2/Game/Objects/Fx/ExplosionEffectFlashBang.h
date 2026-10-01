// [INFERRED] path: beside PersistentExplosionEffectManager; no TU path witness was found.
// Full paired ELF/PE instructions read.
#ifndef GAME_OBJECTS_FX_EXPLOSIONEFFECTFLASHBANG_H
#define GAME_OBJECTS_FX_EXPLOSIONEFFECTFLASHBANG_H
#include "IExplosionEffect.h"
#include "../../../Math/Vec3.h"
namespace dice { namespace hfe { namespace world {
class CExplosionEffectFlashBang : public IExplosionEffect
{
public:
    CExplosionEffectFlashBang(const Vec3&, const IProjectileObject*);
    virtual void handleImmediateEffect(IPlayer*, const IProjectileObject*);
    virtual void handlePersistentEffect(IPlayer*);
    virtual bool updateAndCull(float);
    virtual ~CExplosionEffectFlashBang();
private:
    Vec3 m_position; // ELF/PE +4, the only instance state
};
#if defined(_M_IX86)
static_assert(sizeof(CExplosionEffectFlashBang) == 16, "retail flashbang allocation");
#endif
} } }
#endif
