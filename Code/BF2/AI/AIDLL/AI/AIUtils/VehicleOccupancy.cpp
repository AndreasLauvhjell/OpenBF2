// [INFERRED] path/name; see VehicleOccupancy.h. Full instructions read from
// libaidll.so and AIDLL_w32ded.dll .
#include "VehicleOccupancy.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIUnit.h"

namespace dice { namespace hfe { namespace ai {

bool vehicleOccupied(unsigned int handle, int side)
{
#if defined(_MSC_VER)
    // [PLATFORM] PE refuses a zero low word before loading/dereferencing the manager.
    Information* primary = (handle & 0xFFFFu) ? informationManager->getInformation(handle) : nullptr;
#else
    // ELF fetches the manager's pool even for a zero low word.
    Information* primary = informationManager->getInformation(handle);
#endif
    if (!primary)
        return false;

    if (!primary->isPrimary())
    {
#if defined(_MSC_VER)
        const unsigned int primaryHandle = primary->getPrimaryObject(side);
        primary = (primaryHandle & 0xFFFFu) ? informationManager->getInformation(primaryHandle) : nullptr;
#else
        // [PLATFORM] ELF retains the manager across each owner-forwarding callback.
        InformationManager* const manager = informationManager;
        const unsigned int primaryHandle = primary->getPrimaryObject(side);
        primary = manager->getInformation(primaryHandle);
#endif
    }

    // The resolved primary deliberately has no null guard .
    IPIUnit* unit = primary->getUnit();
    if (!unit)
        return false;
    if (unit->isOccupied())
        return true;

    int iterator; // The first-secondary callback initializes this output, as shipped.
#if defined(_MSC_VER)
    const unsigned int firstHandle = primary->getFirstSecondaryObject(iterator, side);
    InformationManager* const firstManager = informationManager;
    // PE's out-of-line lookup loads the manager before entry, but tests
    // the low word before dereferencing it. Do not use the unguarded manager wrapper here.
    Information* secondary = (firstHandle & 0xFFFFu) ? firstManager->getInformation(firstHandle) : nullptr;
#else
    InformationManager* const firstManager = informationManager;
    const unsigned int firstHandle = primary->getFirstSecondaryObject(iterator, side);
    Information* secondary = firstManager->getInformation(firstHandle);
#endif
    while (secondary)
    {
        unit = secondary->getUnit();
        if (unit && unit->isOccupied())
            return true;
#if defined(_MSC_VER)
        const unsigned int nextHandle = primary->getSecondaryObject(iterator, side);
        secondary = (nextHandle & 0xFFFFu) ? informationManager->getInformation(nextHandle) : nullptr;
#else
        InformationManager* const manager = informationManager;
        const unsigned int nextHandle = primary->getSecondaryObject(iterator, side);
        secondary = manager->getInformation(nextHandle);
#endif
    }
    return false;
}

} } }
