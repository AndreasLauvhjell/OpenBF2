// Path: [INFERRED] beside the attested SupplyObject.cpp. PE/ELF
#ifndef GAME_OBJECTS_GAME_SUPPLYDEPOT_H
#define GAME_OBJECTS_GAME_SUPPLYDEPOT_H
#include "../Base/SimpleObject.h"
#include "../Base/SimpleObjectTemplate.h"
#include <vector>

namespace dice { namespace hfe { namespace world {

class SupplyDepotTemplate : public SimpleObjectTemplate
{
public:
    SupplyDepotTemplate();
    virtual ~SupplyDepotTemplate();
    virtual unsigned int getClassID() const;
    virtual void* queryImplementation(unsigned int);
    virtual IObject* createObject() const;
    virtual bool makeScript(io::IStream*);
    void addAmmoType(int, int, float, float);
    void addVehicleType(std::string, int, float, float);
    void setHealth(int, float, float);
    bool hasAmmoType(int);

    float m_radius;                   // ELF +0x164 / PE +0x210
    int m_team;
    bool m_workOnVehicles;
    bool m_workOnSoldiers;
    std::vector<int> m_ammoTypes;      // ELF +0x170 / PE vector base +0x21C
    std::vector<float> m_initialAmmo;
    std::vector<float> m_maxAmmo;
    std::vector<float> m_ammoRate;
    std::vector<float> m_inverseAmmoRate;
    std::vector<float> m_ammoRegeneration;
    float m_healRate;                 // ELF +0x1B8 / PE +0x27C
    float m_inverseHealRate;
    float m_initialHealth;
    float m_maxHealth;                // retail ctor leaves this uninitialized
    float m_healthRegeneration;
    std::vector<std::string> m_vehicleNames;
    std::vector<float> m_initialRepair;
    std::vector<float> m_maxRepair;
    std::vector<float> m_repairRate;
    std::vector<float> m_inverseRepairRate;
    std::vector<float> m_repairRegeneration;
    float m_updateInterval;           // ELF +0x214 / PE +0x2F0
};

class SupplyDepot : public SimpleObject
{
public:
    explicit SupplyDepot(const SupplyDepotTemplate*);
    virtual ~SupplyDepot();
    virtual void init();
    virtual void destroy(bool);
    virtual void handleUpdate(float, unsigned int);
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
    void update(float);
    void reset();
    void reloadAmmo(IObject*);
    void healSoldier(IObject*);
    void repairVehicle(IObject*);
    void workOnSoldiers();
    void workOnVehicles();
    bool getWorkOnVehicles();
    bool getWorkOnSoldiers();
    bool hasVehicleType(unsigned int);
    bool isHealing();
    bool isRepairing();
    bool isReloading();
    void debugInfo();
private:
    const SupplyDepotTemplate* m_depotTemplate; // ELF +0x138 / PE +0x150
    int m_team;
    std::vector<float> m_ammo;
    std::vector<int> m_ammoToGive;
    std::vector<float> m_ammoTime;
    float m_health;
    std::vector<float> m_repair;
    std::vector<unsigned int> m_vehicleIds;
    float m_healthToGive;              // ELF +0x180 / PE +0x1AC; first update writes it
    bool m_hasRepair;
    bool m_hasAmmo;                    // ctor leaves uninitialized; update clears before use
    bool m_hasHeal;
    float m_lastUpdate;                // initialized only on game-server interface arm
};

} } }
#endif
