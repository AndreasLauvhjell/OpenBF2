// Path:     [INFERRED] Game/GameServer/Vote.cpp - see Vote.h. Contents: recovered from bf2.i64
//           (ELF, symbolised); every body

#include "Vote.h"

#include "../../Core/System.h"      // g_system->getExactTime() - the vote clock (System +0x18)

namespace dice {
namespace hfe {

// Leaves m_countsValid, m_votesNeeded, and m_potentialVoters uninitialized, as in the original.
// Initialize vote state through issueVote before querying it.
Vote::Vote()
    : m_inProgress(false)
    , m_enabled(true)
    , m_endTime(-1.0f)
    , m_minOption(0)
    , m_maxOption(1)
    , m_majority(0.6666667f)
    , m_voteItem(0)
    , m_issuer(0)
    , m_type(VTMap)
{
}

// : string, vector, map - member order in reverse, as the compiler emits.
Vote::~Vote()
{
}

std::vector<int>& Vote::getVoteCount()
{
    if (!m_countsValid)
        sumVotes();
    return m_voteCounts;
}

// : everyone who could vote has, OR the vote is running and its clock ran out.
bool Vote::voteComplete()
{
    std::vector<int>& counts = getVoteCount();
    if (counts[1] + counts[0] >= m_potentialVoters)
        return true;
    if (m_inProgress)
        return !voteInProgress();
    return false;
}

// empty.
void Vote::update(float)
{
}

// : only while running and only an option inside [min, max]; insert_unique - a
// second ballot from the same player is silently ignored (the first stands) but still returns
// true and still invalidates the tally.
bool Vote::layVote(int playerId, int option)
{
    if (!m_inProgress || option > m_maxOption || option < m_minOption)
        return false;
    m_playerVotes.insert(std::pair<const int, int>(playerId, option));
    m_countsValid = false;
    return true;
}

// : -1 when no vote is running or the player has not voted; otherwise the option.
// The lookup is find() followed by operator[] on a hit - the shipped double walk.
int Vote::getPlayerVote(int playerId)
{
    if (!m_inProgress)
        return -1;
    if (m_playerVotes.find(playerId) == m_playerVotes.end())
        return -1;
    return m_playerVotes[playerId];
}

// : the argument-to-field map is in the header's layout table. The end time is
// the exact clock plus `time` (getVotingTime()); the ballots and the tally are cleared.
void Vote::issueVote(float time, int item, std::string itemName, int issuer,
                     int minOption, int maxOption, int votesNeeded, unsigned int potentialVoters)
{
    m_minOption = minOption;                        // +0x08
    m_maxOption = maxOption;                        // +0x0C
    m_voteItem  = item;                             // +0x30
    m_voteItemName.assign(itemName);                // +0x34
    m_issuer    = issuer;                           // +0x38
    m_playerVotes.clear();
    m_inProgress = true;
    m_voteCounts.erase(m_voteCounts.begin(), m_voteCounts.end());
    m_countsValid = false;
    m_votesNeeded = votesNeeded;                    // +0x40
    m_endTime = static_cast<float>(g_system->getExactTime() + time); // +0x04; float spill
    m_potentialVoters = (int)potentialVoters;       // +0x44
}

// : end time 0, not running, tally emptied and invalidated. The ballots stay
// (issueVote clears them next time).
void Vote::cancelVote()
{
    m_endTime = 0.0f;
    m_inProgress = false;
    m_voteCounts.erase(m_voteCounts.begin(), m_voteCounts.end());
    m_countsValid = false;
}

// : the clock has not reached the end time. Independent of m_inProgress.
bool Vote::voteInProgress()
{
    const float now = static_cast<float>(g_system->getExactTime());
    return m_endTime > now;
}

float Vote::timeRemaining()
{
    const float now = static_cast<float>(g_system->getExactTime());
    return m_endTime - now;
}

// Rebuilds the tally using each ballot's option as a direct index; supported callers use minimum option zero.
void Vote::sumVotes()
{
    m_voteCounts.assign((size_t)(m_maxOption - m_minOption + 1), 0);
    for (std::map<int, int>::iterator it = m_playerVotes.begin(); it != m_playerVotes.end(); ++it)
        ++m_voteCounts[it->second];
    m_countsValid = true;
}

void  Vote::setMajority(float majority) { m_majority = majority; }
float Vote::getMajority() const         { return m_majority; }

} // namespace hfe
} // namespace dice
