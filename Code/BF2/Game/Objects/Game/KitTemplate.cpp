// Kit-template defaults, creation, and serialization.
#include "KitTemplate.h"
#include "Kit.h"
#include <cstdlib>                                  // std::abort - setAbilityRestoreRate's shipped guard
#include <utility>                                  // std::make_pair - addKitVehicleMod
#include "../../../Core/Ids.h"
#include "../IObject.h"                             // getTemplate
#include "../IObjectTemplate.h"                     // getNetworkableInfo

namespace dice {
namespace hfe {
namespace world {

// Initializes kit defaults, including an unassigned creation index and disabled ability meter.
// The unidentified word before vehicle mods remains uninitialized.
KitTemplate::KitTemplate()
    : m_defaultGeomIndex(0)                     // 408
    , m_timeToLiveAfterDeath(30.0f)
    , m_kitType(13)                             // 416
    , m_kitTeam(0)                              // 420
    , m_creationIndex(-1)                       // 424  NOT 0
    , m_defaultWeaponIndex(3)                   // 428
    , m_dropGeom(1)                             // 432
    , m_dropHeadwear()                          // 436  the shared empty std::string
    , m_sprintStaminaDissipationFactor(1.0f)
    , m_kitVehicleMods()                        // 444..455  empty
    , m_abilityRestoreRate(-1.0f)               // 456  a disabled sentinel
    , m_abilityUpdate(0.0f)                     // 460
    , m_abilityInVehicleMaterial(0)             // 464
    , m_abilityInVehicleStrength(0)             // 468
    , m_abilityInVehicleRadius(0)               // 472
    // 476 (+0x1DC) - NOT BY THE ELF ctor. See the header.
    , _list1E0(0)                               // 480  see below
    , m_unlockItems()                          // native scratch vector, ELF484/clientPE2B8
{
    // [UNRESOLVED] List element type; storage remains null instead of constructing a guessed sentinel.
}

// . Vtable +0x10.
unsigned int KitTemplate::getClassID() const
{
    return CID_KitTemplate;
}

// Recognizes KitTemplate and ItemTemplate class ids, then delegates ancestor lookup to BundleTemplate.
void* KitTemplate::queryImplementation(unsigned int cid)
{
    if (cid == CID_KitTemplate || cid == CID_ItemTemplate)
        return this;
    return BundleTemplate::queryImplementation(cid);
}

// non-virtual; Kit::getType tail-jumps into it.
int KitTemplate::getKitType() const { return m_kitType; }

// Creates a Kit with its kit-specific replication behavior.
IObject* KitTemplate::createObject() const
{
    return new Kit(this);
}

// Reads the kit creation index and selected unlock level for CreateKitEvent.
// The unlock level remains -1 until first pickup initializes it.
unsigned int kitCreationIndex(IObject* kit)
{
    return static_cast<Kit*>(kit)->getCreationIndex();
}

int kitChosenUnlockLevel(IObject* kit)
{
    return static_cast<Kit*>(kit)->m_chosenUnlockLevel;
}

// Stores the ability restore rate and its 30 Hz per-tick increment.
// Rates of 0.2 or greater abort; use this setter so the derived increment stays synchronized.
void KitTemplate::setAbilityRestoreRate(float rate)
{
    if (rate >= 0.2f)
        std::abort();

    m_abilityRestoreRate = rate;
    m_abilityUpdate = rate / 30.0f;
}

// addKitVehicleMod - ELF 0x8185BF0. The console's two-argument method; retail takes the name BY
// value (it copies once more into the element) and appends the pair. The element type here is the
// tree's std::pair<std::string, VehicleMod> reading of the retail KitVehicleMod struct - a string
// plus the int at +4, which is what IPlayerControlObject::addVehicleMod is handed.
void KitTemplate::addKitVehicleMod(std::string name, int mod)
{
    m_kitVehicleMods.push_back(std::make_pair(name, static_cast<VehicleMod>(mod)));
}

} // namespace world
} // namespace hfe
} // namespace dice
