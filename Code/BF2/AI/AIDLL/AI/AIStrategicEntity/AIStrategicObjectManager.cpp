// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIStrategicObjectManager operations.

#include "AIStrategicObjectManager.h"

#include "AIStrategicArea.h"                          // createFromControlPoint constructs one
#include "SAWayPoint.h"                               // the destructor and postLoadInitAreas own them
#include "../AIUtils/checkPointer.h"
#include "../AIUtils/DebugRendering/AIRenderUtil.h"   // addStrategicObject's null-guarded tail
#include "../Main/AIDllFrontier.h"
#include "../../../../Core/ConsoleObjectMacro.h"      // the aiStrategicArea.* binding at the foot

namespace dice {
namespace hfe {
namespace ai {

AIStrategicObjectManager* AIStrategicObjectManager::m_instance = 0;

// "Closest" is the greatest virtual gravity
// starting at -1, not a distance calculation here. Strict comparison preserves
// first-wins ties and ignores unordered results. Both ports reload the selected
// pointer after gravity and the vector end on each iteration; neither guards nulls.
AIStrategicObject* AIStrategicObjectManager::getClosestObject(
    const Vec3& position, int side) const
{
    AIStrategicObject* closest = 0;
    float best = -1.0f;
    for (std::vector<AIStrategicObject*>::const_iterator it = m_strategicObjects.begin();
         it != m_strategicObjects.end(); ++it)
    {
        const double gravity = (*it)->gravity(position, side);
        if (gravity > best)
        {
            best = static_cast<float>(gravity);
            closest = *it;
        }
    }
    return closest;
}

// WPContainer - , each.
// mov eax, this
// test a2, a2
// jz ret ; -> this
// lea eax, [this+12] ; -> this+12
// so any non-zero selects the second list. Written literally.

std::vector<SAWayPoint*>& AIStrategicObjectManager::WPContainer::getWayPoints(int type)
{
    if (type)
        return m_wayPointsB;

    return m_wayPointsA;
}

const std::vector<SAWayPoint*>& AIStrategicObjectManager::WPContainer::getWayPoints(int type) const
{
    if (type)
        return m_wayPointsB;

    return m_wayPointsA;
}

// Initializes strategic-area collections and the active selection.

AIStrategicObjectManager::AIStrategicObjectManager()
    : m_areasInitialised(false)
    , m_b84(false)
    , m_wayPointsToDrawFirst(-1)
    , m_wayPointsToDrawSecond(-1)
{
    m_strategicObjects.resize(0);
    m_objectsByCategory.resize(0);

    if (!m_instance)
        m_instance = this;

    m_activeArea = m_strategicAreas.end();

    m_categoryNames.resize(0);
}

// Deletes owned areas, waypoints and bridges, and clears the singleton if it refers to this object.

AIStrategicObjectManager::~AIStrategicObjectManager()
{
    if (m_instance == this)
        m_instance = 0;

    for (std::map<std::string, AIStrategicArea*>::iterator area = m_strategicAreas.begin();
         area != m_strategicAreas.end(); )
    {
        if (area->second)
            delete area->second;

        m_strategicAreas.erase(area++);
    }

    for (std::map<std::pair<int, int>, WPContainer>::iterator entry = m_wayPoints.begin();
         entry != m_wayPoints.end(); ++entry)
    {
        std::vector<SAWayPoint*>& first = entry->second.getWayPoints(0);

        for (std::vector<SAWayPoint*>::iterator point = first.begin();
             point != first.end(); ++point)
        {
            if (*point)
                delete *point;
        }

        std::vector<SAWayPoint*>& second = entry->second.getWayPoints(1);

        for (std::vector<SAWayPoint*>::iterator point = second.begin();
             point != second.end(); ++point)
        {
            if (*point)
                delete *point;
        }
    }

    deleteBridges();
}

// Initializes areas and waypoints after level loading, before pathfinding-map initialization.
// Waypoint containers use an unordered area pair as their key.

void AIStrategicObjectManager::postLoadInitAreas()
{
    WPContainer prototype;

    for (std::map<std::string, AIStrategicArea*>::iterator entry = m_strategicAreas.begin();
         entry != m_strategicAreas.end(); ++entry)
    {
        AIStrategicArea* const area = entry->second;

        area->postLoadInit();

        for (std::vector<AIStrategicObject*>::iterator neighbour = area->m_neighbours.begin();
             neighbour != area->m_neighbours.end(); ++neighbour)
        {
            const int mine  = area->m_index;
            const int other = (*neighbour)->m_index;

            const int low  = (mine < other)  ? mine : other;
            const int high = (mine >= other) ? mine : other;

            m_wayPoints.insert(
                std::pair<std::pair<int, int>, WPContainer>(std::pair<int, int>(low, high),
                                                            prototype));
        }
    }

    for (std::map<std::pair<int, int>, WPContainer>::iterator entry = m_wayPoints.begin();
         entry != m_wayPoints.end(); ++entry)
    {
        std::vector<SAWayPoint*>& first = entry->second.getWayPoints(0);

        for (std::vector<SAWayPoint*>::iterator point = first.begin();
             point != first.end(); ++point)
        {
            (*point)->init();
        }

        std::vector<SAWayPoint*>& second = entry->second.getWayPoints(1);

        for (std::vector<SAWayPoint*>::iterator point = second.begin();
             point != second.end(); ++point)
        {
            (*point)->init();
        }
    }
}

// The emitted loop walks the raw node chain and frees each `BridgeSegment*` with a bare
// `operator delete` - no destructor call, which is what `delete` compiles to for a trivially
// destructible type and is how `BridgeSegment` was pinned as a plain record. It then calls
// `_List_base::__clear`, which in this libstdc++ both frees the nodes and re-links the header, so
// the source is `clear` and calling it twice (here and again from `~list`) is safe.

void AIStrategicObjectManager::deleteBridges()
{
    for (std::list<BridgeSegment*>::iterator i = m_bridgeSegments.begin();
         i != m_bridgeSegments.end(); ++i)
    {
        delete *i;
    }

    m_bridgeSegments.clear();
}

// Tracks each announced AI-enabled bridge segment by object pointer. Lookup stops at
// the first match; unknown objects are ignored by update and removal operations.
void AIStrategicObjectManager::createBridgeSegment(IAIObject* object)
{
    BridgeSegment* const segment = new BridgeSegment;   // zeroes the position only - +4 stays unset
    segment->m_object = object;
    segment->m_destroyed = false;
    // The position is zeroed again, as its own statement: the PE stores it a second time outside
    // the new-expression's null-checked constructor block, the ELF copies a zeroed 12-byte temporary.
    const Vec3 origin = { 0.0f, 0.0f, 0.0f };
    segment->m_position = origin;
    m_bridgeSegments.push_back(segment);
}

// The record is freed with a bare delete (a plain struct), then its node is erased.
void AIStrategicObjectManager::removeBridgeSegment(IAIObject* object)
{
    for (std::list<BridgeSegment*>::iterator i = m_bridgeSegments.begin();
         i != m_bridgeSegments.end(); ++i)
    {
        if ((*i)->m_object == object)
        {
            delete *i;
            m_bridgeSegments.erase(i);
            return;
        }
    }
}

void AIStrategicObjectManager::bridgeSegmentDestroyed(IAIObject* object, int value)
{
    for (std::list<BridgeSegment*>::iterator i = m_bridgeSegments.begin();
         i != m_bridgeSegments.end(); ++i)
    {
        if ((*i)->m_object == object)
        {
            (*i)->m_destroyed = true;
            (*i)->m_i4 = value;
            return;
        }
    }
}

// Only the flag is cleared; +4 keeps the value the destruction wrote.
void AIStrategicObjectManager::bridgeSegmentRepaired(IAIObject* object)
{
    for (std::list<BridgeSegment*>::iterator i = m_bridgeSegments.begin();
         i != m_bridgeSegments.end(); ++i)
    {
        if ((*i)->m_object == object)
        {
            (*i)->m_destroyed = false;
            return;
        }
    }
}
// [FRONTIER] Reset operations require the remaining strategic-area lifecycle behavior.
// [RETIRE-WHEN: aidll-02-skeleton] Implement and validate area resets and collection rebuilding.

bool AIStrategicObjectManager::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicObjectManager::reset");
}

bool AIStrategicObjectManager::resetAreas()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicObjectManager::resetAreas");
}

// The accessors that are two or three instructions. Each is transcribed from its own body; none of
// them needs a type this file does not have.
// `BF2AI::clear` destroys the object through this rather than through the
// static, which is the only reason it exists.
AIStrategicObjectManager* AIStrategicObjectManager::getInstance()
{
    return m_instance;
}

// - `return (char *)this + 4;`
const std::vector<AIStrategicObject*>& AIStrategicObjectManager::getStrategicObjects() const
{
    return m_strategicObjects;
}

// - the byte difference shifted right by two, i.e. the element count.
int AIStrategicObjectManager::getNumberOfStrategicObjects() const
{
    return (int)m_strategicObjects.size();
}

// Returns the category's object vector. The caller must supply a valid category index.
const std::vector<AIStrategicObject*>&
AIStrategicObjectManager::getStrategicObjectsOfCategory(int category) const
{
    return m_objectsByCategory[category];
}

// - same shape, four bytes per `std::string`, again unchecked.
const std::string& AIStrategicObjectManager::getCategoryName(int category) const
{
    return m_categoryNames[category];
}

int AIStrategicObjectManager::getNumberOfCategories()
{
    return (int)m_categoryNames.size();
}

// - `return (char *)this + 32;`
const std::map<std::string, AIStrategicArea*>& AIStrategicObjectManager::getStrategicAreas() const
{
    return m_strategicAreas;
}

// Returns null while no area is active - `m_activeArea` starts at `end` and
// the constructor is the only thing that puts it there.
AIStrategicArea* AIStrategicObjectManager::getActiveArea()
{
    if (m_activeArea != m_strategicAreas.end())
        return m_activeArea->second;

    return 0;
}

// This one checks both ends, and returns null rather than clamping.
AIStrategicObject* AIStrategicObjectManager::getStrategicObject(int index) const
{
    if (index < 0)
        return 0;

    if (index >= (int)m_strategicObjects.size())
        return 0;

    return m_strategicObjects[index];
}

// Disables waypoint drawing by clearing the first key component; leaves the second unchanged.
void AIStrategicObjectManager::clearWaypointToDraw()
{
    m_wayPointsToDrawFirst = -1;
}

// [FRONTIER] Waypoint drawing cleanup remains unimplemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding AIStrategicObjectManager operation.
void AIStrategicObjectManager::undrawWayPoints() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIStrategicObjectManager::undrawWayPoints");
}

// Returns the category id for an existing name. The caller must ensure that exact
// name exists; merely having a nonempty category map is insufficient.
int AIStrategicObjectManager::getCategoryFromName(const std::string& name) const
{
    return m_categories.find(name)->second;
}

// Adds an object to the category table and assigns its index. The object and category must be valid.
// Other category rows retain null entries at that index.
void AIStrategicObjectManager::addStrategicObject(AIStrategicObject* object)
{
    const int index = (int)m_strategicObjects.size();

    m_strategicObjects.resize(index + 1);
    m_strategicObjects[index] = object;
    object->m_index = index;

    for (int category = 0; category < (int)m_objectsByCategory.size(); ++category)
    {
        m_objectsByCategory[category].resize(index + 1);

        if (category == object->m_category)
            m_objectsByCategory[category][index] = object;
        else
            m_objectsByCategory[category][index] = 0;
    }

    if (AIRenderUtil::m_instance)
        AIRenderUtil::m_instance->updateNumberOfStrategicAreas();
}

// Initializes areas with explicit bounds. Control-point areas wait for postLoadInit().
void AIStrategicObjectManager::initAreas()
{
    for (std::map<std::string, AIStrategicArea*>::iterator i = m_strategicAreas.begin();
         i != m_strategicAreas.end(); ++i)
    {
        if (!i->second->m_fromControlPoint)
            i->second->init();
    }
}

// Creates a strategic area from a named control point and selects it.
// Returns false and clears the active selection when creation is refused.
bool AIStrategicObjectManager::createFromControlPoint(const std::string& name,
                                                      unsigned controlPointId,
                                                      float radius,
                                                      const std::string& category)
{
    if (m_strategicAreas.find(name) != m_strategicAreas.end())
    {
        m_activeArea = m_strategicAreas.end();
        return false;
    }

    AIStrategicArea* const area = checkPointer(new AIStrategicArea(name, controlPointId, radius));
    if (!area)
    {
        m_activeArea = m_strategicAreas.end();
        return false;
    }

    if (getNumberOfCategories() <= 0)
        area->m_category = 0;
    else
        area->m_category = getCategoryFromName(category);

    AIStrategicObjectManager::getInstance()->addStrategicObject(area);

    m_strategicAreas[name] = area;
    m_areaNames[area] = name;
    m_activeArea = m_strategicAreas.find(name);
    return true;
}

// Console bindings for strategic-area configuration.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

namespace {

const char* const kAiStrategicAreaConsoleTag = "aiStrategicArea";

void consoleAiStrategicAreaCreateFromControlPoint(const std::string& name,
                                                  unsigned int controlPointId,
                                                  float radius)
{
    // No null check on the singleton, matching every other console forwarder in this DLL:
    // `BF2AI::init` builds the manager as step 17 during `initDll`, and `GameLogic::load` runs
    // `StrategicAreas.ai` long afterwards. The `isObjectActive` test below is the real guard.
    AIStrategicObjectManager::getInstance()->createFromControlPoint(
        name, controlPointId, radius, std::string());   // [CHOSEN] empty category - see above
}

} // anonymous namespace

BF2_CONSOLE_OBJECT("aiStrategicArea", 2, 0)
BF2_CONSOLE_METHOD_3(createFromControlPoint, consoleAiStrategicAreaCreateFromControlPoint,
                     AIStrategicObjectManager::getInstance() != 0,
                     std::string, unsigned int, float, kAiStrategicAreaConsoleTag)

// Handed to the console registry by AIModule::init, beside the `ai` and `aiSettings` lists.
ConsoleObjectList& aiStrategicAreaConsoleObjects()
{
    return consoleBindingList(kAiStrategicAreaConsoleTag);
}

} // namespace ai
} // namespace hfe
} // namespace dice
