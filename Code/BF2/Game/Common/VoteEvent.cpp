// Path:     [INFERRED] Game/Common/VoteEvent.cpp - see VoteEvent.h. Contents: recovered from
//           bf2.i64 (ELF, symbolised); every body

#include "VoteEvent.h"

#include "ClientVoteManager.h"                    // executeClient's shipped empty sink
#include "../IGame.h"                              // game - isPlayingDemo (+0xE0)
#include "../GameServer/ServerVoteManager.h"       // executeServer -> receiveVote
#include "../../IO/BitStream.h"

namespace dice {
namespace hfe {

// The GE type id - .rodata dword GEVote = 39 (VoteEvent::getType 0x8082080).
namespace {
const int GEVote = 39;

} // namespace

int VoteEvent::getType() const
{
    return GEVote;
}

//  / 0x8081BE0, 25 B each: an id of -1 travels as 255 (the byte-wide "none").
void VoteEvent::setVoteCaster(int playerId)
{
    m_voteCaster = (unsigned char)(playerId == -1 ? 255 : playerId);
}

void VoteEvent::setVoteItem(int item)
{
    m_voteItem = (unsigned char)(item == -1 ? 255 : item);
}

// : not while a demo plays; hand it to the client vote manager.
void VoteEvent::executeClient()
{
    if (!game->isPlayingDemo())
        ClientVoteManager::getInstance()->receiveVote(*this);
}

// : the server half - the connection id is the caster's player id.
bool VoteEvent::executeServer(int playerId)
{
    ServerVoteManager::getInstance()->receiveVote(playerId, *this);
    return true;
}

// the wire, read side (VoteEvent.h). Every 8-bit field is read into a zeroed
// int and narrowed to its member; the time is 32 raw bits straight into +0x1C.
bool VoteEvent::deSerialize(io::BitStream* stream)
{
    int v = 0;
    stream->readBits(&v, 8);  m_voteItem     = (unsigned char)v;
    v = 0; stream->readBits(&v, 8);  m_voteOption   = v;
    v = 0; stream->readBits(&v, 8);  m_votesFor     = (unsigned char)v;
    v = 0; stream->readBits(&v, 8);  m_voteItem     = (unsigned char)v;    // twice, as shipped
    v = 0; stream->readBits(&v, 8);  m_votesAgainst = (unsigned char)v;
    v = 0; stream->readBits(&v, 8);  m_votesNeeded  = (unsigned char)v;
    v = 0; stream->readBits(&v, 8);  m_voteCaster   = (unsigned char)v;
    v = 0; stream->readBits(&v, 8);  m_voteStatus   = v;
    v = 0; stream->readBits(&v, 8);  m_voteType     = v;
    stream->readBits(&m_time, 32);
    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// the write side: getType() in the registry's type-id width first, then the
// same ten fields in the same order (each 8-bit field widened to an int first).
bool VoteEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());   // 7-bit registered-event width
    int v;
    v = m_voteItem;      stream->writeBits(&v, 8);
    v = m_voteOption;    stream->writeBits(&v, 8);
    v = m_votesFor;      stream->writeBits(&v, 8);
    v = m_voteItem;      stream->writeBits(&v, 8);         // twice, as shipped
    v = m_votesAgainst;  stream->writeBits(&v, 8);
    v = m_votesNeeded;   stream->writeBits(&v, 8);
    v = m_voteCaster;    stream->writeBits(&v, 8);
    v = m_voteStatus;    stream->writeBits(&v, 8);
    v = m_voteType;      stream->writeBits(&v, 8);
    stream->writeBits(&m_time, 32);
    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
