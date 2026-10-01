// Owns strategies and their conditions.
// [FRONTIER] Operations with unresolved nested parameter types remain undeclared.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

#ifndef AI_AIDLL_AI_MAIN_STRATEGYMANAGER_H
#define AI_AIDLL_AI_MAIN_STRATEGYMANAGER_H

#include <list>
#include <map>
#include <string>

#include "AIDllTypes.h"

namespace dice {
namespace hfe {
namespace ai {

class SCArea;
class Strategy;
class StrategyCondition;
class StrategyPrerequisite;

class StrategyManager
{
public:
    // Object, strategy and modifier association.
    class ObjectStrategyModifierTuple
    {
    public:
        char m_opaque[12];      // [UNRESOLVED] fields; size attested
    };

    StrategyManager();                              // 0x?? /
    ~StrategyManager();                             // 0x??, - not virtual

    virtual void reset();                           // the only virtual, vtable slot 0

    static StrategyManager* getInstance();
    static void setInstance(StrategyManager *);

    bool getEnableDebug();
    void setEnableDebug(bool);
    bool isDebugEnabled();

    Strategy* getActiveStrategy();
    std::string getActiveStrategyName();            // , returns by value
    StrategyPrerequisite* getActivePrerequisite();
    StrategyCondition* getActiveCondition();
    Strategy* getForcedStrategy(int);

    std::string getActivePrerequisiteName();
    std::string getActiveConditionName();
    Strategy* getStrategy(std::string);
    StrategyCondition* getCondition(std::string);
    StrategyPrerequisite* getPrerequisite(std::string);
    void setActiveStrategy(std::string);
    void setActivePrerequisite(std::string);
    void setActiveCondition(std::string);
    void setPrerequisite(std::string);
    void forceStrategy(std::string, int);
    void createStrategy(std::string);
    void createPrerequisite(std::string);
    void addCondition(std::string);
    void addCondition(std::string, float);
    void addSpecificObjectModifier(std::string, float);
    void addRequiredPrecedingStrategy(std::string);
    void addProhibitedPrecedingStrategy(std::string);
    void completeStrategies();
    void completeConditions();
    void listStrategies();
    void listPrerequisites();
    void listConditions();

    // the layout, from the 545-byte constructor cross-checked against the destructor
    // the three registries each come as A (name, active-pointer, map) triple, in the same order.
    /*  +4 */ std::string m_activeStrategyName;      // = "No Strategies"
    /*  +8 */ Strategy* m_activeStrategy;            // = 0
    /* +12 */ std::map<std::string, Strategy*> m_strategies;
    /* +24 */ std::list<ObjectStrategyModifierTuple> m_objectModifiers;
    /* +28 */ std::string m_activePrerequisiteName;  // = ""
    /* +32 */ StrategyPrerequisite* m_activePrerequisite;   // = 0
    /* +36 */ std::map<std::string, StrategyPrerequisite*> m_prerequisites;
    // Active condition iterator, initialized to end() and checked before dereference.
    /* +48 */ std::map<std::string, StrategyCondition*>::iterator m_activeCondition;
    /* +52 */ std::map<std::string, StrategyCondition*> m_conditions;
    /* +64 */ std::list<SCArea*> m_areas;
    /* +68 */ bool m_enableDebug;                    // = false
    // Forced strategy by team - 1. Callers must supply team 1 or 2; no bounds check is performed.
    /* +72 */ Strategy* m_forcedStrategy[2];         // = { 0, 0 }, indexed [team - 1]

    // first wins, and the destructor does test `== this`.
    static StrategyManager* m_instance;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_STRATEGYMANAGER_H
