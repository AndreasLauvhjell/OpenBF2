// AI runtime lifecycle, population management and simulation updates.

#include "AIMain.h"

#include <algorithm>   // std::min - spawnBots' bot budget
#include <string>

#include "AIDllFrontier.h"                                        // the twenty named stops below
#include "../AIUtils/checkPointer.h"                       // the ctor's dead null check
#include "../AIBotBehaviour/CustomCurve.h"                 // SCurve / JCurve / DecleiningSlopeCurve
#include "../../../AIInterface/Module/BFConsole.h"         // IBFModuleCourier::aiMainInit
#include "BotManager.h"                                           // getNumberOfActiveBots
#include "CommonSingletonServer.h"                                 // m_iaiGame / Settings / Effector
#include "AISettingsInterfaces.h"                                  // IAISettings
#include "../../../AIInterface/BFKnowledge/BFGame.h"              // IAIGame
#include "../../../AIInterface/BFKnowledge/BFGameEffector.h"      // IAIGameEffector::addBot
#include "IAIPathfinding.h"                                        // IAIPathfindingSetup::instance
#include "../AIStrategicMap/AerialHeightMap.h"                     // readHeightMapFromFile
#include "../AIKnowledge/BFInformationEnvironment.h"                // built and dropped by init
#include "../AIUtils/DebugRendering/AIRenderUtil.h"                // built and dropped by init
#include "../AIStrategic/SAI.h"                                   // ISAI::create, one per side
#include "../AIStrategicEntity/AIStrategicObjectManager.h"        // setSAIEnable's refusal test
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"           // IAITerrain, the world bounds
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"            // IAIPlayer - newPlayerCreated
#include "../AIBot/BotMain.h"                                      // Bot - switchSides
#include "../../../AIInterface/BFKnowledge/BFObjectEnvironment.h"  // isValidPlayerId
#include "UserOrderManager.h"                                     // recruitBot's first refusal
#include <utility>                                                // the per-bot pair series
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"     // getPlayerFromId, the id sweep
#include "../../../AIInterface/BFKnowledge/BFTimer.h"             // action's three timer methods
#include "../../../AIInterface/BFKnowledge/BFCollisionHandler.h"  // updateBotProjectiles
#include "../AIBot/SquadManager.h"                                // update(double)
#include "AIMineManager.h"                                       // IAIMineManager::update

namespace dice {
namespace hfe {
namespace ai {

// Active IAIMain instance; construction replaces it unconditionally.
IAIMain* IAIMain::m_instance = 0;

// Construction replaces the active IAIMain pointer. Destruction clears it unconditionally.
// Only one IAIMain may be alive when callers use this singleton.
IAIMain::IAIMain()
{
    m_instance = this;
}

IAIMain::~IAIMain()
{
    m_instance = 0;
}


// Returns whether initialization has completed. [INFERRED] State-field name.
bool AIMain::isInitialised()
{
    return m_initialised;
}

// These eight methods intentionally do nothing in the dedicated-server image.

// The AI's debug rendering entry point - inert on a dedicated server build, which is the
// only build this pair comes from. So none of the ~20 show*/render* console switches this class
// exposes can produce any output here; they set flags that only a client-side AIMain would read.
void AIMain::draw()
{
}

// The sound-event hook is empty, so it never fills the sound queue.
// [UNRESOLVED] SoundEffect members cannot be established from this path.
void AIMain::queueSoundEffect(const Vec3& /*pos*/, int /*a*/, int /*b*/)
{
}

// These five commander notifications intentionally do nothing in AIMain.
void AIMain::commanderOrderAccepted(int /*a*/)
{
}

void AIMain::commanderOrderRejected(int /*a*/)
{
}

void AIMain::appliedForCommander(int /*a*/)
{
}

void AIMain::acceptedAsCommander(int /*a*/)
{
}

void AIMain::resignedAsCommander(int /*a*/)
{
}

// Creates and activates bots up to the configured population budget when a round starts.
void AIMain::spawnBots()
{
    IAIGame* const game = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame);
    if (!game->isPlaying())
        return;

    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    if (!settings->getAutoSpawnBots())
        return;

    int wanted = settings->getMaxNBots();
    if (settings->getmaxBotsIncludeHumans())
        wanted = std::min(wanted, game->getMaxNumberOfPlayers()) - m_humanCount;

    IAIGameEffector* const effector =
        static_cast<IAIGameEffector*>(CommonSingletonServer::m_iaiGameEffector);

    for (int missing = wanted - m_botManager->getNumberOfActiveBots(); missing > 0; --missing)
    {
        int a;
        int b;
        std::string name;

        effector->addBot(a, b, name);
        activateBot(a, b, name);
    }
}

// Constructs and destroys the AI runtime. Remaining dependencies use named frontier stops.
AIMain::AIMain()
    // Keep BotManager allocation in the initializer list to preserve its position between
    // the surrounding container constructions.
    : m_aiSystemQuotient(0.15f)     // the float constant, widened - see AIMain.h
    , m_dword12(0.875)
    , m_componentQuotient(0.125)
    , m_double28(0.0)
    , m_dword36(0)
    , m_double40(3.0)
    , m_dword100(0)
    , m_initialised(false)
    , m_saiEnabled(false)
    , m_dword108(0)
    , m_dword112(0)
    , m_float120(100.0f)
    , m_dword128(1)
    , m_dword132(0)
    , m_flag136(false)
    , m_flag137(true)
    , m_flag138(false)
    , m_flag139(true)
    , m_flag140(true)
    , m_flag141(false)
    , m_dword144(0)
    , m_flag148(false)
    , m_componentCursor(0)
    , m_dword168(64)
    , m_dword172(64)
    // FLT_MAX in A double, as the float constant and widened - that is what the binary
    // stores and the `f` suffix is what reproduces it exactly. A "no value yet" sentinel.
    , m_double176(-3.4028235e38f)
    , m_double184(-3.4028235e38f)
    , m_botManager(checkPointer(new BotManager))
    , m_flag196(false)
    , m_botCount(0)
    , m_humanCount(0)
{
    // The four vector members initialize their own storage.

    // Initializes all three shared curve tables for each AIMain construction.
    SCurve::init();
    JCurve::init();
    DecleiningSlopeCurve::init();

    // `aiMainInit` is `IBFModuleCourier` slot +0x10, and the name says what this line is: the
    // AI telling the engine that its top-level object now exists. It is the only outward call the
    // constructor makes.
    static_cast<IBFModuleCourier*>(CommonSingletonServer::m_iBFModuleCourier)->aiMainInit();

    // Clear SAI pointers only after curve initialization and the courier callback.
    // createSAI later fills side slots 1 and 2; slot 0 remains unused.
    m_sai[0] = 0;
    m_sai[1] = 0;
    m_sai[2] = 0;
}

AIMain::~AIMain()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::~AIMain");
}

// The twenty remaining virtual slots, in vtable order. Addresses and sizes are in the [FRONTIER]
// list at the bottom of this file; nothing here adds evidence.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]

void AIMain::addNewPlayer(world::IPlayer* /*player*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::addNewPlayer");
}

void AIMain::popPlayer()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::popPlayer");
}

void AIMain::registerFreeBot(Bot* /*bot*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::registerFreeBot");
}

// Activates the bot associated with a player ID.
bool AIMain::activateBot(int playerId, int side, const std::string& name)
{
    IAIObjectEnvironment* const environment =
        static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment);

    if (!environment->isValidPlayerId(playerId))
        return false;

    if (m_botManager->reactivateBot(playerId, side, name))
        return true;

    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

    return addUnspawnedBot(playerId, side, settings->getStandardPersonality(side));
}

// Adds an unspawned bot to either player-directed orders or its side's strategic AI.
bool AIMain::addUnspawnedBot(int playerId, int side, float* personality)
{
    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

    if (m_botManager->getBotCount() >= settings->getMaxNBots())
        return false;

    m_botManager->addUnspawnedBot(playerId, side, personality);

    // the pool'S length is the hand-off. See the block above.
    Bot* const bot = m_botManager->getBot(
                         static_cast<unsigned int>(m_botManager->getBotCount() - 1));

    if (!UserOrderManager::m_instance->recruitBot(bot, side))
    {
        if (m_sai[side])
            m_sai[side]->addBot(bot);
    }

    m_perBot48.push_back(std::pair<float, float>(0.0f, 0.0f));
    m_perBot60.push_back(std::pair<float, float>(0.0f, 0.0f));
    m_perBot72.push_back(std::pair<float, float>(0.0f, 0.0f));
    m_perBot84.push_back(0.0f);

    return true;
}

void AIMain::deactivateBot(int& /*a*/, int /*b*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::deactivateBot(int&, int)");
}

void AIMain::deactivateBot(int /*a*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::deactivateBot(int)");
}

// Updates population counts when a player is created.
// A human joining may remove a bot when humans count toward the configured population limit.
void AIMain::newPlayerCreated(bool isAI)
{
    if (isAI)
        ++m_botCount;
    else
        ++m_humanCount;

    if (!isAI)
    {
        const IAISettings* const settings =
            static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

        if (settings->getmaxBotsIncludeHumans())
        {
            if (m_botCount > 0)
            {
                IAIGame* const game = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame);

                if (m_humanCount + m_botCount > game->getMaxNumberOfPlayers())
                {
                    IAIPlayerFactory* const factory =
                        static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory);

                    int playersOnSide[2] = { 0, 0 };

                    // Preserve the distinct missing-candidate sentinels for each side.
                    int candidate[2];
                    candidate[0] = -1;
                    candidate[1] = -2;

                    for (int id = 0; id <= 255; ++id)
                    {
                        IAIPlayer* const player = factory->getPlayerFromId(id);

                        if (player)
                        {
                            const int side = player->getSide();

                            if (static_cast<unsigned int>(side - 1) <= 1)
                            {
                                ++playersOnSide[side - 1];

                                // Keep the last AI player found on each side.
                                if (player->getIsAIPlayer())
                                    candidate[side - 1] = player->getId();
                            }
                        }
                    }

                    int pick = 2;

                    if (playersOnSide[0] > playersOnSide[1])
                        pick = 1;

                    // Fall back to the other side when this side has no candidate.
                    if (candidate[pick - 1] < 0)
                        pick = (pick == 1) + 1;

                    // A missing candidate remains negative; preserve the absent second guard.
                    const int victimId = candidate[pick - 1];

                    IAIGameEffector* const effector =
                        static_cast<IAIGameEffector*>(CommonSingletonServer::m_iaiGameEffector);

                    effector->terminatePlayer(factory->getPlayerFromId(victimId));
                    deactivateBot(victimId);
                    effector->terminatePlayer(victimId);

                    --m_botCount;
                }
            }
        }
    }
}

void AIMain::reactivateBot(int /*a*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::reactivateBot");
}

void AIMain::switchSides(int /*a*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::switchSides(int)");
}

// Moves a bot between sides and updates both strategic AI owners.
void AIMain::switchSides(int side, int playerId)
{
    Bot* const bot = IBotManager::instance->getBotFromPlayerId(playerId);

    if (bot)
    {
        if (bot->getSide())
        {
            // Re-read the side after the first virtual call.
            if (bot->getSide() != side)
            {
                if (side)
                {
                    // Remove from the opposite side before updating the bot.
                    getSAI((side == 1) + 1)->removeBot(bot);

                    bot->setSide(side);

                    getSAI(side)->addBot(bot);
                }
            }
        }
    }
}

// Recovered caller: . This retires the entry stop, not the
// scheduled callees' individual frontiers (RW-102); no completed tick is implied by linking it.
bool AIMain::action()
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double budget = timer->getTimeIncrease() * m_aiSystemQuotient;
    const double deadline = timer->getUpdateTime() + budget;

    if (!m_pendingBots.empty())
    {
        for (std::list<Bot*>::iterator it = m_pendingBots.begin(); it != m_pendingBots.end(); ++it)
        {
            const int side = (*it)->getSide();
            if (m_sai[side])
                m_sai[side]->addBot(*it);
        }
        m_pendingBots.clear();
    }

    if (m_saiEnabled)
    {
#if defined(_MSC_VER)
        // [PLATFORM] PE multiplies by the component share before 0.95, then reads the
        // cursor after getNSides. ELF reverses the factors and captures cursor + 1 first.
        m_sai[m_componentCursor + 1]->update(budget * m_componentQuotient * 0.95);
        const unsigned int sides = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)
            ->getNSides();
        m_componentCursor = (m_componentCursor + 1) % sides;
#else
        m_sai[m_componentCursor + 1]->update(budget * 0.95 * m_componentQuotient);
        const unsigned int next = m_componentCursor + 1;
        const unsigned int sides = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)
            ->getNSides();
        m_componentCursor = next % sides;
#endif
    }

#if defined(_MSC_VER)
    // [PLATFORM] PE retains the squad singleton before getGameTime; ELF loads it afterward.
    SquadManager* const squads = SquadManager::m_instance;
    const double gameTime = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
#else
    const double gameTime = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    SquadManager* const squads = SquadManager::m_instance;
#endif
    squads->update(gameTime);
    static_cast<IBFCollisionHandler*>(CommonSingletonServer::m_iBFCollisionHandler)
        ->updateBotProjectiles();

    const double remaining = deadline - timer->getUpdateTime();
    m_botManager->action(remaining);
    static_cast<IAIMineManager*>(CommonSingletonServer::m_iAIMineManager)->update();
    static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment)
        ->damageSeverelyDamagedVehicles();
#if defined(_MSC_VER)
    // PE also computes the final remainder; neither port consumes the value.
    (void)(deadline - timer->getUpdateTime());
#else
    timer->getUpdateTime();
#endif
    return true; // Explicit EAX=1 (ELF) / AL=1 (PE), after every scheduled call returns.
}

bool AIMain::getSAIEnable()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::getSAIEnable");
}

// an unchecked side-indexed pointer read.
SAI* AIMain::getSAI(int side)
{
    return m_sai[side];
}

void AIMain::updateAIObject(IAIObject* /*o*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::updateAIObject");
}

void AIMain::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::reset");
}

void AIMain::resetLevel()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::resetLevel");
}

void AIMain::setAISystemQuotient(const double& /*q*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::setAISystemQuotient");
}

double AIMain::getAISystemQuotient() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::getAISystemQuotient");
}

void AIMain::setPlayerExitedVehicle(IAIObject* /*o*/, int /*a*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::setPlayerExitedVehicle");
}

void AIMain::setPlayerEnteredVehicle(IAIObject* /*o*/, IAIObject* /*v*/, int /*a*/)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIMain::setPlayerEnteredVehicle");
}

// Initializes the AI synchronously from the level script and returns a status message.
// The integer argument is unused.
std::string AIMain::init(int)
{
    // built first, before any work. The message is constructed at the top of the body and
    // returned unchanged - there is no failure path that could alter it.
    std::string result("AIMain initiated.");

    m_initialised = true;

    // two self-installing singletons - see the block in AIMain.h. Both pointers are dropped.
    checkPointer(new BFInformationEnvironment());
    checkPointer(new AIRenderUtil());

    AISingletonServer::updateStaticMethods();

    // the two file reads. See the block above.
    AerialHeightMap::m_instance->readHeightMapFromFile();

    createSAI();
    setSAIEnable(true);

    IAIPathfindingSetup::instance->init();
    IAIPathfindingSetup::instance->loadQuadTrees();

    // the defaulting rule, as A fixup. See the block above.
    const float recomended =
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getRecomendedViewDistance();

    if (static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)->getViewDistance() <= 0.0f)
        static_cast<IAISettingsModifier*>(CommonSingletonServer::m_iaiSettingsModifier)
            ->setViewDistance(recomended);

    return result;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered]
// The methods below stop until their remaining behavior is implemented and validated.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

// Creates one strategic AI instance for each side.
void AIMain::createSAI()
{
    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);
    const IAITerrain* const terrain =
        static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);

    for (int side = 1; side < settings->getNSides() + 1; ++side)
    {
        // `Vec2` is a plain aggregate here, with no two-argument constructor.
        Vec2 min;
        min.x = 0.0f;
        min.y = 0.0f;

        Vec2 max;
        max.x = static_cast<float>(terrain->getWorldSizeX());
        max.y = static_cast<float>(terrain->getWorldSizeZ());

        m_sai[side] = ISAI::create(side, m_dword168, m_dword172, min, max);
    }
}

// Sets the requested state and returns the value accepted, or false when refused.
bool AIMain::setSAIEnable(bool enable)
{
    if (AIStrategicObjectManager::getInstance()->getNumberOfStrategicObjects())
    {
        m_saiEnabled = enable;
        return enable;
    }

    Debug(DebugTypeError, "AI/AIDLL/AI/Main/AIMain.cpp", 910, "")
        << "There are no strategic areas defined! SAI will not be enabled. Game may be unstable.";

    m_saiEnabled = false;
    return false;
}

// [FRONTIER] The remaining declarations require implementations before use.

} // namespace ai
} // namespace hfe
} // namespace dice
