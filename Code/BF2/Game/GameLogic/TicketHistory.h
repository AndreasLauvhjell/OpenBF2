// Round ticket history: map, mode token, player limit, victory result and both teams' names/tickets.
// The owning vector is copied by value where requested; STL layout is compiler-dependent.

#ifndef GAME_GAMELOGIC_TICKETHISTORY_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_TICKETHISTORY_H

#include <string>
#include <vector>

namespace dice {
namespace hfe {

struct TicketHistory
{
    struct Ticket                                    // 36 bytes
    {
        struct Pair                                  // the +0x14 / +0x1C {string, int} stride
        {
            std::string text;                        // +0x00
            int         value;                       // +0x04  NOT zeroed by the default ctor
        };

        std::string string0;                         // +0x00  current map name
        std::string string4;                         // +0x04  "$gamemode_fixme" in the stock sender
        int         dword8;                          // +0x08; map player limit, uninitialized by construction.
        int         dwordC;                          // +0x0C  victory type; three ints uninitialised
        int         dword10;                         // +0x10  winning team
        Pair        pair[2];                         // +0x14  team name + current tickets

        // : the four strings are constructed empty (each a refcount bump on the
        // shared empty rep) and nothing else is written - the five ints keep whatever was there.
        Ticket() {}
        //  / 0x8057530: member-wise, in declaration order - the compiler-emitted copy and
        // assignment ARE the shipped bodies.
    };

    std::vector<Ticket> tickets;                     // +0x00, 12 bytes
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_TICKETHISTORY_H
