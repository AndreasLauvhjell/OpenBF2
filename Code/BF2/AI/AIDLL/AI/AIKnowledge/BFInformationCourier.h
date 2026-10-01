// Registers and queries information and weapon handles across the engine/AI boundary.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_BFINFORMATIONCOURIER_H
#define AI_AIDLL_AI_AIKNOWLEDGE_BFINFORMATIONCOURIER_H

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include "../Main/AIDllTypes.h"
#include "IPIMobile.h"

namespace dice {
namespace hfe {
namespace ai {

// Information-courier interface shared by the engine and DLL. Registration guards the
// roster static, which may stay stale until updateStaticMethods refreshes it.
class IAIInformationCourier
{
public:
    IAIInformationCourier();
    virtual ~IAIInformationCourier();
    virtual Information* getInformation(unsigned int handle) const = 0;         // +0x08
    virtual bool isRealInformation(unsigned int handle) const = 0;              // +0x0C
    virtual void freeInformation(unsigned int handle) = 0;                      // +0x10
    virtual void setInformation(unsigned int handle, Information* information) = 0;   // +0x14
    virtual unsigned int registrateInformation(Information* information) = 0;   // +0x18
    virtual WeaponInfo* getWeaponInfo(unsigned int handle) const = 0;           // +0x1C
    virtual void freeWeaponInfo(unsigned int handle) = 0;                       // +0x20
    virtual void setWeaponInfo(unsigned int handle, WeaponInfo* weaponInfo) = 0;      // +0x24
    virtual unsigned int registrateWeaponInfo(WeaponInfo* weaponInfo) = 0;      // +0x28
    virtual void borrowArmament(unsigned int lender, unsigned int borrower) = 0;      // +0x2C
    virtual void resetArmament(unsigned int handle) = 0;                        // +0x30
    virtual void saveCurrentPolygon(IAIObject* object) = 0;                     // +0x34
    virtual void updateCurrentPolygon(IAIObject* object) = 0;                   // +0x38
};

// BFInformationCourier - sizeof == 68, from BF2AI::init step 4. ctor / PE
// dtor / .
class BFInformationCourier : public IAIInformationCourier
{
public:
    BFInformationCourier();
    virtual ~BFInformationCourier();

    // IAIInformationCourier
    virtual Information* getInformation(unsigned int handle) const;
    virtual bool isRealInformation(unsigned int handle) const;
    virtual void freeInformation(unsigned int handle);
    virtual void setInformation(unsigned int handle, Information* information);
    virtual unsigned int registrateInformation(Information* information);
    virtual WeaponInfo* getWeaponInfo(unsigned int handle) const;
    virtual void freeWeaponInfo(unsigned int handle);
    virtual void setWeaponInfo(unsigned int handle, WeaponInfo* weaponInfo);
    virtual unsigned int registrateWeaponInfo(WeaponInfo* weaponInfo);
    virtual void borrowArmament(unsigned int lender, unsigned int borrower);
    virtual void resetArmament(unsigned int handle);
    virtual void saveCurrentPolygon(IAIObject* object);
    virtual void updateCurrentPolygon(IAIObject* object);

private:
    // saveCurrentPolygon and
    // updateCurrentPolygon (both images) show what the sixteen dwords hold: the object whose position
    // was saved, and three copies of that object's IPIMobile position records (IPIMobile +8, +28 or
    // +48 by its +68 flag, and +48), each {polygon, quad tree, position} - the shape
    // IAIPathfinding::getPolygon fills (two unsigned outputs) plus the point it was asked about.
    // Constructed {-1, -1, (0, 0, 0)} three times after a null object, in both images.
    /* +0x04 */ IAIObject*          m_savedObject;
    /* +0x08 */ IPIMobile::ValidPos m_saved[3]; // +0x08, +0x1C, +0x30; ends at +0x44
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_BFINFORMATIONCOURIER_H
