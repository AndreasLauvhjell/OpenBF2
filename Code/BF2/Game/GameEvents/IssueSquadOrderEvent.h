// Squad/commander order or request. The server verifies leader/commander authority;
// clients apply the vetted result. Connection ids are byte-truncated for authority
// checks. Target object and geometry are alternative payloads. Geometry travels
// component-major (all x, then y, then z); only those floats are initialized by default.

#ifndef GAME_GAMEEVENTS_ISSUESQUADORDEREVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_ISSUESQUADORDEREVENT_H

#include "../Common/GameEvent.h"
#include "../GameLogic/SquadManager/Squad.h"

namespace dice {
namespace hfe {

// Only the low byte of SquadOrder::orderId survives serialization.
// [UNRESOLVED] OrderType enumerator names; dispatch recognizes values 0, 1, and 2.
enum OrderType : int;

class IssueSquadOrderEvent : public GameEvent                // 0x38
{
public:
    // attested: (int, int, SquadOrder*, OrderType). 0x8074410 (C2) / 0x80744C0 (C1).
    // A NULL SquadOrder IS accepted: the constructor tests it and simply leaves the payload at
    // its zeroed default, so the event goes out as "order id 0, no target, three zero points".
    IssueSquadOrderEvent(int teamId, int squadId, SquadOrder* order, OrderType type);
    IssueSquadOrderEvent();                                  // the maker's form - zeroes the points only
    virtual ~IssueSquadOrderEvent() {}

    virtual int  getType() const;                            // GEIssueSquadOrder (24)
    virtual void executeClient();                            // NO permission checks
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned char  m_teamId;
    /* +0x0D */ unsigned char  m_squadId;
    /* +0x0E */ unsigned char  m_orderId;
    /* +0x10 */ Vec3           m_points[3];
    /* +0x34 */ unsigned short m_targetNetId;
    /* +0x36 */ unsigned char  m_orderType;
    /* +0x37 */ bool           m_flag37;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_ISSUESQUADORDEREVENT_H
