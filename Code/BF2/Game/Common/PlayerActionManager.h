// Player input transport (stream-manager ID 0); recent actions are resent without acknowledgments.
// [INFERRED] Header beside attested Game/Common/PlayerActionManager.cpp.
// Shipped defect: the received 4-bit action count is unchecked against a six-element array.
// Keep that defect visible when auditing packet input.

#ifndef GAME_COMMON_PLAYERACTIONMANAGER_H     // [INFERRED] guard style
#define GAME_COMMON_PLAYERACTIONMANAGER_H

#include "IStreamManager.h"
#include "PlayerAction.h"

namespace dice {
namespace hfe {

namespace world { class IPlayer; }

class PlayerActionManager : public IStreamManager
{
public:
    PlayerActionManager();
    virtual ~PlayerActionManager();

    // IBase
    virtual void  addRef();                                      // +0x00
    virtual int   getRef();                                      // +0x04
    virtual int   release();                                     // +0x08
    virtual void* queryInterface(unsigned int iid) const;        // +0x0C

    // IStreamManager
    virtual int   getStreamManagerId() const;                    // +0x10
    virtual void  setStreamManagerId(int id);                    // +0x14
    virtual bool  processReceivedPacket(io::BitStream* stream);  // +0x18
    virtual void  handlePacketStatus(PacketStatus& status, bool& delivered);
                                                                 // +0x1C    (empty)
    // return type see IStreamManager.h. A caller does
    // `transmitResult >> 3` on it; this is the BIT count written.
    virtual unsigned int transmit(io::BitStream* stream, PacketStatus& status,
                                  unsigned int budget);
    virtual void  setConnectionId(int connectionId);             // +0x24
    virtual int   getConnectionId();                             // +0x28
    virtual int   getError();                                    // +0x2C

    // NON-virtual
    void setPlayer(world::IPlayer* player);
    bool add(const PlayerAction& action);
    bool processRcvdPlayerActions(int count);

public:
    // The +4/+8 read/write counters are inherited from IStreamManager and remain uninitialized
    // by the constructor, matching ELF 0x809D1A0 and PE 0x6173C0.
    /* +0x00C */ int             m_refCount;
    /* +0x010 */ int             m_connectionId;
    /* +0x014 */ int             m_error;
    /* +0x018 */ PlayerAction    m_sendActions[3];
    /* +0x06C */ int             m_sendActionCount;
    /* +0x070 */ int             m_dword70;              // [UNRESOLVED role]
    /* +0x074 */ int             m_streamManagerId;      // = -1, write-once
    /* +0x078 */ int             m_recvActionHighWater;  // = -1
    /* +0x07C */ bool            m_byte7C;               // [UNRESOLVED role]
    /* +0x080 */ int             m_playerInterpolationTime;
    /* +0x084 */ world::IPlayer* m_player;
    /* +0x088 */ PlayerAction    m_recvActions[6];       // SIX. The wire says fifteen.
};

#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] Only the inherited vptr and native player pointer widen.
// PlayerAction's scalar records and their field-by-field codec retain the x86 widths.
static_assert(offsetof(PlayerActionManager, m_refCount) == 16, "native x64 PlayerActionManager prefix");
static_assert(offsetof(PlayerActionManager, m_connectionId) == 20, "native x64 PlayerActionManager connection ID");
static_assert(offsetof(PlayerActionManager, m_error) == 24, "native x64 PlayerActionManager error");
static_assert(offsetof(PlayerActionManager, m_player) == 136, "native x64 player pointer alignment");
static_assert(sizeof(PlayerActionManager) == 0x138, "native x64 PlayerActionManager layout");
#else
static_assert(offsetof(PlayerActionManager, m_refCount) == 12, "retail PlayerActionManager prefix");
static_assert(offsetof(PlayerActionManager, m_connectionId) == 16, "retail PlayerActionManager connection ID");
static_assert(offsetof(PlayerActionManager, m_error) == 20, "retail PlayerActionManager error");
static_assert(sizeof(PlayerActionManager) == 0x130, "x86 PlayerActionManager has no STL members");
#endif
static_assert(sizeof(PlayerAction) == 28, "player-action record width remains fixed on both ports");

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_PLAYERACTIONMANAGER_H
