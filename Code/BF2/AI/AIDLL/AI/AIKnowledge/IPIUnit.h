// Unit information plug-in with no real/known split or extra lifecycle virtuals.
// [INFERRED] Source path follows adjacent AIKnowledge classes.
#ifndef AI_AIDLL_AI_AIKNOWLEDGE_IPIUNIT_H
#define AI_AIDLL_AI_AIKNOWLEDGE_IPIUNIT_H

#include "../../../../IO/Input/Input.h"
#include "../../../../Math/Vec3.h"
#include "InformationPlugIn.h"

namespace dice { namespace hfe { namespace ai {

class IPIUnit : public InformationPlugIn
{
public:
    explicit IPIUnit(Information* information);
    virtual ~IPIUnit();
    virtual int getType();
    virtual void setTime();
    virtual void setTime(double time);

    void calculateStrengths();
    float getAmmo();
    float getAmmo(double time);

    bool calculateStrMod(float health);
    unsigned int getOccupier(int side);
    unsigned int getOccupiedObject(int side);
    int getOccupyingPlayerId(int side);
    int getNBattleStrengths();
    int getNStrategicStrengths();
    int getEquipmentType();
    bool isOccupied();
    bool useNoPathfindingToGetToObject() const;
    bool hasExposedSoldier() const;
    bool getAlwaysTargetSoldier() const;
    float getCullDistance() const;
    float getCullDistanceSqr() const;
    float getEntryRadius();
    io::PlayerInputMap getSelectKey();
    double getLastPlayerPossesionTime() const;
    int getTeamLock() const;
    bool isTeamLocked(int side) const;
    int getMissileLockStatus() const;
    void setMissileLockStatus(int status);
    bool mustBorrowArmament() const;
    const Vec3& getEntryPositionOfNonMobile() const;

    // [INFERRED] accessor name; both SAI::updateStrengths bodies inline this +8 load and
    // index six float entries. The constructor allocates getNBattleStrengths floats here.
    float* getBattleStrengths() const { return m_battleStrengths; }

    // [INFERRED] accessor name. Area::update reads the existing +0x0C pointer on
    // both ports, supplies two float buckets to SampleHelper and indexes them by status.
    float* getStrategicStrengths() const { return m_strategicStrengths; }

private:
    // Offsets agree in both shipped x86 ports. The constructor intentionally calls getAmmo before
    // initializing m_ammo/m_ammoTime, and never initializes m_pointer24. Preserve that order.
    /* +0x08 */ float* m_battleStrengths;
    /* +0x0C */ float* m_strategicStrengths;
    /* +0x10 */ float m_strengthMod;
    /* +0x14 */ float m_ammo;
    /* +0x18 */ double m_ammoTime;
    /* +0x20 */ bool m_ownsPointer24; // pointee semantics remain [UNRESOLVED]
    /* +0x24 */ void* m_pointer24;
};

} } }
#endif
