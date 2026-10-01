// Transfers data in chunks of at most 63 bytes through a connection's DataBlockManager.
// The leading bit selects either a new-transfer ID/length or a payload chunk;
// fields from the other form are neither sent nor updated. Both directions are supported.
// An oversized chunk passed to the constructor is a fatal error.

#ifndef GAME_GAMEEVENTS_DATABLOCKEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_DATABLOCKEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class DataBlockEvent : public GameEvent                      // 0x58
{
public:
    enum { MAX_CHUNK_SIZE = 63 };

    // attested: _ZN4dice3hfe14DataBlockEventC2EPvj is (void*, unsigned int). Builds the chunk form
    // (m_isNewBlock = false). Aborts if n exceeds MAX_CHUNK_SIZE - see the header block.
    DataBlockEvent(void* data, unsigned int size);
    DataBlockEvent();                                        // the maker's form

    // Announces block ID and total length without a payload so the receiver can
    // allocate its assembly buffer.
    DataBlockEvent(unsigned int blockEventId, unsigned int blockLength);   // inlined, no symbol
    virtual ~DataBlockEvent() {}

    virtual int  getType() const;                            // GEDataBlock (4)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ bool          m_isNewBlock;
    /* +0x0D */ // three bytes of alignment padding
    /* +0x10 */ unsigned int  m_blockEventId;
    /* +0x14 */ unsigned int  m_blockLength;
    /* +0x18 */ unsigned char m_size;
    /* +0x19 */ unsigned char m_data[MAX_CHUNK_SIZE];
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_DATABLOCKEVENT_H
