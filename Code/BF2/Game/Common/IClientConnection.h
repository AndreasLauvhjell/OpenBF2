// Path:     [INFERRED] beside the attested Game/Common/ServerConnection.h.
// Contents: the complete source-level IClientConnection vtable, recovered from bf2.i64 and
//           bf2_w32ded.exe.i64.  The typed interface is important across the two ABIs: Itanium
//           emits D1/D0 at +0x00/+0x04, while MSVC emits one scalar-deleting slot at +0x00.

#ifndef GAME_COMMON_ICLIENTCONNECTION_H
#define GAME_COMMON_ICLIENTCONNECTION_H

namespace dice {
namespace hfe {

class IStreamManager;
class DataBlockManager;
class StringManager;
namespace world { class IPlayer; }

class IClientConnection
{
public:
    virtual ~IClientConnection() = 0;
    virtual void addStreamManager(IStreamManager*) = 0;
    virtual IStreamManager* getStreamManager(int) = 0;
    virtual DataBlockManager* getDataBlockManager() = 0;
    virtual StringManager* getStringManager() = 0;
    virtual world::IPlayer* getPlayer() = 0;
    virtual void setPlayer(world::IPlayer*) = 0;
    virtual bool getIsRecordingConnection() const = 0;
    virtual int debugGetNumActiveGhosts() = 0;
    virtual void debugShowActiveGhosts() = 0;
    virtual void setContentValid(bool) = 0;
    virtual bool getContentValid() const = 0;
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_ICLIENTCONNECTION_H
