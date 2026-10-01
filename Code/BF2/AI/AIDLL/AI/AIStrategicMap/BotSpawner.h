// Bot spawn selection and placement.
// [INFERRED] Source placement.
#ifndef AI_AIDLL_AI_AISTRATEGICMAP_BOTSPAWNER_H
#define AI_AIDLL_AI_AISTRATEGICMAP_BOTSPAWNER_H

#include <vector>

namespace dice { namespace hfe {
enum KitType : int;
namespace ai {
class Bot;
class SquadMember;
class AIStrategicObject;

// Name/type attested by ELF vector<SpawnChoice> instantiations. Field names and
// this header placement are [INFERRED]; both selectors' stores pin the 28-byte
// x86 record. The information count is recorded but not used in this weight.
struct SpawnChoice
{
    int group;
    float strategicValue;
    int informationCount;
    float freeStrength;
    AIStrategicObject* object;
    float distance;
    float weight;
};

// These static contracts imply no instance layout, lifecycle or factory.
class BotSpawner
{
public:
    static bool findSuitableSpawnGroup(float* enemyStrengths, float* enemyCounts,
                                      float* friendlyStrengths, float* friendlyCounts,
                                      int strategicObjectId, Bot* bot);
    static int findSuitableSpawnGroup(int side, AIStrategicObject*& object, Bot* bot);
    static int findSuitableKit(const float* enemyStrengths, const float* enemyCounts,
                               const float* friendlyStrengths, const float* friendlyCounts,
                               AIStrategicObject* object, int side, Bot* bot);
    static KitType GetRandomKit(SquadMember* member);

    // Shared static vector. Ordinary
    // static construction/destruction; retained capacity across selector calls.
    // Public access is [CHOSEN, not recovered], not an instance-layout claim.
    static std::vector<SpawnChoice> m_pointWeightVector;
};

} } }
#endif
