// Bundle with a copied skeleton and first- and third-person animation instances.
// Owns the skeleton and releases both animation systems. Triggers are forwarded to both systems.
// [INFERRED] Source placement; geometry pose submission remains an isolated renderer ABI call.

#ifndef GAME_OBJECTS_GAME_ANIMATEDBUNDLE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_ANIMATEDBUNDLE_H

#include "../Base/Bundle.h"
#include "../IAnimatedBundle.h"

namespace dice {
namespace anim {
class IAnimationSystem;
class Skeleton;
}
namespace hfe {
namespace world {

class AnimatedBundleTemplate;

class AnimatedBundle : public Bundle, public IAnimatedBundle     // IAnimatedBundle = the +0x13C sub-object
{
public:
    explicit AnimatedBundle(const AnimatedBundleTemplate* objectTemplate);
    virtual ~AnimatedBundle();

    // the quartet, refcount at +0x140
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    virtual void  handleVisualUpdate(float, float);  // (IObject slot 42)

    // IAnimatedBundle
    virtual void  setTriggerMessage(int message);
    virtual void  clearTriggerMessage(int message);
    virtual void  setActiveAnimationSystem(int index);
    virtual void* getActiveAnimationSystem();

protected:
    // (+0x13C = the IAnimatedBundle vptr - the compiler places it; +0x80 = the IConsoleSaveable
    // vptr the SimpleObject chain already carries)
    /* 0x140 */ int             m_refCount;                 // ctor 1
    /* 0x144 */ anim::IAnimationSystem* m_animationSystems[2]; // [0] 1P (template +0x184), [1] 3P (+0x188)
    /* 0x14C */ int             m_activeAnimationSystemIndex;   // ctor 0
    /* 0x150 */ anim::Skeleton* m_skeleton;                 // ctor 0, or a copy of the template's (+0x180)
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ANIMATEDBUNDLE_H
