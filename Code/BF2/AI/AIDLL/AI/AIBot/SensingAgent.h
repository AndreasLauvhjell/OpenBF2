// Owns and executes a bot's sensing plan.

#ifndef AI_AIDLL_AI_AIBOT_SENSINGAGENT_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_SENSINGAGENT_H

#include <vector>

#include "../../../../Math/Vec3.h"     // Vec2 and Vec3 live here, and both are typedefs

namespace dice {
namespace hfe {
namespace ai {

class BAPContext;
// the plan'S base class. `SensingAgent` holds one `BAPStatement*` and nothing else about the
// statement tree; see the deletePlan block in the .cpp for how far the evidence goes.
class BAPStatement;
class BotMain;
class BotSenses;
class Information;
class ISensingAgentPattern;

class SensingAgent
{
public:
    // The second parameter is in the ABI and is never read - see the header block.
    SensingAgent(BotMain* owner, unsigned char behaviourCount);
    ~SensingAgent();                            // [FRONTIER]

    // [UNRESOLVED] Methods requiring unknown nested-type layouts remain undeclared.
    void reset();
    // Called by `BotMain::reset` when a bot loses its active plan - losing a plan releases the
    // bot's aim override.
    void setOverride(bool);
    void deletePlan();                          // - (one arm frontier)
    void clearPlanTargetInformation();
    // attested as a nested type by `std::vector<SensingAgent::FocusInformation>`; its members are
    // [UNRESOLVED] - nothing recovered so far constructs one.
    struct FocusInformation;

private:
    /*   0 */ int m_zero0[6];              // +0..+20, all 0
    // neither is A gap - the constructor skips them and `reset` writes both. A field the
    // constructor misses is not necessarily unused; check the reset before calling it dead.
    /*  24 */ int m_int24;                 // reset: -1
    /*  28 */ int m_int28;                 // reset: 0
    /*  32 */ int m_zero32[3];             // +32, +36, +40
    /*  44 */ bool m_flag44;               // reset: false
    // the eight-quadrant weight ring - the constructor's only container, and the only thing it
    // sizes. Eight zeroed floats; see the header block.
    /*  48 */ std::vector<float> m_quadrantWeights;
    // [INFERRED] Plan-target handle, cleared to zero and passed to BotMain when checking
    // plan invalidation; the neighboring words are also cleared.
    /*  60 */ unsigned int m_planTarget60;  // clearPlanTargetInformation: 0
    /*  64 */ int m_zero64[3];             // +64, +68, +72
    /*  76 */ unsigned int m_planTarget76;  // clearPlanTargetInformation: 0
    /*  80 */ int m_zero80[5];             // +80..+96
    /* 100 */ char m_gap100[4];            // [UNRESOLVED] - never written
    /* 104 */ BotSenses* m_senses;         // new(0x94) - the mechanism this agent steers
    /* 108 */ BotMain*   m_bot;            // the ctor argument
    // the whole tail is `reset`'s, not the constructor's - see the note at +24.
    /* 112 */ double m_double112;          // reset: the game time at reset
    /* 120 */ bool m_flag120;              // reset: false
    /* 121 */ bool m_flag121;              // reset: false
    /* 122 */ bool m_flag122;              // reset: false
    /* 123 */ char m_pad123;
    /* 124 */ double m_double124;          // reset: 0.0
    /* 132 */ bool m_flag132;              // reset: false
    /* 133 */ bool m_flag133;              // reset: false
    /* 134 */ bool m_flag134;              // ctor: false
    /* 135 */ char m_pad135;
    // the plan itself - one owned statement, guarded everywhere it is touched.
    /* 136 */ BAPStatement* m_plan;        // ctor: 0
    // Set true by `reset` and again by `deletePlan`, and nothing recovered so far reads it.
    // Meaning [UNRESOLVED]; the name stays neutral rather than guess at `m_planInvalid`.
    /* 140 */ bool m_flag140;              // reset: true, deletePlan: true
    /* 141 */ char m_pad141[3];
    /* 144 */ BAPContext* m_bapContext;    // new(4) - shared vocabulary with BotMain's own
    /* 148 */ bool m_flag148;              // reset: true
    /* 149 */ char m_pad149[3];
    /* 152 */ float m_reactionTime;        // reset: a random float in [0.5, 1.0) - see the .cpp
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SENSINGAGENT_H
