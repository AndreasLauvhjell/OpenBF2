// A selectable spawn group owning a list of spawn-point references and eligibility
// flags. Dynamic groups recompute their average position each tick. SpawnManager
// registers groups as static ghosts at fixed network ids. [INFERRED] Header placement.

#ifndef GAME_GAMELOGIC_SPAWNSYSTEM_SPAWNGROUP_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_SPAWNSYSTEM_SPAWNGROUP_H

#include <string>
#include <vector>

#include "../../../Core/IBase.h"
#include "../../../IO/Network/NetworkableBase.h"
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

class ISpawnPoint;

// Spawn-group interface with a virtual destructor after the IBase methods.
// getPos returns a borrowed reference; getSpawnPoint returns null if none qualifies.
class ISpawnGroup : public IBase
{
public:
    virtual ~ISpawnGroup() {}                                        // slots 4/5
    virtual void  setTeam(int team) = 0;                             // 6  (+0x18)
    virtual int   getTeam() const = 0;                               // 7  (+0x1C)
    virtual void  setSquad(int squad) = 0;                           // 8  (+0x20)
    virtual int   getSquad() const = 0;                              // 9  (+0x24)
    virtual void  setActive(bool active) = 0;                        // 10 (+0x28)
    virtual bool  getActive(bool isAI) const = 0;                    // 11 (+0x2C)
    virtual void  setPos(Vec3 position) = 0;                         // 12 (+0x30) - by value
    virtual const Vec3& getPos() = 0;                                // 13 (+0x34)
    virtual void  setOnlyForHuman(bool only) = 0;                    // 14 (+0x38)
    virtual bool  getOnlyForHuman() const = 0;                       // 15 (+0x3C)
    virtual std::vector<ISpawnPoint*>& getSpawnPoints() = 0;         // 16 (+0x40)
    virtual ISpawnPoint* getSpawnPoint(bool isAI) const = 0;         // 17 (+0x44)
    virtual void  setControlPointId(unsigned int id) = 0;            // 18 (+0x48)
    virtual unsigned int getControlPointId() = 0;                    // 19 (+0x4C)
};

class SpawnGroup : public ISpawnGroup, public io::NetworkableBase
{
public:
    SpawnGroup();                                    //  (C2) / 0x81110D0 (C1), 186 B - read in full
    virtual ~SpawnGroup();

    // the quartet, refcount at +0x40
    virtual void  addRef();                          // 15 B
    virtual int   getRef();                          // 11 B
    virtual int   release();                         // 57 B
    virtual void* queryInterface(unsigned int) const;// 45 B

    // ISpawnGroup
    virtual void  setTeam(int team);                 // 24 B  - only while m_teamSettable
    virtual int   getTeam() const;                   // 11 B
    virtual void  setSquad(int squad);               // 14 B
    virtual int   getSquad() const;                  // 11 B
    virtual void  setActive(bool active);            // 14 B
    virtual bool  getActive(bool isAI) const;        // 161 B - the server asks the points
    virtual void  setPos(Vec3 position);             // 50 B
    virtual const Vec3& getPos();                    // 131 B - lazy average, per-tick for dynamics
    virtual void  setOnlyForHuman(bool only);        // 14 B
    virtual bool  getOnlyForHuman() const;           // 12 B
    virtual std::vector<ISpawnPoint*>& getSpawnPoints();   // 11 B
    virtual ISpawnPoint* getSpawnPoint(bool isAI) const;   // 218 B - a random active one
    virtual void  setControlPointId(unsigned int id);      // 14 B
    virtual unsigned int getControlPointId();        // 11 B

    // INetworkable, through NetworkableBase
    virtual bool  init(void* object);                // the baseline block
    virtual void  updateStateMask(unsigned int* mask);   // [FRONTIER] the wire
    virtual bool  getNetUpdate(io::BitStream&, io::BaseLineData*, unsigned int, int);   // 624 B - [FRONTIER]
    virtual void  setNetUpdate(io::BitStream&, float, bool, io::BaseLineData*, int);    // 1,624 B - [FRONTIER]
    virtual unsigned int getMinimumBitSize() const;  // 10 B - 40

    // non-virtual
    void addSpawnPoint(ISpawnPoint* point);          // 149 B - (+ the aiOnly latch)
    void removeSpawnPoint(ISpawnPoint* point);       // 149 B -
    void calcNewPos();                               // 236 B - the average
    void getUnsignedWorldPosition(unsigned char* outX, unsigned char* outZ);   // 517 B - the
                                                     //   8-bit map position the create event carries

    // The manager reads these raw (getCreateSpawnGroup / _removeGroup / createSpawnGroupOnClients /
    // getGroupsForTeam / canTeamGetAnySpawnGroup) - the ControlPoint/ControlPointTemplate arrangement.
    friend class SpawnManager;

protected:
    // layout +0x40..+0x83 - every ctor value from the 186-byte constructor, read in full. IDA's
    // struct names kept where it had them (refs, team, squad, positionX/Y/Z, active, spawnPoints,
    // humanOnly, controlPointId, hasSpawnPoints, spawnGroupId, tickCount).
    /* 0x40 */ int   m_refCount;                     // ctor 1
    /* 0x44 */ int   m_team;                         // ctor -1
    /* 0x48 */ int   m_squad;                        // ctor -1
    /* 0x4C */ Vec3  m_position;                     // ctor (0, 0, 0) - the average of the points
    /* 0x58 */ std::string m_string58;               // [UNRESOLVED name] ctor "" - nothing recovered reads it
    /* 0x5C */ bool  m_active;                       // ctor true
    /* 0x5D */ bool  m_teamSettable;                 // [CHOSEN] ctor true - setTeam's gate; nothing
                                                     //   recovered clears it
    /* 0x5E */ char  _pad5E[2];
    /* 0x60 */ std::vector<ISpawnPoint*> m_spawnPoints;   // ctor empty
    /* 0x6C */ bool  m_positionDirty;                // ctor true - calcNewPos clears; add/remove and
                                                     //   SpawnManager::makeAverageGroupPositionDirty
                                                     //   set (that function's raw +0x6C poke)
    /* 0x6D */ bool  m_positionLocked;               // [CHOSEN] ctor false - getPos skips the recompute
                                                     //   while set; nothing recovered sets it
    /* 0x6E */ bool  m_onlyForAI;                    // ctor false - addSpawnPoint: the group is AI-only
                                                     //   while every point is (see the .cpp)
    /* 0x6F */ bool  m_onlyForHuman;                 // ctor false - set/getOnlyForHuman
    /* 0x70 */ unsigned int m_controlPointId;        // ctor -1
    /* 0x74 */ bool  m_hasSpawnPoints;               // ctor false - addSpawnPoint sets, remove recomputes
    /* 0x75 */ char  _pad75[3];
    /* 0x78 */ int   m_spawnGroupId;                 // ctor -1 - the manager stamps it (getCreateSpawnGroup)
    /* 0x7C */ int   m_tickCount;                    // ctor 0 - getPos's per-tick re-average stamp
    // Replication baseline mirrors the six CreateSpawnGroupEvent wire fields.
    struct BaseLine                                  // 16 B; init() news four of them (0x40)
    {
        /* +0x00 */ int           team;              // getTeam()
        /* +0x04 */ unsigned char active;            // getActive(false)
        /* +0x05 */ unsigned char onlyForAI;         // m_onlyForAI (+0x6E)
        /* +0x06 */ unsigned char hasSpawnPoints;    // m_hasSpawnPoints - written first, tested 4th
        /* +0x07 */ unsigned char posX;              // getUnsignedWorldPosition's pair -
        /* +0x08 */ unsigned char posZ;              //   the quantised 8-bit map position
        /* +0x09 */ char          _pad09[3];
        /* +0x0C */ int           squad;             // getSquad()
    };
    /* 0x80 */ BaseLine* m_baselines;                // ctor 0 - init() news [4]; the dtor delete[]s it
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SPAWNSYSTEM_SPAWNGROUP_H
