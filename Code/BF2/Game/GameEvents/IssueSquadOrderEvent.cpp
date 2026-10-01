#include "IssueSquadOrderEvent.h"

#include "../../Core/IBase.h"
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../GameLogic/SquadManager/ISquadManager.h" // typed order/authority dispatch

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GEIssueSquadOrder (0x8889A64) = 24.
const int GEIssueSquadOrder = 24;

// The three values of OrderType this binary exercises. [CHOSEN names] - the ELF compares against
// bare 0, 1 and 2, and no enumerator name appears anywhere.
const int ORDER_TYPE_SQUAD     = 0;
const int ORDER_TYPE_COMMANDER = 1;
const int ORDER_TYPE_REQUEST   = 2;

inline ISquadManager* squadManagerIface()
{
    return static_cast<ISquadManager*>(squadManager);
}
} // namespace

// The maker's form - 0x805F070's inlined copy: nine zero stores across +0x10..+0x33 and nothing
// else. The seven id/flag fields are left as heap garbage, deliberately.
IssueSquadOrderEvent::IssueSquadOrderEvent()
    : GameEvent()
{
    for (int i = 0; i < 3; ++i)
        m_points[i].x = m_points[i].y = m_points[i].z = 0.0f;
}

// Truncates team and squad IDs to bytes and copies an optional order.
// A null order leaves target, order ID, and flag fields uninitialized; points are zeroed.
IssueSquadOrderEvent::IssueSquadOrderEvent(int teamId, int squadId, SquadOrder* order,
                                           OrderType type)
    : GameEvent()
    , m_teamId(static_cast<unsigned char>(teamId))           // +0x0C
    , m_squadId(static_cast<unsigned char>(squadId))         // +0x0D
    , m_orderType(static_cast<unsigned char>(static_cast<int>(type)))   // +0x36
{
    for (int i = 0; i < 3; ++i)
        m_points[i].x = m_points[i].y = m_points[i].z = 0.0f;

    if (!order)
        return;                                  // silent, and leaves three fields uninitialised

    m_orderId     = static_cast<unsigned char>(order->orderId);      // +0x0E - truncates
    m_targetNetId = order->targetNetId;                              // +0x34
    m_flag37      = order->flag2C;                                   // +0x37
    for (int i = 0; i < 3; ++i)
        m_points[i] = order->points[i];                              // +0x10 + 12i
}

int IssueSquadOrderEvent::getType() const
{
    return GEIssueSquadOrder;
}

namespace {
// Both execute arms rebuild a SquadOrder from the event's fields before dispatching - the same six
// copies in the same order. Written once here; the ELF has it twice, inlined.
SquadOrder rebuildOrder(unsigned char orderId, unsigned short targetNetId,
                        const Vec3* points, bool flag)
{
    SquadOrder order;
    order.orderId     = orderId;
    order.targetNetId = targetNetId;
    order.flag2C      = flag;
    for (int i = 0; i < 3; ++i)
        order.points[i] = points[i];
    return order;
}
} // namespace

// Applies a server-approved order without further permission checks; unknown types are ignored.
void IssueSquadOrderEvent::executeClient()
{
    if (!squadManager)
        return;

    SquadOrder order = rebuildOrder(m_orderId, m_targetNetId, m_points, m_flag37);

    if (m_orderType == ORDER_TYPE_SQUAD)
        squadManagerIface()->issueSquadOrder(m_teamId, m_squadId, order, true);
    else if (m_orderType == ORDER_TYPE_COMMANDER)
        squadManagerIface()->issueCommanderOrder(m_teamId, m_squadId, order, true);
    else if (m_orderType == ORDER_TYPE_REQUEST)
        squadManagerIface()->issueRequest(m_teamId, m_squadId, order, true);
}

// Checks both authority roles before dispatch, comparing the connection ID as a byte.
// Rejected orders are silently ignored and still return true.
bool IssueSquadOrderEvent::executeServer(int connectionId)
{
    if (!squadManager)
        return true;

    const int sender = static_cast<int>(static_cast<unsigned char>(connectionId));   // truncated

    const bool isSquadLeader =
        squadManagerIface()->getSquadLeader(m_teamId, m_squadId) == sender;          // +0x3C
    const bool isCommander =
        squadManagerIface()->getCommander(m_teamId) == sender;                       // +0x70

    SquadOrder order = rebuildOrder(m_orderId, m_targetNetId, m_points, m_flag37);

    if (m_orderType == ORDER_TYPE_SQUAD)
    {
        if (isSquadLeader)
            squadManagerIface()->issueSquadOrder(m_teamId, m_squadId, order, false);
    }
    else if (m_orderType == ORDER_TYPE_COMMANDER)
    {
        if (isCommander)
            squadManagerIface()->issueCommanderOrder(m_teamId, m_squadId, order, false);
    }
    else if (m_orderType == ORDER_TYPE_REQUEST)
    {
        if (isSquadLeader || isCommander)
            squadManagerIface()->issueRequest(m_teamId, m_squadId, order, false);
    }

    return true;
}

// Reads squad before team. Place orders clear the target ID; object orders retain existing points.
bool IssueSquadOrderEvent::deSerialize(io::BitStream* stream)
{
    int squadId = 0;
    stream->readBits(&squadId, 8);               // first
    m_squadId = static_cast<unsigned char>(squadId);

    int teamId = 0;
    stream->readBits(&teamId, 8);
    m_teamId = static_cast<unsigned char>(teamId);

    int orderId = 0;
    stream->readBits(&orderId, 8);
    m_orderId = static_cast<unsigned char>(orderId);

    int hasTarget = 0;
    stream->readBits(&hasTarget, 1);
    if (hasTarget == 1)
    {
        int targetNetId = 0;
        stream->readBits(&targetNetId, 16);
        m_targetNetId = static_cast<unsigned short>(targetNetId);
    }
    else
    {
        m_targetNetId = 0;
        // component-major - every x, then every y, then every z. See the header block.
        for (int c = 0; c < 3; ++c)
            for (int p = 0; p < 3; ++p)
                stream->readBits(&(&m_points[p].x)[c], 32);
    }

    int orderType = 0;
    stream->readBits(&orderType, 8);
    m_orderType = static_cast<unsigned char>(orderType);

    int flag37 = 0;
    stream->readBits(&flag37, 1);
    m_flag37 = (flag37 == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x80747B0, 592 B. The exact mirror; the selector is `m_targetNetId != 0`.
bool IssueSquadOrderEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int squadId = m_squadId;
    stream->writeBits(&squadId, 8);

    int teamId = m_teamId;
    stream->writeBits(&teamId, 8);

    int orderId = m_orderId;
    stream->writeBits(&orderId, 8);

    int hasTarget = (m_targetNetId != 0) ? 1 : 0;
    stream->writeBits(&hasTarget, 1);

    if (m_targetNetId != 0)
    {
        int targetNetId = m_targetNetId;
        stream->writeBits(&targetNetId, 16);
    }
    else
    {
        for (int c = 0; c < 3; ++c)
            for (int p = 0; p < 3; ++p)
                stream->writeBits(&(&m_points[p].x)[c], 32);
    }

    int orderType = m_orderType;
    stream->writeBits(&orderType, 8);

    int flag37 = m_flag37 ? 1 : 0;
    stream->writeBits(&flag37, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
