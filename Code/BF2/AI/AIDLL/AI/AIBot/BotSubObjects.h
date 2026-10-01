// Aim-correction state used by BotMain.

#ifndef AI_AIDLL_AI_AIBOT_BOTSUBOBJECTS_H
#define AI_AIDLL_AI_AIBOT_BOTSUBOBJECTS_H

namespace dice {
namespace hfe {
namespace ai {

class BotMain;

// Aim-correction state; construction leaves the first four bytes uninitialized.
class FireCorrectionData
{
public:
    FireCorrectionData();                       // for the class

private:
    /*  0 */ char m_gap0[4];                    // [UNRESOLVED] - not initialised by the ctor
    /*  4 */ bool m_flag4;                      // = true
    /*  5 */ bool m_flag5;                      // = true
    /*  6 */ char m_pad6[2];
    /*  8 */ int  m_zero8[15];                  // +8..+64, all zeroed
    /* 68 */ char m_gap68[16];                  // [UNRESOLVED] - the ctor skips +68..+83
    /* 84 */ bool m_flag84;                     // = false
    /* 85 */ char m_pad85[3];
    /* 88 */ int  m_zero88[9];                  // +88..+120, all zeroed
    /* 124 */ char m_gap124[4];                 // [UNRESOLVED] - 128 - 124
};


} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTSUBOBJECTS_H
