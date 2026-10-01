// Spawn marker derived directly from Object. [INFERRED] Source placement.
// Initialization and destruction skip SpawnManager registration during precaching; position and flag changes still invalidate group averages.

#ifndef GAME_OBJECTS_SPAWNPOINT_H     // [INFERRED] guard style
#define GAME_OBJECTS_SPAWNPOINT_H

#include "Object.h"
#include "ISpawnPoint.h"
#include "SpawnPointTemplate.h"

namespace dice {
namespace hfe {

// Spawn object exposing ISpawnPoint to spawn managers and groups.
class SpawnPoint : public world::Object, public ISpawnPoint
{
public:
    explicit SpawnPoint(const SpawnPointTemplate* objectTemplate);   // read in full
    virtual ~SpawnPoint();                                           // (D1) / 62 B (D0)

    // the quartet, refcount at +0x130
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// IBase/IObject -> this,
                                                     // ISpawnPoint -> +0x12C, ConsoleSaveable
                                                     // -> +0x80

    // the Object overrides
    virtual void  destroy(bool force);               // precache-gated manager remove
    virtual void  setAbsolutePosition(const Vec3&);  // + group-average dirty
    virtual void  updateFlags(unsigned int set, unsigned int clear);   // likewise
    virtual bool  makeScript(io::IStream*);          // Object's, bool-normalised
    virtual void  init();                            // precache-gated manager add

    // the seventeen additions, +0x180..+0x1C0 in ELF slot order
    // Return types marked [INFERRED] where only the body's shape argues them; the four big
    // spawn-selection bodies are [FRONTIER].
    virtual bool  spawn(world::IObject* obj, float); // +0x180 teleport obj here; the
                                                     // float is ignored, as shipped
    virtual bool  isOccupied(float) const;           // +0x184 the float is ignored too
    virtual void  setActive(bool);                   // +0x188
    virtual bool  getActive(bool, int);              // +0x18C [FRONTIER]
    virtual world::IObject* getObjectToEnter(bool, int);
    virtual bool  getEnterOnSpawn();                 // +0x194
    virtual bool  getAIEnterOnSpawn();               // +0x198
    virtual void  setSpawnGroupId(int);              // +0x19C
    virtual int   getSpawnGroupId();                 // +0x1A0 the slot updateFlags
    // Returns false after five failed attempts; out receives (-4000, -4000, -4000).
    virtual bool  getSpawnPosition(Vec3& out, bool checkAI); // +0x1A4 the spawn
    // Faces back toward the marker after displacement; otherwise uses the marker's orientation.
    virtual const Vec3& getSpawnRotation();          // +0x1A8
                                                     // return, read the body before calling
    virtual void  setOverrideEnterOnSpawn(bool);     // +0x1AC
    virtual bool  getOverrideEnterOnSpawn();         // +0x1B0
    virtual void  setProneOnSpawn(bool);             // +0x1B4
    virtual bool  getProneOnSpawn();                 // +0x1B8 ORs the +0x156 byte in
    virtual bool  getOnlyForAI() const;              // +0x1BC forwards to the template
    virtual bool  getOnlyForHuman() const;           // +0x1C0 likewise

    // Tests a candidate relative to base; candidate may be adjusted even on failure.
    bool findValidSpawnPos(const Vec3& base, Vec3& candidate);

private:
    // Layout +0x12C..+0x157, every ctor default from the 192-byte read.
    // (+0x12C: the ISpawnPoint sub-object's vptr - a real base now, see the class comment)
    /* +0x130 */ int   m_refCount;                   // ctor 1
    /* +0x134 */ const SpawnPointTemplate* m_spawnTemplate;   // the ctor argument, typed -
                                                     // Object's own +0x14 holds it untyped
    /* +0x138 */ float m_spawnPreventionDelay;       // = template +0x44
    /* +0x13C */ int   m_spawnGroupId;               // ctor 0
    /* +0x140 */ float m_lastSpawnTime;              // [CHOSEN] name; ctor = minus the prevention
                                                     // delay, so isOccupied() is false at t=0
    /* +0x144 */ bool  m_active;                     // = template->m_active (true by default)
    /* +0x145 */ char  _pad145[3];
    // Last position returned by this spawn point.
    /* +0x148 */ Vec3  m_lastSpawnPosition;          // [CHOSEN name]
    /* +0x154 */ bool  m_overrideEnterOnSpawn;       // ctor false
    /* +0x155 */ bool  m_proneOnSpawn;               // ctor false
    /* +0x156 */ bool  m_bool156;                    // [UNRESOLVED] - getProneOnSpawn ORs it in
    /* +0x157 */ char  _pad157[1];
};


// Queries ISpawnPoint on a non-null object and returns the result without validation.
ISpawnPoint* getSpawnPoint(world::IObject* object);


// Vertical offset placing the soldier's feet at the spawn marker.
float getSoldierYPosOffset();

} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_SPAWNPOINT_H
