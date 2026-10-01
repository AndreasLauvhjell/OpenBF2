// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining BFRadioComm operations.

#include "BFRadioComm.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../../Core/ClassManager.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IBFRadioComm::IBFRadioComm()
{
    if (!CommonSingletonServer::m_iBFRadioComm)
        classManager->registerSimpleSingleton(std::string("IBFRadioComm"), this);
}

IBFRadioComm::~IBFRadioComm()
{
    if (CommonSingletonServer::m_iBFRadioComm == this)
        classManager->unregisterSimpleSingleton(std::string("IBFRadioComm"));
}

BFRadioComm::BFRadioComm()
{
}

BFRadioComm::~BFRadioComm()
{
}

// - the single virtual, and the AI's only way to speak.
int BFRadioComm::sendAIRadioMessage(int playerId, ChannelMessageType channel, AIRadioChannelType type, unsigned char a, int b, unsigned char c, unsigned char d)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFRadioComm::sendAIRadioMessage");
}

} // namespace ai
} // namespace hfe
} // namespace dice
