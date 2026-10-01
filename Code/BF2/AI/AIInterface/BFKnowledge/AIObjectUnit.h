// Combat strength, ammunition and entry information for an AI object.
// init fills ammunition and strategic strengths; the constructor does not initialize those values.
// Double alignment differs between ELF and MSVC; these fields do not cross the DLL boundary.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTUNIT_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTUNIT_H

#include "IAIObjectPlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

class AITemplateUnit;

class AIObjectUnit : public IAIObjectUnit
{
public:
    AIObjectUnit(IAIObject* object, AITemplateUnit* aiTemplate);
    virtual ~AIObjectUnit();

    virtual int getId() const;                                  // +0x08
    virtual void createInformationPlugIn(unsigned int handle, bool real);   // +0x0C
    virtual void init();                                        // +0x10
    virtual void update();                                      // +0x14

    virtual int getAmmo() const;                                // +0x18, (the .inl)
    virtual int getMaxAmmo() const;                             // +0x1C
    virtual const int* getBattleStrengths() const;              // +0x20
    virtual const int* getStrategicStrengths() const;           // +0x24
    virtual int getBattleStrength(int index) const;             // +0x28
    virtual int getStrategicStrength(int index) const;          // +0x2C
    virtual void setBattleStrength(int index, int value);       // +0x30
    virtual void setStrategicStrength(int index, int value);    // +0x34
    virtual int getEquipmentType() const;                       // +0x38
    virtual bool useNoPathfindingToGetToObject() const;         // +0x3C
    virtual bool hasExposedSoldier() const;                     // +0x40
    virtual bool getAlwaysTargetSoldier() const;                // +0x44
    virtual bool isOccupied() const;                            // +0x48
    virtual float getCullDistance() const;                      // +0x4C
    virtual float getCullDistanceSqr() const;                   // +0x50
    virtual float getEntryRadius() const;                       // +0x54
    virtual IAIObject* getOccupier() const;                     // +0x58
    virtual IAIObject* getOccupiedObject() const;               // +0x5C
    virtual double getLastPlayerPossesionTime() const;          // +0x60
    virtual void setLastPlayerPossesionTime(double time);       // +0x64
    virtual int getTeamLock() const;                            // +0x68
    virtual bool mustBorrowArmament(IAIObject* vehicle) const;  // +0x6C
    virtual int getOccupyingPlayerId() const;                   // +0x70
    virtual io::PlayerInputMap getSelectKey() const;            // +0x74
    virtual void setMissileLockStatus(int status);              // +0x78
    virtual int getMissileLockStatus() const;                   // +0x7C
    virtual const Vec3& getEntryPositionOfNonMobile() const;    // +0x80

private:
    // AITemplateUnit::initFromObject writes a secondary's position key through +0x08 directly
    // (: `mov eax, [eax+8]; mov [eax+20h], ebx`).
    friend class AITemplateUnit;

    /* +0x08 */ AITemplateUnit* m_template;
    /* +0x0C */ int* m_battleStrengths;
    /* +0x10 */ int* m_strategicStrengths;
    /* +0x14 */ double m_lastPlayerPossesionTime;
    /* +0x1C */ int m_missileLockStatus;
    /* +0x20 */ int m_maxAmmo;
    /* +0x24 */ Vec3 m_entryPositionOfNonMobile;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTUNIT_H
