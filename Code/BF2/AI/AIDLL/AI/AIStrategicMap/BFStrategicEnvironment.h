// Exposes the AI strategic environment.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGICMAP_BFSTRATEGICENVIRONMENT_H
#define AI_AIDLL_AI_AISTRATEGICMAP_BFSTRATEGICENVIRONMENT_H

#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include "../Main/AIDllTypes.h"

namespace dice {
namespace hfe {
namespace ai {

// IAIStrategicEnvironment - 14 vtable slots.
// The first-wins guard reads AISingletonServer::m_iaiStrategicEnvironment - the DLL-side half of the roster.
class IAIStrategicEnvironment
{
public:
    IAIStrategicEnvironment();
    virtual ~IAIStrategicEnvironment();
    virtual int getEnemyStrengths(Bot *) = 0;
    virtual int getEnemyTypes(Bot *) = 0;
    virtual int getTemperature(Bot *) = 0;
    virtual float getTemperature(int, int) = 0;
    virtual int getSecurity(Bot *) = 0;
    virtual float getSecurity(int, int) = 0;
    virtual int getControl(Bot *) = 0;
    virtual float getControl(int, int) = 0;
    virtual int getFriendlyBattleStr(Bot *) = 0;
    virtual float getFriendlyBattleStr(int, int) = 0;
    virtual int getHostileBattleStr(Bot *) = 0;
    virtual float getHostileBattleStr(int, int) = 0;
};

// BFStrategicEnvironment - sizeof == 4, from BF2AI::init step 6.
class BFStrategicEnvironment : public IAIStrategicEnvironment
{
public:
    BFStrategicEnvironment();
    virtual ~BFStrategicEnvironment();

    // IAIStrategicEnvironment
    virtual int getEnemyStrengths(Bot *);
    virtual int getEnemyTypes(Bot *);
    virtual int getTemperature(Bot *);
    virtual float getTemperature(int, int);
    virtual int getSecurity(Bot *);
    virtual float getSecurity(int, int);
    virtual int getControl(Bot *);
    virtual float getControl(int, int);
    virtual int getFriendlyBattleStr(Bot *);
    virtual float getFriendlyBattleStr(int, int);
    virtual int getHostileBattleStr(Bot *);
    virtual float getHostileBattleStr(int, int);

};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGICMAP_BFSTRATEGICENVIRONMENT_H
