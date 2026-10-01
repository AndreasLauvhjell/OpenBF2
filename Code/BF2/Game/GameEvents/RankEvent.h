// Server-to-client promotion notice. Type must be 1; other types and server
// execution are fatal. Rank is encoded as rank + 1, preserving -1 as no rank;
// the encoded value must be in [0,31]. Construction may execute locally on a
// listen server. [UNRESOLVED] m_field14 is transported but not interpreted.

#ifndef GAME_GAMEEVENTS_RANKEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_RANKEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class RankEvent : public GameEvent                           // 0x1C
{
public:
    // [UNRESOLVED] Enumerator names; only value 1 is currently identified.
    enum RankEventTypes : int;

    RankEvent(RankEventTypes type, unsigned int newRank,
              unsigned int field14, int playerId);
    RankEvent() : m_rankType(0), m_newRank(0), m_field14(0), m_playerId(0) {}   // the maker's form
    virtual ~RankEvent() {}

    virtual int  getType() const;                            // GERank (26)
    virtual void executeClient();                            // can abort
    virtual bool executeServer(int connectionId);            // always aborts
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);           // can abort

protected:
    /* +0x0C */ int          m_rankType;     // held as int - see the opaque enum note above
    /* +0x10 */ unsigned int m_newRank;      // 1 is legal and means "no rank"
    /* +0x14 */ unsigned int m_field14;      // [UNRESOLVED] - carried, never read
    /* +0x18 */ int          m_playerId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_RANKEVENT_H
