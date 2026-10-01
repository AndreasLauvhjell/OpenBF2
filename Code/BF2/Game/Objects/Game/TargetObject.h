// Commander target with server-controlled activation, lifetime and empty-vehicle handling.
// Initialization queues a heartbeat; destruction leaves queued messages for the normal messaging lifecycle.

#ifndef GAME_OBJECTS_GAME_TARGETOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_TARGETOBJECT_H

#include "../Base/Bundle.h"
#include "../TargetObjectTemplate.h"

namespace dice {
namespace hfe {
class TargetObjectNetworkable;
namespace world {

// Bundle exposing target activation state. Lifetime timing is initialized when the target is activated.
class TargetObject : public Bundle
{
public:
    explicit TargetObject(const TargetObjectTemplate* objectTemplate);  // read in full;
                                                     // NO server/client fork (both siblings
                                                     // fork; this ctor is the same on both ends)
    virtual ~TargetObject();                         // (D1) / 51 B (D0) - vptr restore only

    // vtable overrides
    virtual void init();                             // slot 8  - (heartbeat at 0.0)
    virtual void destroy(bool force);                // slot 13 - (deregister only)
    virtual void handleUpdate(float, unsigned int);  // slot 44 - : the time-to-live
                                                     // expiry check (args unused, NO base call)
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
                                                     // slot 65 - (29 -> id init;
                                                     // always -> updateActiveStatus)
    virtual void initNetworkableComp();              // slot 94 -

    // non-virtual, written
    bool initTargetId();                             // networkable's u16 id, else the
                                                     // gameLogic +0x0C fallback allocator
    bool getTargetActive();                          // m_targetActive && root not hidden
    int  getTargetType() const;                      // template targetType (+0x17C)
    int  getTargetObjectId();
    // [REBUILD CONVENIENCE] The shipped damage-assist path reads this field directly at +0x14C;
    // keep that access typed without claiming an additional retail virtual/member symbol.
    int  getTargetPlayerId() const;
    void setTargetPlayerId(int playerId);

    // The activation state machine is recovered. Only its client-side nametag integration remains
    // declared and unwritten; sizes are the ELF's.
    void updateActiveStatus(TemplateMessage, IPlayer*);
    void setTargetActive(bool active, bool notify);       // ; handleUpdate's
                                                          // expiry calls (false, true)
    void addToNametagManager();                           // [FRONTIER]
    void removeFromNametagManager();                      // [FRONTIER]

private:
    // TargetObjectNetworkable samples the raw authoritative latch and low byte of
    // m_targetPlayerId.
    // Their PE offsets are +0x15D/+0x164 while ELF uses +0x145/+0x14C, so keep this typed.
    friend class ::dice::hfe::TargetObjectNetworkable;

    // layout +0x13C..+0x15B, every write read from the 133-byte constructor's raw disassembly.
    /* 0x13C */ const TargetObjectTemplate* m_template;  // the shadow, as in the siblings
    /* 0x140 */ int   m_targetObjectId;              // ctor -1; initTargetId fills: the
                                                     // networkable's u16 id, else counter-10000
    /* 0x144 */ bool  m_registeredWithGameLogic;     // [CHOSEN] ctor false; destroy's gate for
                                                     // removeTargetObject; set by the recovered
                                                     // updateActiveStatus machine
    /* 0x145 */ bool  m_targetActive;                // [CHOSEN] ctor false; handleUpdate /
                                                     // getTargetActive gate
    /* 0x146 */ bool  m_notifyOnDeactivate;          // [CHOSEN] ctor false; setTargetActive's second argument (message 30 sets it)
    /* 0x147 */ char  _pad147;
    /* 0x148 */ unsigned int m_activatedTick;        // [CHOSEN] ctor 0; handleUpdate's deadline
                                                     // base: + template timeToLiveTicks,
                                                     // compared unsigned against getTickCount
    /* 0x14C */ int   m_targetPlayerId;              // ctor -1; target weapon enable stamps the
                                                     // owning designator and damage awards assist
    /* 0x150 */ int   _dword150;                     // [UNRESOLVED] - NOT BY THE ctor
    /* 0x154 */ int   m_emptySinceTick;              // [CHOSEN] ctor 0 - the empty-vehicle countdown's start tick
    /* 0x158 */ int   m_occupantTeam;                // [CHOSEN] ctor -1 - the last occupant's team (IPlayer +0xE8)
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_TARGETOBJECT_H
