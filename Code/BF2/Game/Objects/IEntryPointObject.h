// Vehicle-seat entry and exit interface. The seat index is passed by reference; EntryPoint leaves it unchanged.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IENTRYPOINTOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_IENTRYPOINTOBJECT_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

class IPlayer;
class IPlayerControlObject;

class IEntryPointObject : public IBase
{
public:
    virtual bool enter(IPlayer* player, int& seat) = 0;                    // +0x10
    virtual bool exit(IPlayer* player) = 0;                                // +0x14
    virtual IPlayer* getPlayer() const = 0;                                // +0x18
    virtual IPlayerControlObject* getLinkedPlayerControlObject() = 0;      // +0x1C

protected:
    ~IEntryPointObject() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IENTRYPOINTOBJECT_H
