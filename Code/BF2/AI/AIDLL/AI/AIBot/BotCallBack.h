// Callback interface for bot events.
// [INFERRED] Source placement.
#ifndef AI_AIDLL_AI_AIBOT_BOTCALLBACK_H
#define AI_AIDLL_AI_AIBOT_BOTCALLBACK_H

namespace dice {
namespace hfe {
namespace ai {

class Bot;

class IBotDeathCallBack
{
public:
    // store the bot. No reference ownership.
    explicit IBotDeathCallBack(Bot* bot) : m_bot(bot) {}
    // only the compiler's vptr store.
    virtual ~IBotDeathCallBack() {}
    virtual void reportDeath() = 0;

protected:
    Bot* m_bot;
};

class IBotSpawnCallBack
{
public:
    // store the bot. No reference ownership.
    explicit IBotSpawnCallBack(Bot* bot) : m_bot(bot) {}
    // only the compiler's vptr store.
    virtual ~IBotSpawnCallBack() {}
    virtual void reportSpawn() = 0;

protected:
    Bot* m_bot;
};

static_assert(sizeof(void*) != 4 || sizeof(IBotDeathCallBack) == 8, "IBotDeathCallBack x86 layout");
static_assert(sizeof(void*) != 4 || sizeof(IBotSpawnCallBack) == 8, "IBotSpawnCallBack x86 layout");

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTCALLBACK_H
