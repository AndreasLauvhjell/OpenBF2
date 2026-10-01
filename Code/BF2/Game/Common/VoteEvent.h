// Vote payload exchanged by server and client vote managers. The default event
// requires its fields to be filled before serialization or execution.

#ifndef GAME_COMMON_VOTEEVENT_H     // [INFERRED] guard style
#define GAME_COMMON_VOTEEVENT_H

#include "GameEvent.h"

namespace dice {
namespace hfe {

class VoteEvent : public GameEvent                     // 0x24
{
public:
    VoteEvent() {}                                     // the inline vtable-only sequence (see above)
    virtual ~VoteEvent() {}                            //  / 0x8082050 - vptr + GameEvent's

    virtual int  getType() const;                      // GEVote (39)
    virtual void executeClient();                      // the client arm
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    void setVoteCaster(int playerId);                  // 1 becomes 255
    void setVoteItem(int item);                        // 1 becomes 255

    int           m_voteOption;                        // +0x0C
    unsigned char m_votesFor;                          // +0x10
    unsigned char m_votesAgainst;                      // +0x11
    unsigned char m_votesNeeded;                       // +0x12
    int           m_voteStatus;                        // +0x14
    int           m_voteType;                          // +0x18
    float         m_time;                              // +0x1C
    unsigned char m_voteItem;                          // +0x20
    unsigned char m_voteCaster;                        // +0x21
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_VOTEEVENT_H
