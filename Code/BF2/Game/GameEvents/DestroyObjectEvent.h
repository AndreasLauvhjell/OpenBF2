// Removes a networked object on clients by its 16-bit ID.
// Server execution retains GameEvent's no-op. [INFERRED] The sending constructor is inferred.

#ifndef GAME_GAMEEVENTS_DESTROYOBJECTEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_DESTROYOBJECTEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class DestroyObjectEvent : public GameEvent                  // 0x10
{
public:
    explicit DestroyObjectEvent(unsigned short netId);       // [INFERRED] - no ELF constructor
    DestroyObjectEvent() : m_netId(0) {}                     // the maker's form
    virtual ~DestroyObjectEvent() {}

    virtual int  getType() const;                            // GEDestroyObject (7)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeServer is deliberately NOT declared - see the header block.

protected:
    /* +0x0C */ unsigned short m_netId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_DESTROYOBJECTEVENT_H
