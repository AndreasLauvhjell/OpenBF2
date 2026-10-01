// Sends the client's miscellaneous, archive and level digests to the server.
// Each digest is exactly 16 bytes; server execution forwards them to the connection's
// content-check handler. Client execution uses the inherited no-op.

#ifndef GAME_GAMEEVENTS_CONTENTCHECKEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_CONTENTCHECKEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

} }                                              // (the include below reopens dice::hfe)
#include "../../IO/ChecksumContext.h"            // the layout the block above
                                                 // inferred is the recovered class; both halves of
                                                 // the checksum are written there now.
namespace dice { namespace hfe {

class ContentCheckEvent : public GameEvent                   // 0x3C
{
public:
    enum { HASH_SIZE = 16 };

    explicit ContentCheckEvent(const io::ChecksumContext* context);
    ContentCheckEvent();                                     // the maker's form
    virtual ~ContentCheckEvent() {}

    virtual int  getType() const;                            // GEContentCheck (46)
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeClient and logInfo are deliberately NOT declared - both slots are inherited.

protected:
    /* +0x0C */ unsigned char m_miscHash[HASH_SIZE];
    /* +0x1C */ unsigned char m_archiveHash[HASH_SIZE];
    /* +0x2C */ unsigned char m_levelHash[HASH_SIZE];
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_CONTENTCHECKEVENT_H
