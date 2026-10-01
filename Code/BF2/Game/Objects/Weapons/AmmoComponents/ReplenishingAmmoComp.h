// Replenishing ammunition for support tools.

#ifndef GAME_OBJECTS_WEAPONS_AMMOCOMPONENTS_REPLENISHINGAMMOCOMP_H
#define GAME_OBJECTS_WEAPONS_AMMOCOMPONENTS_REPLENISHINGAMMOCOMP_H

#include <iosfwd>
#include <vector>

#include "AmmoComp.h"
#include "../../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class IPlayer;
class ISoldier;

enum ReplenishingType : int
{
    RTHeal = 0,
    RTAmmo = 1
};

// Retail emits these in the central weapons-vocabulary TU, not in the attested component TU.
// They are declared here so the template contract is complete; ModuleWeapon.cpp owns the bodies.
std::ostream& operator<<(std::ostream& stream, ReplenishingType type);
std::istream& operator>>(std::istream& stream, ReplenishingType& type);

class ReplenishingAmmoCompTemplate;

class ReplenishingAmmoComp : public AmmoComp
{
public:
    explicit ReplenishingAmmoComp(ReplenishingAmmoCompTemplate* objectTemplate);
    virtual ~ReplenishingAmmoComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void* getAbilityObjectList();
    virtual bool getAbilityWorking();
    virtual float getAbility() const;
    virtual bool requestFire(bool released, bool alternate);
    virtual void postFire(WeaponFireEvent& event);
    virtual void fireStop(bool alternate);
    virtual bool requestReload(bool automatic);
    virtual void enable();

    virtual void queueReload();
    virtual bool canBeRefilled();
    virtual float refillPercent(float percent, bool testOnly);
    virtual unsigned int refillAbsolute(unsigned int amount, bool testOnly);
    virtual float getReloadTimeLeftInPercent() const;
    virtual int getMaxNrOfMags() const;
    virtual int getMagVectorSize() const;
    virtual std::vector<int>* getAmmo();

    virtual bool canReload() const;

private:
    void updateAbilityObjectAim();
    void updateAbilityObjectRadius();
    bool isAbilityWorkPossible(IObject* object);
    void taxAbility(float amount);
    ISoldier* getSoldier() const;

    // ELF layout: AmmoComp 0x5C, this leaf 0x84. The first vector has no recovered readers or
    // writers; only construction/destruction attest it. Its trivial 4-byte element spelling stays
    // [UNRESOLVED], while the vector representation and member order are fixed.
    /* +0x5C */ int m_refCount;
    /* +0x60 */ std::vector<unsigned int> m_unusedValues;
    /* +0x6C */ bool m_abilityStopped;
    /* +0x6D */ bool m_releaseRequired;
                 // +0x6E..+0x6F padding
    /* +0x70 */ ReplenishingAmmoCompTemplate* m_replenishingTemplate;
    /* +0x74 */ float m_targetRefreshTimer;
    /* +0x78 */ std::vector<WeakPtr<IObject> > m_abilityObjects;
};

class ReplenishingAmmoCompTemplate : public AmmoCompTemplate
{
public:
    ReplenishingAmmoCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual bool makeScript(io::IStream* stream);
    virtual ~ReplenishingAmmoCompTemplate();

    void setAbilityDrain(float value);
    void setAbilityStrength(float value);
    void setAbilityCost(float value) { m_abilityCost = value; }
    void setAbilityMaterial(int value) { m_abilityMaterial = value; }
    void setAbilityRadius(float value) { m_abilityRadius = value; }
    void setReplenishingType(ReplenishingType value) { m_replenishingType = value; }
    void setOnlyActiveWhileFiring(bool value) { m_onlyActiveWhileFiring = value; }

    float getAbilityDrain() const { return m_abilityDrain; }
    float getAbilityDrainPerTick() const { return m_abilityDrainPerTick; }
    float getAbilityStrength() const { return m_abilityStrength; }
    float getAbilityStrengthPerTick() const { return m_abilityStrengthPerTick; }
    float getAbilityCost() const { return m_abilityCost; }
    int getAbilityMaterial() const { return m_abilityMaterial; }
    float getAbilityRadius() const { return m_abilityRadius; }
    ReplenishingType getReplenishingType() const { return m_replenishingType; }
    bool getOnlyActiveWhileFiring() const { return m_onlyActiveWhileFiring; }

private:
    // ELF: AmmoCompTemplate 0x48, this leaf 0x70. The PE preserves this order after its larger
    // MSVC std::string-bearing base.
    int m_refCount;
    float m_abilityDrain;
    float m_abilityDrainPerTick;
    float m_abilityStrength;
    float m_abilityStrengthPerTick;
    float m_abilityCost;
    int m_abilityMaterial;
    float m_abilityRadius;
    ReplenishingType m_replenishingType;
    bool m_onlyActiveWhileFiring;
};

// Two free helpers are externally named in the ELF and live in the same attested TU.
bool checkObjectTeam(IObject* object, IPlayer* player);
IWeaponObject* getPrimWeapon(IPlayer* player);

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_AMMOCOMPONENTS_REPLENISHINGAMMOCOMP_H
