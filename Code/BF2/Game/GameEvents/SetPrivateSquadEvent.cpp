#include "SetPrivateSquadEvent.h"

#include "../../Core/IBase.h"
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../GameLogic/SquadManager/ISquadManager.h" // typed SquadManager dispatch

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GESetPrivateSquad (0x888A5E4) = 23.
const int GESetPrivateSquad = 23;

// The trailing suppress flag prevents received squad changes from being broadcast again.
} // namespace

//  (C2) / 0x807AF20 (C1), 59 B. GameEvent's ctor, the vptr, three byte stores.
SetPrivateSquadEvent::SetPrivateSquadEvent(int team, int squadId, bool isPrivate)
    : GameEvent()
    , m_team(static_cast<unsigned char>(team))                       // +0x0C
    , m_squadId(static_cast<unsigned char>(squadId))                 // +0x0D
    , m_isPrivate(isPrivate)                                         // +0x0E
{
}

int  SetPrivateSquadEvent::getType() const { return GESetPrivateSquad; }
void SetPrivateSquadEvent::logInfo(bool, int) {}

// executeClient - 0x807B110, 77 B. Apply it, suppressing the echo.
void SetPrivateSquadEvent::executeClient()
{
    if (squadManager)
        static_cast<ISquadManager*>(squadManager)->setIsPrivate(
            m_team, m_squadId, m_isPrivate, true);                                    // +0x20
}

// Changes privacy only when the connection's low byte matches the squad leader.
// Does not require a server-interface query. Returns true for ignored requests too.
bool SetPrivateSquadEvent::executeServer(int connectionId)
{
    if (!squadManager)
        return true;

    const int leader = static_cast<ISquadManager*>(squadManager)->getSquadLeader(
        m_team, m_squadId);                                                            // +0x3C

    if (leader == static_cast<unsigned char>(connectionId))          // byte-truncated, see above
        static_cast<ISquadManager*>(squadManager)->setIsPrivate(
            m_team, m_squadId, m_isPrivate, false);                                    // +0x20

    return true;
}

// deSerialize - 0x807AF70, 167 B. squad first, then team - see the header.
bool SetPrivateSquadEvent::deSerialize(io::BitStream* stream)
{
    int squadId = 0;
    stream->readBits(&squadId, 8);
    m_squadId = static_cast<unsigned char>(squadId);

    int team = 0;
    stream->readBits(&team, 8);
    m_team = static_cast<unsigned char>(team);

    int isPrivate = 0;
    stream->readBits(&isPrivate, 1);
    m_isPrivate = (isPrivate == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x807B020, 231 B. The same swapped order.
bool SetPrivateSquadEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int squadId = m_squadId;
    stream->writeBits(&squadId, 8);

    int team = m_team;
    stream->writeBits(&team, 8);

    int isPrivate = m_isPrivate ? 1 : 0;
    stream->writeBits(&isPrivate, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
