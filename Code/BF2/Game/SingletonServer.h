// Nametag and touch-item interface shared by the null and renderer implementations.
// Overload order is ABI-sensitive; do not add a destructor without checking both ports.
// [FRONTIER] [RETIRE-WHEN: nametag-interface-tail] The null implementation's tail differs
// from the real renderer's table. Do not expose or call those extra slots until a
// compatible interface is established for both implementations.

#ifndef GAME_SINGLETONSERVER_H     // [INFERRED] guard style
#define GAME_SINGLETONSERVER_H

#include <string>

#include "../Core/IBase.h"
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {

struct SquadOrder;                      // canonical layout lives in GameLogic/SquadManager/Squad.h
namespace world {
class IPlayer;
class IObject;
class ControlPoint;
}

class INametagManager : public IBase
{
public:
    virtual void  init(const std::string&, const std::string&) = 0;            // +0x10
    virtual void  update() = 0;                                                // +0x14
    virtual void  draw() = 0;                                                  // +0x18
    virtual bool  addTouchItem(world::IPlayer*) = 0;                           // ELF +0x1C / PE +0x30
    virtual void  addTouchItem(world::ControlPoint*) = 0;                      // +0x20 - the slot
                                                                               //   ControlPoint::
                                                                               //   init/setFlag hit
    virtual void  addTouchItem(world::IObject*) = 0;                           // +0x24
    virtual void  addTouchItem(SquadOrder*, int, const Vec3&) = 0;             // +0x28
    virtual void  addTouchItem(SquadOrder*, int, world::IObject*) = 0;         // +0x2C
    virtual void  addTouchItem(world::IObject*, int) = 0;                      // +0x30
    virtual void* getColors(int) = 0;                                          // +0x34 [UNRESOLVED
    virtual void* getTransformations(int) = 0;                                 // +0x38  return
    virtual void* getAlphaArray(int) = 0;                                      // +0x3C  types]
    virtual void  rebufferAll() = 0;                                           // +0x40
    virtual void  enableBuffering() = 0;                                       // +0x44
    virtual void  clearAll() = 0;                                              // +0x48
    virtual void  removeItem(int type, void* item) = 0;                        // +0x4C - destroy's
                                                                               //   call, type 2 =
                                                                               //   control point
    virtual void  removeItem(int type, void* item, int) = 0;                   // +0x50
    virtual const void* getItems() const = 0;                                  // +0x54 [UNRESOLVED
                                                                               //   concrete type]
    virtual bool  get3dMapOn() const = 0;                                      // +0x58
    virtual void  set3dMapOn(bool) = 0;                                        // +0x5C
    virtual void  setSpectatedPlayer(world::IPlayer*) = 0;                     // +0x60
    virtual world::IPlayer* getSpectatedPlayer() const = 0;                    // +0x64
    virtual bool  isDemoPlayback() const = 0;                                  // +0x68
};

// Publishes shared server singletons, including the nametag manager used by control points.
class SingletonServer
{
public:
    static void init();                          // BF2Engine calls it at boot

    // ds:0x8AB0980. getSingleton("NametagManager") if a module offers one; the 16-byte
    // NullNametagManager otherwise - which on the dedicated server is always.
    static INametagManager* m_iNametagManager;
};

} // namespace hfe
} // namespace dice

#endif // GAME_SINGLETONSERVER_H
