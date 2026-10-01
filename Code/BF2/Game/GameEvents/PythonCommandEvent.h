// Deliver a command id, player and up to four integers to Python event 34.
// Argument count trims trailing zeros. Deserialization bounds writes to four
// entries while consuming all encoded arguments, preserving stream position.
// executeLocally dispatches immediately to the selected client/server arm.

#ifndef GAME_GAMEEVENTS_PYTHONCOMMANDEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_PYTHONCOMMANDEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class PythonCommandEvent : public GameEvent                  // 0x2C
{
public:
    // The array's real size. The wire's four-bit count can exceed it - see the header block.
    enum { MaxArgs = 4 };

    // attested: (int, int, bool, bool, int, int, int, int). 0x8077FE0 (C2) / 0x8078080 (C1).
    // The two bools are `execute locally` and `as the client`, in that order.
    PythonCommandEvent(int commandId, int playerId, bool executeLocally, bool asClient,
                       int arg0, int arg1, int arg2, int arg3);
    PythonCommandEvent();                                    // the maker's form - zeroes the args only
    virtual ~PythonCommandEvent() {}

    virtual int  getType() const;                            // GEPythonCommand (49)
    virtual void executeClient();                            // HUD only; dead here
    virtual bool executeServer(int connectionId);            // the Python hand-off
    virtual bool deSerialize(io::BitStream* stream);         // bounded, see the .cpp
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int m_field0C;                   // never read or written by this binary
    /* +0x10 */ int m_playerId;
    /* +0x14 */ int m_commandId;
    /* +0x18 */ int m_args[MaxArgs];
    /* +0x28 */ int m_argCount;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_PYTHONCOMMANDEVENT_H
