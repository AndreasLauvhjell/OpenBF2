// Player-controlled parachute with delayed disable and soldier-detachment camera tracking.
// It is also a firearm projectile template; precaching creates a temporary object without allocating network ids.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_PARACHUTE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_PARACHUTE_H

#include "PlayerControlObject.h"

namespace dice {
namespace hfe {
class SimpleObjectNetworkable;
namespace world {

class ParachuteTemplate;

class Parachute : public PlayerControlObject
{
public:
    // (C2) / the C1 twin: PlayerControlObject base, revector, m_refCount = 1,
    // m_freeCameraWeak = 0, then reset() - which seeds the timer/message trio.
    explicit Parachute(const ParachuteTemplate* objectTemplate);

    // own quartet, refcount +0x3BC
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                         // delete on the 1->0 edge
    virtual void* queryInterface(unsigned int) const;// PCO's four-interface table verbatim
    virtual ~Parachute();                            // (D1) - drops m_freeCameraWeak

    virtual void init();                             // postMessage(32, 1/3s) + PCO::init +
                                                     // the parachute-debug physics-off arm
    virtual void disableCachedObject();              // reset() + chain
    virtual void enableCachedObject();               // postMessage(32, 1/3s) + chain
    virtual void destroy(bool force);                // a pure chain, but an OWN slot in the
                                                     // ELF table; preserved as one
    virtual void handleUpdate(float dt, unsigned int flags);   // the disable countdown

    // player interaction
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
    virtual bool enter(IPlayer*);
    virtual void exit();
    virtual void exitPlayer();

    virtual bool allowEnterOtherVehicle();           // true (the IPlayerControlObject
                                                     // default is false; a parachutist may grab
                                                     // a vehicle mid-air)
    // replaces AnimatedBundle::setTriggerMessage's animation-system forward - the override does
    // NOT chain. 29 B: previous <- current, current <- message.
    virtual void setTriggerMessage(int message);

private:
    // The replicator snapshots the current trigger directly (ELF +0x3C4).
    friend class ::dice::hfe::SimpleObjectNetworkable;
    // Non-virtual helpers, both TU-local in retail:
    void reset();      // timer -1.0f, both messages 0, drop + null the weak block
    void disable();    // server-only: kick the occupant, updateFlags(0, 0x102),
                       // postMessage(33, 0.1s). Reached from handleUpdate's timer expiry.
    void updateAnimationState();   // altitude/timer trigger selection + child propagation
    void detachSoldier();          // unparent, re-enable physics, ragdoll, camera trace

    /* 0x3BC */ int   m_refCount;
    /* 0x3C0 */ float m_disableTimer;
    /* 0x3C4 */ int   m_triggerMessage;
    /* 0x3C8 */ int   m_prevTriggerMessage;
    /* 0x3CC */ int*  m_freeCameraWeak;               // raw WeakPtr<IObject>::Ctrl (+0 count/+4 target)
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_PARACHUTE_H
