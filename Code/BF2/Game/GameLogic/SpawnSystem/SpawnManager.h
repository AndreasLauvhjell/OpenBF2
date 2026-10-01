// Registers spawn groups and points and maps server group ids to client-visible ids.
// Squad leaders provide dynamic spawn points. [INFERRED] Header placement.

#ifndef GAME_GAMELOGIC_SPAWNSYSTEM_SPAWNMANAGER_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_SPAWNSYSTEM_SPAWNMANAGER_H

#include <list>
#include <map>
#include <string>

#include "../../../Core/IBase.h"
#include "../../../Core/Ids.h"      // IID_IBase, IID_ISpawnManager (0xC486/50310) - queryInterface

namespace dice {
namespace hfe {

class ISpawnPoint;
class ISpawnGroup;
namespace world { class IPlayer; }

// [UNRESOLVED] its own interface. `_ZTVN4dice3hfe13ISpawnManagerE` (0x8890F40) exists and
// SpawnManager's constructor calls `ISpawnManager::ISpawnManager(this)`, so it is a real base - but
// its vtable's slots are not separately readable here (SpawnManager overrides all of them). Modelled
// as an abstract base carrying exactly the slots SpawnManager implements, in vtable order.
class ISpawnManager : public IBase
{
protected:
    ~ISpawnManager() {}
};

class SpawnManager : public ISpawnManager
{
public:
    // Keep the IBase methods before the virtual destructor to preserve interface order.
    virtual void  addRef();                                   // +0x00
    virtual int   getRef();                                   // +0x04
    virtual int   release();                                  // +0x08
    virtual void* queryInterface(unsigned int iid) const;     // +0x0C

    SpawnManager();
    virtual ~SpawnManager();                                  // +0x10 / +0x14

    // Spawn-manager interface; preserve the declared parameter widths and virtual order.
    virtual void  addSpawnPoint(ISpawnPoint* point, int group);          // +0x18
    virtual void  remove(const ISpawnPoint* point);                      // +0x1C
    virtual void  removeAll();                                           // +0x20
    virtual void  removeGroup(int groupId);                              // +0x24
    virtual ISpawnGroup* getSpawnGroup(int groupId);                     // +0x28
    // Returns borrowed storage of group IDs, not group pointers.
    virtual const std::list<int>& getGroupsForTeam(int team);            // +0x2C
    virtual const std::list<int>& getGroupsForPlayer(world::IPlayer* player);   // +0x30
    virtual bool  canTeamGetAnySpawnGroup(int team);                     // +0x34
    // return (the PCO batch): PlayerControlObject::init stores the result
    // into its int m_spawnGroupId and compares it against -1 - it returns the group ID, not a
    // pointer. The old ISpawnGroup* guess is retired on that caller evidence.
    virtual int createDynamicSpawnGroup();                               // +0x38
    virtual int createExistingDynamicSpawnGroup(int groupId);            // +0x3C   - the id
                                                                         // Returns borrowed storage for the spawn-group map.
    virtual const std::map<int, ISpawnGroup*>& getSpawnGroups();         // +0x40

    virtual int   getMappedSpawnGroupId(int groupId, int forWhom);       // +0x44
    // Creates the client's replicated group and returns its spawn-group ID.
    virtual int   createClientSpawnGroup(unsigned char spawnGroupId, unsigned int netId, int team,
                                         bool active, bool onlyForAI, bool hasSpawnPoints,
                                         unsigned char quantisedX, unsigned char quantisedZ);
                                                                         // +0x48
    virtual void  removeClientSpawnGroup(int groupId);                   // +0x4C
    virtual void  createAllSpawnGroupsOnClient(int client);              // +0x50

    // The factory ClassManager::createInstance calls. Signature fixed by ClassRegInfo::create.
    static IBase* create(unsigned int iid, IBase* outer);

    // Finds or creates a group and registers its fixed network ID.
    // Server-created groups are announced to playing clients; clients must request clientCreated explicitly.
    ISpawnGroup* getCreateSpawnGroup(unsigned int groupId, unsigned int netId, bool clientCreated);
    void _removeGroup(int groupId, bool clientRemoved);
    bool createSpawnGroupOnClients(int connectionId, ISpawnGroup* group);   //  (-1 = all)
    void removeSpawnGroupOnClients(int connectionId, ISpawnGroup* group);
    void listSpawnGroups();                                              // console

private:
    // layout, sizeof == 52 (0x34), every offset from the constructor
    // /* +0x00 */ vptr
    /* +0x04 */ int m_refCount;

    // +0x08. allocate(12), self-linked -> one _List_node_base sentinel, i.e. an empty std::list.
    // [UNRESOLVED] element type; std::list<ISpawnPoint*> is [INFERRED] from addSpawnPoint/remove.
    std::list<ISpawnPoint*> m_spawnPoints;

    // +0x0C header, +0x10 node count. allocate(24) = 16-byte node base + an 8-byte value_type, so
    // the mapped type is one pointer. getSpawnGroups() returns &this[+0x0C] and getSpawnGroup(int)
    // keys it by id, which is where <int, ISpawnGroup*> comes from. [INFERRED] from those two.
    std::map<int, ISpawnGroup*> m_spawnGroups;

    // [UNRESOLVED] Constructor leaves this field uninitialized.
    int m_unknown14;

    /* +0x18 */ int m_nextGroupId;   // set to 1. [INFERRED] name - the only int the ctor seeds
                                     // Maps (control-point ID, authored group) to allocated group ID using m_nextGroupId.
    std::map<std::pair<int, int>, int> m_mappedGroupIds;

    // +0x24. also never  BY THE constructor. [UNRESOLVED].
    int m_unknown24;

    // +0x28. allocate(12), self-linked - a second empty std::list. IDA's field name for it.
    // element type std::list<int> - the ELF's getGroupsForTeam/ForPlayer
    // instantiate std::list<int>::insert on it with the map KEY (the group id).
    std::list<int> m_groupsForTeam;

    /* +0x2C */ std::string m_soldierSpawn;   // seeded "soldier_spawn"
    /* +0x30 */ std::string m_vehicleSpawn;   // seeded "vehicle_spawn"

public:
    // Invalidates a group's cached average position. Missing groups are ignored.
    void makeAverageGroupPositionDirty(int groupId);

private:
    void markGroupDirty(ISpawnGroup* group);  // the +0x6C write - SpawnGroup is real
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SPAWNSYSTEM_SPAWNMANAGER_H
