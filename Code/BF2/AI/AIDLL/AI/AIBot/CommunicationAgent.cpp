// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining CommunicationAgent operations.

#include "CommunicationAgent.h"

#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/CommunicationHub.h"
#include "../../../AIInterface/Module/BFConsole.h"   // IBFModuleCourier, AIRadioMessageSettings

namespace dice {
namespace hfe {
namespace ai {

// Caches voice timing settings for the eighteen radio message types.
CommunicationAgent::CommunicationAgent(BotMain* owner)
    : m_bot(owner)
{
    // eighteen. See the header block.
    m_receivedMessages.resize(18);
    m_receiveTimeToLive.resize(18);
    m_maxSendPeriod.resize(18);

    IBFModuleCourier* const courier =
        static_cast<IBFModuleCourier*>(CommonSingletonServer::m_iBFModuleCourier);

    for (unsigned int type = 0; type <= 17; ++type)
    {
        int voice;
        if (courier->getVoiceFromAiMessageType(static_cast<AIRadioMessageType>(type), voice))
        {
            AIRadioMessageSettings settings;
            courier->getAIMessageSettings(voice, settings);
            // Crossed, and in this order - see the block above.
            m_receiveTimeToLive[type] = settings.m_receiveTimeToLive;
            m_maxSendPeriod[type]     = settings.m_maxSendPeriod;
        }
        else
        {
            m_receiveTimeToLive[type] = 0.0f;
            m_maxSendPeriod[type]     = 0.0f;
        }
    }

    reset();
}

// Resets message state and acquires the shared communication hub.
void CommunicationAgent::reset()
{
    // The shared hub, re-read on every reset. See the block above.
    m_hub = CommunicationHub::m_instance;

    m_lastSentTime.resize(0);
    m_lastSentTime.resize(18);

    for (unsigned int type = 0; type <= 17; ++type)
        m_receivedMessages[type].clear();

    m_map20.clear();

    // No clear on a priority_queue - see the block above.
    while (!m_queue.empty())
        m_queue.pop();

    m_double60 = 0.0;
    m_int92 = -1;
    // This order is the binary's.
    m_double104 = 0.0;
    m_double96 = 0.0;
}

// [FRONTIER] Remaining message operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

// [FRONTIER] Services one bot's radio on the BotManager communication tick.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when radio update behavior is recovered.
void CommunicationAgent::update()
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationAgent::update");
}

} // namespace ai
} // namespace hfe
} // namespace dice
