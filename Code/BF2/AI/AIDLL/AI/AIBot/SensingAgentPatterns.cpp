// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining SensingAgentPatterns operations.

#include "SensingAgentPatterns.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// The base owns the shared id field used by every pattern's getId/reset pair.
// Constructors and destructors perform no additional member initialization or cleanup.
ISensingAgentPattern::~ISensingAgentPattern()
{
}

SensingAgentTurretPattern::SensingAgentTurretPattern()
{
}

SensingAgentTurretPattern::~SensingAgentTurretPattern()
{
}

int SensingAgentTurretPattern::getId() const
{
    return m_id;
}

void SensingAgentTurretPattern::reset(int id)
{
    m_id = id;
}

SensingAgentMouseFreeLookPattern::SensingAgentMouseFreeLookPattern()
{
}

SensingAgentMouseFreeLookPattern::~SensingAgentMouseFreeLookPattern()
{
}

int SensingAgentMouseFreeLookPattern::getId() const
{
    return m_id;
}

void SensingAgentMouseFreeLookPattern::reset(int id)
{
    m_id = id;
}

SensingAgentSoldierPattern::SensingAgentSoldierPattern()
{
}

SensingAgentSoldierPattern::~SensingAgentSoldierPattern()
{
}

int SensingAgentSoldierPattern::getId() const
{
    return m_id;
}

void SensingAgentSoldierPattern::reset(int id)
{
    m_id = id;
}

SensingAgentNullPattern::SensingAgentNullPattern()
{
}

SensingAgentNullPattern::~SensingAgentNullPattern()
{
}

int SensingAgentNullPattern::getId() const
{
    return m_id;
}

void SensingAgentNullPattern::reset(int id)
{
    m_id = id;
}

// [FRONTIER] Pattern plan creation and soldier helpers require the look/aim plan runtime.
// [RETIRE-WHEN: aidll-02-skeleton] Replace each stop when its plan behavior is recovered.
void SensingAgentTurretPattern::createPlan(SensingAgent *, Bot *, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentTurretPattern::createPlan");
}

void SensingAgentTurretPattern::createPlan(SensingAgent *, Bot *, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentTurretPattern::createPlan");
}

void SensingAgentMouseFreeLookPattern::createPlan(SensingAgent *, Bot *, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentMouseFreeLookPattern::createPlan");
}

void SensingAgentMouseFreeLookPattern::createPlan(SensingAgent *, Bot *, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentMouseFreeLookPattern::createPlan");
}

void SensingAgentSoldierPattern::createPlan(SensingAgent *, Bot *, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentSoldierPattern::createPlan");
}

void SensingAgentSoldierPattern::createPlan(SensingAgent *, Bot *, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentSoldierPattern::createPlan");
}

void SensingAgentSoldierPattern::initPlan(SensingAgent *, Bot *, IPIControlInfo *&, BotScript *&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentSoldierPattern::initPlan");
}

void SensingAgentSoldierPattern::endPlan(BAPALook *, Bot *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentSoldierPattern::endPlan");
}

// Null-pattern planning still requires an implementation; these overloads stop visibly.
void SensingAgentNullPattern::createPlan(SensingAgent *, Bot *, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentNullPattern::createPlan");
}

void SensingAgentNullPattern::createPlan(SensingAgent *, Bot *, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SensingAgentNullPattern::createPlan");
}

} // namespace ai
} // namespace hfe
} // namespace dice
