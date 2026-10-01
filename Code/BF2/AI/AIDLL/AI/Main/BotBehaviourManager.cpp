
#include "BotBehaviourManager.h"

#include "../AIBotBehaviour/UrgeCurve.h"     // UCConstant / UCLinear / UCXInverse - the three factories
#include "../AIUtils/checkPointer.h"         // inlined 26 times in the constructor
#include "AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

BotBehaviourManager* BotBehaviourManager::m_instance = 0;

// Behavior factories registered by name.

namespace {

// - builds one `BBAvoid`.
class BBCreateFunctionAvoid : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBMoveTo`.
class BBCreateFunctionMoveTo : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBMoveToFixed`.
class BBCreateFunctionMoveToFixed : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBMedicAssist`.
class BBCreateFunctionMedicAssist : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBIdle`.
class BBCreateFunctionIdle : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFire`.
class BBCreateFunctionFire : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFire3d`.
class BBCreateFunctionFire3d : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFireInfantery`.
class BBCreateFunctionFireInfantery : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFireUnarmed`.
class BBCreateFunctionFireUnarmed : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFireLargeBore`.
class BBCreateFunctionFireLargeBore : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBTakeCover`.
class BBCreateFunctionTakeCover : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBTakeCoverInfantry`.
class BBCreateFunctionTakeCoverInfantry : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBFireDriver`.
class BBCreateFunctionFireDriver : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBChange`.
class BBCreateFunctionChange : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBChangeLandingCraft`.
class BBCreateFunctionChangeLandingCraft : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBChangeTeleport`.
class BBCreateFunctionChangeTeleport : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBSuicide`.
class BBCreateFunctionSuicide : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBCallForMedic`.
class BBCreateFunctionCallForMedic : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBRevive`.
class BBCreateFunctionRevive : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBTakeCover3d`.
class BBCreateFunctionTakeCover3d : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBC4`.
class BBCreateFunctionC4 : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBRunOverSoldier`.
class BBCreateFunctionRunOverSoldier : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBBridgeRepair`.
class BBCreateFunctionBridgeRepair : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBDefuseMines`.
class BBCreateFunctionDefuseMines : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBRandom`.
class BBCreateFunctionRandom : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

// - builds one `BBTriggerable`.
class BBCreateFunctionTriggerable : public BBCreateFunction
{
public:
    virtual int operator()(int behaviourId, std::string& name, int mode,
                           UrgeCurve* urgeCurve, float* weights);
};

} // anonymous namespace

// Initializes the behavior registry and its owned collections.

BotBehaviourManager::BotBehaviourManager()
{
    m_instance = this;

    checkPointer(m_createFunctions["BBAvoid"]              = new BBCreateFunctionAvoid());
    checkPointer(m_createFunctions["BBMoveTo"]             = new BBCreateFunctionMoveTo());
    checkPointer(m_createFunctions["BBMoveToFixed"]        = new BBCreateFunctionMoveToFixed());
    checkPointer(m_createFunctions["BBMedicAssist"]        = new BBCreateFunctionMedicAssist());
    checkPointer(m_createFunctions["BBIdle"]               = new BBCreateFunctionIdle());
    checkPointer(m_createFunctions["BBFire"]               = new BBCreateFunctionFire());
    checkPointer(m_createFunctions["BBFire3d"]             = new BBCreateFunctionFire3d());
    checkPointer(m_createFunctions["BBFireInfantery"]      = new BBCreateFunctionFireInfantery());
    checkPointer(m_createFunctions["BBFireUnarmed"]        = new BBCreateFunctionFireUnarmed());
    checkPointer(m_createFunctions["BBFireLargeBore"]      = new BBCreateFunctionFireLargeBore());
    checkPointer(m_createFunctions["BBTakeCover"]          = new BBCreateFunctionTakeCover());
    checkPointer(m_createFunctions["BBTakeCoverInfantry"]  = new BBCreateFunctionTakeCoverInfantry());
    checkPointer(m_createFunctions["BBFireDriver"]         = new BBCreateFunctionFireDriver());
    checkPointer(m_createFunctions["BBChange"]             = new BBCreateFunctionChange());
    checkPointer(m_createFunctions["BBChangeLandingCraft"] = new BBCreateFunctionChangeLandingCraft());
    checkPointer(m_createFunctions["BBChangeTeleport"]     = new BBCreateFunctionChangeTeleport());
    checkPointer(m_createFunctions["BBSuicide"]            = new BBCreateFunctionSuicide());
    checkPointer(m_createFunctions["BBCallForMedic"]       = new BBCreateFunctionCallForMedic());
    checkPointer(m_createFunctions["BBRevive"]             = new BBCreateFunctionRevive());
    checkPointer(m_createFunctions["BBTakeCover3d"]        = new BBCreateFunctionTakeCover3d());
    checkPointer(m_createFunctions["BBC4"]                 = new BBCreateFunctionC4());
    checkPointer(m_createFunctions["BBRunOverSoldier"]     = new BBCreateFunctionRunOverSoldier());
    checkPointer(m_createFunctions["BBBridgeRepair"]       = new BBCreateFunctionBridgeRepair());
    checkPointer(m_createFunctions["BBDefuseMines"]        = new BBCreateFunctionDefuseMines());
    checkPointer(m_createFunctions["BBRandom"]             = new BBCreateFunctionRandom());
    checkPointer(m_createFunctions["BBTriggerable"]        = new BBCreateFunctionTriggerable());
}

// Clears behavior state and registry containers. Factory objects are not deleted here.

BotBehaviourManager::~BotBehaviourManager()
{
    m_instance = 0;
    reset();
}

// The only virtual, slot 0.
// It drops everything the `.ai` scripts defined - the behaviour names, their ids and the urge
// curves - and keeps the twenty-six create functions, which are compiled in and outlive any level.
// The curve loop is read literally: for each node, load the mapped pointer at `+20`, and if it is
// non-null call `(*(*ptr + 4))(ptr)` - Itanium vtable slot 1, the deleting destructor, i.e. source
// `delete`. Then advance, rebalance-for-erase, release the key string and free the node.

bool BotBehaviourManager::reset()
{
    m_behaviourIds.clear();

    std::map<std::string, UrgeCurve*, NoCaseStringCompare>::iterator it = m_urgeCurves.begin();
    while (it != m_urgeCurves.end())
    {
        if (it->second)
            delete it->second;
        m_urgeCurves.erase(it++);
    }

    m_behaviourNames.clear();
    return true;
}

// The behaviour name/id table.

// Associates a behavior ID with its script-visible name.
void BotBehaviourManager::setBehaviourName(int id, std::string name)
{
    m_behaviourNames.resize(id + 1);
    m_behaviourNames[id] = name;
    m_behaviourIds[name] = id;
}

// . -1 for "no such behaviour"; `create` below tests for exactly that.
int BotBehaviourManager::getBehaviourId(std::string name)
{
    std::map<std::string, int, NoCaseStringCompare>::iterator it = m_behaviourIds.find(name);
    if (it != m_behaviourIds.end())
        return it->second;

    return -1;
}

// Requires a nonnegative id. The signed upper-bound comparison intentionally permits
// negative values, which would index before the vector.
std::string BotBehaviourManager::getBehaviourName(int id)
{
    if (id < (int)m_behaviourNames.size())
        return m_behaviourNames[id];

    return "";
}

// Creates a named behavior through a registered factory.

int BotBehaviourManager::create(std::string behaviourName, std::string createFunctionName,
                                std::string name, int mode, std::string urgeCurveName,
                                float* weights, std::string& message)
{
    const int behaviourId = getBehaviourId(behaviourName);
    if (behaviourId == -1)
    {
        message = "'" + behaviourName + "' doesn't exist";
        return 0;
    }

    UrgeCurve* urgeCurve = 0;
    std::map<std::string, UrgeCurve*, NoCaseStringCompare>::iterator curve =
        m_urgeCurves.find(urgeCurveName);
    if (curve != m_urgeCurves.end())
        urgeCurve = curve->second;

    BBCreateFunction* const createFunction = m_createFunctions[createFunctionName];
    if (createFunction)
        return (*createFunction)(behaviourId, name, mode, urgeCurve, weights);

    message = "AIConsole::constructBehaviour, internal error";
    return 0;
}

// Creates and registers an urgency curve under the supplied name.

std::string BotBehaviourManager::createUCConstant(std::string& name, float value)
{
    m_urgeCurves[name] = new UCConstant(value);
    return "UCConstant created";
}

std::string BotBehaviourManager::createUCLinear(std::string& name, float slope, float offset)
{
    m_urgeCurves[name] = new UCLinear(slope, offset);
    return "UCLinear created";
}

std::string BotBehaviourManager::createUCXInverse(std::string& name, float numerator, float offset,
                                                  float slope, float bias)
{
    m_urgeCurves[name] = new UCXInverse(numerator, offset, slope, bias);
    return "UCXInverse created";
}

// The twenty-six frontier bodies.

namespace {

// Creation of BBAvoid and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionAvoid::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionAvoid::operator()");
}

// Creation of BBMoveTo and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionMoveTo::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionMoveTo::operator()");
}

// Creation of BBMoveToFixed and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionMoveToFixed::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionMoveToFixed::operator()");
}

// Creation of BBMedicAssist and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionMedicAssist::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionMedicAssist::operator()");
}

// Creation of BBIdle and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionIdle::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionIdle::operator()");
}

// Creation of BBFire and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionFire::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionFire::operator()");
}

// Creation of BBFire3d and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionFire3d::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionFire3d::operator()");
}

// Creation of BBFireInfantery and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionFireInfantery::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionFireInfantery::operator()");
}

// Creation of BBFireUnarmed and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionFireUnarmed::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionFireUnarmed::operator()");
}

// Creation of BBFireLargeBore and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionFireLargeBore::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionFireLargeBore::operator()");
}

// Creation of BBTakeCover and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionTakeCover::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionTakeCover::operator()");
}

// Creation of BBTakeCoverInfantry and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionTakeCoverInfantry::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionTakeCoverInfantry::operator()");
}

// Creation of BBFireDriver and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionFireDriver::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionFireDriver::operator()");
}

// Creation of BBChange and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionChange::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionChange::operator()");
}

// Creation of BBChangeLandingCraft and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionChangeLandingCraft::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionChangeLandingCraft::operator()");
}

// Creation of BBChangeTeleport and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionChangeTeleport::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionChangeTeleport::operator()");
}

// Creation of BBSuicide and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionSuicide::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionSuicide::operator()");
}

// Creation of BBCallForMedic and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionCallForMedic::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionCallForMedic::operator()");
}

// Creation of BBRevive and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionRevive::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionRevive::operator()");
}

// Creation of BBTakeCover3d and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionTakeCover3d::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionTakeCover3d::operator()");
}

// Creation of BBC4 and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionC4::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionC4::operator()");
}

// Creation of BBRunOverSoldier and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionRunOverSoldier::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionRunOverSoldier::operator()");
}

// Creation of BBBridgeRepair and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionBridgeRepair::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionBridgeRepair::operator()");
}

// Creation of BBDefuseMines and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionDefuseMines::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionDefuseMines::operator()");
}

// Creation of BBRandom and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionRandom::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionRandom::operator()");
}

// Creation of BBTriggerable and registration with the behavior list remain unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Replace the stop when this factory behavior is recovered.
int BBCreateFunctionTriggerable::operator()(int, std::string&, int, UrgeCurve*, float*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::(anonymous namespace)::BBCreateFunctionTriggerable::operator()");
}

} // anonymous namespace

} // namespace ai
} // namespace hfe
} // namespace dice
