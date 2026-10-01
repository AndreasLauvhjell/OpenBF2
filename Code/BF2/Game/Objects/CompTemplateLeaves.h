// Concrete weapon-component template declarations. [INFERRED] Source placement.
// Destructor placement follows each base; some chains have no virtual destructor.
// LPTargetCompTemplate precaches resources; SoldierBasedRecoilCompTemplate builds recoil graphs when attached.

#ifndef GAME_OBJECTS_COMPTEMPLATELEAVES_H     // [INFERRED] guard style
#define GAME_OBJECTS_COMPTEMPLATELEAVES_H

// Aggregate component-template includes.
#include "ExplosivesFireCompTemplate.h"
#include "DefaultAmmoCompTemplate.h"
#include "SingleFireCompTemplate.h"
#include "SpawnObjectFireCompTemplate.h"
#include "ThrownFireCompTemplate.h"
#include "ToggleCameraCompTemplate.h"
#include "Weapons/AmmoComponents/ReplenishingAmmoComp.h"
#include "Weapons/DeviationComponents/DeviationCompTemplate.h"
#include "Weapons/DeviationComponents/SimpleDeviationComp.h"
#include "Weapons/DeviationComponents/SoldierDeviationComp.h"
#include "Weapons/DeviationComponents/TargetDeviationComp.h"
#include "Weapons/FireComponents/LPFireComp.h"
#include "Weapons/FireComponents/MultiFireComp.h"
#include "Weapons/RecoilComponents/SoldierBasedRecoilComp.h"
#include "Weapons/RecoilComponents/WeaponBasedRecoilComp.h"
#include "Weapons/SoundComponents/DefaultSoundComp.h"
#include "Weapons/TargetComponents/LPTargetComp.h"
#include "Weapons/TargetComponents/MultiFireArmTargetComp.h"
#include "Weapons/TargetComponents/SimpleTargetComp.h"
#include "Weapons/ZoomComponents/DefaultZoomComp.h"
#include "Weapons/AnimationComponents/DefaultAnimationComp.h"
#include "Weapons/AnimationComponents/ThrownAnimationComp.h"

#include "CompTemplateBases.h"

namespace dice {
namespace hfe {
namespace world {

// The eight every leaf overrides. Signatures from IObjectTemplateComponent.h; not re-guessed.
#define BF2_COMP_TEMPLATE_LEAF                                              \
    virtual void  addRef();                                                 \
    virtual int   getRef();                                                 \
    virtual int   release();                                                \
    virtual void* queryInterface(unsigned int) const;                       \
    virtual unsigned int getClassID() const;                                \
    virtual IObjectComponent* createComponent();                            \
    virtual void* queryImplementation(unsigned int);                        \
    virtual bool  makeScript(io::IStream*)


} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_COMPTEMPLATELEAVES_H
