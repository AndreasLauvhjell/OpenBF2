// Lazy server vote manager for map, kick and mutiny votes. PlayerManager must
// exist before ticks do work. Successful map votes change the map after five seconds.
// Votes reject self-voting and duplicate ballots; kicks are queued until the game
// logic drains them. Team votes use teams 1 and 2.

#ifndef GAME_GAMESERVER_SERVERVOTEMANAGER_H     // [INFERRED] guard style
#define GAME_GAMESERVER_SERVERVOTEMANAGER_H

#include <list>

#include "Vote.h"

namespace dice {
namespace hfe {

class VoteEvent;

class ServerVoteManager
{
public:
    static ServerVoteManager* getInstance();
    static void release();

    ServerVoteManager();
    // ~ServerVoteManager: implicit in the ELF (no symbol) - release() runs the members' dtors
    // inline (list clear + header free, the seven Votes in reverse) and operator delete.

    void  reset();
    int   getNumberOfPotentialVoters(Vote::VoteType type, int team, int item);
    int   calcVotesNeeded(Vote::VoteType type, Vote& vote, int team, int item);
    void  setVoteMapMajority(float majority);
    float getVoteMapMajority() const;
    void  setVoteKickPlayerMajority(float majority, int team);
    float getVoteKickPlayerMajority(int team) const;
    void  setEnableMapVote(bool enable);
    bool  getEnableMapVote() const;
    void  setEnableKickPlayerVote(bool enable, int team);
    bool  getEnableKickPlayerVote(int team) const;
    float getVotingTime() const;
    int   getMinPlayersForIssue() const;
    bool  getTeamVoteOnly() const;
    void  updateKickPlayers();
    void  kickPlayer(int playerId, bool);                                     // the
                                                                              //   bool is unused
    void  changeMap(int mapIndex);
    bool  voteInProgressConflictsWithNewIssue(Vote::VoteType type, int team);
    bool  castVote(Vote::VoteType type, Vote::VoteStatus status, int playerId, int item,
                   Vote::VoteOption option, int team);
    Vote* getVoteObjectFromType(Vote::VoteType type, int team);
    void  update(float dt);
    bool  handlePrematureVoteEnd(Vote& vote);
    void  updateVote(Vote& vote, float dt);
    void  discardVote(int playerId, Vote::VoteType type, int item, Vote::VoteStatus status);
    void  receiveMapVote(int playerId, const VoteEvent& event);
    void  receiveMutinyVote(int playerId, const VoteEvent& event);
    void  receiveKickVote(int playerId, const VoteEvent& event);
    void  updateVotingParticipationStatus(int playerId, Vote::VoteOption option);
    void  updateClientState(int playerId);
    void  receiveVote(int playerId, const VoteEvent& event);

    Vote            m_mapVote;                     // +0x000
    bool            m_mapChangePending;            // +0x048
    int             m_nextMapIndex;                // +0x04C
    double          m_mapChangeRequestTime;        // +0x050
    Vote            m_kickVotes[3];                // +0x058
    Vote            m_mutinyVotes[3];              // +0x130
    std::list<int>  m_playersToKick;               // +0x208

private:
    static ServerVoteManager* _instance;           // ds:0x8AACAC0
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMESERVER_SERVERVOTEMANAGER_H
