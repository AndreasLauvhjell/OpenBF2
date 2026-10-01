// [CHOSEN, not recovered] Grouped sensing-pattern declarations.
// Split into their individual translation units when their plan creation is implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

#ifndef AI_AIDLL_AI_AIBOT_SENSINGAGENTPATTERNS_H
#define AI_AIDLL_AI_AIBOT_SENSINGAGENTPATTERNS_H

#include "../Main/AIDllTypes.h"

namespace dice {
namespace hfe {
namespace ai {

class BAPALook;
class Bot;
class BotScript;
class IPIControlInfo;
class SensingAgent;

class ISensingAgentPattern
{
public:
    ISensingAgentPattern() {}
    virtual ~ISensingAgentPattern();

    // Preserve virtual order: reset precedes the other pattern operations.
    virtual void reset(int) = 0;                                        // +0x08
    virtual void createPlan(SensingAgent *, Bot *, const Vec3&) = 0;    // +0x0C
    virtual void createPlan(SensingAgent *, Bot *, unsigned int) = 0;   // +0x10
    virtual int getId() const = 0;                                      // +0x14

    // Uninitialised by construction - see the block above.
    /* +4 */ int m_id;
};

// The turret aimer. createPlan 821 / .
class SensingAgentTurretPattern : public ISensingAgentPattern
{
public:
    SensingAgentTurretPattern();
    virtual ~SensingAgentTurretPattern();

    // Preserve virtual order: reset occupies the first slot.
    virtual void reset(int);
    virtual void createPlan(SensingAgent *, Bot *, const Vec3&);
    virtual void createPlan(SensingAgent *, Bot *, unsigned int);
    virtual int getId() const;
};

// The mouse-look aimer, and the heaviest of the four: createPlan 2,293 / .
class SensingAgentMouseFreeLookPattern : public ISensingAgentPattern
{
public:
    SensingAgentMouseFreeLookPattern();
    virtual ~SensingAgentMouseFreeLookPattern();

    // Preserve virtual order: reset occupies the first slot.
    virtual void reset(int);
    virtual void createPlan(SensingAgent *, Bot *, const Vec3&);
    virtual void createPlan(SensingAgent *, Bot *, unsigned int);
    virtual int getId() const;
};

// The on-foot aimer - the only one of the four with helpers of its own.
class SensingAgentSoldierPattern : public ISensingAgentPattern
{
public:
    SensingAgentSoldierPattern();
    virtual ~SensingAgentSoldierPattern();

    // Preserve virtual order: reset occupies the first slot.
    virtual void reset(int);
    virtual void createPlan(SensingAgent *, Bot *, const Vec3&);
    virtual void createPlan(SensingAgent *, Bot *, unsigned int);
    virtual int getId() const;

    // Non-virtual, and unique to this pattern: it brackets the plan it builds.
    void initPlan(SensingAgent *, Bot *, IPIControlInfo *&, BotScript *&);
    void endPlan(BAPALook *, Bot *);
};

// The null object. Both createPlan overloads are and do nothing useful.
class SensingAgentNullPattern : public ISensingAgentPattern
{
public:
    SensingAgentNullPattern();
    virtual ~SensingAgentNullPattern();

    // Preserve virtual order: reset occupies the first slot.
    virtual void reset(int);
    virtual void createPlan(SensingAgent *, Bot *, const Vec3&);
    virtual void createPlan(SensingAgent *, Bot *, unsigned int);
    virtual int getId() const;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_SENSINGAGENTPATTERNS_H
