// Weapon access and selection for an AI object. Collected weapon interfaces remain owned by their engine objects.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTARMAMENT_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTARMAMENT_H

#include <set>
#include <vector>

#include "IAIObjectPlugIn.h"

namespace dice {
namespace hfe {
namespace world { class IObject; }
namespace ai {

class AITemplateArmament;

class AIObjectArmament : public IAIObjectArmament
{
public:
    AIObjectArmament(IAIObject* object, AITemplateArmament* aiTemplate);
    virtual ~AIObjectArmament();

    virtual int getId() const;                                  // +0x08
    virtual void createInformationPlugIn(unsigned int handle, bool real);   // +0x0C
    virtual void init();                                        // +0x10
    virtual void update();                                      // +0x14

    virtual int isAntiAircraft() const;                         // +0x18
    virtual int getActiveWeapon() const;                        // +0x1C
    virtual void setActiveWeapon(unsigned int index, int side); // +0x20
    virtual IWeapon* getWeapon(unsigned int index) const;       // +0x24
    virtual void setLastFiredWeapon();                          // +0x28
    virtual IWeapon* getLastFiredWeapon() const;                // +0x2C
    virtual const std::vector<IWeapon*>& getWeapons() const;    // +0x30
    virtual int getWeaponIdFromActivationId(int activation, int fire) const;   // +0x34
    virtual bool rearm(IWeapon* weapon, float amount, int side);    // +0x38
    virtual bool getProjectilePosition(Vec3& out) const;        // +0x3C
    virtual bool getProjectileSpeed(Vec3& out) const;           // +0x40

    // Non-virtual, , / . [INFERRED void] - init discards nothing.
    void traverseObject(world::IObject* object);

private:
    /* +0x08 */ std::vector<IWeapon*> m_weapons;
    /* +0x14 */ int m_lastFiredWeapon;
    /* +0x18 */ AITemplateArmament* m_template;
    /* +0x1C */ unsigned int m_activeWeapon;
    /* +0x20 */ std::set<unsigned int> m_informationHandles;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTARMAMENT_H
