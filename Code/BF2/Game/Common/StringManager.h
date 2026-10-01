// Chunks and reassembles chat strings with their routing header.
// Server receipt validates sender/type/channel before posting a completed-message event.
// Keep worker virtuals before the destructor and call them through the typed interface.

#ifndef GAME_COMMON_STRINGMANAGER_H     // [INFERRED] guard style
#define GAME_COMMON_STRINGMANAGER_H

#include <deque>
#include <string>

#include "../../Core/EventManager.h"       // EventNode - StringReceivedEvent derives from it
#include "../IGame.h"                      // TargetChannel, ChannelMessageType

namespace dice {
namespace hfe {

class GameEventManager;

// Text buffer with a trailing twelve-byte header. m_size counts text and header in
// one byte; callers must keep text below 244 bytes because enqueueString does not
// prevent size wrap. Sending and receiving release the buffer through different owners.
struct StringDataBlock
{
    /* +0x00 */ unsigned char  m_size;
    /* +0x01 */ unsigned char  m_position;
    /* +0x02 */ unsigned char  m_pad[2];
    /* +0x04 */ unsigned char* m_data;
    /* +0x08 */ unsigned char* m_header;
};

// Twelve-byte string header. Sender ID and flags are bytes; -1 sender is encoded
// as zero and larger IDs truncate. [UNRESOLVED] m_unknown9's meaning.
struct StringDataHeader
{
    /* +0x00 */ int           m_channel;
    /* +0x04 */ int           m_type;
    /* +0x08 */ unsigned char m_senderId;
    /* +0x09 */ unsigned char m_unknown9;      // [UNRESOLVED]
    /* +0x0A */ unsigned char m_flags;
    /* +0x0B */ unsigned char m_pad;
};

// Completed-string event owning the entire received buffer, including its trailing
// header. Consumers use m_messageLength to exclude that header. Dispatched as
// ECNetwork event 6 with sender, channel, and flags for chat handling.
class StringReceivedEvent : public EventNode
{
public:
    // Server-local chat preserves full dwords; received wire bytes widen at their caller.
    // [INFERRED] parameter signedness from say's mangled unsigned parameters; width is observed.
    // Inlined GameServer::say: ELF 0x80AF021..0x80AF02A / PE 0x4435EE..0x4435FA.
    StringReceivedEvent(int senderId, unsigned int targetId, unsigned int flags,
                        int channel, int type,
                        unsigned int messageLength, unsigned char* message);
    virtual ~StringReceivedEvent();
    virtual int getType() const;                             // : returns 11

    /* +0x1C */ int            m_senderId;
    /* +0x20 */ int            m_targetId;    // header +0x09, widened to a dword -
                                              //   GameServer.cpp's StringMessageEW::getTargetId (0x80B9860)
                                              //   reads it whole and the server re-broadcast hands it to
                                              //   Game::outputChatString as the target. [INFERRED name]
    /* +0x24 */ int            m_flags;       // header +0x0A, widened to a dword - Python arg 4, and the
                                              //   target player ID on the Player channel (adminToPlayerEvent
                                              //   compares it with each receiver's id, 0x80AEB27)
    /* +0x28 */ int            m_channel;     // header +0x00
    /* +0x2C */ int            m_type;        // header +0x04
    /* +0x30 */ unsigned int   m_messageLength;
    /* +0x34 */ unsigned char* m_message;     // owned - see above
};

// Queues and reassembles chat blocks through the supplied event manager. Chat
// retains the ordinary three-chunks-per-tick limit even during demo recording.
class StringManager
{
public:
    StringManager(GameEventManager* gameEvents);

    // slot order - these four are declared before the destructor; see the header note.
    virtual bool enqueueString(TargetChannel channel, ChannelMessageType type, int senderId,
                               const std::string& text,
                               int unknown9, int flags);               // +0x00
    virtual void update();                                             // +0x04
    virtual bool newStringDataBlock(unsigned int size);                // +0x08
    virtual bool addStringDataChunk(void* chunk, int length,
                                    bool trustSenderId);               // +0x0C

    virtual ~StringManager();

public:
    /* +0x004 */ std::deque<StringDataBlock*> m_pending;
    /* +0x02C */ StringDataBlock*             m_sending;
    /* +0x030 */ StringDataBlock*             m_receiving;
    /* +0x034 */ GameEventManager*            m_gameEvents;
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_STRINGMANAGER_H
