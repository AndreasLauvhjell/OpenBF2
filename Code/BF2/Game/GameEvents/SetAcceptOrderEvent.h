// Replicates acceptance or rejection of a squad order.
// Team, squad and order enums narrow to bytes; the wire puts squad before team
// and encodes acceptance as one bit.

#ifndef GAME_GAMEEVENTS_SETACCEPTORDEREVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_SETACCEPTORDEREVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

// Opaque squad-order enums. [UNRESOLVED] Enumerator names; fixed int underlying
// types permit typed parameters without inventing values.
enum OrderType : int;            // [UNRESOLVED] enumerators
enum SquadOrderType : int;       // [UNRESOLVED] enumerators

class SetAcceptOrderEvent : public GameEvent                 // 0x14
{
public:
    SetAcceptOrderEvent(int team, int squadId, bool isAccepted,
                        OrderType orderType, SquadOrderType squadOrderType);
    SetAcceptOrderEvent()
        : m_team(0), m_squadId(0), m_isAccepted(false)
        , m_orderType(0), m_squadOrderType(0) {}             // the maker's form
    virtual ~SetAcceptOrderEvent() {}

    virtual int  getType() const;                            // GESetAcceptOrder (27)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned char m_team;
    /* +0x0D */ unsigned char m_squadId;
    /* +0x0E */ bool          m_isAccepted;
    /* +0x0F */ unsigned char m_orderType;
    /* +0x10 */ unsigned char m_squadOrderType;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_SETACCEPTORDEREVENT_H
