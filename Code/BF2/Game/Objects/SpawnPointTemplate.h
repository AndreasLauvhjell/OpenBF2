// Authored spawn-point settings, enabled by default. [INFERRED] Source placement.
// Virtual order and field order differ for the human/AI restriction pair; preserve both.

#ifndef GAME_OBJECTS_SPAWNPOINTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_SPAWNPOINTTEMPLATE_H

#include "ObjectTemplate.h"
#include "ISpawnPointTemplate.h" // Secondary interface at +0x30.

namespace dice {
namespace hfe {

// ISpawnPointTemplate is now a real second base (ISpawnPointTemplate.h - the 32-slot secondary
// table read in full). The eighteen-plus accessor slots below are its overrides; setSpawnRotation /
// getSpawnRotation are the two the template leaves to the interface's own default bodies.
class SpawnPointTemplate : public world::ObjectTemplate, public ISpawnPointTemplate   // ELF: @+0x30
{
public:
    SpawnPointTemplate();                                 // read in full

    // The registered factory. This class uses the BF2_FACTORY shape - its OWN static create
    // (0x856CB60) - not the ClassFactory<T> template the ObjectTemplate families use.
    static IBase* create(unsigned int iid, IBase* outer);

    virtual void  addRef();                               // refcount +0x34
    virtual int   getRef();
    virtual int   release();                              // delete on 0
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;              // reads the CID global
    virtual ~SpawnPointTemplate();
    virtual void* queryImplementation(unsigned int);      // own CID or CID_ObjectTemplate
    virtual world::IObject* createObject() const;         // new(0x158) SpawnPoint(this)
                                                          // [FRONTIER - the object class]
    virtual bool  makeScript(io::IStream*);

    // the console surface: eighteen slots at +0xE8..+0x14C, every one a plain accessor
    virtual void  setSpawnPositionOffset(Vec3&);
    virtual Vec3& getSpawnPositionOffset();
    virtual void  setSpawnPreventionDelay(float);
    virtual float getSpawnPreventionDelay();
    virtual void  setGroup(unsigned int);
    virtual unsigned int getGroup();
    virtual void  setControlPointId(unsigned int);
    virtual unsigned int getControlPointId() const;
    virtual void  setActive(bool);                        // the pair the old header missed
    virtual bool  getActive();
    virtual void  setEnterOnSpawn(bool);
    virtual bool  getEnterOnSpawn();
    virtual void  setAIEnterOnSpawn(bool);
    virtual bool  getAIEnterOnSpawn();
    virtual void  setSpawnAsParaTroper(bool);             // [sic] dice's spelling
    virtual bool  getSpawnAsParaTroper();
    virtual void  setMinSpawnHeight(float);
    virtual float getMinSpawnHeight() const;
    virtual void  setScatterSpawnPositions(bool);
    virtual bool  getScatterSpawnPositions();
    virtual void  setOnlyForAI(bool);
    virtual bool  getOnlyForAI() const;
    virtual void  setOnlyForHuman(bool);
    virtual bool  getOnlyForHuman() const;
    virtual void  setAllowSpawnCloseToVehicle(bool);
    virtual bool  getAllowSpawnCloseToVehicle() const;

    // layout (ctor values are the ELF's)
    /* 0x34 */ int   m_refCount;                 // ctor 1
    /* 0x38 */ Vec3  m_spawnPositionOffset;      // ctor (0, 1.02f, 0) - NOT a zero vector; the
                                                 // y default lifts the spawn slightly off the
                                                 // ground. 0x3F828F5C exactly.
    /* 0x44 */ float m_spawnPreventionDelay;     // ctor 0
    /* 0x48 */ unsigned int m_group;             // ctor 0
    /* 0x4C */ unsigned int m_controlPointId;    // ctor -1 (as an unsigned: 0xFFFFFFFF)
    /* 0x50 */ bool  m_active;                   // ctor true
    /* 0x51 */ bool  m_enterOnSpawn;             // ctor false
    /* 0x52 */ bool  m_aiEnterOnSpawn;           // ctor false
    /* 0x53 */ bool  m_spawnAsParaTroper;        // ctor false  [sic]
    /* 0x54 */ float m_minSpawnHeight;           // ctor -1.0f - a sentinel, not a height
    /* 0x58 */ bool  m_scatterSpawnPositions;    // ctor false
    /* 0x59 */ bool  m_onlyForHuman;             // ctor false
    /* 0x5A */ bool  m_onlyForAI;                // ctor false
    /* 0x5B */ bool  m_allowSpawnCloseToVehicle; // ctor false
};

} // namespace hfe
} // namespace dice


#endif // GAME_OBJECTS_SPAWNPOINTTEMPLATE_H
