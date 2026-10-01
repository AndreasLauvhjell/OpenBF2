// Server-controlled object spawner with per-team templates, slot tracking and respawn countdowns.
// Registers with GameLogic during init and unregisters during destruction. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_OBJECTSPAWNER_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_OBJECTSPAWNER_H

#include <map>
#include <vector>

#include "../Base/SimpleObject.h"
#include "../IObjectSpawner.h"

namespace dice {
namespace hfe {
namespace world {

class ObjectSpawnerTemplate;
class ICollisionMesh;

// Finds the first engine template depth-first through children and siblings; leaves out unchanged on miss.
void getEngineFromChildsAndSiblings(IObject* object, IObject*& out);

// Exposes team and spawner control through one secondary interface.
// Reset, spawning and placement helpers are nonvirtual.
class ObjectSpawner : public SimpleObject, public IObjectSpawner
{
public:
    explicit ObjectSpawner(const ObjectSpawnerTemplate* objectTemplate);   // (C2) / 831 B (C1) - read in full
    virtual ~ObjectSpawner();                        // (D1) / 288 B (D0)

    // the quartet, refcount at +0x13C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// the five-case table above

    virtual void  init();                            // SimpleObject::init, createdOnLoad, gameLogic->addObjectSpawner
    virtual void  destroy(bool force);               // (one banked arm - see the .cpp)
    virtual void  handleUpdate(float dt, unsigned int);   // the spawn state machine
    virtual bool  makeScript(io::IStream*);

    // IObjectSpawner (slots 96..109 of the primary, 4..17 of the +0x138 table)
    virtual void setTeam(int team);                  // + the game-status-3 arm
    virtual int  getTeam();
    virtual void setActive(bool active);
    virtual bool getActive();
    virtual void setControledById(int id);           // (compare-then-store)
    virtual int  getControledById();
    virtual void setTeamOnStart(unsigned int team);  // writes teamOnStart AND team
    virtual int  getTeamOnStart();
    virtual void setControlPointId(int id);          // = the virtual setControledById
    virtual int  getControlPointId();                // the same field as getControledById
    virtual void setVisibleTeam(int team);
    virtual int  getVisibleTeam();
    virtual void setSpawnOffset(Vec3 offset);
    virtual Vec3 getSpawnOffset();

    // non-virtual
    void  reset();                                   // back to the template's start state
    void  calcSpawnDelay();                          // (the ratio compiled out - see .cpp)
    ICollisionMesh* getCollisionMesh(int unusedIndex);   // per-team lazy cache
    bool  getHasUseButton();                         // template useButtonRadius > 0
    float getUseButtonRadius();
    int   getLastSpawned() const;
    void  dropSpawnedObject();                       // clears the hold flag
    int   getHoldObjectId();                         // lastSpawned if held, else -1
    int   spawnObject();                             // creates the object; returns its id or -1
    bool  getSnappedPosition(Vec3& position);        // (returns whether it snapped - movzx al)
    void  snapSpawnedObject(IObject* object);
    void  moveIntersectingSoldiers(IObject* object);
    void  playEffectOnObjectSpawner();

protected:
    // layout +0x13C..+0x1BB - every ctor value from the 1,013-byte constructor, read at the
    // instruction level (the ELF puts the IObjectSpawner vptr at +0x138; MSVC places it where it
    // likes - internal).
    /* 0x13C */ int   m_refCount;                    // ctor 1

    // Per-team cache of the collision mesh of the object this spawner issues (getCollisionMesh
    // indexes it by m_team, not by its argument): { mesh, "still worth trying" }. ctor {0, true}
    // x3; the dtor deletes the meshes (ICollisionMesh slot 5 - the deleting dtor).
    struct CollisionMeshSlot
    {
        ICollisionMesh* mesh;                        // +0
        bool            valid;                       // +4
    };
    /* 0x140 */ CollisionMeshSlot m_collisionMeshes[3];   // +0x140 +0x148 / +0x150

    /* 0x158 */ const ObjectSpawnerTemplate* m_template;   // ctor = the argument
    /* 0x15C */ int   m_controlPointId;              // ctor = template +0x1BC (its -1 default);
                                                     // set/getControledById AND get/setControlPointId
                                                     // all address this field (dice kept both names)
    /* 0x160 */ std::vector<int> m_spawnedObjectIds; // ctor empty; server: maxNrOfObjectSpawned x -1
    /* 0x16C */ int   m_lastSpawnedId;               // ctor -1
    /* 0x170 */ float m_spawnDelay;                  // [CHOSEN] the countdown; -1.0f = nothing pending.
                                                     // NOT written by the ctor unless the server
                                                     // arm runs (then calcSpawnDelay or -1.0f)
    /* 0x174 */ std::vector<float> m_timeToLive;     // ctor empty; server: n x template timeToLive
    /* 0x180 */ bool  m_active;                      // ctor = template activeOnStart
    /* 0x181 */ char  _pad181[3];
    /* 0x184 */ int   m_team;                        // ctor = template team - selects the object slot
    /* 0x188 */ int   m_teamOnStart;                 // ctor = template team
    /* 0x18C */ std::map<unsigned int, const IObjectTemplate*> m_spawnedTemplates;   // [CHOSEN]
                                                     // ctor empty (the SGI header at +0x18C, size
                                                     // at +0x190); spawnObject's territory
    /* 0x198 */ int   m_nrOfObjectToSpawn;           // ctor = template nrOfObjectToSpawn
    /* 0x19C */ bool  m_useButtonPressed;            // [CHOSEN] ctor false - handleUpdate's gate when
                                                     // the template has a use-button radius
    /* 0x19D */ char  _pad19D[3];
    /* 0x1A0 */ Vec3  m_spawnOffset;                 // ctor = template spawnOffset
    /* 0x1AC */ const IObjectTemplate* m_effectOnSpawnTemplate;   // server only: getTemplate(template
                                                     // effectOnSpawn). left uninitialised on a
                                                     // client, as shipped
    /* 0x1B0 */ bool  m_holdObject;                  // ctor = template holdObject; dropSpawnedObject clears
    /* 0x1B1 */ bool  _bool1B1;                      // [UNRESOLVED] ctor false
    /* 0x1B2 */ char  _pad1B2[2];
    /* 0x1B4 */ int   m_visibleTeam;                 // ctor 1
    /* 0x1B8 */ int   m_blockedSpawnCount;           // [CHOSEN name] ctor 0 - spawnObject's blocked-tick
                                                     // After five blocked ticks, destroy the blockers.
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_OBJECTSPAWNER_H
