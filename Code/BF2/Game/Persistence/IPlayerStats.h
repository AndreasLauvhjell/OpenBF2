// Path:     [INFERRED] beside the attested Game/Persistence/PlayerStats.cpp implementation.
// Contents: IPlayerStats' complete ten-slot primary interface, recovered from PlayerStats' ELF
//           vtable at 0x88C7220 and the six concrete mangled method names/bodies.

#ifndef GAME_PERSISTENCE_IPLAYERSTATS_H
#define GAME_PERSISTENCE_IPLAYERSTATS_H

#include <string>

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {

class IPlayerStats : public IBase
{
public:
    virtual bool getStats(int profileId, const std::string& query,
                          bool asynchronous, bool notifyPython) = 0;        // +0x10
    virtual void tick() = 0;                                                // +0x14; both ports tail-call void HTTPRequest::update
    virtual int getNumKeyMaps() = 0;                                       // +0x18
    virtual int getNumKeys() = 0;                                          // +0x1C
    virtual bool getKey(unsigned int mapIndex, int keyIndex,
                        std::string& key, std::string& value) = 0;          // +0x20
    virtual std::string getKeyValue(unsigned int mapIndex,
                                    const std::string& key) = 0;            // +0x24

protected:
    ~IPlayerStats() {}
};

} // namespace hfe
} // namespace dice

#endif // GAME_PERSISTENCE_IPLAYERSTATS_H
