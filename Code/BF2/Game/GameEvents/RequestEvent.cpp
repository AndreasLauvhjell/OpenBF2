#include "RequestEvent.h"

#include <cstring>                               // memset

#include "../IGame.h"                            // sendGameEventIfConnected
#include "../IGameLogic.h"                       // gameLogic->isServer()
#include "../../AI/AIInterface/AIInstance.h"     // aiInstance()->isBotsGame()  (ds:0x8AC37F8)
#include "../../Core/EventManager.h"             // eventManager->postEvent
#include "../../Core/IBase.h"
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../GameLogic/SquadManager/ISquadManager.h" // typed +0x3C/+0x70 dispatch

namespace dice {
namespace hfe {

extern IBase* squadManager;                      // ds:0x8ACBFA4 - Core/Globals.cpp (IBase*, no layout)

namespace {
// .rodata dword GERequest (0x888A424) = 50.
const int GERequest = 50;

// The two notifications. ECHud (2) event 25 to the commander; ecai (3) event 16 to the bots.
const unsigned int NOTIFY_HUD_REQUEST = 25u;
const unsigned int NOTIFY_AI_REQUEST  = 16u;

// The one request type the AI arm reacts to. A bare 1 in the ELF.
const int REQUEST_TYPE_AI = 1;

// "No commander" - getCommander's sentinel, compared as a bare 255.
const int NO_COMMANDER = 255;

// The width loop both halves carry inline - see ConnectionTypeEvent.cpp for the derivation.
unsigned int bitsToHold(unsigned int highest)
{
    unsigned int bits = 0;
    do
        ++bits;
    while (((1u << bits) - 1u) < highest);
    return bits;
}

inline ISquadManager* squadManagerIface()
{
    return static_cast<ISquadManager*>(squadManager);
}

// HUD request payload; the middle word remains uninitialized.
struct HudRequestBlock                                       // 0x0C
{
    int squadId;                                 // +0x00 <- m_squadId
    int hole;                                    // +0x04 never  BY THE ELF
    int requestType;                             // +0x08 <- m_requestType
};

// AI request payload; its layout differs from SquadOrder despite matching its size.
// Only the two explicitly initialized ranges are zeroed.
struct AiRequestBlock                                        // 0x30
{
    int  teamId;                                 // +0x00
    int  squadId;                                // +0x04
    char body[0x28];                             // +0x08..+0x2F - see the note above
};
} // namespace

//  (C2) / 0x8079BD0 (C1). Three stores; no zeroing, no loopback.
RequestEvent::RequestEvent(int teamId, int squadId, RequestTypes type)
    : GameEvent()
    , m_requestType(static_cast<int>(type))                  // +0x0C
    , m_teamId(static_cast<unsigned char>(teamId))           // +0x10
    , m_squadId(static_cast<unsigned char>(squadId))         // +0x11
{
}

int RequestEvent::getType() const
{
    return GERequest;
}

// Builds the 12-byte HUD payload, including its uninitialized middle word. Without
// a commander the notification stays local. The second arm is independent of the
// first arm's squad-manager and permission checks.
bool RequestEvent::executeServer(int connectionId)
{
    HudRequestBlock hud;
    hud.squadId     = m_squadId;                 // +0x00
    hud.requestType = m_requestType;             // +0x08 - hud.hole is deliberately untouched

    const int sender = static_cast<int>(static_cast<unsigned char>(connectionId));   // truncated

    if (squadManager)
    {
        const bool isSquadLeader =
            squadManagerIface()->getSquadLeader(m_teamId, m_squadId) == sender;        // +0x3C
        const bool isCommander =
            squadManagerIface()->getCommander(m_teamId) == sender;                     // +0x70

        if (isSquadLeader || isCommander)
        {
            if (squadManagerIface()->getCommander(m_teamId) == NO_COMMANDER)
            {
                eventManager->postEvent(ECHud, NOTIFY_HUD_REQUEST, &hud, 0.0f);        // +0x14
            }
            else
            {
                PostRemoteEvent* relay = new PostRemoteEvent(ECHud, NOTIFY_HUD_REQUEST,
                                                             false, &hud, 12, 0.0f);
                game->sendGameEventIfConnected(
                    squadManagerIface()->getCommander(m_teamId), relay);               // the third lookup
            }
        }
    }

    if (gameLogic->isServer() && m_requestType == REQUEST_TYPE_AI &&
        ai::g_aiInstance && ai::aiInstance()->isBotsGame())                            // +0x1C
    {
        AiRequestBlock ai;
        memset(ai.body,        0, 0x0C);         // +0x08..+0x13; +0x14..+0x1F stays untouched
        memset(ai.body + 0x18, 0, 0x0C);         // +0x20..+0x2B; +0x2C..+0x2F stays untouched
        ai.teamId  = m_teamId;
        ai.squadId = m_squadId;
        eventManager->postEvent(ECAI, NOTIFY_AI_REQUEST, &ai, 0.0f);                   // +0x14
    }

    return true;
}

// deSerialize - 0x8079C10, 192 B. The squad id comes first and the team id second - the reverse
// of memory order, exactly as IssueSquadOrderEvent does it.
bool RequestEvent::deSerialize(io::BitStream* stream)
{
    int squadId = 0;
    stream->readBits(&squadId, 8);               // first
    m_squadId = static_cast<unsigned char>(squadId);

    int teamId = 0;
    stream->readBits(&teamId, 8);
    m_teamId = static_cast<unsigned char>(teamId);

    int requestType = 0;
    stream->readBits(&requestType, bitsToHold(4));   // three bits
    m_requestType = requestType;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8079CD0, 224 B. The exact mirror, and the type is NOT range-checked on the way out.
bool RequestEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int squadId = m_squadId;
    stream->writeBits(&squadId, 8);

    int teamId = m_teamId;
    stream->writeBits(&teamId, 8);

    int requestType = m_requestType;
    stream->writeBits(&requestType, bitsToHold(4));

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
