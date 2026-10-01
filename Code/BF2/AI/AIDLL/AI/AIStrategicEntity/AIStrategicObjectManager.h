// Owns strategic areas, waypoints and bridges, and tracks the active area for console commands.

#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECTMANAGER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECTMANAGER_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

// Vec2 and Vec3 cannot be forward-declared here: in this tree they are typedefs to
// `dice::hfe::Math::Vec2/Vec3`, not classes in `dice::hfe`, so a local `struct Vec3;` is a
// redefinition with a different basic type (C2371). The header has to come in.
#include "../../../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace ai {

class AIStrategicArea;
class AIStrategicObject;
class IAIObject;
class SAWayPoint;

class AIStrategicObjectManager
{
public:
    // Waypoint collections for a pair of connected strategic areas.
    class WPContainer
    {
    public:
        std::vector<SAWayPoint*>& getWayPoints(int type);
        const std::vector<SAWayPoint*>& getWayPoints(int type) const;

    private:
        /*  +0 */ std::vector<SAWayPoint*> m_wayPointsA;   // [UNRESOLVED] which list this is
        /* +12 */ std::vector<SAWayPoint*> m_wayPointsB;
    };

    // Bridge segment record linking a position and the affected strategic objects.
    class BridgeSegment
    {
    public:
        BridgeSegment()
        {
            m_position.x = 0.0f;
            m_position.y = 0.0f;
            m_position.z = 0.0f;
        }

        /*  +0 */ bool m_destroyed;       // 0 at creation; `bridgeSegmentDestroyed` sets it to 1
        /*  +4 */ int m_i4;              // [UNRESOLVED] - written only by `bridgeSegmentDestroyed`
        /*  +8 */ IAIObject* m_object;   // the search key for all three of the methods above
        /* +12 */ Vec3 m_position;       // [INFERRED] - three zeroed floats, never read here
    };

    AIStrategicObjectManager();
    ~AIStrategicObjectManager();                    // , not virtual

    virtual bool reset();                           // - slot 0
    virtual bool resetAreas();                      // - slot 1

    static AIStrategicObjectManager* getInstance();

    // strategic objects
    const std::vector<AIStrategicObject*>& getStrategicObjects() const;
    int getNumberOfStrategicObjects() const;
    const std::vector<AIStrategicObject*>& getStrategicObjectsOfCategory(int) const;
    AIStrategicObject* getStrategicObject(int) const;
    AIStrategicObject* getStrategicObject(const std::string&) const;
    AIStrategicObject* getStrategicObjectOfCategory(int, int) const;
    void addStrategicObject(AIStrategicObject*);
    void removeStrategicObject(int);
    AIStrategicObject* getClosestObject(const Vec3&, int) const;
    AIStrategicObject* getClosestObject(const Vec3&, int, bool) const;
    AIStrategicObject* getClosestObject(const Vec3&, int, int, int) const;
    AIStrategicObject* getClosestObjectinCategory(const Vec3&, int, int, int, int) const;

    // strategic areas
    AIStrategicArea* getStrategicArea(int) const;
    AIStrategicArea* getStrategicAreaOfCategory(int, int) const;
    const std::map<std::string, AIStrategicArea*>& getStrategicAreas() const;
    std::string getStrategicAreaNames();
    AIStrategicArea* getActiveArea();
    void setActive(std::string);
    std::string getActive() const;
    // Initializes strategic areas; has no return value.
    void initAreas();
    void postLoadInitAreas();
    Vec2 getPosition() const;
    bool isInside(const std::string&, const Vec2&);
    void create(std::string, Vec2, Vec2, float, float, const std::string&);
    // Creates an area from a control point. Returns false for duplicate names or allocation
    // failure and true after successful creation.
    bool createFromControlPoint(const std::string&, unsigned, float, const std::string&);
    void removeActive();
    void setFlankPosition(Vec3, Vec3);
    std::string list() const;

    // neighbours
    void addNeighbour(std::string);
    void addTwoWayNeighbour(std::string, std::string);
    void removeAllNeighbours();
    std::string getNeighbours();

    // categories
    int getNumberOfCategories();
    const std::string& getCategoryName(int) const;
    int getCategoryFromName(const std::string&) const;
    void createNewCategory(const std::string&);

    // object type flags
    void addObjectTypeFlag(int);
    void removeObjectTypeFlag(int);
    void setObjectTypeFlags(int);
    int getObjectTypeFlags();
    void removeAllObjectTypeFlags();
    std::string getObjectTypeFlagMapping() const;

    // order positions
    bool setOrderPosition(std::string, const Vec3&);
    Vec3 getOrderPosition(std::string);
    std::string listOrderPositions();
    std::string checkAllOrderPositions() const;

    // waypoints
    void addWayPoint(const Vec3&, const std::string&, const std::string&, const std::string&);
    void removeWayPoint(const Vec3&, const std::string&, const std::string&, const std::string&);
    void clearWayPoints(const std::string&, const std::string&, const std::string&);
    const std::vector<SAWayPoint*>& getWaypoints(int, int, int) const;
    const std::vector<SAWayPoint*>& getWaypoints(int, const AIStrategicArea*, const AIStrategicArea*) const;
    void drawWayPoints() const;
    void undrawWayPoints() const;
    void setWayPointsToDraw(const std::string&, const std::string&);
    void clearWaypointToDraw();
    std::string checkAllWayPoints() const;

    // bridges
    void createBridgeSegment(IAIObject*);
    void removeBridgeSegment(IAIObject*);
    void deleteBridges();
    void resetBridges();
    void bridgeSegmentDestroyed(IAIObject*, int);
    void bridgeSegmentRepaired(IAIObject*);
    bool getClosestDestroyedBridgeSegment(Vec3, bool, int, Vec3&, float&);

    static AIStrategicObjectManager* m_instance;

private:
    // layout, , read from the constructor and confirmed member by member against the
    // destructor's reverse-order teardown
    // MSVC's `sizeof` will differ - libstdc++ 3.x puts a `_Rb_tree` in twelve bytes (a header-node
    // pointer, a node count and four bytes of empty comparator) and a `std::list` in four.
    /*   +4 */ std::vector<AIStrategicObject*> m_strategicObjects;
    /*  +16 */ std::vector<std::vector<AIStrategicObject*> > m_objectsByCategory;
    // [UNRESOLVED] Cleared by construction and resets, but no implemented method sets or
    // reads it. The current name does not establish its meaning.
    /*  +28 */ bool m_areasInitialised;
    /*  +32 */ std::map<std::string, AIStrategicArea*> m_strategicAreas;
    // The reverse index. Its value type is `std::string` - read straight off the destructor's
    // `_Rb_tree<AIStrategicArea*, pair<AIStrategicArea* const, std::string>, ...>` teardown.
    /*  +44 */ std::map<AIStrategicArea*, std::string> m_areaNames;
    // An iterator, not a pointer - one word in this libstdc++, initialised to `m_strategicAreas.end`
    // by the constructor and compared against `end` by every reader (`getActiveArea`
    // `getObjectTypeFlags`, `setOrderPosition`, `getPosition`).
    /*  +56 */ std::map<std::string, AIStrategicArea*>::iterator m_activeArea;
    /*  +60 */ std::map<std::string, int> m_categories;
    /*  +72 */ std::vector<std::string> m_categoryNames;
    // Written 0 by the constructor and never touched again anywhere in this class. [UNRESOLVED].
    /*  +84 */ bool m_b84;
    /*  +88 */ std::map<std::pair<int, int>, WPContainer> m_wayPoints;
    // The two halves of a `m_wayPoints` key, both -1 for "nothing selected".
    // `clearWaypointToDraw` resets only the first of the two. `drawWayPoints` tests
    // only the first as well, so the stale second half is never read - but it is left stale.
    /* +100 */ int m_wayPointsToDrawFirst;
    /* +104 */ int m_wayPointsToDrawSecond;
    /* +108 */ std::list<BridgeSegment*> m_bridgeSegments;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGICENTITY_AISTRATEGICOBJECTMANAGER_H
