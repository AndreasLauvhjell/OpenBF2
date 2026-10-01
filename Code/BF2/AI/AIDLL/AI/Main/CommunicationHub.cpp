// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining CommunicationHub operations.

#include "CommunicationHub.h"

#include "AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

CommunicationHub* CommunicationHub::m_instance = 0;

// Constructs two empty strength vectors and publishes only the first hub instance.
// Call reset before using the three timestamps, which construction leaves uninitialized.
CommunicationHub::CommunicationHub()
{
    // m_time4, m_time12 and m_time44 deliberately not initialised - the binary writes none of
    // them. The two vectors default-construct on their own.
    if (!m_instance)
        m_instance = this;
}

// it does test `== this` before nulling the static, unlike `AerialHeightMap` one step earlier.
// The two vectors are then torn down in reverse, which is the compiler's own work.
CommunicationHub::~CommunicationHub()
{
    if (m_instance == this)
        m_instance = 0;
}

// [FRONTIER] Behavior methods stop until their strategic-object dependencies are implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void CommunicationHub::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationHub::reset");
}

void CommunicationHub::enemySpottedByAI(IAIPlayer*, IAIObject*, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationHub::enemySpottedByAI");
}

void CommunicationHub::enemySpottedByHuman(IAIObject*, unsigned char) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationHub::enemySpottedByHuman");
}

void CommunicationHub::filterAndSendAIRadioMessage(BotMain*, int, ChannelMessageType,
                                                   AIRadioChannelType, AIRadioMessageType,
                                                   unsigned char, int, unsigned char,
                                                   unsigned char)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationHub::filterAndSendAIRadioMessage");
}

void CommunicationHub::filterAndReceiveAIRadioMessage(const AIRadioMessageSettings&, int, int, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommunicationHub::filterAndReceiveAIRadioMessage");
}

} // namespace ai
} // namespace hfe
} // namespace dice
