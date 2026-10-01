// Path:     [INFERRED] - beside its header; see the placement note there.

#include "ObjectPredicator.h"

#include "../../Core/Ids.h"
#include "../../Game/Objects/IObject.h"
#include "../../Game/Objects/IObjectTemplate.h"
#include "../../Game/Objects/GameLogicComp/IArmorGLComp.h"
#include "../../Game/Objects/GameLogicComp/ArmorGLCompTemplate.h"
#include "../../Game/Objects/ObjectLayoutAccess.h"
#include "../../Physics/MaterialManager.h"

namespace dice {
namespace hfe {
namespace world {

extern IBase* materialManager;                        // ds:0x8AD2820 - Core/Globals.cpp

// `mov eax, 1`.
bool ObjectPredicator::predicateSurface(int /*surface*/)
{
    return true;
}

// (object->m_flags & m_flags) == m_flags - every requested bit must be set.
bool ObjectFlagPredicator::predicate(IObject* object)
{
    return (object->getFlags() & m_flags) == m_flags;
}

// The four derived predicators

bool ObjectFlagPredicatorEx::predicate(IObject* object)
{
    if ((m_flags & object->getFlags()) != m_flags)
        return false;
    return (m_clearFlags & ~object->getFlags()) == m_clearFlags;
}


bool ObjectIgnorePredicator::predicate(IObject* object)
{
    return object != m_ignored && ObjectFlagPredicator::predicate(object);
}


RootObjectIgnorePredicator::RootObjectIgnorePredicator(unsigned int flags, IObject* object)
    : ObjectFlagPredicator(flags), m_root(getRootParent(object))
{
}

bool RootObjectIgnorePredicator::predicate(IObject* object)
{
    return getRootParent(object) != m_root && ObjectFlagPredicator::predicate(object);
}

// the interface query first (slot 3), then the flags.
bool ObjectInterfacePredicator::predicate(IObject* object)
{
    if (!object->queryInterface(m_iid))
        return false;
    return ObjectFlagPredicator::predicate(object);
}

// / This is the inverse-interface sibling, not the positive
// ObjectInterfacePredicator: projectiles are excluded while ordinary static/vehicle objects pass.
bool ObjectIgnoreInterfacePredicator::predicate(IObject* object)
{
    if (object->queryInterface(m_iid))
        return false;
    return ObjectFlagPredicator::predicate(object);
}

// the interface query, and nothing else - it does not chain to the flag test the
// way ObjectInterfacePredicator does. (Its flags are 0 at every use site, so the two agree in
// practice; the difference is recorded because it is real.)
bool PCOPredicator::predicate(IObject* object)
{
    return object->queryInterface(IID_IPlayerControlObject) != 0;         // slot 3
}

// See the header for what this decides and why. Transcribed branch for branch:
// the two `armor && armorTemplate` tests really are asked twice on the ELF's paths, and the
// projectile arm really does fall into the same ignore test rather than returning true.
bool AbilityPredicator::predicate(IObject* object)
{
    typedef object_detail::LayoutAccess L;

    IObject* const root = getRootParent(object);
    IArmorGLComp* const armor = static_cast<IArmorGLComp*>(L::armorComponent(root));   // +0x3C
    ArmorGLCompTemplate* const armorTemplate = static_cast<ArmorGLCompTemplate*>(
        root->getTemplate()->getComponentFromClassID(CID_ArmorGLCompTemplate));

    if (m_allowParachute && !armor
        && object->getTemplate()->getClassID() == CID_ParachuteTemplate)
        return true;

    if (!armor || !armorTemplate)
    {
        // Nothing armored here. Only a projectile gets as far as the ignore test.
        if (!root->queryInterface(IID_IProjectileObject))
            return false;
        return ObjectIgnorePredicator::predicate(object);
    }

    if (armor->isDestroyed() && !armorTemplate->getCanBeRepairedWhenWreck())
        return ObjectIgnorePredicator::predicate(object);

    // the material gate - the kit's ability material must actually do something to this surface.
    const int material = armor->getDefaultMaterial();                     // +0x9C
    const float mod = static_cast<MaterialManager*>(materialManager)
        ->getDamageMod(m_abilityMaterial, static_cast<unsigned int>(material));   // +0x58
    if (mod > 0.0f)
        return ObjectIgnorePredicator::predicate(object);
    return false;
}

} // namespace world
} // namespace hfe
} // namespace dice
