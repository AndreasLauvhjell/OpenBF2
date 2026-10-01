// Projectile detonation that applies replenishment effects.

#include "ReplenishDetonationComp.h"

#include <sstream>
#include <string>

#include "../ProjectileCompTemplates.h"
#include "../GenericProjectile.h"
#include "../../GameLogicComp/IArmorGLComp.h"
#include "../../../IGameLogic.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

// (77 B); PE is inlined by createComponent at 0x5B86B0. Both allocate 0x44 bytes
// and put the weak target, trigger tick and template pointer at +0x38/+0x3C/+0x40.
ReplenishDetonationComp::ReplenishDetonationComp(
    ReplenishDetonationCompTemplate* objectTemplate)
    : DefaultDetonationComp(objectTemplate)
    , m_refCount(1)
    , m_replenishTarget()
    , m_replenishTick(0u)
    , m_template(objectTemplate)
{
}

ReplenishDetonationComp::~ReplenishDetonationComp() {}

void ReplenishDetonationComp::addRef() { ++m_refCount; }
int ReplenishDetonationComp::getRef() { return m_refCount; }

int ReplenishDetonationComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* ReplenishDetonationComp::queryInterface(unsigned int iid) const
{
    ReplenishDetonationComp* const self =
        const_cast<ReplenishDetonationComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IProjectileComp || iid == IID_IProjectileDetonationComp)
        return static_cast<IProjectileDetonationComp*>(self);
    return 0;
}

unsigned int ReplenishDetonationComp::getInterfaceID() const
{
    return IID_IProjectileDetonationComp;
}

// . Activation preserves the base state reset, then drops the retained
// trigger candidate and clears its five-tick validity stamp.
void ReplenishDetonationComp::activate()
{
    DefaultDetonationComp::activate();
    m_replenishTarget = WeakPtr<IObject>();
    m_replenishTick = 0u;
}

// is a direct tail delegation. Keep both arguments even though the base currently
// does not use the flags value on every arm.
void ReplenishDetonationComp::updateComponent(float seconds, unsigned int flags)
{
    DefaultDetonationComp::updateComponent(seconds, flags);
}

// . A candidate can trip the replenisher only if the ordinary
// mine/claymore predicate accepts it AND a test-only heal/ammo call says it would do work.
bool ReplenishDetonationComp::checkIfObjectCanTrigger(IObject* candidate)
{
    if (!DefaultDetonationComp::checkIfObjectCanTrigger(candidate))
        return false;
    if (!doReplenish(candidate, true))
        return false;

    // Retail dereferences candidate here without another guard after the two successful tests.
    m_replenishTarget = candidate->getWeakPtr();
    m_replenishTick = static_cast<unsigned int>(WorldPref::getTickCount());
    return true;
}

// . The unsigned comparison deliberately makes the stored candidate
// valid for strictly fewer than five elapsed ticks; a zero stamp is the inactive sentinel.
void ReplenishDetonationComp::detonate(IObject* source, int collisionType, Vec3 position,
                                       unsigned int tick, Vec3 normal,
                                       CollisionFaceInfo* face)
{
    DefaultDetonationComp::detonate(source, collisionType, position, tick, normal, face);

    if (m_replenishTick != 0u
        && m_replenishTick + 5u
            > static_cast<unsigned int>(WorldPref::getTickCount()))
    {
        IObject* const target = m_replenishTarget.get();
        if (target)
            doReplenish(target, false);
    }
}

// (394 B) / PE 0x5B8290 (286 B). The first armor component in the candidate's
// parent chain is the recipient. The global game-logic pointer and projectile pointer are both
// intentionally unguarded, matching retail after the three explicit gates below.
bool ReplenishDetonationComp::doReplenish(IObject* candidate, bool testOnly)
{
    float replenishedAmount = 0.0f;

    IArmorGLComp* armor = 0;
    for (IObject* object = candidate; object; object = object->getParent())
    {
        armor = static_cast<IArmorGLComp*>(object->getArmorComponent());
        if (armor)
            break;
    }

    if (!armor
        || !(m_template->getReplenishingStrength() > 0.0f)
        || !gameLogic->isServer())
    {
        return false;
    }

    IGameLogic* const logic = gameLogic;
    const int targetMaterial = armor->getDefaultMaterial();

    if (m_template->getReplenishingType() != static_cast<ReplenishingType>(0))
    {
        const int sourceMaterial = m_initObject->getMaterial();
        IPlayer* const player = static_cast<GenericProjectile*>(m_initObject)->getPlayer();
        IObject* const target = armor->getObject();
        const float strength = m_template->getReplenishingStrength();
        return logic->replenishAmmo(target, strength, player,
                                    sourceMaterial, targetMaterial,
                                    &replenishedAmount, testOnly);
    }

    const int sourceMaterial = m_initObject->getMaterial();
    IPlayer* const player = static_cast<GenericProjectile*>(m_initObject)->getPlayer();
    IObject* const target = armor->getObject();
    const float strength = m_template->getReplenishingStrength();
    return logic->heal(target, strength, player,
                       sourceMaterial, targetMaterial,
                       &replenishedAmount, testOnly, false);
}

// The derived template begins immediately after DefaultDetonationCompTemplate in both images:
// ELF +0xE4, PE +0x114. Its only state is a second refcount, strength and type, in that order.
ReplenishDetonationCompTemplate::ReplenishDetonationCompTemplate()
    : DefaultDetonationCompTemplate()
    , m_refCount(1)
    , m_replenishingStrength(0.0f)
    , m_replenishingType(static_cast<ReplenishingType>(0))
{
}

void ReplenishDetonationCompTemplate::addRef() { ++m_refCount; }
int ReplenishDetonationCompTemplate::getRef() { return m_refCount; }

int ReplenishDetonationCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* ReplenishDetonationCompTemplate::queryInterface(unsigned int iid) const
{
    ReplenishDetonationCompTemplate* const self =
        const_cast<ReplenishDetonationCompTemplate*>(this);
    if (iid == IID_IBase
        || iid == IID_IObjectTemplateComponent
        || iid == IID_IProjectileCompTemplate)
    {
        return self;
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int ReplenishDetonationCompTemplate::getClassID() const
{
    return CID_ReplenishDetonationCompTemplate;
}

IObjectComponent* ReplenishDetonationCompTemplate::createComponent()
{
    return new ReplenishDetonationComp(this);
}

// This leaf deliberately answers both its own class id and its DefaultDetonation base class id;
// the latter is how the inherited detonation console properties remain active for this subtype.
void* ReplenishDetonationCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ReplenishDetonationCompTemplate
        || classId == CID_DefaultDetonationCompTemplate)
    {
        return this;
    }
    return 0;
}

// . Base script first; type is unconditional, strength only when
// nonzero. The enum inserter is shared with ReplenishingAmmoComp and writes the symbolic spelling.
bool ReplenishDetonationCompTemplate::makeScript(io::IStream* stream)
{
    if (!DefaultDetonationCompTemplate::makeScript(stream))
        return false;

    std::ostringstream script;
    script << "ObjectTemplate.detonation.replenishingType "
           << m_replenishingType << std::endl;
    if (m_replenishingStrength != 0.0f)
    {
        script << "ObjectTemplate.detonation.replenishingStrength "
               << m_replenishingStrength << std::endl;
    }

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
