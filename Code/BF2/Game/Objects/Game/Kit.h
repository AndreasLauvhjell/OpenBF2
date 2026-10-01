// Soldier loadout whose unlock levels reparent existing parts between the kit and recycle bin.
// Inherits Item's interface set and Bundle's class id. [INFERRED] File split; original Kit and KitTemplate shared kit.cpp.

#ifndef GAME_OBJECTS_GAME_KIT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_KIT_H

#include <map>
#include <vector>
#include <string>
#include <utility>
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // VehicleMod - getKitVehicleMods

#include "../Item.h"

namespace dice {
namespace hfe {
class GameClient;   // createKit's direct store - see the friend line in Kit below
namespace world {

class KitTemplate;

class Kit : public Item
{
public:
    // (C1) / 0x8183BB0 (C2). Two containers built empty, then seven fields - five of them
    // copied off the KitTemplate. chosenUnlockLevel starts at -1 ("nothing chosen"), and
    // currentAbility starts at 1.0f only when the template HAS an ability restore rate (-1.0f is
    // that field's disabled sentinel), else -1.0f.
    explicit Kit(const KitTemplate* objectTemplate);
    virtual ~Kit();                                       // the maps, nothing else

    // the eight overrides
    virtual void init();
    virtual void disableCachedObject();
    virtual void initNetworkableComp();
    virtual void handleMessage(TemplateMessage message, IPlayer* player, bool remote);
    virtual void handlePickup(IPlayer* player, IObject* obj);
    virtual void handleDrop(IPlayer* player, IObject* obj);
    virtual bool enableItem();
    virtual void disableItem();

    // the ONE new virtual, primary slot +0x1B8
    // the ELF's return type is `dice::hfe::KitType`; this tree types KitTemplate::m_kitType as int
    // and has no enumerators for it, so int it stays until the enum is recovered.
    virtual int getType() const;

    // non-virtual members
    // `[[this+0x184]+0x1A8]`, i.e. the kit's OWN template pointer then
    // KitTemplate::m_creationIndex. One of the two CreateKitEvent operands.
    unsigned int getCreationIndex() const;

    void updateUnlockStatus();                            // see the header note
    void updateGeometry(bool pickedUp);
    void setGeomIndex(int index);                         // -1 means "default"
    bool checkIfPartExists(IObject* skip, int itemIndex) const;

    // The ability meter (the supply/repair/heal budget a kit carries).
    void  updateAbility();
    float getAbilityUpdate() const;                       // off the template
    float getCurrentAbility() const;
    void  setCurrentAbility(float ability);
    bool  taxAbility(float cost);                         // spend, false if short

    // Returns the template's vehicle-category/modifier pairs.
    std::vector<std::pair<std::string, VehicleMod> >* getKitVehicleMods();

    // The two halves of "hand this kit's contents to a soldier", called from handlePickup.
    void giveSoldierKitItems(IObject* soldier, IPlayer* player);
    void addObjectToSoldier(IObject* soldier, IPlayer* player, IObject* obj);

    // The CreateKitEvent operand accessors in KitTemplate.cpp read m_chosenUnlockLevel directly, as
    // both of retail's producers do (`mov eax,[kit+174h]` inline in GameServer.cpp and
    // ServerConnection.cpp - no getter symbol exists on either side).
    friend int kitChosenUnlockLevel(IObject* kit);
    // ELF 819E732 / PE 5948B4: the PCO compares the live index before choosing crew/default.
    friend class PlayerControlObject;
    // Client kit creation stores the chosen unlock level directly.
    friend class ::dice::hfe::GameClient;

protected:
    // The kit's own typed template. Object holds the same pointer at +0x14; Kit also keeps it at
    // +0x184 assigned from the constructor's argument. Both are read in the ELF - updateAbility and
    // getAbilityUpdate go through +0x14, getCreationIndex and updateGeometry through +0x184 - and
    // they are the same pointer, so this one accessor serves both.
    const KitTemplate* kitTemplate() const;

    // Offsets describe the 32-bit ELF; native map sizes differ by compiler.
    /* 340 */ std::map<IObject*, IObject*> m_partOwners;                    // +0x154 node 24 B
    /* 352 */ std::map<IObject*, std::vector<IObject*> > m_itemContainers;  // +0x160 node 32 B
    /* 364 */ IObject* m_recycleBin;                     // +0x16C built by init, holds suppressed parts
    /* 368 */ float m_currentAbility;                    // +0x170
    /* 372 */ int   m_chosenUnlockLevel;                 // +0x174 ctor -1
    /* 376 */ int   m_geomIndex;                         // +0x178 ctor = template m_defaultGeomIndex
    /* 380 */ int   m_defaultGeomIndex;                  // +0x17C ctor = the same
    /* 384 */ int   m_dropGeomIndex;                     // +0x180 ctor = template m_dropGeom
    /* 388 */ const KitTemplate* m_kitTemplate;          // +0x184
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_KIT_H
