// Vehicle and object spawn configuration. [INFERRED] Source placement.
// The unindexed setter assigns slot zero; spawnDelay and setSpawnDelay both set the minimum and maximum.

#ifndef GAME_OBJECTS_OBJECTSPAWNERTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_OBJECTSPAWNERTEMPLATE_H

#include <string>

#include "Base/SimpleObjectTemplate.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class ObjectSpawnerTemplate : public SimpleObjectTemplate /* , IObjectSpawnerTemplate @ +0x164 */
{
public:
    ObjectSpawnerTemplate();                         // read in full; replaces the implicit
                                                     // ctor whose garbage the old instances wore

    virtual void  addRef();                          // refcount +0x168
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;   // the Bundle-shaped five... see .cpp
    virtual unsigned int getClassID() const;
    virtual ~ObjectSpawnerTemplate();                // 493/498 B - string releases, implicit here
    virtual void* queryImplementation(unsigned int); // flat 3-cid (own/Simple/Object)
    virtual IObject* createObject() const;           // new(0x1BC) ObjectSpawner [FRONTIER -
                                                     // the object class is the next recovery]
    virtual void  preCache() const;                  // overridden; see the .cpp
    virtual bool  makeScript(io::IStream*);

    // the 38 additions, +0x114..+0x1A8, ELF order
    virtual void setObjectTemplate(unsigned int index, std::string name);   // +0x114
    virtual void setObjectTemplate(std::string name);               // +0x118 = (0, name)
    // Both return a newly materialized string (ELF hidden sret; PE 0x56FDF0 / 0x56FE70).
    virtual std::string getObjectTemplate() const;                 // +0x11C
    virtual std::string getObjectTemplate(unsigned int) const;     // +0x120
    virtual void  setSpawnDelay(float);              // +0x124 writes MIN and MAX
    virtual void  spawnDelay(float);                 // +0x128 byte-identical twin
    virtual void  setMaxSpawnDelay(float);           // +0x12C
    virtual float getMaxSpawnDelay();                // +0x130
    virtual void  setMinSpawnDelay(float);           // +0x134
    virtual float getMinSpawnDelay();                // +0x138
    virtual void  setSpawnDelayAtStart(bool);        // +0x13C
    virtual bool  getSpawnDelayAtStart();            // +0x140
    virtual void  setTimeToLive(float);              // +0x144
    virtual float getTimeToLive();                   // +0x148
    virtual void  setDistance(float);                // +0x14C the false alarm's exhibit A
    virtual float getDistance();                     // +0x150
    virtual void  setTeam(unsigned int);             // +0x154
    virtual unsigned int getTeam();                  // +0x158
    virtual void  setActiveOnStart(bool);            // +0x15C
    virtual bool  getActiveOnStart();                // +0x160
    virtual void  setNrOfObjectToSpawn(int);         // +0x164
    virtual int   getNrOfObjectToSpawn();            // +0x168
    virtual void  setRadius(float);                  // +0x16C exhibit B
    virtual float getRadius();                       // +0x170
    virtual void  setMaxNrOfObjectSpawned(unsigned int);   // +0x174
    virtual unsigned int getMaxNrOfObjectSpawned();  // +0x178
    virtual void  setUseButtonRadius(float);         // +0x17C
    virtual float getUseButtonRadius();              // +0x180
    virtual void  setEffectOnSpawn(std::string);     // +0x184 by value, assign
    virtual const std::string& getEffectOnSpawn();   // +0x188
    virtual void  setSpawnOffset(Vec3);              // +0x18C BY value
    virtual Vec3  getSpawnOffset();                  // +0x190 sret, the radiusOffset shape
    virtual void  setDamageWhenLost(float);          // +0x194
    virtual float getDamageWhenLost();               // +0x198
    virtual void  setHoldObject(bool);               // +0x19C
    virtual bool  getHoldObject();                   // +0x1A0
    virtual void  setTeamOnVehicle(bool);            // +0x1A4
    virtual bool  getTeamOnVehicle();                // +0x1A8

protected:
    // Runtime spawners consume these template fields directly.
    friend class ObjectSpawner;

    // layout +0x164..+0x1C7, every default from the 316-byte ctor; accessor bodies corroborate
    // every offset. SimpleObjectTemplate occupies 0x000..0x163.
    /* +0x164 */ void* pVMT_IObjectSpawnerTemplate;  // the second base's vptr - raw slot, the
                                                     // interface class is unrecovered
    /* +0x168 */ int   m_refCount;                   // ctor 1
    /* +0x16C */ std::string m_effectOnSpawn;        // ctor empty; set/getEffectOnSpawn's target

    // The three spawnable slots - `ObjectTemplate.setObjectTemplate <index> <name>` in .con files.
    // setObjectTemplate stores the name and, when non-empty, resolves the pointer through
    // objectTemplateManager immediately; preCache walks the pointers.
    struct SpawnSlot
    {
        IObjectTemplate* resolved;                   // +0x170/+0x178/+0x180 - ctor 0
        std::string      name;                       // +0x174/+0x17C/+0x184 - ctor empty
    };
    /* +0x170 */ SpawnSlot m_objectTemplates[3];

    /* +0x188 */ float m_minSpawnDelay;              // ctor 30.0f
    /* +0x18C */ float m_maxSpawnDelay;              // ctor 60.0f
    /* +0x190 */ float m_timeToLive;                 // ctor 60.0f
    /* +0x194 */ float m_distance;                   // ctor 50.0f
    /* +0x198 */ unsigned int m_team;                // ctor 0
    /* +0x19C */ int   m_nrOfObjectToSpawn;          // ctor -1 (= all slots)
    /* +0x1A0 */ unsigned int m_maxNrOfObjectSpawned;// ctor 1
    /* +0x1A4 */ float m_radius;                     // ctor 5.0f
    /* +0x1A8 */ float m_useButtonRadius;            // ctor 0
    /* +0x1AC */ Vec3  m_spawnOffset;                // ctor (0, 0, 0)
    /* +0x1B8 */ float m_damageWhenLost;             // ctor 50.0f
public:
    /* +0x1BC */ int   m_controlPointId;              // ctor -1. NO accessor - the console
                                                     // Console properties and ObjectSpawner access this field directly.
protected:
    /* +0x1C0 */ bool  m_holdObject;                 // ctor false
    /* +0x1C1 */ bool  m_teamOnVehicle;              // ctor false
    // [UNRESOLVED] name. GameLogic sets this latch for dynamic bundles with respawn delay -1.
public:
    /* +0x1C2 */ bool  m_byte1C2;                    // ctor 0, no accessor - see above
protected:
    /* +0x1C3 */ bool  m_spawnDelayAtStart;          // ctor false
    /* +0x1C4 */ bool  m_activeOnStart;              // ctor true
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_OBJECTSPAWNERTEMPLATE_H
