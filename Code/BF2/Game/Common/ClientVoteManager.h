// Lazy client voting state and UI notifications. Notification methods are inert in
// dedicated mode; client paths use the local player and HUD. release() owns singleton teardown.

#ifndef GAME_COMMON_CLIENTVOTEMANAGER_H     // [INFERRED] guard style
#define GAME_COMMON_CLIENTVOTEMANAGER_H

#include <string>
#include "../GameServer/Vote.h"

namespace dice {
namespace hfe {

class VoteEvent;
// Numeric display-message mapping; original enumerator names are unknown:
// 0 cooldown; 1 vote in progress; 2 cannot kick self; 3 target unavailable;
// 4 cannot kick host; 5 no message; 6 too few players; 7 new-vote prompt;
// 8 running tally; 9 passed; 10 failed; 11 own yes/no ballot;
// 12 player left; 13 commander left during mutiny.
enum ClientVoteMessage : int;

class ClientVoteManager
{
public:
    ClientVoteManager();
    static ClientVoteManager* getInstance();
    static void release();                            // (~GameLogic)
    //  (empty - folded) / client 5978A0, 137 B: ask the server for the running
    // vote (VoteEvent status 9, VSClientState). Caller: Player::setTeam, for the local human.
    void requestStateUpdate(int playerId);
    //  (empty - folded) / client 599810, 757 B: the sink VoteEvent::executeClient
    // hands every vote broadcast to.
    void receiveVote(const VoteEvent& event);
    //  / client 597930, 28 B: a kick or mutiny vote (types 2 / 3) has a target
    // player. Mangled `...28voteTypeRequiresTargetPlayerENS0_4Vote8VoteTypeE`; both images take
    // `this` (ELF: the type at [ebp+0xC]; PE: ecx, `retn 4`) - a non-static member.
    bool voteTypeRequiresTargetPlayer(Vote::VoteType type);
    //  / client 597950, 195 B: does a new issue concern the local player? Mangled
    // `...25votePertainsToLocalClientERKNS0_9VoteEventE`; non-static in both images.
    bool votePertainsToLocalClient(const VoteEvent& event);
    bool sendMutinyIssue();                          // ELF81285A0/client599460; HUD7B1F50
    //  (460 B) / client 599690, both Caller: the scoreboard's kick
    // columns (HudScoreboard::singleClick, 7A4570).
    bool sendKickIssue(int playerId);
    //  (295 B, _ZN4dice3hfe17ClientVoteManager12sendMapIssueEi) / client 599580
    // (262 B), both Caller: the vote map's list click (HudVoteMap::singleClick,
    // ); the dedicated ELF has none. Both bodies materialise the bool (ELF edx 0/1, PE AL).
    bool sendMapIssue(int mapIndex);
    bool sendVote(Vote::VoteOption);                  // ELF81289F0/client597B30
    int getNumYesVotesCasted(); // ELF8128340/client597C30
    int getNumNoVotesCasted();  // ELF81283E0/client597CA0
    // [INFERRED] accessor names/constness; full PE copies native strings by value.
    std::string getVoteIssuerName() { return m_string14; } // retail+2C
    std::string getVoteTargetName() { return m_string0C; } // retail+0C
    bool participatesInVoting();                     // ELF8128D10; inlined in client599460
    bool getDisallowedForTimePeriod();                // ELF8128320/client5977B0; ordered >0
    //  (empty - folded) / client 597D60, 5,361 B: the HUD_HUD_KICKVOTE_* chat
    // lines and the ballot (see the ClientVoteMessage table above). The bool lets the message
    // through for a player who is NOT taking part in votes - receiveVote passes true for the new
    // vote's prompt and the four results, false for everything else.
    void displayMessage(ClientVoteMessage, bool);

#if !DEDICATED
    // [PLATFORM] Client frame/voting implementations; dedicated counterparts remain separate.
    void update(float dt);
    void resetActiveVote();
    void setVotingParticipationStatus(bool participate);
#endif

#if !DEDICATED
    // Client-only voting check; requires a local human player.
    bool votingEnabled();
    // Returns true when serverInfo is absent; otherwise reads its team-only flag.
    // Use the typed field because PE and ELF string layouts shift its offset.
    bool getTeamVoteOnly();
    // Checks availability for a new issue. Requires a local player when the manager exists.
    bool voteIssueAllowed();
#endif

    // Offsets describe the ELF layout; MSVC string layout differs. Positional field
    // names remain where external callers depend on them.
    /* +0x00 */ bool        m_messagesEnabled; // ctor true [INFERRED name]
    /* +0x01 */ bool        m_messageDisplayed; // ctor false; +02/+03 padding unwritten
    // Active vote type; zero means no vote.
    /* +0x04 */ int         m_dword04;
    // Active map index or target player ID, depending on vote type; defaults to 254.
    /* +0x08 */ int         m_dword08;
    // Target player name used in vote messages; empty without a valid kick or mutiny target.
    /* +0x0C */ std::string m_string0C;
    // ID of the player who issued the vote.
    /* +0x10 */ int         m_voteCaster;     // [INFERRED name] - VoteEvent's own name for the field
    // Vote issuer name used by message formatting.
    /* +0x14 */ std::string m_string14;
    // [INFERRED] Vote reminder/prompt timers and cached team state.
    /* +0x18 */ float       m_reminderTimer;  // ctor 0; counts UP, displayMessage(8) at 30 s
    /* +0x1C */ float       m_promptTimer;    // ctor 0; counts UP while !m_messageDisplayed,
                                              // Votes still needed, used by the vote message formatter.
    /* +0x20 */ int         m_votesNeeded;    // [INFERRED name] - VoteEvent's own name for the field
    /* +0x24 */ float       m_voteTimeRemaining; // PE+54, received event time then decremented
    /* +0x28 */ float       m_disallowedTime; // client+58; ctor0, received issue sets300
    /* +0x2C */ int         m_localTeam;      // ctor 0; the local human's team, re-read every frame

    // The original ELF/client-PE layouts differ through std::string sizes (4/28 bytes):
    // class sizes are 48/96 bytes respectively. Native compiler layouts may differ.

private:
    static ClientVoteManager* s_instance;     // ds:0x8AACA20
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_CLIENTVOTEMANAGER_H
