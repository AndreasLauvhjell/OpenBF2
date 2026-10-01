// Per-bot behavior urgency state. BotMain initializes m_double16 after construction.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOT_BOTMEMORY_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_BOTMEMORY_H

namespace dice {
namespace hfe {
namespace ai {

class BotBehaviour;

class BotMemory
{
public:
    BotMemory();
    ~BotMemory();

    void reset();                   // [FRONTIER]
    // Adjusts the urgency assigned to behaviour by amount.
    void modifyBehaviourUrgency(BotBehaviour* behaviour, double amount);   // [FRONTIER]

    // Retail x86 field offsets.
    /*  0 */ int    m_int0;         // = -1 and BotMain writes it again from outside
    /*  4 */ int    m_int4;         // = -1
    /*  8 */ float  m_float8;       // = 1.0f
    /* 12 */ int    m_int12;        // = 1
    /* 16 */ double m_double16;     // Initialized by BotMain after construction.
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTMEMORY_H
