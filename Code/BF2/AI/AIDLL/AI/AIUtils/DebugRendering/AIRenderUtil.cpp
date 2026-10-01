// [INFERRED] Source placement.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIRenderUtil operations.

#include "AIRenderUtil.h"

#include "../../AIStrategicEntity/AILandingZoneManager.h"
#include "../../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../../DrawPrimitives/DPManager.h"
#include "../../Main/AIConsole.h"
#include "../../Main/AIDllFrontier.h"
#include "../../Main/AISettingsInterfaces.h"
#include "../../Main/BotManager.h"
#include "../../Main/CommonSingletonServer.h"

namespace dice {
namespace hfe {
namespace ai {

AIRenderUtil* AIRenderUtil::m_instance = 0;

// Primitive records require a live draw manager. Each deletePrimitives fetches it once
// and releases each handle through the matching primitive-type operation.

AIRenderUtil::BotPrimitives::BotPrimitives()
    : m_line0(0)
    , m_line1(0)
    , m_circle0(0)
    , m_circle1(0)
    , m_line2(0)
    , m_line3(0)
    , m_text3d(0)
{
}

AIRenderUtil::BotPrimitives::BotPrimitives(DrawPrimitive* line0, DrawPrimitive* line1,
                                           DrawPrimitive* circle0, DrawPrimitive* circle1,
                                           DrawPrimitive* line2, DrawPrimitive* line3,
                                           DrawPrimitive* text3d)
    : m_line0(line0)
    , m_line1(line1)
    , m_circle0(circle0)
    , m_circle1(circle1)
    , m_line2(line2)
    , m_line3(line3)
    , m_text3d(text3d)
{
}

// declared and empty on purpose - see the header. Removing it would change which temporaries get
// a destructor call in the emitted code.
AIRenderUtil::BotPrimitives::~BotPrimitives()
{
}

void AIRenderUtil::BotPrimitives::deletePrimitives()
{
    IDPManager* const manager = static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager);

    manager->deleteLine(m_line0);
    manager->deleteLine(m_line1);
    manager->deleteCircle(m_circle0);
    manager->deleteCircle(m_circle1);
    manager->deleteLine(m_line2);
    manager->deleteLine(m_line3);
    manager->delete3dText(m_text3d);
}

AIRenderUtil::StrategicAreaPrimitives::StrategicAreaPrimitives()
    : m_line0(0)
    , m_line1(0)
    , m_circle0(0)
    , m_circle1(0)
    , m_text3d(0)
{
}

AIRenderUtil::StrategicAreaPrimitives::StrategicAreaPrimitives(
        DrawPrimitive* line0, DrawPrimitive* line1, const std::vector<DrawPrimitive*>& lines,
        DrawPrimitive* circle0, DrawPrimitive* circle1, DrawPrimitive* text3d,
        const std::vector<DrawPrimitive*>& circles)
    : m_line0(line0)
    , m_line1(line1)
    , m_lines(lines)
    , m_circle0(circle0)
    , m_circle1(circle1)
    , m_text3d(text3d)
    , m_circles(circles)
{
}

void AIRenderUtil::StrategicAreaPrimitives::deletePrimitives()
{
    IDPManager* const manager = static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager);

    manager->deleteLine(m_line0);
    manager->deleteLine(m_line1);

    for (std::vector<DrawPrimitive*>::iterator it = m_lines.begin(); it != m_lines.end(); ++it)
        manager->deleteLine(*it);

    m_lines.resize(0);

    manager->deleteCircle(m_circle0);
    manager->deleteCircle(m_circle1);
    manager->delete3dText(m_text3d);

    // Deletes circle primitives without clearing the vector; its entries remain after cleanup.
    for (std::vector<DrawPrimitive*>::iterator it = m_circles.begin(); it != m_circles.end(); ++it)
        manager->deleteCircle(*it);
}

AIRenderUtil::LandingZonePrimitives::LandingZonePrimitives()
    : m_line0(0)
    , m_line1(0)
    , m_line2(0)
    , m_line3(0)
    , m_text3d(0)
{
}

AIRenderUtil::LandingZonePrimitives::LandingZonePrimitives(DrawPrimitive* line0,
                                                           DrawPrimitive* line1,
                                                           DrawPrimitive* line2,
                                                           DrawPrimitive* line3,
                                                           DrawPrimitive* text3d)
    : m_line0(line0)
    , m_line1(line1)
    , m_line2(line2)
    , m_line3(line3)
    , m_text3d(text3d)
{
}

void AIRenderUtil::LandingZonePrimitives::deletePrimitives()
{
    IDPManager* const manager = static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager);

    manager->deleteLine(m_line0);
    manager->deleteLine(m_line1);
    manager->deleteLine(m_line2);
    manager->deleteLine(m_line3);
    manager->delete3dText(m_text3d);
}

AIRenderUtil::PatrolPathPrimitives::PatrolPathPrimitives()
    : m_text3d(0)
{
}

AIRenderUtil::PatrolPathPrimitives::PatrolPathPrimitives(
        DrawPrimitive* text3d,
        const std::vector<std::pair<DrawPrimitive*, DrawPrimitive*> >& segments)
    : m_segments(segments)
    , m_text3d(text3d)
{
}

void AIRenderUtil::PatrolPathPrimitives::deletePrimitives()
{
    IDPManager* const manager = static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager);

    // One line and one circle per patrol node: the leg and the waypoint marker.
    for (std::vector<std::pair<DrawPrimitive*, DrawPrimitive*> >::iterator it = m_segments.begin();
         it != m_segments.end(); ++it)
    {
        manager->deleteLine(it->first);
        manager->deleteCircle(it->second);
    }

    m_segments.resize(0);
    manager->delete3dText(m_text3d);
}

AIRenderUtil::GuardPositionPrimitives::GuardPositionPrimitives()
    : m_marker(static_cast<DrawPrimitive*>(0), static_cast<DrawPrimitive*>(0))
    , m_text3d(0)
{
}

AIRenderUtil::GuardPositionPrimitives::GuardPositionPrimitives(
        DrawPrimitive* text3d, const std::pair<DrawPrimitive*, DrawPrimitive*>& marker)
    : m_marker(marker)
    , m_text3d(text3d)
{
}

void AIRenderUtil::GuardPositionPrimitives::deletePrimitives()
{
    IDPManager* const manager = static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager);

    manager->deleteCircle(m_marker.first);
    manager->deleteLine(m_marker.second);
    manager->delete3dText(m_text3d);
}

AIRenderUtil::CoverPositionPrimitives::CoverPositionPrimitives()
    : m_line0(0)
    , m_line1(0)
{
}

AIRenderUtil::CoverPositionPrimitives::CoverPositionPrimitives(
        DrawPrimitive* line0, DrawPrimitive* line1, const std::vector<DrawPrimitive*>& linesA,
        const std::vector<DrawPrimitive*>& linesB, const std::vector<DrawPrimitive*>& linesC)
    : m_line0(line0)
    , m_line1(line1)
    , m_linesA(linesA)
    , m_linesB(linesB)
    , m_linesC(linesC)
{
}

void AIRenderUtil::CoverPositionPrimitives::deletePrimitives()
{
    IDPManager* const manager = static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager);

    manager->deleteLine(m_line0);
    manager->deleteLine(m_line1);

    // All three vectors are emptied here - compare StrategicAreaPrimitives, which forgets one.
    for (std::vector<DrawPrimitive*>::iterator it = m_linesA.begin(); it != m_linesA.end(); ++it)
        manager->deleteLine(*it);
    m_linesA.resize(0);

    for (std::vector<DrawPrimitive*>::iterator it = m_linesB.begin(); it != m_linesB.end(); ++it)
        manager->deleteLine(*it);
    m_linesB.resize(0);

    for (std::vector<DrawPrimitive*>::iterator it = m_linesC.begin(); it != m_linesC.end(); ++it)
        manager->deleteLine(*it);
    m_linesC.resize(0);
}

// Sizes debug-primitive collections for bots, strategic areas and landing zones.
AIRenderUtil::AIRenderUtil()
    : m_text2dA(0)
    , m_text2dB(0)
{
    if (!m_instance)
        m_instance = this;

    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

    const int maxBots = settings->getMaxNBots();
    if (maxBots != -1)
        m_botPrimitives.resize(maxBots);

    m_strategicAreaPrimitives.resize(
        AIStrategicObjectManager::getInstance()->getNumberOfStrategicObjects());

    m_landingZonePrimitives.resize(
        AILandingZoneManager::m_instance->getNumberOfLandingZones());

    m_pathCircleA = 0;
    m_pathCircleB = 0;
    m_pathLine = 0;
}

// `unrenderAll` first, and that is what makes the two maps leak-free - see the header block.
AIRenderUtil::~AIRenderUtil()
{
    if (m_instance == this)
        m_instance = 0;

    unrenderAll();
}

bool AIRenderUtil::init()
{
    return true;
}

// empty in retail - recovered, not stubbed.
void AIRenderUtil::exit()
{
}

// The thirteen empty bodies. Five bytes each in the shipped DLL. See the file header: these are
// recovered source, and a diagnostic stop in any of them would be a fabrication.

void AIRenderUtil::renderBot(BotMain*, const std::pair<float, float>&,
                             const std::pair<float, float>&, float, const std::pair<float, float>&)
{
}

void AIRenderUtil::renderPath(int, bool, const std::vector<Vec3>&, unsigned int)
{
}

void AIRenderUtil::renderWaypointList(BotMain*)
{
}

void AIRenderUtil::renderPathFinding(BotMain*)
{
}

void AIRenderUtil::renderAiming(BotMain*)
{
}

void AIRenderUtil::renderStrategicArea(AIStrategicArea*)
{
    // Its unrender counterpart at is of real work. Nothing this build draws
    // can ever need destroying, and the destroy path is kept anyway.
}

void AIRenderUtil::renderLandingZone(AILandingZone*, const Vec3&)
{
}

void AIRenderUtil::renderBotStats(BotMain*, const std::pair<float, float>&,
                                  const std::pair<float, float>&, float,
                                  const std::pair<float, float>&)
{
}

void AIRenderUtil::renderBehaviourStats(BotMain*, std::vector<std::string>&)
{
}

void AIRenderUtil::renderObstacleInfo(Bot*)
{
}

void AIRenderUtil::initAIMeshRendering()
{
}

void AIRenderUtil::cleanUpAIMeshRendering()
{
}

void AIRenderUtil::renderAIMeshes()
{
}

// Draws a two-point sensing line: side 1 is red, other sides blue, with a pale color
// when unseen. Clears and refills the member vertex vector on every call.
void AIRenderUtil::addSensingLine(const Vec3& from, const Vec3& to, int side, bool visible)
{
    m_sensingLine.resize(0);
    m_sensingLine.push_back(from);
    m_sensingLine.push_back(to);

    unsigned int colour;
    unsigned int paleColour;

    if (side == 1)
    {
        colour     = 0xFFFF0000u;      // red
        paleColour = 0xFFFFAAAAu;      // pale red
    }
    else
    {
        colour     = 0xFF0000FFu;      // blue
        paleColour = 0xFFAAAAFFu;      // pale blue
    }

    if (!visible)
        colour = paleColour;

    static_cast<IDPStaticCreator*>(CommonSingletonServer::m_iDPStaticCreator)
        ->createStaticLine(0.1f, colour, m_sensingLine, 0.03f);
}

// Clears debug rendering. The manager null check covers only the two direct calls;
// primitive-record cleanup reached afterward still requires a live manager.
void AIRenderUtil::unrenderAll()
{
    if (CommonSingletonServer::m_iDPManager)
    {
        IDPManager* const manager = static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager);

        manager->delete2dText(m_text2dA);
        m_text2dA = 0;
        manager->delete2dText(m_text2dB);
        m_text2dB = 0;
    }

    unrenderAllBots();
    unrenderAllStrategicAreas();
    unrenderAllPatrolPaths();
    unrenderAllGuardPositions();
    unrenderAllCoverPositions();
    unrendenderPathfindingDebug();

    AIConsole::m_instance->removeCameraText();
}

// [FRONTIER] Bot primitive cleanup requires the remaining bot rendering contract.
// [RETIRE-WHEN: botmain-bases] Complete and validate the corresponding AIRenderUtil operation.
void AIRenderUtil::unrenderBot(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::unrenderBot (Bot::getBotId, botmain-bases)");
}

// [FRONTIER] Releases every bot's debug primitives once the required bot interfaces are available.
// [RETIRE-WHEN: botmain-bases] Remove when the corresponding implementation is complete and validated.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void AIRenderUtil::unrenderAllBots()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::unrenderAllBots (IBotManager, botmanager-bases)");
}

// [FRONTIER] Area primitive cleanup requires its validated primitive index.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding AIRenderUtil operation.
void AIRenderUtil::unrenderStrategicArea(AIStrategicArea*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::unrenderStrategicArea (AIStrategicArea layout)");
}

// Copies the strategic-area map before iterating it to remove debug rendering.
// Preserve the by-value snapshot and its owned string keys.
void AIRenderUtil::unrenderAllStrategicAreas()
{
    AIStrategicObjectManager* const manager = AIStrategicObjectManager::getInstance();
    if (!manager)
        return;

    manager->undrawWayPoints();

    const std::map<std::string, AIStrategicArea*> areas = manager->getStrategicAreas();
    for (std::map<std::string, AIStrategicArea*>::const_iterator it = areas.begin();
         it != areas.end(); ++it)
        unrenderStrategicArea(it->second);
}

// Destroys primitives, deletes their owned record, then erases the map entry.
// Existing entries must hold nonnull records; the later null check does not guard first use.
void AIRenderUtil::unrenderPatrolPath(PatrolPath* path)
{
    const std::map<PatrolPath*, PatrolPathPrimitives*>::iterator it =
        m_patrolPathPrimitives.find(path);

    if (it == m_patrolPathPrimitives.end())
        return;

    it->second->deletePrimitives();
    if (it->second)
        delete it->second;

    m_patrolPathPrimitives.erase(it);
}

void AIRenderUtil::unrenderAllPatrolPaths()
{
    std::map<PatrolPath*, PatrolPathPrimitives*>::iterator it = m_patrolPathPrimitives.begin();
    while (it != m_patrolPathPrimitives.end())
    {
        it->second->deletePrimitives();
        if (it->second)
            delete it->second;

        m_patrolPathPrimitives.erase(it++);
    }
}

void AIRenderUtil::unrenderGuardPosition(GuardPosition* position)
{
    const std::map<GuardPosition*, GuardPositionPrimitives*>::iterator it =
        m_guardPositionPrimitives.find(position);

    if (it == m_guardPositionPrimitives.end())
        return;

    it->second->deletePrimitives();
    if (it->second)
        delete it->second;

    m_guardPositionPrimitives.erase(it);
}

void AIRenderUtil::unrenderAllGuardPositions()
{
    std::map<GuardPosition*, GuardPositionPrimitives*>::iterator it =
        m_guardPositionPrimitives.begin();

    while (it != m_guardPositionPrimitives.end())
    {
        it->second->deletePrimitives();
        if (it->second)
            delete it->second;

        m_guardPositionPrimitives.erase(it++);
    }
}

// an upper bound and no lower one - a negative index indexes before the buffer. Preserved.
void AIRenderUtil::unrenderCoverPosition(int index)
{
    if (index < static_cast<int>(m_coverPositionPrimitives.size()))
        m_coverPositionPrimitives[index].deletePrimitives();
}

void AIRenderUtil::unrenderAllCoverPositions()
{
    for (std::vector<CoverPositionPrimitives>::iterator it = m_coverPositionPrimitives.begin();
         it != m_coverPositionPrimitives.end(); ++it)
        it->deletePrimitives();
}

// dice's spelling - "unrendender". Kept.
// this is what names the last three members: two circles and a line, by their delete slots.
void AIRenderUtil::unrendenderPathfindingDebug()
{
    if (!CommonSingletonServer::m_iDPManager)
        return;

    IDPManager* const manager = static_cast<IDPManager*>(CommonSingletonServer::m_iDPManager);

    manager->deleteCircle(m_pathCircleA);
    m_pathCircleA = 0;
    manager->deleteCircle(m_pathCircleB);
    m_pathCircleB = 0;
    manager->deleteLine(m_pathLine);
    m_pathLine = 0;
}

// Grows primitive storage when the world gains entities; never shrinks it.
// Preserves the second size read used to compute the resize argument.

void AIRenderUtil::updateNumberOfStrategicAreas()
{
    const int count = AIStrategicObjectManager::getInstance()->getNumberOfStrategicObjects();

    if (count > static_cast<int>(m_strategicAreaPrimitives.size()))
        m_strategicAreaPrimitives.resize(count);
}

void AIRenderUtil::updateNumberOfLandingZones()
{
    const int count = AILandingZoneManager::m_instance->getNumberOfLandingZones();

    if (count > static_cast<int>(m_landingZonePrimitives.size()))
        m_landingZonePrimitives.resize(count);
}

// [FRONTIER] Remaining console-driven rendering operations stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding AIRenderUtil operation.

void AIRenderUtil::setCoordinate(const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::setCoordinate");
}

void AIRenderUtil::renderStrategies()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::renderStrategies");
}

void AIRenderUtil::drawArrow(float, unsigned int, const Vec3&, const Vec3&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::drawArrow");
}

void AIRenderUtil::drawAxisPoint(float, unsigned int, const Vec3&, const Vec3&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::drawAxisPoint");
}

void AIRenderUtil::renderPatrolPath(const std::string&, PatrolPath*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::renderPatrolPath");
}

void AIRenderUtil::renderGuardPosition(const std::string&, GuardPosition*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::renderGuardPosition");
}

void AIRenderUtil::renderCoverPosition(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::renderCoverPosition");
}

void AIRenderUtil::redenderPathfindingDebug()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIRenderUtil::redenderPathfindingDebug");
}

} // namespace ai
} // namespace hfe
} // namespace dice
