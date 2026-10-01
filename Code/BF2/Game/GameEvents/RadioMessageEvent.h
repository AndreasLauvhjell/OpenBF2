// Bidirectional radio/voice event. The server relay replaces the sender id with
// the observed connection id. Channel uses five wire bits, type three and team two.
// Payload fields remain uninitialized until filled by a producer or deserializer.
// [UNRESOLVED] m_field0F is relayed as flags; its individual bits are not identified.

#ifndef GAME_GAMEEVENTS_RADIOMESSAGEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_RADIOMESSAGEEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class RadioMessageEvent : public GameEvent                   // 0x18
{
public:
    RadioMessageEvent() {}                                   // initialises nothing - see above
    virtual ~RadioMessageEvent() {}

    virtual int  getType() const;                            // GERadioMessage (20)
    virtual void executeClient();                            // dead on a server
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // public - GameServer.cpp's relay writes all six from outside the class.
    /* +0x0C */ unsigned char m_messageId;
    /* +0x0D */ unsigned char m_senderId;
    /* +0x0E */ unsigned char m_team;
    /* +0x0F */ unsigned char m_field0F;
    /* +0x10 */ int           m_channel;
    /* +0x14 */ int           m_type;
};

// Local radio-delivery node; type 13 is required by server enqueue and dispatch.
// [PLATFORM] EventNode padding differs between ABIs; access payload members by name.
class RadioMessageReceivedEvent : public EventNode
{
public:
    RadioMessageReceivedEvent(int connectionId,
                              unsigned char messageId, unsigned char senderId,
                              unsigned char team, unsigned char field0F,
                              int channel, int type)
        : m_connectionId(static_cast<unsigned char>(connectionId))
        , m_messageId(messageId)
        , m_senderId(senderId)
        , m_team(team)
        , m_field0F(field0F)
        , m_channel(channel)
        , m_type(type) {}

    virtual ~RadioMessageReceivedEvent() {}          //  / 0x808B7F0 - trivial pair
    virtual int getType() const { return 13; }

    /* +0x1C */ unsigned char m_connectionId;        // 1 on the client arm; the sender on the server
    /* +0x1D */ unsigned char m_messageId;
    /* +0x1E */ unsigned char m_senderId;
    /* +0x1F */ unsigned char m_team;
    /* +0x20 */ unsigned char m_field0F;
    /* +0x24 */ int           m_channel;
    /* +0x28 */ int           m_type;
};

// Kept as a factory because both call sites in the ELF inline `operator new(0x2Cu)` plus the field
// stores rather than calling a constructor symbol - there is no ctor to bind to. The signature is
// the one the two callers already use.
EventNode* createRadioMessageReceivedEvent(int connectionId,
                                           unsigned char messageId, unsigned char senderId,
                                           unsigned char team, unsigned char field0F,
                                           int channel, int type);

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_RADIOMESSAGEEVENT_H
