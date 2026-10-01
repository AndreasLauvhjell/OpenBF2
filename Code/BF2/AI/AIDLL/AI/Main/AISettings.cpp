// AI configuration, bot names and console bindings.

#include "AISettings.h"


#include "AIDllFrontier.h"
#include "../../../../Core/Debug.h"
#include "../../../../Core/ClassManager.h"
#include "CommonSingletonServer.h"
#include "../../../../Core/ConsoleObjectMacro.h"   // the aiSettings.* bindings at the foot
#include "AIMain.h"                                // IAIMain::spawnBots - setAutoSpawnBots' trigger
#include "AILODManager.h"                          // setViewDistance's second broadcast
#include "AIRandom.h"                            // addBotName's weight draw
#include "IBotManager.h"                           // setViewDistance walks the bot pool
#include "../AIBot/BotMain.h"                      // Bot::isDeactivated / Bot::getPlayer
#include "../AIPathFinding/Material/Material.h"     // materialId console spelling and extractor
#include "../AIPathFinding/Material/MaterialManager.h"
#include "../AIUtils/checkPointer.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"  // IAIPlayer::setViewDistance, slot +0x68

namespace dice {
namespace hfe {
extern ClassManager* classManager;

namespace ai {

// Returns whether AI configuration overrides the game menu settings.
bool AISettings::overrideMenuSettings() const
{
    return m_overrideMenuSettings;
}

// The same byte, under a second name. See above.
bool AISettings::getOverrideMenuSettings() const
{
    return m_overrideMenuSettings;
}


// Recovered accessors and the remaining explicitly named stops follow in shipped declaration order.
// The production stops below diagnose and abort; none returns a fabricated value.
int AISettings::getNumberOfBehaviours() const
{
    return m_numberOfBehaviours;
}

int AISettings::getNumberOfVehicles() const
{
    return static_cast<int>(m_behaviours.size());
}

float* AISettings::getVehicleMods(int vehicle) const
{
    return m_vehicleMods[vehicle];
}

BotBehaviour* AISettings::getBehaviour(int vehicle, int index) const
{
    return m_behaviours[vehicle][index];
}

BotBehaviour* AISettings::getDefaultBehaviour(int vehicle) const
{
    const int index = m_defaultBehaviour[vehicle];
    if (index != -1)
        return m_behaviours[vehicle][index];
    return 0;
}

void* AISettings::getPlanInterpreter(int vehicle) const
{
    return m_planInterpreters[vehicle];
}

PlanGenerator* AISettings::getPlanGenerator(int vehicle, int index) const
{
    return m_planGenerators[vehicle][index];
}

float AISettings::getStatsViewDistance() const
{
    return m_statsViewDistance;
}

bool AISettings::getDrawBotStats() const
{
    return m_drawBotStats;
}

int AISettings::getNBattleStrengths()
{
    return 6;
}

int AISettings::getNStrategicStrengths()
{
    return 2;
}

int AISettings::getInformationGridDimension() const
{
    return m_informationGridDimension;
}

float AISettings::getSAIUpdateFrequency() const
{
    return m_saiUpdateFrequency;
}

// [INFERRED] reference spelling, proven borrowed string object.
const std::string& AISettings::getVehicle(int vehicle) const
{
    if (vehicle < 0 || vehicle >= static_cast<int>(m_vehicleNames.size()))
    {
        Debug(DebugTypeError, __FILE__, __LINE__, "") << "Error: Illegal vehicle id: " << vehicle;

        // Shipped fallthrough if Debug ever returns; DebugTypeError aborts in this tree.
        return m_sensingAgentPatternName;
    }

    return m_vehicleNames[vehicle];
}

int AISettings::getVehicle(const std::string& name) const
{
    // (!) guarded, and the sibling below is not. Same map type, same find, opposite discipline.
    const std::map<std::string, int>::const_iterator i = m_vehicleIndex.find(name);
    if (i == m_vehicleIndex.end())
        return -1;
    return i->second;
}

// Every lookup/index is unchecked in both images.
const std::string& AISettings::getVehicleClusterMapping(const std::string& name) const
{
    const int vehicle = m_vehicleIndex.find(name)->second;
    return m_vehicleNames[m_clusterCostTypes[vehicle]];
}

int AISettings::getClusterCostType(int index) const
{
    return m_clusterCostTypes[index];
}

const std::vector<int>& AISettings::getClusterCostTypes() const
{
    return m_clusterCostTypes;
}

int AISettings::getNumberOfVehicleGroups() const
{
    return static_cast<int>(m_vehicleGroupNames.size());
}

int AISettings::getVehicleGroup(int index) const
{
    return m_vehicleGroups[index];
}

int AISettings::getVehicleGroup(const std::string& name) const
{
    // Requires an existing group name; the lookup result is dereferenced without checking end().
    return m_vehicleGroupIndex.find(name)->second;
}

const std::string& AISettings::getVehicleGroupName(int group) const
{
    return m_vehicleGroupNames[group];
}

float* AISettings::getStandardPersonality(int side) const
{
    // (!) 1-based. The binary computes `this + 176 + 4*side`, i.e. this array at +180 subscripted
    // [side - 1]. See the layout note.
    return m_standardPersonality[side - 1];
}

BotMoralAdmin* AISettings::getStandardMoralAdmin(int side) const
{
    return m_standardMoralAdmin[side - 1];
}

bool AISettings::hasActiveRenderer() const
{
    // This unsupported operation emits an error diagnostic.
    Debug(DebugTypeError, __FILE__, __LINE__, "") << "Does not work.";
    return false;
}

bool AISettings::getDraw() const
{
    return m_draw;
}

// - a bare member return, the shape every accessor in this run has.
const std::map<int, ISensingAgentPattern*>& AISettings::getSensingAgentPatterns() const
{
    return m_sensingAgentPatterns;
}

ISensingAgentPattern* AISettings::getSensingAgentPattern(int id) const
{
    const std::map<int, ISensingAgentPattern*>::const_iterator i = m_sensingAgentPatterns.find(id);
    if (i == m_sensingAgentPatterns.end())
        return 0;
    return i->second;
}

ISensingAgentPattern* AISettings::getSensingAgentNullPattern() const
{
    return m_sensingAgentNullPattern;
}

int AISettings::getManDownEquipmentType()
{
    // Preserve virtual dispatch so derived getVehicle implementations are honored.
    return getVehicle(m_manDownEquipment);
}

bool AISettings::isValidMaterial(int vehicle, unsigned char material) const
{
    return m_vehicleMaterialManagers[vehicle]->isValidMaterial(material);
}

float AISettings::getVehicleMaterialCost(const std::string& name, unsigned char material) const
{
    return m_vehicleMaterialManagers[getVehicle(name)]->getMaterialCost(material);
}

float AISettings::getVehicleMaterialCost(int vehicle, unsigned char material) const
{
    return m_vehicleMaterialManagers[vehicle]->getMaterialCost(material);
}

MaterialManager* AISettings::getVehicleMaterialManager(int vehicle) const
{
    return m_vehicleMaterialManagers[vehicle];
}

// Returns the next bot name and rotates the roster for the following request.
const std::string& AISettings::getBotName() const
{
    m_botNames.push_back(m_botNames.front());
    m_botNames.pop_front();

    return m_botNames.back().second;
}

// The shared bot value narrows, the settings value does not.
void AISettings::setMaxNumberOfBehaviours(int count)
{
    BotMain::setNBehaviours(static_cast<unsigned char>(count));
    m_numberOfBehaviours = count;
}

void AISettings::setVehicleMods(int, float *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::setVehicleMods");
}

void AISettings::addBehaviour(int, int, BotBehaviour *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::addBehaviour");
}

void AISettings::setDefaultBehaviour(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::setDefaultBehaviour");
}

void AISettings::addInterpreterEntry(int, BAPEntry *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::addInterpreterEntry");
}

void AISettings::addInterpreterEntry(int, BotLodLevel, BAPEntry *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::addInterpreterEntry");
}

void AISettings::removeInterpreterEntry(int, BotLodLevel, BAPEntry *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::removeInterpreterEntry");
}

void AISettings::activateDefaultInterpreter(int, BAPStatementType, BotLodLevel)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::activateDefaultInterpreter");
}

void AISettings::addGenerator(int, int, PlanGenerator *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::addGenerator");
}

void AISettings::setBasicBotWeights(float *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::setBasicBotWeights");
}

void AISettings::setStatsViewDistance(float distance)
{
    m_statsViewDistance = distance;
}

void AISettings::setDrawBotStats(bool draw)
{
    m_drawBotStats = draw;
}

void AISettings::setInformationGridDimension(int dimension)
{
    m_informationGridDimension = dimension;
}

void AISettings::setNSides(int sides)
{
    m_nSides = sides;
}

void AISettings::setSAIUpdateFrequency(float frequency)
{
    m_saiUpdateFrequency = frequency;
}

void AISettings::setStandardPersonality(float* weights, int side)
{
    m_standardPersonality[side - 1] = weights;
}

void AISettings::setStandardMoralAdmin(BotMoralAdmin* admin, int side)
{
    m_standardMoralAdmin[side - 1] = admin;
}

void AISettings::setRespawnAllowed(bool allowed)
{
    m_respawnAllowed = allowed;
}

// Duplicate insertion failure is deliberately ignored; the three
// parallel vectors still grow, while the name map retains the first id.
void AISettings::setVehicle(const std::string& name)
{
    const int id = static_cast<int>(m_vehicleIndex.size());
    m_vehicleIndex.insert(std::make_pair(name, id));
    m_vehicleNames.push_back(name);
    m_vehicleMaterialManagers.push_back(checkPointer(new MaterialManager));
    m_clusterCostTypes.push_back(-1);
}

// Both map finds are intentionally unguarded.
void AISettings::setVehicleClusterMapping(const std::string& vehicle,
                                          const std::string& clusterVehicle)
{
    const int vehicleId = m_vehicleIndex.find(vehicle)->second;
    const int clusterVehicleId = m_vehicleIndex.find(clusterVehicle)->second;
    m_clusterCostTypes[vehicleId] = clusterVehicleId;
}

// Retail uses resize rather than push_back and ignores duplicates.
void AISettings::createVehicleGroup(const std::string& name)
{
    const int id = static_cast<int>(m_vehicleGroupNames.size());
    m_vehicleGroupNames.resize(m_vehicleGroupNames.size() + 1, name);
    m_vehicleGroupIndex.insert(std::make_pair(name, id));
}

void AISettings::addVehicleToVehicleGroup(int vehicle, const std::string& group)
{
    // [PLATFORM] This single expression is load-bearing: PE captures the destination before the
    // virtual lookup; ELF reloads the vector after it. C++ operand evaluation order permits both.
    m_vehicleGroups[vehicle] = getVehicleGroup(group);
}

void AISettings::addVehicleToVehicleGroup(const std::string& vehicle,
                                          const std::string& group)
{
    const int vehicleId = getVehicle(vehicle);

    // Shipped defect in both images: the known-vehicle arm reports "unknown"; the negative id arm
    // performs the unchecked assignment. Preserve the inverted test and the unsafe missing-name arm.
    if (vehicleId >= 0)
    {
        Debug(DebugTypeError, __FILE__, __LINE__, "")
            << "Vehicle of name " << vehicle << " is unknown.";
        return;
    }

    m_vehicleGroups[vehicleId] = getVehicleGroup(group);
}

// Stores the auto-spawn flag. Enabling also requests immediate spawning when IAIMain
// exists; disabling performs no spawn action.
void AISettings::setAutoSpawnBots(bool enable)
{
    m_autoSpawnBots = enable;

    if (enable && IAIMain::m_instance)
        IAIMain::m_instance->spawnBots();
}

void AISettings::setDraw(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::setDraw");
}

void AISettings::setSensingAgentPatternName(const std::string&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::setSensingAgentPatternName");
}

void AISettings::setSensingAgentNullPattern(ISensingAgentPattern* pattern)
{
    m_sensingAgentNullPattern = pattern;
}

void AISettings::addSensingAgentPattern(int, ISensingAgentPattern *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::addSensingAgentPattern");
}

void AISettings::setManDownEquipment(const std::string& equipment)
{
    m_manDownEquipment = equipment;
}

void AISettings::setVehicleMaterialCost(const std::string& vehicle,
                                        unsigned char material,
                                        float cost)
{
    m_vehicleMaterialManagers[getVehicle(vehicle)]->setMaterialCost(material, cost);
}

// Adds a name to the bot-name roster used by getBotName.
void AISettings::addBotName(const std::string& first, const std::string& last)
{
    const unsigned int count = static_cast<unsigned int>(m_botNames.size());

    float weight;

    if (count <= 5)
    {
        // The narrow range - see the header note. The zero terms are the binary's.
        weight = 0.0f + AIRandom::m_instance->nextFloat() * (0.40000001f - 0.0f);
    }
    else
    {
        weight = AIRandom::m_instance->nextFloat();
    }

    std::list<std::pair<float, std::string> >::iterator position = m_botNames.begin();

    // the dereference comes first. See the header note.
    while (weight > position->first && position != m_botNames.end())
        ++position;

    m_botNames.insert(position, std::pair<float, std::string>(weight, first + " " + last));
}

void AISettings::setBotNameUnused(const std::string&)
{
    // (*) five bytes in the binary - a bare `ret`. dice declared it, the .ai console binds it, and
    // it does nothing at all. A recovered empty body, not a stub.
}


// Initializes AI settings and their default values.
AISettings::AISettings()
    : m_maxNBots(-1)
    , m_numberOfBehaviours(0)
    , m_viewDistance(-1.0f)
    , m_statsViewDistance(30.0f)
    , m_botSkill(0.75f)
    , m_saiUpdateFrequency(35.0f)
    , m_informationGridDimension(32)
    , m_nSides(0)
    , m_respawnAllowed(true)
    , m_draw(false)
    , m_autoSpawnBots(false)
    , m_maxBotsIncludeHumans(true)
    , m_ignoreCommanderDistance(30.0f)
    , m_overrideMenuSettings(false)
    , m_drawBotStats(true)
{
    // Defect 1: m_sensingAgentNullPattern is deliberately not initialised here. See above.

    // Both arrays are indexed [side - 1]; the constructor nulls all four words.
    m_standardMoralAdmin[0]  = 0;
    m_standardMoralAdmin[1]  = 0;
    m_standardPersonality[0] = 0;
    m_standardPersonality[1] = 0;

    // The one piece of AI tuning data this constructor gives up outright.
    m_armourClassValues[0] =  1.0f;
    m_armourClassValues[1] =  3.0f;
    m_armourClassValues[2] =  8.0f;
    m_armourClassValues[3] = 15.0f;
    m_armourClassValues[4] = 10.0f;
    m_armourClassValues[5] = 12.0f;

    // Defect 3: two no-ops, in this order, on two vectors that are empty. Reproduced because the
    // calls are in the binary.
    m_vehicleGroupNames.resize(m_vehicleGroupNames.size());
    m_vehicleGroups.resize(m_vehicleGroups.size());
}


// Registers the first settings/modifier instance using roster-static guards.
// Those guards can remain stale until updateStaticMethods refreshes the roster.
IAISettings::IAISettings()
{
    if (!CommonSingletonServer::m_iaiSettings)
        classManager->registerSimpleSingleton("IAISettings", this);
}

IAISettings::~IAISettings()
{
    if (CommonSingletonServer::m_iaiSettings == this)
        classManager->unregisterSimpleSingleton("IAISettings");
}

IAISettingsModifier::IAISettingsModifier()
{
    if (!CommonSingletonServer::m_iaiSettingsModifier)
        classManager->registerSimpleSingleton("IAISettingsModifier", this);
}

IAISettingsModifier::~IAISettingsModifier()
{
    if (CommonSingletonServer::m_iaiSettingsModifier == this)
        classManager->unregisterSimpleSingleton("IAISettingsModifier");
}

// Remaining recovered field accessors. The constructor now supplies their proven backing fields;
// AISettings teardown remains a separate named frontier below.

int AISettings::getMaxNBots() const
{
    return m_maxNBots;
}

bool AISettings::getmaxBotsIncludeHumans() const
{
    return m_maxBotsIncludeHumans;
}

float AISettings::getIgnoreCommanderDistance() const
{
    return m_ignoreCommanderDistance;
}

float AISettings::getViewDistance() const
{
    return m_viewDistance;
}

float AISettings::getBotSkill() const
{
    return m_botSkill;
}

int AISettings::getNSides() const
{
    return m_nSides;
}

bool AISettings::isRespawnAllowed() const
{
    return m_respawnAllowed;
}

bool AISettings::getAutoSpawnBots() const
{
    return m_autoSpawnBots;
}

const float* AISettings::getArmourClassValues() const
{
    return m_armourClassValues;
}

float AISettings::getArmourClassValue(int index) const
{
    return m_armourClassValues[index];
}

void AISettings::setMaxNBots(int n)
{
    m_maxNBots = n;
}

void AISettings::maxBotsIncludeHumans(bool include)
{
    m_maxBotsIncludeHumans = include;
}

void AISettings::setIgnoreCommanderDistance(float distance)
{
    m_ignoreCommanderDistance = distance;
}

void AISettings::setBotSkill(float skill)
{
    m_botSkill = skill;
}

// Sets the AI view distance and propagates it to every live bot player.
void AISettings::setViewDistance(float distance)
{
    m_viewDistance = distance;

    IBotManager* const botManager = IBotManager::instance;
    if (botManager)
    {
        for (unsigned int i = 0; i < static_cast<unsigned int>(botManager->getBotCount()); ++i)
        {
            Bot* const bot = botManager->getBot(i);

            if (!bot->isDeactivated())
                bot->getPlayer()->setViewDistance(m_viewDistance);
        }
    }

    AILODManager* const lod = AILODManager::m_instance;
    if (lod)
    {
        if (lod->m_basicLODDistance > distance)
        {
            lod->setBasicLODDistance(distance);
        }
        else if (distance > lod->m_basicLODDistance
              && lod->m_maxLODDistance > lod->m_basicLODDistance)
        {
            lod->setBasicLODDistance(lod->m_maxLODDistance > distance
                                         ? distance
                                         : lod->m_maxLODDistance);
        }
    }
}

void AISettings::setOverrideMenuSettings(bool override_)
{
    m_overrideMenuSettings = override_;
}

void AISettings::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::reset");
}


// [FRONTIER] Destruction requires cleanup of owned settings collections.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding AISettings operation.
AISettings::~AISettings()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AISettings::~AISettings");
}

// Console bindings for bot population and startup settings.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

namespace {

const char* const kAiSettingsConsoleTag = "aiSettings";

// Every forwarder takes the same receiver, and the [CHOSEN] caveat above covers all of them.
AISettings* consoleAiSettings()
{
    return static_cast<AISettings*>(
        static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings));
}

void consoleAiSettingsSetMaxNBots(int n)                { consoleAiSettings()->setMaxNBots(n); }
void consoleAiSettingsSetNSides(int sides)              { consoleAiSettings()->setNSides(sides); }
void consoleAiSettingsSetBotSkill(float skill)          { consoleAiSettings()->setBotSkill(skill); }
void consoleAiSettingsMaxBotsIncludeHumans(bool include){ consoleAiSettings()->maxBotsIncludeHumans(include); }
void consoleAiSettingsSetInformationGridDimension(int d){ consoleAiSettings()->setInformationGridDimension(d); }
// the only forwarder here that is not A store: `setAutoSpawnBots` also calls
// `IAIMain::spawnBots`, so this console line has a side effect the other five do not.
void consoleAiSettingsSetAutoSpawnBots(bool enable) { consoleAiSettings()->setAutoSpawnBots(enable); }
// two arguments for one name: the console splits `addBotName T. Karlsson` on whitespace and
// the receiver puts the space back. See addBotName's own note.
void consoleAiSettingsAddBotName(std::string first, std::string last)
                                                        { consoleAiSettings()->addBotName(first, last); }

// The recovered roster classes dispatch through the registered write interface and report
// inactive while it is absent. Their argument type strings and relative order are paired in both
// generated-console images.
IAISettingsModifier* consoleAiSettingsModifier()
{
    return static_cast<IAISettingsModifier*>(CommonSingletonServer::m_iaiSettingsModifier);
}

void consoleAiSettingsSetMaxNumberOfBehaviours(int count)
{
    consoleAiSettingsModifier()->setMaxNumberOfBehaviours(count);
}

void consoleAiSettingsSetVehicle(std::string name)
{
    consoleAiSettingsModifier()->setVehicle(name);
}

void consoleAiSettingsSetVehicleClusterMapping(std::string vehicle,
                                                std::string clusterVehicle)
{
    consoleAiSettingsModifier()->setVehicleClusterMapping(vehicle, clusterVehicle);
}

void consoleAiSettingsSetVehicleMaterialCost(std::string vehicle,
                                              materialId material,
                                              float cost)
{
    consoleAiSettingsModifier()->setVehicleMaterialCost(vehicle, material, cost);
}

void consoleAiSettingsSetManDownEquipment(std::string equipment)
{
    consoleAiSettingsModifier()->setManDownEquipment(equipment);
}

void consoleAiSettingsCreateVehicleGroup(std::string name)
{
    consoleAiSettingsModifier()->createVehicleGroup(name);
}

void consoleAiSettingsAddVehicleToVehicleGroup(std::string vehicle,
                                                std::string group)
{
    consoleAiSettingsModifier()->addVehicleToVehicleGroup(vehicle, group);
}

// Generic generated arguments retain the shared temp-and-commit extraction rule. materialId is the
// one recovered exception: its generated class calls the AI material extractor directly into the
// byte-sized static storage.
template <class T>
void convertAiSettingsRosterArgument(const std::string& text, T& storage)
{
    T value = storage;
    std::istringstream stream(text);
    stream >> value;
    if (!stream.fail())
        storage = value;
}

void convertAiSettingsRosterArgument(const std::string& text, materialId& storage)
{
    std::istringstream stream(text);
    ::dice::hfe::ai::operator>>(stream, storage);
}

} // anonymous namespace

// Console bindings for AI settings and vehicle definitions.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
BF2_CONSOLE_OBJECT("aiSettings", 2, 0)
BF2_CONSOLE_METHOD_1(setMaxNBots, consoleAiSettingsSetMaxNBots, int, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_1(setNSides, consoleAiSettingsSetNSides, int, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_1(setBotSkill, consoleAiSettingsSetBotSkill, float, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_1(maxBotsIncludeHumans, consoleAiSettingsMaxBotsIncludeHumans, bool, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_1(setInformationGridDimension, consoleAiSettingsSetInformationGridDimension, int, kAiSettingsConsoleTag)
// Exposes the auto-spawn setting used by level scripts.
BF2_CONSOLE_METHOD_1(setAutoSpawnBots, consoleAiSettingsSetAutoSpawnBots, bool, kAiSettingsConsoleTag)
// Registers the command used by BotNames.ai to populate the bot-name roster.
BF2_CONSOLE_METHOD_2(addBotName, consoleAiSettingsAddBotName, std::string, std::string, kAiSettingsConsoleTag)

// ELF generated classes 262/267/268/269/270/283/284 and the matching PE CRT initializer order. The
// temporary BF2_CC_CONVERT specialization keeps every ordinary conversion unchanged while routing
// the `materialId` spelling through Material.cpp rather than std's character extractor.
#pragma push_macro("BF2_CC_CONVERT")
#undef BF2_CC_CONVERT
#define BF2_CC_CONVERT(T, oneBasedIndex)                                                           \
        if (index == (oneBasedIndex))                                                              \
        {                                                                                          \
            static T s;                                                                            \
            convertAiSettingsRosterArgument(value, s);                                             \
            m_args[(oneBasedIndex) - 1] = reinterpret_cast<std::string*>(&s);                      \
        }

BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setMaxNumberOfBehaviours,
                                   consoleAiSettingsSetMaxNumberOfBehaviours,
                                   CommonSingletonServer::m_iaiSettingsModifier != 0,
                                   int, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setVehicle, consoleAiSettingsSetVehicle,
                                   CommonSingletonServer::m_iaiSettingsModifier != 0,
                                   std::string, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_2_STATIC_ACTIVE(setVehicleClusterMapping,
                                   consoleAiSettingsSetVehicleClusterMapping,
                                   CommonSingletonServer::m_iaiSettingsModifier != 0,
                                   std::string, std::string, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_3_STATIC_ACTIVE(setVehicleMaterialCost,
                                   consoleAiSettingsSetVehicleMaterialCost,
                                   CommonSingletonServer::m_iaiSettingsModifier != 0,
                                   std::string, materialId, float, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(setManDownEquipment,
                                   consoleAiSettingsSetManDownEquipment,
                                   CommonSingletonServer::m_iaiSettingsModifier != 0,
                                   std::string, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(createVehicleGroup,
                                   consoleAiSettingsCreateVehicleGroup,
                                   CommonSingletonServer::m_iaiSettingsModifier != 0,
                                   std::string, kAiSettingsConsoleTag)
BF2_CONSOLE_METHOD_2_STATIC_ACTIVE(addVehicleToVehicleGroup,
                                   consoleAiSettingsAddVehicleToVehicleGroup,
                                   CommonSingletonServer::m_iaiSettingsModifier != 0,
                                   std::string, std::string, kAiSettingsConsoleTag)

#pragma pop_macro("BF2_CC_CONVERT")

// Handed to the console registry by AIModule::init, beside the `ai` object's list.
ConsoleObjectList& aiSettingsConsoleObjects()
{
    return consoleBindingList(kAiSettingsConsoleTag);
}

} // namespace ai
} // namespace hfe
} // namespace dice
