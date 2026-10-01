// Combines per-behavior urgency generators for a bot.
// [FRONTIER] Remaining merger behavior requires implementation.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AIBOTBEHAVIOUR_URGENCYMERGER_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOTBEHAVIOUR_URGENCYMERGER_H

namespace dice {
namespace hfe {
namespace ai {

class Bot;

// UrgencyGenerator - the abstract base. vtable off_4B6E88: , , then nine pure virtuals.
// Its destructor body is one instruction - the vptr store C++ emits - so the class
// owns nothing. It is an interface with a virtual destructor and no state.
class UrgencyGenerator
{
public:
    virtual ~UrgencyGenerator();

    virtual float getUrgency(Bot* bot) = 0;                         // +0x08
    virtual float getActiveUrgency(Bot* bot) = 0;                   // +0x0C
    virtual void  updateActiveUrgency(Bot* bot) = 0;                // +0x10
    virtual bool  hasChangedTarget(Bot* bot) = 0;                   // +0x14
    virtual bool  quotientChanged(Bot* bot, float a, float b) = 0;  // +0x18
    virtual bool  hasChanged(Bot* bot, float a, float b) = 0;       // +0x1C
    virtual UrgencyGenerator* getChosenGenerator() = 0;             // +0x20
    virtual void  reset(Bot* bot) = 0;                              // +0x24
    virtual int   getSize() = 0;                                    // +0x28
};

// UrgencyMerger - (`operator new(0x14)` in BotManager's loop).
class UrgencyMerger : public UrgencyGenerator
{
public:
    // Combines count generators with an optional weight array; BotManager passes null weights.
    UrgencyMerger(int count, float* weights, UrgencyGenerator** generators);
    virtual ~UrgencyMerger();

    virtual float getUrgency(Bot* bot);
    virtual float getActiveUrgency(Bot* bot);
    virtual void  updateActiveUrgency(Bot* bot);
    virtual bool  hasChangedTarget(Bot* bot);
    virtual bool  quotientChanged(Bot* bot, float a, float b);
    virtual bool  hasChanged(Bot* bot, float a, float b);
    virtual UrgencyGenerator* getChosenGenerator();
    virtual void  reset(Bot* bot);
    virtual int   getSize();

private:
    /*  0 */ // vptr - off_4B6E48
    /*  4 */ int                m_count;         // the generator count; getSize returns it
    /*  8 */ float*             m_weights;       // null from BotManager's pool
    /* 12 */ UrgencyGenerator** m_generators;
    /* 16 */ void*              m_chosen;        // = 0 [INFERRED] from getChosenGenerator's offset
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOTBEHAVIOUR_URGENCYMERGER_H
