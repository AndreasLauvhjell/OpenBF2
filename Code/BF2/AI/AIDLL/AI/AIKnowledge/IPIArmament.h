// Armament information attached to an AI information record.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENT_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENT_H

#include <vector>

#include "../../../../Math/Vec3.h"
#include "InformationPlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

class Information;
class WeaponInfo;

class IPIArmament : public InformationPlugIn
{
public:
    explicit IPIArmament(Information* information);
    virtual ~IPIArmament();

    virtual int  getType();                                             // +2: 4
    virtual void setTime();                                             // +3
    virtual void setTime(double time);                                  // +4: empty

    virtual int  getActiveWeapon() const;                               // +5
    virtual void setActiveWeapon(int weapon, int side);                 // +6
    virtual unsigned int getWeapon(int index) = 0;                      // +7
    virtual std::vector<unsigned int>& getWeapons() = 0;                // +8
    virtual int  getWeaponIdFromActivationId(int activation, int fire) const; // +9
    virtual WeaponInfo* getLastFiredWeapon();                           // +10
    virtual int  isAntiAircraft() const;                                // +11
    virtual void updateWeapons() = 0;                                   // +12
    virtual void rearm(WeaponInfo* weapon, float amount, int side);     // +13, returns void
    virtual bool getProjectilePosition(Vec3& out) const;                // +14
    virtual bool getProjectileSpeed(Vec3& out) const;                   // +15

protected:
    void updateWeaponInternal(int index, bool real);
    void updateWeaponsInternal(bool real);

    /* +0x08 ELF */ std::vector<unsigned int> m_weapons;

    // Both shipped allocations contain this final dword (ELF +0x14, PE +0x18), but no audited
    // IPIArmament/IPIArmamentReal body initializes or reads it. Preserve the extent without
    // assigning it a type or behavior.
    unsigned char m_unresolvedFinalDword[4];
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIKNOWLEDGE_IPIARMAMENT_H
