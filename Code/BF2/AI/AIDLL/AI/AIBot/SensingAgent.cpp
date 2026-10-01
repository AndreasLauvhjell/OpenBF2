// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SensingAgent operations.

#include "SensingAgent.h"

#include "BotSenses.h"
#include "../AIPlanInterpreter/BAPContext.h"
#include "SensingAgentPatterns.h"           // ISensingAgentPattern::reset - the shared patterns
#include "../Main/AIRandom.h"               // the per-life reaction time
#include "../Main/AISettingsInterfaces.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"   // IBFTimer::getGameTime
#include <map>
#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Creates sensing state and an eight-sector attention vector for the bot.
SensingAgent::SensingAgent(BotMain* owner, unsigned char /*behaviourCount*/)
    : m_flag134(false)
    , m_plan(0)
{
    for (int i = 0; i < 6; ++i)
        m_zero0[i] = 0;                    // +0..+20
    for (int i = 0; i < 3; ++i)
    {
        m_zero32[i] = 0;                   // +32, +36, +40
        m_zero64[i] = 0;                   // +64, +68, +72
    }
    for (int i = 0; i < 5; ++i)
        m_zero80[i] = 0;                   // +80..+96

    // the mechanism this agent steers. , and it takes the bot rather than the agent
    // so `BotSenses` answers to the bot directly and this object is a policy layer over it.
    m_senses = checkPointer(new BotSenses(owner));
    m_bot    = owner;

    m_bapContext = checkPointer(new BAPContext());

    // eight quadrants, zeroed. A live resize; see the block above.
    m_quadrantWeights.resize(8, 0.0f);

    // the last statement, and it is of its own. A sensing agent is only half built
    // until `reset` has run - the same constructor/reset split `BotMain` uses.
    reset();
}

// [FRONTIER] Remaining sensing-plan operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

SensingAgent::~SensingAgent()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgent::~SensingAgent");
}

// Deletes the owned plan and marks planning state for refresh.
// [FRONTIER] Plan destruction stops until the required statement type is implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void SensingAgent::deletePlan()
{
    if (m_plan)
    {
        // Retail here is `delete m_plan;` - vtable byte-slot +8, the deleting destructor. See the
        // block above for why this tree cannot write it yet.
        BF2_AI_FRONTIER("dice::hfe::ai::SensingAgent::deletePlan (non-null plan)");
        m_plan = 0;
    }
    m_flag140 = true;
}

// Clears the current plan's target information.
void SensingAgent::clearPlanTargetInformation()
{
    m_planTarget60 = 0;
    m_planTarget76 = 0;
}

// Resets sensing state and draws a new reaction delay in [0.5, 1.0).
void SensingAgent::reset()
{
    m_flag120 = false;
    m_flag121 = false;
    m_double112 = 0.0;
    m_flag122 = false;
    m_flag132 = false;
    m_flag140 = true;
    m_flag133 = false;

    // the shared patterns, RE-indexed. See the block above.
    IAISettings* const settings =
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    const std::map<int, ISensingAgentPattern*>& patterns = settings->getSensingAgentPatterns();

    int index = 0;
    for (std::map<int, ISensingAgentPattern*>::const_iterator it = patterns.begin();
         it != patterns.end(); ++it)
    {
        it->second->reset(index++);
    }

    deletePlan();

    m_int24  = -1;
    m_int28  = 0;
    m_flag44 = false;
    m_flag148 = true;
    m_zero80[0] = 0;                       // +80
    m_zero80[1] = 0;                       // +84

    // through the buffer, and the size is not touched. See the block above.
    for (unsigned q = 0; q <= 7; ++q)
        m_quadrantWeights[q] = 0.0f;

    clearPlanTargetInformation();

    // game time, not wall-clock.
    m_double112 =
        static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    m_double124 = 0.0;

    // Chooses a reaction time in [0.5, 1.0), preserving the arithmetic order.
    m_reactionTime = 0.5f + AIRandom::m_instance->nextFloat() * (1.0f - 0.5f);

    m_senses->reset();
}

// Ignores the override while m_flag132 is set. Otherwise stores the requested value
// and the current game time. reset clears the refusal flag.
void SensingAgent::setOverride(bool value)
{
    // The refusal. See the block above.
    if (!m_flag132)
    {
        m_flag122 = value;
        m_double124 =
            static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    }
}

} // namespace ai
} // namespace hfe
} // namespace dice
