// Per-bot navigation state and owned paths. The bot owner is borrowed.

#ifndef AI_AIDLL_AI_AIBOT_BOTNAVIGATION_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_BOTNAVIGATION_H

#include <set>
#include <vector>

#include "../Main/AIPathfindingSearchTypes.h"   // QTPath, ClusterNodePath - both already written
#include "../../../../Math/Vec3.h"        // Vec2 and Vec3 both live here, and both are typedefs
                                          // to dice::hfe::Math::* - never forward-declare either

namespace dice {
namespace hfe {
namespace ai {

class BotMain;
class Information;

class BotNavigation
{
public:
    explicit BotNavigation(BotMain* owner);
    ~BotNavigation();                           // [FRONTIER]

    // [FRONTIER] Navigation methods below remain incomplete unless implemented explicitly.
    void update(bool& a, bool* b);
    void updateOccationally();                  // dice's spelling - "Occationally"
    void updatePath(Vec2& a, bool& b, bool& c, float d);
    void updateNavPoints();
    void updateObstacles();
    void updateLastPoint(Information* info);
    void updateSpaceLeftAndRight();
    void updateClosestDistance();

    void initPathfinding(const Vec3& to, float a, float b);
    void initPathfinding(unsigned int a, unsigned int b, const Vec3& to, float c, float d);
    void doPolygonSearch(bool& a, bool* b);
    void doClusterSearch(bool& a, bool* b);
    void calculatePoints();
    void optimizePoints();
    bool testDirectPath();
    void retrace();

    bool stillOnPath();
    bool hasIllegalPath();
    // [INFERRED] accessor name for the +192 byte, which retail reads inline (BotManager::
    // actionComAgent). initPathfinding and updateOccationally set it; clearProgress
    // checkProgress, doPolygonSearch and setIsStuck clear it; updatePath tests it.
    bool isPathSearchPending() const { return m_flag192; }
    // [INFERRED] accessor names for the +193 and + (PE +0xC9 / +0x148), both read inline
    // by BotManager::actionPathfinding (, ).
    // +193: set to 1 only on initPathfinding's search-started path , cleared by the
    // constructor, initPathfinding's other exits and reset. +316: set by initPathfinding
    // cleared by update and reset.
    bool isSearchActive() const { return m_flag193; }
    bool isSearchNew() const { return m_flag316; }
    bool isActive();
    // returns true, unconditionally, in both images.
    bool isRunning();
    void checkProgress();
    void clearProgress();
    bool checkArrivedAtPoint(float distance);
    void setIsStuck(bool stuck);

    bool pointWithinObstacleRadius(const Vec3& point);
    void getFirstPointBeyondDist(const Vec2& from, float distance, Vec2& out);
    void getSecondPoint(Vec2& out);
    void getLargestAngleDiff(float& out);
    void getTraceData(int a, unsigned int& b, unsigned int& c, Vec3& d);

    void getGrenadeWarning(Vec3& position, double& time);
    void setGrenadeWarning(const Vec3& position, double time, bool b);

    void reset();                          // - (one arm frontier)
    void resetAll();

    // attested as a nested type by the container instantiations, which spell
    // `std::vector<dice::hfe::ai::BotNavigation::NavigationPoint>` in full. sizeof is 40, from
    // the constructor's divide; the members are [UNRESOLVED] because nothing recovered so far builds
    // one. Declared as a sized block rather than guessed at.
    struct NavigationPoint
    {
        // Initializes the index to -1 and clears the preceding 36 bytes.
        // [UNRESOLVED] The contents of that byte range remain unknown.
        NavigationPoint()
            : m_index36(-1)
        {
            for (int i = 0; i < 36; ++i)
                m_bytes[i] = 0;
        }

        char m_bytes[36];       // [UNRESOLVED]
        int  m_index36;         // = -1
    };

private:
    /*   0 */ BotMain* m_bot;              // the ctor argument, and the first store
    /*   4 */ float m_float4;              // = 2.0f [UNRESOLVED]
    /*   8 */ float m_float8;              // = 2.0f [UNRESOLVED]
    /*  12 */ char  m_gap12[4];            // [UNRESOLVED] - never written
    /*  16 */ int   m_int16;               // = -1
    /*  20 */ int   m_int20;               // = -1
    /*  24 */ int   m_zero24[3];           // +24..+32, all 0
    /*  36 */ int   m_int36;               // = -1
    /*  40 */ bool  m_flag40;              // = false
    /*  41 */ char  m_pad41[3];
    /*  44 */ int   m_zero44[3];           // +44..+52, all 0
    /*  56 */ int   m_int56;               // not a gap - `reset` zeroes it
    // Owns polygon and cluster path reservations, released through virtual slots +156 and
    // +164 during reset. [UNRESOLVED] The interface type is not yet known.
    /*  60 */ void* m_pathOwner;            // = 0
    /*  64 */ int   m_zero64;               // = 0
    /*  68 */ char  m_gap68[4];            // [UNRESOLVED] - never written
    /*  72 */ QTPath* m_polygonPath;       // new(0x18) - the fine path
    /*  76 */ int   m_int76;               // = -1
    /*  80 */ char  m_gap80[8];            // [UNRESOLVED] - never written
    /*  88 */ int   m_zero88[3];           // +88, +92, +96
    // the walkable result - see the header note. The constructor's dead resize targets this.
    /* 100 */ std::vector<NavigationPoint> m_points;
    /* 112 */ char  m_gap112[4];           // [UNRESOLVED] - never written
    /* 116 */ int   m_int116;              // = 0
    /* 120 */ char  m_gap120[4];           // [UNRESOLVED] - never written
    /* 124 */ ClusterNodePath* m_clusterPath;   // new(0x1C) - the coarse path
    /* 128 */ int   m_int128;              // = -1
    // `allocate(20)` with 0 / 0 / self / self is an `_Rb_tree` header at 16 + sizeof(value_type)
    // so the value type is four bytes - a set of pointers or handles. Which is [UNRESOLVED];
    // `void*` holds the width and claims nothing.
    /* 132 */ std::set<void*> m_set132;
    /* 144 */ double m_double144;          // = 0.0
    /* 152 */ double m_double152;          // = 0.0
    /* 160 */ double m_double160;          // = 0.0
    /* 168 */ double m_double168;          // = 0.0
    /* 176 */ int   m_zero176[3];          // +176, +180, +184
    // is 9,999,999.0f - a "no distance yet" sentinel, and a third kind after the
    // DBL_MAX timestamps and -9999.0 magnitudes BotMain uses. Do not normalise them together.
    /* 188 */ float m_sentinel188;         // = 9999999.0f
    /* 192 */ bool  m_flag192;             // = false
    /* 193 */ bool  m_flag193;             // = false
    /* 194 */ bool  m_flag194;             // not padding - `reset` clears it
    /* 195 */ bool  m_flag195;             // = false
    /* 196 */ char  m_gap196[4];           // [UNRESOLVED] - never written
    /* 200 */ int   m_int200;              // = -1
    /* 204 */ int   m_int204;              // = -1
    /* 208 */ int   m_int208;              // = -1
    /* 212 */ int   m_int212;              // = -1
    /* 216 */ int   m_zero216[3];          // +216, +220, +224
    /* 228 */ char  m_gap228[12];          // [UNRESOLVED] - never written
    /* 240 */ int   m_int240;              // = 0
    /* 244 */ double m_double244;          // not a gap - `reset` zeroes it
    // resetAll clears this field.
    /* 252 */ int   m_int252;              // resetAll: 0
    /* 256 */ int   m_zero256[3];          // +256, +260, +264
    /* 268 */ double m_double268;          // = 0.0
    /* 276 */ bool  m_flag276;             // = false
    /* 277 */ char  m_pad277[3];
    /* 280 */ int   m_zero280[5];          // +280..+296
    /* 300 */ double m_double300;          // Initially -1.0; independent of the distance sentinel.
    /* 308 */ int   m_zero308[2];          // +308, +312
    // `reset`'s first statement clears the byte here.
    /* 316 */ bool  m_flag316;             // reset: false
    /* 317 */ char  m_pad317[3];
    /* 320 */ int   m_zero320[6];          // +320..+340
    /* 344 */ char  m_gap344[12];          // +344..+355 - the ctor never reaches them [UNRESOLVED]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTNAVIGATION_H
