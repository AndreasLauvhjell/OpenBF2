// Client game-logic implementation, including local input, collision and menu requests.
// Server-owned actions retain their empty or default client behavior. The class is
// available to both images, but the engine's client branch creates it.
#ifndef GAME_GAMELOGIC_CLIENTGAMELOGIC_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_CLIENTGAMELOGIC_H

#include <list>
#include <string>

#include "GameLogic.h"
#include "TicketHistory.h"                 // m_ticketHistory (+0x208) - the vector's real element type
#include "../../Core/IBase.h"              // IBase - create()'s return and its aggregation argument
#include "../../Core/WeakPtr.h"            // the 12-parameter giveDamage's list

namespace dice {
namespace hfe {

class ClientGameLogic : public GameLogic
{
public:
    ClientGameLogic();
    virtual ~ClientGameLogic();

    // the IBase quartet, all four this class's own (as on ServerGameLogic)
    virtual void  addRef();                           // +0x00
    virtual int   getRef();                           // +0x04
    virtual int   release();                          // +0x08
    virtual void* queryInterface(unsigned int) const; // +0x0C

    static IBase* create(unsigned int iid, IBase* outer);   // the ClassManager factory

    // lifecycle
    virtual bool  init();                             // +0x10  : GameLogic::init(), true
    virtual bool  close();                            // +0x1C  / PE 0x45C340: false
    virtual void  update(float deltaTime);            // +0x20
    virtual void  reset();                            // +0x24   - re-entrancy guarded
    virtual void  restartDemoPlayback();              // +0x28
    virtual void  runNextLevel();                     // +0x2C  : empty
    virtual void  setNextLevel(int);                  // +0x30  : empty

    // the player requests - PostRemoteEvents to the server
    virtual void  suicide();                                                // +0x68
    virtual bool  killPlayer(world::IPlayer*, bool, bool);                  // +0x6C
    virtual bool  reSpawnPlayer(world::IPlayer*);                           // +0x70  : false
    virtual bool  selectSpawnGroup(world::IPlayer*, int);                   // +0x74
    virtual bool  selectTeam(world::IPlayer*, int);                         // +0x78
    virtual bool  selectKitAndUnlockLevel(world::IPlayer*, int, int);       // +0x7C

    // the ticket surface - inert on the client
    virtual void  setTicketState(unsigned int team, int);                   // +0x8C
    virtual int   getTicketState(unsigned int team);                        // +0x90  : 0
    virtual void  setDefaultNumberOfTickets(unsigned int, unsigned int);    // +0x94
    virtual unsigned int getDefaultNumberOfTickets(unsigned int);           // +0x98  : 0
    virtual void  setDefaultNumberOfTicketsEx(unsigned int, unsigned int,
                                              unsigned int);                // +0x9C
    virtual unsigned int getDefaultNumberOfTicketsEx(unsigned int,
                                                     unsigned int);         // +0xA0  : 0
    virtual void  setDefaultTimeToNextAIWave(float);                        // +0xA4
    virtual float getDefaultTimeToNextAIWave();                             // +0xA8  : 0.0
    virtual void  setTicketLossAtEndPerMin(float);                          // +0xAC
    virtual float getTicketLossAtEndPerMin();                               // +0xB0  : 0.0
    virtual void  setTicketLossPerMin(unsigned int team, float);            // +0xB4
    virtual float getTicketLossPerMin(unsigned int team);                   // +0xB8  : 0.0

    // effects, damage, pickups, collisions
    virtual void  playEffect(Vec3, const world::IObjectTemplate*, Vec3);    // +0xE0  : empty
    virtual bool  giveDamage(world::IObject*, float, world::IPlayer*,
                             std::list<WeakPtr<world::IPlayer> >*,
                             world::IObject*, int, world::IObject*,
                             world::IObject*, Vec3, Vec3, int,
                             bool, bool, bool);                             // +0xE4  : false
    virtual void  handleExplosion(Vec3, float, int, float, world::IPlayer*,
                                  world::IArmorGLComp*, int, world::IObject*,
                                  world::IObject*, float, world::IObject*,
                                  world::IObject*, float, bool, bool);      // +0xE8
    virtual void  handlePickup(world::IObject*, world::IPlayer*,
                               world::IObject*, bool);                      // +0xEC
    virtual void  handleDrop(world::IObject*, world::IPlayer*,
                             world::IObject*, bool);                        // +0xF0
    virtual bool  handleCollisionForProjectile(world::IObject*, world::IObject*,
                             const Vec3&, const Vec3&, const Vec3&, int, int,
                             world::CollisionFaceInfo*,
                             world::CollisionFaceInfo*);                    // +0xF4
    virtual bool  handleCollisionForReplenisher(bool, world::IObject*,
                             world::IObject*, const Vec3&, const Vec3&,
                             const Vec3&, int, int);                        // +0xF8
    virtual bool  handleCollisionForResurrecter(world::IObject*, world::IObject*,
                             float, float, const Vec3&, const Vec3&,
                             const Vec3&, int, int);                        // +0xFC
    virtual void  handleCollision(world::IObject*, world::IObject*,
                             const Vec3&, const Vec3&, const Vec3&, int, int,
                             world::CollisionFaceInfo*,
                             world::CollisionFaceInfo*);                    // +0x100 : empty
    virtual void  spawnMultipleObjects(const world::IObjectTemplate*, int); // +0x114
    virtual world::IObject* spawnObject(const std::string&, Vec3, Vec3, int);           // +0x118 : 0
    virtual world::IObject* spawnObject(const world::IObjectTemplate*, Vec3, Vec3, int); // +0x11C : 0

    // input, camera, events, tickets
    virtual void  checkPlayerTriggers(world::IPlayer*, world::IObject*,
                                      io::PlayerInput, bool);               // +0x158
    virtual void  toggleLocalFreeCamera();                                  // +0x208
    virtual TicketHistory& getTicketHistory();                              // +0x278
    virtual void  setTicketHistory(const TicketHistory&);                   // +0x27C
    virtual void  handleEvent(EventCategory, unsigned int, EventNode*, void*);   // +0x2D4

    // this class's OWN virtuals - slots 193..195, after IGameLogic's 193
    // The 12-parameter giveDamage. [INFERRED] name from the mangling (`...ibb`); no caller found.
    virtual bool  giveDamage(world::IObject*, float, world::IPlayer*,
                             std::list<WeakPtr<world::IPlayer> >*,
                             int, world::IObject*, world::IObject*,
                             Vec3, Vec3, int, bool, bool);                  // slot 193  0x80C4AA0-like, 7 B: false
    virtual bool  getHasPendingExit();
    virtual void  clearPendingExit();

    // Non-virtual (not in either vtable): 0x80C5C40, 5 B - empty. No caller located.
    void createObject(std::string);

private:
    // Offsets are the ELF object's. +0x1EC..+0x21F are this class's; GameLogic ends at +0x1EC.
    int           m_refCount;                 // +0x1EC = 1
    int           m_dword1F0;                 // +0x1F0 = 0  [UNRESOLVED] - the ctor zeroes it first
    int           m_dword1F4;                 // +0x1F4 = 0  [UNRESOLVED]
    int           m_dword1F8;                 // +0x1F8 = 0  [UNRESOLVED]
    int           m_dword1FC;                 // +0x1FC = 0  [UNRESOLVED]
    int           m_dword200;                 // +0x200 = 0  [UNRESOLVED]
    int           m_dword204;                 // +0x204 = 0  [UNRESOLVED]
    TicketHistory m_ticketHistory;            // +0x208  the vector get/setTicketHistory expose
    int           m_pendingExitTick;          // +0x214  set by checkPlayerTriggers, read by getHasPendingExit
                                              //   (`+ 60 > tick`), cleared by clearPendingExit
    int           m_exitKeyDownTick;          // +0x218  the tick the exit key went down; 0 while up
    int           m_lastExitClearedTick;      // +0x21C  clearPendingExit's tick stamp
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_CLIENTGAMELOGIC_H
