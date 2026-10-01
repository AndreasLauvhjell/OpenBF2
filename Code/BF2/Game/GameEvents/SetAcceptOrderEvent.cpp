#include "SetAcceptOrderEvent.h"

#include "../../Core/IBase.h"
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../GameLogic/SquadManager/ISquadManager.h" // typed SquadManager dispatch

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GESetAcceptOrder (0x888A4A4) = 27.
const int GESetAcceptOrder = 27;

} // namespace

//  (C2) / 0x807A3F0 (C1), 71 B. GameEvent's ctor, the vptr, five byte stores. The two
// enum arguments are narrowed to a byte on the way in - the ELF really does store them with
// byte-sized moves into +0x0F and +0x10.
SetAcceptOrderEvent::SetAcceptOrderEvent(int team, int squadId, bool isAccepted,
                                         OrderType orderType, SquadOrderType squadOrderType)
    : GameEvent()
    , m_team(static_cast<unsigned char>(team))                                    // +0x0C
    , m_squadId(static_cast<unsigned char>(squadId))                              // +0x0D
    , m_isAccepted(isAccepted)                                                    // +0x0E
    , m_orderType(static_cast<unsigned char>(static_cast<int>(orderType)))        // +0x0F
    , m_squadOrderType(static_cast<unsigned char>(static_cast<int>(squadOrderType)))  // +0x10
{
}

int  SetAcceptOrderEvent::getType() const { return GESetAcceptOrder; }
void SetAcceptOrderEvent::logInfo(bool, int) {}

// executeClient - 0x807A670, 93 B. Apply it, suppressing the echo.
void SetAcceptOrderEvent::executeClient()
{
    if (squadManager)
        static_cast<ISquadManager*>(squadManager)->setIsAccepted(
            m_team, m_squadId, m_isAccepted,
            static_cast<OrderType>(m_orderType),
            static_cast<SquadOrderType>(m_squadOrderType), true);                 // +0x24
}

// Allows the squad leader or team commander to answer an order. Both lookups run
// before authorization is tested and compare against the connection id's low byte.
// Returns true even when the manager is absent or the sender is unauthorized.
bool SetAcceptOrderEvent::executeServer(int connectionId)
{
    if (!squadManager)
        return true;

    const int sender = static_cast<unsigned char>(connectionId);   // byte-truncated, see above

    const bool isSquadLeader =
        (static_cast<ISquadManager*>(squadManager)->getSquadLeader(m_team, m_squadId) == sender);
    const bool isCommander =
        (static_cast<ISquadManager*>(squadManager)->getCommander(m_team) == sender);

    if (isSquadLeader || isCommander)
        static_cast<ISquadManager*>(squadManager)->setIsAccepted(
            m_team, m_squadId, m_isAccepted,
            static_cast<OrderType>(m_orderType),
            static_cast<SquadOrderType>(m_squadOrderType), false);                // +0x24

    return true;
}

// deSerialize - 0x807A450, 234 B. Squad first, then team - see the header.
bool SetAcceptOrderEvent::deSerialize(io::BitStream* stream)
{
    int squadId = 0;
    stream->readBits(&squadId, 8);
    m_squadId = static_cast<unsigned char>(squadId);

    int team = 0;
    stream->readBits(&team, 8);
    m_team = static_cast<unsigned char>(team);

    int isAccepted = 0;
    stream->readBits(&isAccepted, 1);
    m_isAccepted = (isAccepted == 1);

    int orderType = 0;
    stream->readBits(&orderType, 8);
    m_orderType = static_cast<unsigned char>(orderType);

    int squadOrderType = 0;
    stream->readBits(&squadOrderType, 8);
    m_squadOrderType = static_cast<unsigned char>(squadOrderType);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x807A540, 296 B. The same order.
bool SetAcceptOrderEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int squadId = m_squadId;
    stream->writeBits(&squadId, 8);

    int team = m_team;
    stream->writeBits(&team, 8);

    int isAccepted = m_isAccepted ? 1 : 0;
    stream->writeBits(&isAccepted, 1);

    int orderType = m_orderType;
    stream->writeBits(&orderType, 8);

    int squadOrderType = m_squadOrderType;
    stream->writeBits(&squadOrderType, 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
