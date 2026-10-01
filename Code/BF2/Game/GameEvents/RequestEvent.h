// Squad request delivered to the commander when the sender is a leader or commander.
// The bots-game request-1 notification is independent of that authority check.
// Connection ids narrow to bytes. The HUD payload deliberately leaves its middle
// word uninitialized while transmitting all twelve bytes.

#ifndef GAME_GAMEEVENTS_REQUESTEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_REQUESTEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class RequestEvent : public GameEvent                        // 0x14
{
public:
    // attested as a nested type - the S1_ substitution in the constructor's mangling is
    // dice::hfe::RequestEvent itself. [UNRESOLVED] enumerators: the wire width provisions five
    // (bitsToHold(4)), and only the value 1 is ever tested. Declared opaque, the same treatment
    // world::Medal gets in MedalEvent.h.
    enum RequestTypes : int;

    // attested: (int, int, RequestTypes). 0x8079B90 (C2) / 0x8079BD0 (C1).
    RequestEvent(int teamId, int squadId, RequestTypes type);
    RequestEvent() {}                                        // the maker's form - initialises nothing
    virtual ~RequestEvent() {}

    virtual int  getType() const;                            // GERequest (50)
    // executeClient IS NOT declared - see the header block. GameEvent's default is inherited.
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int           m_requestType;   // held as int - see the opaque enum note above
    /* +0x10 */ unsigned char m_teamId;
    /* +0x11 */ unsigned char m_squadId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_REQUESTEVENT_H
