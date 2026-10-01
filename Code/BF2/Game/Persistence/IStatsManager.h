// Ranked statistics backend interface. Preserve virtual order, including the trailing
// destructor. getGame returns the upstream SDK's opaque statsgame pointer.
// [INFERRED] Header placement.

#ifndef GAME_PERSISTENCE_ISTATSMANAGER_H     // [INFERRED] guard style
#define GAME_PERSISTENCE_ISTATSMANAGER_H

#include <string>

#include "../../Core/IBase.h"

struct statsgame_s;

namespace dice {
namespace hfe {

namespace world { class IPlayer; }

class IStatsManager : public IBase
{
public:
    virtual bool connect() = 0;                                  // +0x10  151 B
    virtual void disconnect() = 0;                               // +0x14   81 B
    virtual bool newGame() = 0;                                  // +0x18  107 B
    virtual bool endGame() = 0;                                  // +0x1C   99 B
    virtual bool isConnected() = 0;                              // +0x20   23 B
    virtual const std::string& getChallenge() = 0;               // +0x24   11 B (a std::string member)
    virtual statsgame_s* getGame() const = 0;                     // +0x28   11 B
    virtual bool think() = 0;                                    // +0x2C   23 B - the per-tick poll
    virtual bool sendSnapshot(const std::string& snapshot) = 0;  // +0x30  405 B
    virtual bool sendFinal(const std::string& snapshot) = 0;     // +0x34  124 B
    virtual bool authenticatePlayer(world::IPlayer* player) = 0; // +0x38   32 B
    virtual ~IStatsManager() {}                                  // +0x3C / +0x40 - last, see above
};

} // namespace hfe
} // namespace dice

#endif // GAME_PERSISTENCE_ISTATSMANAGER_H
