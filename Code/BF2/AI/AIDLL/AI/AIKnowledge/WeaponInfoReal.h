// [INFERRED] path: the paired ELF gives WeaponInfoReal its own translation unit; the filename
// follows the class. so / AIDLL_w32ded.dll reads.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_WEAPONINFOREAL_H
#define AI_AIDLL_AI_AIKNOWLEDGE_WEAPONINFOREAL_H

#include "WeaponInfo.h"

namespace dice { namespace hfe { namespace ai {

class WeaponInfoReal : public WeaponInfo
{
public:
    explicit WeaponInfoReal(IWeapon* weapon);
    virtual ~WeaponInfoReal();

    virtual bool   update();
    virtual int    getShotsInMag(int magazine);
    virtual int    getAmmo() const;
    virtual int    getCurrentAmmo() const;
    virtual int    getNumberOfMags() const;
    virtual bool   isReadyToFire() const;
    virtual double getTimeOfFiring() const;
    virtual float  getHeat() const;
    virtual bool   usesHeat() const;
    virtual float  getSecurity() const;
    virtual float  getSecurity(double time) const;
};

} } }
#endif
