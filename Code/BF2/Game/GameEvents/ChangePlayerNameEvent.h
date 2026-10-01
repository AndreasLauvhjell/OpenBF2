// Renames a player using a fixed 32-byte wire buffer without a length field.
// Constructors cap names at 31 characters and terminate them; the player overload
// repeats getName() for each copied character. Execution requires a terminated name.

#ifndef GAME_GAMEEVENTS_CHANGEPLAYERNAMEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_CHANGEPLAYERNAMEEVENT_H

#include <string>

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {
namespace world { class IPlayer; }

class ChangePlayerNameEvent : public GameEvent               // 0x30
{
public:
    enum { NAME_BUFFER_SIZE = 32, MAX_NAME_LENGTH = NAME_BUFFER_SIZE - 1 };

    explicit ChangePlayerNameEvent(world::IPlayer* player);
    ChangePlayerNameEvent(int playerId, const std::string& name);
    ChangePlayerNameEvent() : m_playerId(0) { m_name[0] = '\0'; }    // the maker's form
    virtual ~ChangePlayerNameEvent() {}

    virtual int  getType() const;                            // GEChangePlayerName (12)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ char          m_name[NAME_BUFFER_SIZE];
    /* +0x2C */ unsigned char m_playerId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_CHANGEPLAYERNAMEEVENT_H
