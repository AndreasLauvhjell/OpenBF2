// Owns AI landing zones. Constructed when the DLL loads and destroyed by BF2AI::clear.
// reset is its only virtual method; the destructor is nonvirtual.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGICENTITY_AILANDINGZONEMANAGER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AISTRATEGICENTITY_AILANDINGZONEMANAGER_H

#include <map>
#include <string>
#include <vector>

// Vec2 and Vec3 are Math aliases, so do not forward-declare them as classes in this namespace.
namespace dice {
namespace hfe {
namespace ai {

class AILandingZone;

// [UNRESOLVED] enumerators - one dword at `createLandingZone`'s last parameter.
enum LandingZoneDirection : int;

// sizeof is 40 (0x28) in retail, from the `operator new(0x28)` in this file's static initialiser
// and it will not be 40 here - two of the members are real STL containers and a third is a string.
class AILandingZoneManager
{
public:
    AILandingZoneManager();

    // not virtual - see the header block. , .
    ~AILandingZoneManager();                         // [FRONTIER]

    // The class's only virtual, and the only entry in its vtable.
    virtual void reset();                            // [FRONTIER]

    // - written inline because the shipped body is the vector size and nothing
    // else. This is the method `AIRenderUtil`'s constructor calls.
    int getNumberOfLandingZones() const
    {
        return static_cast<int>(m_zones.size());
    }

    // [FRONTIER] Remaining landing-zone operations require implementation.

    // set by this file'S static initialiser, not by any `init`. See the header block.
    static AILandingZoneManager* m_instance;

private:
    /* +0x00 */ // vptr - one slot, `reset`
    /* +0x04 */ std::map<std::string, AILandingZone*> m_zonesByName;
    /* +0x10 */ std::vector<AILandingZone*> m_zones;      // what getNumberOfLandingZones measures

    // [UNRESOLVED] Two words initialized to zero; their roles are not yet established.
    /* +0x1C */ int m_unresolved1C;
    /* +0x20 */ int m_unresolved20;

    // constructed from A literal, and the literal is the class's whole personality:
    // "No valid zone is active".
    /* +0x24 */ std::string m_activeZoneName;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGICENTITY_AILANDINGZONEMANAGER_H
