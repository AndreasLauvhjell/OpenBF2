// Removes the identified player on clients. Server execution and logging remain inherited.

#ifndef GAME_GAMEEVENTS_DESTROYPLAYEREVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_DESTROYPLAYEREVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class DestroyPlayerEvent : public GameEvent                  // 0x10
{
public:
    explicit DestroyPlayerEvent(int playerId);               // [INFERRED] - no ELF constructor
    DestroyPlayerEvent() : m_playerId(0) {}                  // the maker's form
    virtual ~DestroyPlayerEvent() {}

    virtual int  getType() const;                            // GEDestroyPlayer (8)
    virtual void executeClient();
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeServer and logInfo are deliberately NOT declared - both slots are inherited.

protected:
    /* +0x0C */ unsigned char m_playerId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_DESTROYPLAYEREVENT_H
