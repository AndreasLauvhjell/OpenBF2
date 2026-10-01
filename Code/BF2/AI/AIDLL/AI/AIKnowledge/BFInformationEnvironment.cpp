// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BFInformationEnvironment operations.

#include "BFInformationEnvironment.h"

#include <string>

#include "AIInformationGrid.h"                             // the grids the constructor builds
#include "../AIUtils/checkPointer.h"                       // inlined four times by the constructor
#include "../../../../Core/ClassManager.h"
#include "../../../../Core/SmartItf.h"           // the classManager global
#include "../Main/AIDllFrontier.h"
#include "../Main/AISettingsInterfaces.h"                      // getNSides, getInformationGridDimension
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"        // getWorldSizeX / getWorldSizeZ

namespace dice {
namespace hfe {
namespace ai {

// Registers query and modifier interfaces in their respective singleton rosters.
IAIInformationEnvironment::IAIInformationEnvironment()
{
    if (!AISingletonServer::m_iaiInformationEnvironment)
        classManager->registerSimpleSingleton(std::string("IAIInformationEnvironment"), this);
}

IAIInformationEnvironment::~IAIInformationEnvironment()
{
    if (AISingletonServer::m_iaiInformationEnvironment == this)
        classManager->unregisterSimpleSingleton(std::string("IAIInformationEnvironment"));
}

IAIInformationEnvironmentModifiers::IAIInformationEnvironmentModifiers()
{
    if (!CommonSingletonServer::m_iaiInformationEnvironmentModifiers)
        classManager->registerSimpleSingleton(std::string("IAIInformationEnvironmentModifiers"), this);
}

IAIInformationEnvironmentModifiers::~IAIInformationEnvironmentModifiers()
{
    if (CommonSingletonServer::m_iaiInformationEnvironmentModifiers == this)
        classManager->unregisterSimpleSingleton(std::string("IAIInformationEnvironmentModifiers"));
}

// Creates two spatial grids for each side, including neutral side zero.
// Grid bounds run from zero to the terrain dimensions.
BFInformationEnvironment::BFInformationEnvironment()
{
    const IAITerrain* const terrain =
        static_cast<const IAITerrain*>(CommonSingletonServer::m_iaiTerrain);
    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

    for (int side = 0; side < settings->getNSides() + 1; ++side)
    {
        // Queries terrain dimensions and grid settings separately for each grid allocation.
        // Preserve the repeated virtual calls instead of sharing cached values.
        {
            // Not centred on the origin - see the block above, and `AIInformationGrid::getMinMax`
            // which divides by the maximum corner alone and so requires this rectangle to start at
            // the origin.
            // `Vec2` is a plain aggregate here, with no two-argument constructor; the binary sets
            // the four floats field by field and so does this.
            Vec2 min;
            min.x = 0.0f;
            min.y = 0.0f;

            Vec2 max;
            max.x = static_cast<float>(terrain->getWorldSizeX());
            max.y = static_cast<float>(terrain->getWorldSizeZ());

            // One number, asked for twice - see the block above. The ELF evaluates the third
            // argument before the second, which is GCC pushing right to left; unobservable, because
            // it is one function called twice.
            m_gridsA.push_back(checkPointer(
                new AIInformationGrid(side, settings->getInformationGridDimension(),
                                      settings->getInformationGridDimension(), min, max)));
        }

        {
            Vec2 min;
            min.x = 0.0f;
            min.y = 0.0f;

            Vec2 max;
            max.x = static_cast<float>(terrain->getWorldSizeX());
            max.y = static_cast<float>(terrain->getWorldSizeZ());

            m_gridsB.push_back(checkPointer(
                new AIInformationGrid(side, settings->getInformationGridDimension(),
                                      settings->getInformationGridDimension(), min, max)));
        }
    }
}

// [FRONTIER] Unimplemented environment queries and modifiers stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

BFInformationEnvironment::~BFInformationEnvironment()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::~BFInformationEnvironment");
}

void BFInformationEnvironment::getFriendlyUnits(std::vector<unsigned int>&, const Vec3&, Bot*, float) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::getFriendlyUnits");
}

void BFInformationEnvironment::getFriendlyAndNeutralUnits(std::vector<unsigned int>&, const Vec3&, Bot*, float) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::getFriendlyAndNeutralUnits");
}

void BFInformationEnvironment::getCover(const Bot&, std::vector<unsigned int>&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::getCover");
}

void BFInformationEnvironment::getHardware(std::vector<unsigned int>&, const Vec3&, Bot*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::getHardware");
}

void BFInformationEnvironment::getHardware(std::vector<unsigned int>&, const Vec3&, float, int, Information*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::getHardware (Information*)");
}

void BFInformationEnvironment::getEnemyObjects(std::vector<unsigned int>&, Bot*, float) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::getEnemyObjects");
}

void BFInformationEnvironment::getObjectsWithinRadius(std::vector<unsigned int>&, float, const Bot&,
                                                     bool, IAIInformationGridPredicate*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::getObjectsWithinRadius (Bot&)");
}

// flatten XZ, then delegate
// virtually to the Vec2 overload (ELF +0x28 / PE +0x1C).
void BFInformationEnvironment::getObjectsWithinRadius(std::vector<unsigned int>& out,
                                                     const Vec3& position, float radius,
                                                     int side, bool which,
                                                     IAIInformationGridPredicate* keep) const
{
    Vec2 flat;
    flat.x = 0.0f;
    flat.y = 0.0f;
    vec3ToVec2XZ(position, flat);
    getObjectsWithinRadius(out, flat, radius, side, which, keep);
}

// Query side zero first
// then look up the requested side after that callback. Both calls remain when
// side is zero. The grid query is const; output is appended to, never cleared.
void BFInformationEnvironment::getObjectsWithinRadius(std::vector<unsigned int>& out,
                                                     const Vec2& position, float radius,
                                                     int side, bool which,
                                                     IAIInformationGridPredicate* keep) const
{
    getInformationGrid(0, which)->getInformationWithinCircle(out, position, radius, keep);
    getInformationGrid(side, which)->getInformationWithinCircle(out, position, radius, keep);
}

// Query neutral side zero first
// then select the requested side after that callback. Both calls borrow the same corners
// output and predicate. Side zero is queried twice when requested; output is never cleared.
void BFInformationEnvironment::getObjectsWithinRectangle(std::vector<unsigned int>& out,
                                                        const Vec2& min, const Vec2& max,
                                                        int side, bool which,
                                                        IAIInformationGridPredicate* keep) const
{
    getInformationGrid(0, which)->getInformationWithinRectangle(out, min, max, keep);
    getInformationGrid(side, which)->getInformationWithinRectangle(out, min, max, keep);
}

void BFInformationEnvironment::getObjectsWithinFrustum(std::vector<unsigned int>&, const Frustum&,
                                                      int, bool, IAIInformationGridPredicate*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::getObjectsWithinFrustum");
}

void BFInformationEnvironment::getAllInformation(std::vector<unsigned int>& out, int side, bool which,
                                                 IAIInformationGridPredicate* keep) const
{
    // Const environment access still invokes the mutable
    // grid sweep, including stale-record removal; it does not use the read-only overload.
    getInformationGrid(side, which)->getAllInformation(out, keep, nullptr);
}

void BFInformationEnvironment::getAllInformation(std::vector<unsigned int>& out, int side, bool which,
                                                 IAIInformationGridPredicate* keep,
                                                 IAIInformationGridPredicate* cull)
{
    // No side/grid guard and no clearing of the output.
    getInformationGrid(side, which)->getAllInformation(out, keep, cull);
}

// the side's grid (unit or other, by `which`) takes the handle
// IAIInformationGrid +2 (ELF +0x08, PE +0x04). No side guard, as in getInformationGrid. The PE
// inlines the grid lookup; the ELF calls it.
void BFInformationEnvironment::addInfo(int side, bool which, unsigned int handle)
{
    getInformationGrid(side, which)->addInfo(handle);
}

// the same, through +6 addAndUpdatePosition (ELF +0x18, PE
// +0x14).
void BFInformationEnvironment::addAndUpdatePosition(int side, bool which, unsigned int handle)
{
    getInformationGrid(side, which)->addAndUpdatePosition(handle);
}

void BFInformationEnvironment::updatePosition(int, bool, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::updatePosition");
}

void BFInformationEnvironment::removeInfo(int, bool, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::removeInfo");
}

void BFInformationEnvironment::safeRemoveInfo(int, bool, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::safeRemoveInfo");
}

void BFInformationEnvironment::destroyRemoveInfo(int, bool, unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BFInformationEnvironment::destroyRemoveInfo");
}

// Selects one of two grid sets by which, then indexes it by side.
// The caller must supply a valid side; no bounds check is performed.
IAIInformationGrid* BFInformationEnvironment::getInformationGrid(int side, bool which) const
{
    return which ? m_gridsB[side] : m_gridsA[side];
}

} // namespace ai
} // namespace hfe
} // namespace dice
