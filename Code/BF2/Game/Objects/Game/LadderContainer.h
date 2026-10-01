// Player-controlled ladder traversal, entered like a vehicle. Owns climb state and a bounded force accumulator.
// Attaches no networkable of its own. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_LADDERCONTAINER_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_LADDERCONTAINER_H

#include "PlayerControlObject.h"
#include "Ladder.h"                         // LadderExitType and the live Ladder pointer

namespace dice {
namespace hfe {
namespace world {

class LadderContainerTemplate;

class LadderContainer : public PlayerControlObject
{
public:
    explicit LadderContainer(const LadderContainerTemplate* objectTemplate);  // read
    virtual ~LadderContainer();                      // vptr restores only

    // own quartet, refcount +0x3BC
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// the base's five-case table verbatim
                                                     // (read; byte-identical dispatch)

    // family-slot overrides
    virtual void handleVisualUpdate(float, float);   // s42
    virtual void handleUpdate(float, unsigned int);  // s44 the climb simulation
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
                                                     // s45 empty (returns false)
    virtual io::INetworkable* getNetworkable() const;// s54 forwards to the occupying
                                                     // soldier's networkable
    virtual void handlePlayerInput(IPlayer*, const io::PlayerInput&, float, unsigned int);
                                                     // s64
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);   // s65
    virtual void initNetworkableComp();              // s94 empty, as shipped

    // the controllable surface (overriding the base's vehicle slots)
    virtual bool enter(IPlayer*);                    // returns false (mount uses _enter)
    virtual void exit();                             // detach from the ladder
    virtual bool getExitPosition(Vec3&);             // (bool, per the ipco slot)
    virtual bool isAllowedToExit();                  // false, as shipped
    virtual int  getTeam();                          // the occupying soldier's team
    virtual Vec3* getExitRotation();                 // &the +0x3E8 rotation block

    // LadderContainer's five NEW virtuals, in the exact retail order (ELF slots 181..185) --
    virtual bool _enter(IPlayer*);                    // the mount sequence
    virtual void _exit();
    virtual void addForce(float force);               // the +/-30 clamp
    virtual bool applyCameraAnimation(const Mat4&, Mat4&);   // ipco camera hook
    virtual bool allowEnterOtherVehicle();            // true

    // non-virtual
    static Vec3 _getExitPosition(Ladder*, const Vec3&, LadderExitType); // , by-value return
    void updateClimbAnimation(float);
    IObject* getCamera();                            // walk children, test ICameraObject
    bool isGettingOff();                             // ; reads ds:0x8AAD2BC + ladder top
    bool canSoldierExit();

private:
    friend class Ladder;
    friend class PlayerControlObject;                // init's class-id-gated animation arm
    // layout +0x3BC..+0x3FB, from the 225-byte constructor
    /* 0x3BC */ int   m_refCount2;                   // ctor 1 (the class's own quartet counter)
    /* 0x3C0 */ Ladder* m_ladder;                    // ctor 0 - the base's init reads this field
                                                     // through a classID-gated downcast (the
                                                     // animation-system arm), exactly as shipped
    /* 0x3C4 */ float m_action;                      // PIAction when mapped, else 0
    /* 0x3C8 */ float m_throttle;                    // PIThrottle, clamped to [-1,1]
    /* 0x3CC */ int   _dword3CC;                     // ctor 0 [UNRESOLVED]
    /* 0x3D0 */ int   _dword3D0;                     // ctor 0 [UNRESOLVED]
    /* 0x3D4 */ int   _dword3D4;                     // ctor 0 [UNRESOLVED]
    /* 0x3D8 */ LadderExitType m_exitType;           // current requested dismount side
    /* 0x3DC */ int   _dword3DC;                     // ctor 0 [UNRESOLVED]
    /* 0x3E0 */ int   _dword3E0;                     // ctor 0 [UNRESOLVED]
    /* 0x3E4 */ int   _dword3E4;                     // ctor 0 [UNRESOLVED]
    /* 0x3E8 */ Vec3  m_exitRotation;                // getExitRotation returns this address
    /* 0x3F4 */ float m_force;                       // ctor 0 - addForce clamps to [-30, +30]
    /* 0x3F8 */ bool  m_skidding;
    /* 0x3F9 */ bool  m_climbingUp;
    /* 0x3FA */ bool  m_climbingDown;
    /* 0x3FB */ bool  m_justExited;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_LADDERCONTAINER_H
