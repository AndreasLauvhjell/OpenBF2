// Response-physics lists and frame/substep coordination.
// [INFERRED] header name; the class is attested to Physics/ResponseManager.cpp
// by removeObject's ELF __FILE__ reference at 0x8370101.
// Response classes select tagged lists; child responses have a separate list.
// Keep startFrame, per-substep stopFrame and once-per-frame postFrameUpdate distinct.

#ifndef PHYSICS_RESPONSEPHYSICSMANAGER_H     // [INFERRED] guard style
#define PHYSICS_RESPONSEPHYSICSMANAGER_H

#include "../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

// Forward declarations suffice for response-manager pointers; implementations include
// the complete interfaces for dispatch.
class IObject;
class IResponsePhysics;

// Intrusive response list with head and tail pointers; helpers accept it by reference.
// Field names and struct spelling are [CHOSEN]. Keep the declaration consistent with IResponsePhysics:
// MSVC distinguishes struct and class names in linkage. Iteration ends at tail->next, or null.
#ifndef DICE_HFE_WORLD_RESPONSEPHYSICSLIST_DEFINED
#define DICE_HFE_WORLD_RESPONSEPHYSICSLIST_DEFINED
struct ResponsePhysicsList
{
    IResponsePhysics* head;     // +0x00  [CHOSEN] name
    IResponsePhysics* tail;     // +0x04  [CHOSEN] name
};
#endif

// Stateless response-manager interface derived from IBase. Preserve virtual method order.
class IResponsePhysicsManager : public IBase
{
public:
    // +0x10 / +0x14. must be declared before the members below - it is what places them.
    virtual ~IResponsePhysicsManager() {}

    virtual void addObject(IObject* obj)                    = 0;   // +0x18
    virtual void removeObject(IObject* obj)                 = 0;   // +0x1C

    // +0x20. A dispatcher: obj non-null steps that one object, obj null steps everything.
    virtual void update(float deltaTime, IObject* obj)      = 0;   // +0x20

    virtual void reset()                                    = 0;   // +0x24
    virtual void resetCachedCollisionObjects()              = 0;   // +0x28

    virtual void startFrame()                               = 0;   // +0x2C
    virtual void stopFrame()                                = 0;   // +0x30
    virtual void postFrameUpdate()                          = 0;   // +0x34
};

// ResponsePhysicsManager - the one implementation. Registered as
// "dice.hfe.world.ResponsePhysicsManager.Basic", CID 50271, by world::PhysicsModule with flags 1;
// the instance lands in `world::responsePhysicsManager` (ds:0x8B27848, Core/Globals.cpp).
class ResponsePhysicsManager : public IResponsePhysicsManager
{
public:
    ResponsePhysicsManager();
    virtual ~ResponsePhysicsManager();                              // +0x10 / +0x14

    virtual void  addRef();                                         // +0x00
    virtual int   getRef();                                         // +0x04
    virtual int   release();                                        // +0x08
    virtual void* queryInterface(unsigned int iid) const;           // +0x0C

    virtual void addObject(IObject* obj);                           // +0x18
    virtual void removeObject(IObject* obj);                        // +0x1C
    virtual void update(float deltaTime, IObject* obj);             // +0x20
    virtual void reset();                                           // +0x24
    virtual void resetCachedCollisionObjects();                     // +0x28
    virtual void startFrame();                                      // +0x2C
    virtual void stopFrame();                                       // +0x30
    virtual void postFrameUpdate();                                 // +0x34

private:
    // Nonvirtual helpers for response-list traversal and the four object-collision paths.
    void setPrevTransformationForResponseList(ResponsePhysicsList& list);
    void updateSingleObject(float deltaTime, IObject* obj);
    void updateAllObjects(float deltaTime);

    void getCollisionObjects(IObject* obj, float a, float b);
    void checkPointObjectVsObjects(float dt, IObject* obj);
    void checkRotationalPointObjectVsObjects(float dt, IObject* obj);
    void checkMeshObjectVsObjects(float dt, IObject* obj);
    void checkSoldierObjectVsObjects(float dt, IObject* obj);

    int                 m_refCount;                             // +0x04
    ResponsePhysicsList m_responsePhysicsList;                   // +0x08   tag 1
    ResponsePhysicsList m_pointResponsePhysicsList;              // +0x10   tag 2
    ResponsePhysicsList m_rotationalPointResponsePhysicsList;    // +0x18   tag 3
    ResponsePhysicsList m_soldierResponsePhysicsList;            // +0x20   tag 4
    ResponsePhysicsList m_childResponsePhysicsList;              // +0x28   tag 5  [CHOSEN] name
    bool                m_cachedCollisionObjectsDirty;           // +0x30   [CHOSEN] name
    char*               m_startFrameMarker;                      // +0x34   [CHOSEN] name
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_RESPONSEPHYSICSMANAGER_H
