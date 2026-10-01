// Tracks one active vote, per-player choices and cached option totals.
// issueVote establishes thresholds and invalidates the tally before it is read.
// [CHOSEN] Option names describe the wire values: for, against and not-yet-voted.

#ifndef GAME_GAMESERVER_VOTE_H     // [INFERRED] guard style
#define GAME_GAMESERVER_VOTE_H

#include <map>
#include <string>
#include <vector>

namespace dice {
namespace hfe {

class Vote
{
public:
    // The three enums are nested (mangled `N4dice3hfe4Vote8VoteTypeE` etc. on ServerVoteManager's
    // signatures). Values from the bodies; names [CHOSEN] where the ELF keeps none.
    enum VoteType
    {
        VTNone       = 0,
        VTMap        = 1,     // the map vote (ServerVoteManager +0x000)
        VTKickPlayer = 2,     // per team (+0x058 + 72*team; team 1 / 2)
        VTMutiny     = 3      // per team (+0x130 + 72*team)
    };
    enum VoteStatus                     // VoteEvent +0x14 - what the event announces
    {
        VSIssue           = 0,          // a new vote (castVote's first ballot; updateClientState)
        VSFailed          = 1,          // updateVote: too few for
        VSPrematureEnd    = 2,          // handlePrematureVoteEnd: target left / commander resigned
        VSPassed          = 3,          // updateVote: counts[0] >= votesNeeded
        VSUpdate          = 4,          // castVote: another ballot laid
        VSDiscardTarget   = 5,          // receiveKickVote: target not fully connected
        VSDiscardConflict = 6,          // castVote: map change pending / conflicting vote in progress
        VSTooFewPlayers   = 7,          // receiveVote: potential voters < minPlayersForVoting
        VSParticipation   = 8,          // updateVotingParticipationStatus
        VSClientState     = 9           // receiveVote: a client asks for the state (updateClientState)
    };
    enum VoteOption
    {
        VOFor      = 0,                 // counts[0] - compared against votesNeeded
        VOAgainst  = 1,                 // counts[1]
        VONotVoted = 2                  // updateClientState lays it for a late joiner
    };

    Vote();                                             //  (the 181 B twin is C1)
    ~Vote();

    std::vector<int>& getVoteCount();                   // sums on demand
    bool  voteComplete();
    void  update(float);                                // empty
    bool  layVote(int playerId, int option);
    int   getPlayerVote(int playerId);                  // 1 = none / not running
    void  issueVote(float time, int item, std::string itemName, int issuer,
                    int minOption, int maxOption, int votesNeeded, unsigned int potentialVoters);
    void  cancelVote();
    bool  voteInProgress();
    float timeRemaining();
    void  sumVotes();
    void  setMajority(float majority);
    float getMajority() const;

    bool                m_inProgress;                   // +0x00
    bool                m_enabled;                      // +0x01
    float               m_endTime;                      // +0x04
    int                 m_minOption;                    // +0x08
    int                 m_maxOption;                    // +0x0C
    float               m_majority;                     // +0x10
    std::map<int, int>  m_playerVotes;                  // +0x14 (+0x18 size)
    std::vector<int>    m_voteCounts;                   // +0x20
    bool                m_countsValid;                  // +0x2C
    int                 m_voteItem;                     // +0x30
    std::string         m_voteItemName;                 // +0x34
    int                 m_issuer;                       // +0x38
    int                 m_type;                         // +0x3C (VoteType)
    int                 m_votesNeeded;                  // +0x40
    int                 m_potentialVoters;              // +0x44
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMESERVER_VOTE_H
