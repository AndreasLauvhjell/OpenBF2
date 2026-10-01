// Team score state and wire serialization. [INFERRED] source path.

#include "TeamScore.h"

namespace dice {
namespace hfe {
namespace world {

// Note what it does NOT touch: +0x00, and the whole +0x44..+0x80 region. The serialiser and
// deserialiser skip that region too, so three independent silences agree it is runtime-only scratch
// rather than a missing recovery. Leaving it uninitialised here is faithful, not an oversight.
TeamScore::TeamScore()
{
    m_tickets     = 0;      // +0x84
    m_roundsWon   = 0;      // +0x88
    m_ticketState = 0;      // +0x8C
    m_ticketBleed = false;  // +0x90 — a byte store

    // +0x04..+0x40: sixteen consecutive dwords, all zeroed. Every one of them IS serialised, so
    // this block is authoritative game state despite its members being unnamed.
    for (int i = 0; i < 16; ++i)
        m_block[i] = 0;
}

// Clamps tickets to 0..9999, applying the lower and upper clamps in sequence.
void TeamScore::setTickets(int n)
{
    m_tickets = n;

    if (m_tickets < 0)
        m_tickets = 0;

    if (m_tickets > 9999)
        m_tickets = 9999;
}

// . floor only — deliberately NO upper clamp, because this path only ever
// decreases. Do not "make it symmetric" with setTickets.
void TeamScore::subTicket(int n)
{
    m_tickets -= n;

    if (m_tickets < 0)
        m_tickets = 0;
}

//  and 0x81074F0 — 17 bytes each, plain stores. These two are what made this class
// reachable at all: the setter sweep found them writing +0x8C and +0x90 directly, which named the
// class and gave two offsets before the constructor supplied the rest.
void TeamScore::setTicketState(int state)
{
    m_ticketState = state;      // +0x8C
}

void TeamScore::setTicketBleed(bool bleeding)
{
    m_ticketBleed = bleeding;   // +0x90
}

} // namespace world
} // namespace hfe
} // namespace dice
