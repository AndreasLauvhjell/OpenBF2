// Per-team score and ticket state; non-polymorphic.
// [INFERRED] Path beside ScoreManager; signatures recovered from the dedicated ELF.
// Ticket accessors preserve their shipped clamping and signed arithmetic.

#ifndef GAME_GAMELOGIC_SCORESYSTEM_TEAMSCORE_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_SCORESYSTEM_TEAMSCORE_H

struct qr2_buffer_s;

namespace dice {
namespace hfe {
namespace io { class IStream; }
class ServerGameLogic;   // spawnPlayer reads m_tickets directly - see the friend
class HudInformationLayer;
class SwiffHost;         // Menu/SwiffHost.h - setEndOfRoundInfo reads m_roundsWon, see the friend
void qr2_player_team_key_callback(int, int, qr2_buffer_s*, void*);

namespace world {

// Non-polymorphic - no vtable, no RTTI record. sizeof >= 0x94.
class TeamScore
{
    // ServerGameLogic also reaches private reset/state fields directly. m_tickets itself is public
    // below because a second TU, PythonHost.cpp, reads it raw with no getter or helper seam.
    friend class ::dice::hfe::ServerGameLogic;
    friend class ::dice::hfe::HudInformationLayer;
    //  / PE 0x616BF0: the browser callback reads the +0x34 counter directly.
    friend void ::dice::hfe::qr2_player_team_key_callback(int, int, qr2_buffer_s*, void*);
    // BF2.exe 0x49B5C3 (SwiffHost::setEndOfRoundInfo, the client's end-of-round parser) reads
    // m_roundsWon (+0x88) straight off getTeamScore's answer. client-only reader; the friendship is
    // harmless in the dedicated image, where SwiffHost does not exist.
    friend class ::dice::hfe::SwiffHost;

public:
    TeamScore();

    void setTickets(int);                 // clamps to 0..9999
    void subTicket(int);                  // floors at 0
    void setTicketState(int);
    void setTicketBleed(bool);

    void writeStateToStream(io::IStream*);   //   [UNRESOLVED] signature
    void readStateFromStream(io::IStream*);  //   [UNRESOLVED] signature

private:
    // ScoreManager::reset (0x8107A50) writes this object's fields directly - nine dwords of the
    // block, setTickets(0), then +0x34 - the way an inlined member reset would; the friendship
    // reproduces that access without inventing a TeamScore::reset the ELF does not have.
    friend class ScoreManager;

    /* +0x00 */ int  m_teamId;            // not written by TeamScore's ctor - ScoreManager's ctor
                                          // stores 1 / 2 into its two members' +0x00 (0x8107777 /
                                          // ); IDB name teamId
    /* +0x04 */ int  m_block[16];         // [UNRESOLVED] +0x04..+0x40, all zeroed by the ctor
    /* +0x44 */ char _gap44[0x40];        // [UNRESOLVED] +0x44..+0x80, ctor does not touch it
public:
    // Public on direct source evidence: python::gl_sendEndOfRoundData reads +0x84 from this object
    // in its own TU (ELF 0x8154E57 / PE 0x534432). There is no getter call or helper seam.
    /* +0x84 */ int  m_tickets;           // clamped 0..9999
private:
    /* +0x88 */ int  m_roundsWon;         // [INFERRED name] - zeroed by the ctor; ScoreManager::setWinner
                                          //   (0x8107A19) increments the winning team's copy once per
                                          //   declared winner, and it replicates under mask bit 0x800
    /* +0x8C */ int  m_ticketState;
    /* +0x90 */ bool m_ticketBleed;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SCORESYSTEM_TEAMSCORE_H
