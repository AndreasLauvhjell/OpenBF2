#include "ToggleFreeCameraEvent.h"

#include "../Common/IClientConnection.h"          // getPlayer
#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../IGameServer.h"                      // getConnection / toggleFreeCamera
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEToggleFreeCamera (0x888AB64) = 40.
const int GEToggleFreeCamera = 40;
} // namespace

int ToggleFreeCameraEvent::getType() const
{
    return GEToggleFreeCamera;
}

// an explicit empty override. The camera change comes back to the client as
// ordinary object/player state, not as this event replayed.
void ToggleFreeCameraEvent::executeClient()
{
}

// Toggles the sending connection's player's camera. Returns false without a server
// interface, connection, or associated player; payload cannot select another player.
bool ToggleFreeCameraEvent::executeServer(int connectionId)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    IClientConnection* connection = server->getConnection(connectionId);        // +0x3C
    if (!connection)
        return false;

    world::IPlayer* player = connection->getPlayer();                           // conceptual +0x18
    if (!player)
        return false;

    server->toggleFreeCamera(player);                                           // +0x48
    return true;
}

// deSerialize - 0x807F2A0, 39 B. There is nothing to read; the body is the error check alone.
bool ToggleFreeCameraEvent::deSerialize(io::BitStream* stream)
{
    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x807F2D0, 110 B. The type id and nothing else.
bool ToggleFreeCameraEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
