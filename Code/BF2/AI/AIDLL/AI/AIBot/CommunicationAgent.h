// Per-bot radio message queue and timing state.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_COMMUNICATIONAGENT_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_COMMUNICATIONAGENT_H

#include <list>
#include <map>
#include <queue>
#include <vector>

namespace dice {
namespace hfe {
namespace ai {

class BotMain;
class CommunicationHub;
class Information;

// Both opaque, and declared this way elsewhere in the tree too - see BFConsole.h for why no
// enumerator name is recoverable from either binary.
enum AIRadioChannelType : int;
enum AIRadioMessageType : int;

// Radio-message record. [UNRESOLVED] Some fields are not yet identified.
struct AIRadioMessageInfo
{
    /*  0 */ unsigned char m_opaque0[4];   // [UNRESOLVED]
    /*  4 */ int m_senderPlayerId;
    /*  8 */ unsigned char m_opaque8[16];  // [UNRESOLVED]
};

class CommunicationAgent
{
public:
    // Queued priority message. The integer at +8 determines ordering.
    struct CAPrioMessage
    {
        // The whole of the ordering. A `std::priority_queue` with `std::less` is a MAX-heap, so
        // the largest value here is what the bot says next.
        bool operator<(const CAPrioMessage& rhs) const { return m_priority < rhs.m_priority; }

        /*  0 */ unsigned char m_opaque0[8];    // [UNRESOLVED]
        /*  8 */ int m_priority;
        /* 12 */ unsigned char m_opaque12[16];  // [UNRESOLVED]
    };

    explicit CommunicationAgent(BotMain* owner);
    // The implicit destructor releases member containers; no custom cleanup is required.

    void reset();
    void update();                                 // - [FRONTIER]

    // attested by a 7-byte body: `return m_receivedMessages.data + type;`
    const std::list<AIRadioMessageInfo>& getReceivedMessages(AIRadioMessageType type) const
    { return m_receivedMessages[type]; }

private:
    typedef std::priority_queue<CAPrioMessage, std::vector<CAPrioMessage>,
                                std::less<CAPrioMessage> > MessageQueue;

    /*   0 */ BotMain* m_bot;                  // the ctor argument, and the first store
    // the constructor never sets this - `reset` does, and the constructor's last statement is a
    // call to `reset`. Same shape as `CommunicationHub`'s own two uninitialised timestamps.
    /*   4 */ CommunicationHub* m_hub;         // = CommunicationHub::m_instance
    // one inbox per message type - eighteen lists, indexed by the enum. See the header block.
    /*   8 */ std::vector<std::list<AIRadioMessageInfo> > m_receivedMessages;
    /*  20 */ std::map<AIRadioMessageType, double> m_map20;   // a time per type [UNRESOLVED]
    // what the bot says next. See the header block.
    /*  32 */ MessageQueue m_queue;            // +32..+47: a 12-byte vector and an empty comparator
    // Eighteen doubles, one per type, zeroed by `reset` and paired with the max-send-period cache
    // below. [INFERRED] that these are the per-type last-sent timestamps.
    /*  48 */ std::vector<double> m_lastSentTime;
    /*  60 */ double m_double60;               // reset: 0.0
    // the two cached columns of `AIRadioMessages.ai`. Filled once, in the constructor.
    /*  68 */ std::vector<float> m_receiveTimeToLive;
    /*  80 */ std::vector<float> m_maxSendPeriod;
    /*  92 */ int m_int92;                     // reset: -1, this subsystem's null-index convention
    /*  96 */ double m_double96;               // reset: 0.0
    /* 104 */ double m_double104;              // reset: 0.0
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_COMMUNICATIONAGENT_H
