// Start a bulk string transfer or append a chunk of at most 63 bytes. Oversize
// construction is fatal. Clients trust the embedded sender; servers replace it
// with the connection id and reject server-only chat channels.
// [UNRESOLVED] The original buffer declaration may include padding beyond 63
// bytes; the operational chunk limit is 63.

#ifndef GAME_GAMEEVENTS_STRINGBLOCKEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_STRINGBLOCKEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class StringBlockEvent : public GameEvent                    // 0x50
{
public:
    enum { MAX_CHUNK_SIZE = 63 };

    // attested: _ZN4dice3hfe16StringBlockEventC2EPvh is (void*, unsigned char). Builds the chunk
    // form. Aborts if size exceeds MAX_CHUNK_SIZE - see the header block.
    StringBlockEvent(void* data, unsigned char size);
    StringBlockEvent();                                      // the maker's form

    // Announces the total string size without a payload; data size remains zero.
    explicit StringBlockEvent(unsigned char stringSize);     // inlined, no symbol
    virtual ~StringBlockEvent() {}

    virtual int  getType() const;                            // GEStringBlock (15)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ bool          m_newString;
    /* +0x0D */ unsigned char m_stringSize;
    /* +0x0E */ unsigned char m_dataSize;
    /* +0x0F */ unsigned char m_dataBuffer[MAX_CHUNK_SIZE];
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_STRINGBLOCKEVENT_H
