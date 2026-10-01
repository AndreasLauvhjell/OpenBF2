// Maintains the physics-node list and dispatches frame updates. Header path [INFERRED].
// Construction starts with one reference; ClassFactory must not add another.
// Preserve the interface destructor before the node-list methods. ELF x86 instances are 16 bytes;
// the rebuilt STL may change container size without changing the public interface.

#ifndef PHYSICS_PHYSICSNODEMANAGER_H     // [INFERRED] guard style
#define PHYSICS_PHYSICSNODEMANAGER_H

#include <list>
#include <string>

#include "../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

// Physics-node interface; this header stores and passes pointers without requiring its definition.
class IPhysicsNode;

// IPhysicsNodeManager - the abstract interface, RTTI _ZTIN4dice3hfe5world19IPhysicsNodeManagerE
// (__si_class_type_info, single base IBase). It carries no state: its constructor (0x835BF50, 14
// bytes) stores nothing but the vptr, which is why PhysicsNodeManager's own m_refCount can sit at
// +0x04.
class IPhysicsNodeManager : public IBase
{
public:
    // +0x10 / +0x14. must be declared before the members below - it is what places them.
    virtual ~IPhysicsNodeManager() {}

    virtual std::list<IPhysicsNode*>& getPhysicsNodeList()          = 0;   // +0x18
    virtual void addPhysicsNode(IPhysicsNode* node)                 = 0;   // +0x1C
    virtual void removePhysicsNode(IPhysicsNode* node)              = 0;   // +0x20

    // +0x24. The signature is from the mangled name, not from the decompiler:
    // _ZN4dice3hfe5world18PhysicsNodeManager6updateEfiPNS1_12IPhysicsNodeE = (float, int,
    // IPhysicsNode*). IDA types the first argument IObject* because it is forwarded verbatim into
    // two calls that do take objects; the disassembly at 0x83619FC reads it from [ebp+0Ch] and
    // never touches it as a pointer. The `int` at [ebp+10h] is read by nothing in the body.
    virtual void update(float deltaTime, int unused, IPhysicsNode* node) = 0;   // +0x24

    virtual void postFrameUpdate()                                  = 0;   // +0x28

    // +0x2C. returns an empty std::string by value, and that is read, not guessed: the body is a
    // refcount bump on _ZNSs20_S_empty_rep_storageE+8 followed by storing +12 (its data pointer)
    // through the hidden sret slot, and it ends `retn 4`. That is GCC 3.2's default-constructed
    // std::string and nothing else. world::BasicPhysicsSystem::listProperties (0x835C400) forwards
    // its OWN sret buffer straight into this slot, which independently fixes the return type.
    virtual std::string listProperties()                            = 0;   // +0x2C

    virtual int getUpdateCount()                                    = 0;   // +0x30
};

// PhysicsNodeManager - the one implementation. Registered as
// "dice.hfe.world.PhysicsNodeManager.Basic", CID 50241, by world::PhysicsModule with flags 1, and
// its instance lands in `world::physicsNodeManager` (ds:0x8B27844, Core/Globals.cpp).
class PhysicsNodeManager : public IPhysicsNodeManager
{
public:
    PhysicsNodeManager();
    virtual ~PhysicsNodeManager();                                      // +0x10 / +0x14

    virtual void  addRef();                                             // +0x00
    virtual int   getRef();                                             // +0x04
    virtual int   release();                                            // +0x08
    virtual void* queryInterface(unsigned int iid) const;               // +0x0C

    virtual std::list<IPhysicsNode*>& getPhysicsNodeList();             // +0x18
    virtual void addPhysicsNode(IPhysicsNode* node);                    // +0x1C
    virtual void removePhysicsNode(IPhysicsNode* node);                 // +0x20
    virtual void update(float deltaTime, int unused, IPhysicsNode* node); // +0x24
    virtual void postFrameUpdate();                                     // +0x28
    virtual std::string listProperties();                               // +0x2C
    virtual int  getUpdateCount();                                      // +0x30

private:
    int                      m_refCount;                                // +0x04
    std::list<IPhysicsNode*> m_physicsNodeList;                         // +0x08
    int                      m_updateCount;                             // +0x0C
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_PHYSICSNODEMANAGER_H
