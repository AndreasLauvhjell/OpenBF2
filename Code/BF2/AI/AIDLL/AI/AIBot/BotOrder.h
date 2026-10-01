// Order data passed to a bot.
// [INFERRED] Source placement.
#ifndef AI_AIDLL_AI_AIBOT_BOTORDER_H
#define AI_AIDLL_AI_AIBOT_BOTORDER_H

namespace dice { namespace hfe { namespace ai {

class BotOrder
{
public:
    BotOrder(float* weights, unsigned char behaviour)
    {
        m_behaviour = behaviour;
        m_weights = weights;
    }

    /* +0 */ float* m_weights;
    /* +4 */ unsigned char m_behaviour;
};

} } }

#endif
