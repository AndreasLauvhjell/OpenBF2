// [TEMP] Friend access to the recovered private icon collectors while preserving
// MSVC's private-member decoration. No production body is replaced.
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "../BF2/Menu/Bf2MenuMapNode.h"

namespace dice { namespace meme {
namespace {
[[noreturn]] void timedIconFixtureUnexpected(const char* detail)
{
    std::fprintf(stderr, "[FRONTIER] hud map timed-icon fixture unexpected %s\n", detail);
    std::fflush(stderr);
    std::abort();
}

// [TEMP] Typed boundary for 7820E0's fallback-only cases. The real base
// populateNetworkIcon false tail leaves a zero texture and a 16x16 size; this
// override supplies exactly that outcome without publishing fabricated network
// or object managers. It lives in this clean-header TU so every inherited
// private virtual retains its real MSVC access decoration.
class TimedIconFallbackNode final : public Bf2MenuMapNode {
public:
    int populateCalls;
    std::vector<int> requestedIds;

    TimedIconFallbackNode(System* system, const char* name)
        : Bf2MenuMapNode(system, name, 0, 0, 0, 0), populateCalls(0) {}

private:
    bool populateNetworkIcon(int id, MapIconRecord* icon, bool asPlayer) override
    {
        if (!icon || asPlayer || icon->texture != 0
                || icon->size.x != 0.0f || icon->size.y != 0.0f)
            timedIconFixtureUnexpected(
                "TimedIconFallbackNode::populateNetworkIcon call shape");
        ++populateCalls;
        requestedIds.push_back(id);
        icon->texture = 0;
        icon->size = hfe::Vec2{16.0f, 16.0f};
        return false;
    }
};

TimedIconFallbackNode* asTimedIconFallbackNode(Bf2MenuMapNode* map)
{
    if (!map)
        timedIconFixtureUnexpected("null TimedIconFallbackNode handle");
    return static_cast<TimedIconFallbackNode*>(map);
}
}

struct Bf2MenuMapNodeFixture {
    static void collectPlayerIcons(Bf2MenuMapNode* map)
    {
        map->collectIconFamily783440();
    }
    static void collectStrategicIcons(Bf2MenuMapNode* map)
    {
        map->collectIconFamily77E500();
    }
    static void collectTimedPositionIcons(Bf2MenuMapNode* map)
    {
        map->collectIconFamily7820E0();
    }
    static void collectUavIcons(Bf2MenuMapNode* map)
    {
        map->collectIconFamily77FC30();
    }
    // Friend access to the private non-virtual line helper preserves its MSVC decoration.
    static void completeLineRecord(Bf2MenuMapNode* map, float scale, MapLineRecord* line)
    {
        map->completeLineRecord(scale, line);
    }
    static void addSquadRequestIcon(Bf2MenuMapNode* map, unsigned int type,
                                    hfe::world::IPlayer* player, const MapIconRecord* icon)
    {
        map->addSquadRequestIcon(type, player, icon);
    }
    static void collectSquadRequestIcons(Bf2MenuMapNode* map)
    {
        map->collectSquadRequestIcons();
    }
    // Friend access to private event pickers.
    static bool pickSquadRequest(Bf2MenuMapNode* map, float scale, hfe::Vec2 position,
                                 float maxDistance)
    {
        return map->pickSquadRequest(scale, position, maxDistance);
    }
    static unsigned short pickEnemyVehicle(Bf2MenuMapNode* map, float scale, hfe::Vec2 position,
                                           float maxDistance)
    {
        return map->pickEnemyVehicle(scale, position, maxDistance);
    }
    static int pickPlayer(Bf2MenuMapNode* map, float scale, float x, float y, float maxDistance)
    {
        return map->pickPlayer(scale, x, y, maxDistance);
    }
    // Friend access to supply-line helpers.
    static void updateSupplyLine(Bf2MenuMapNode* map, System* system)
    {
        map->updateSupplyLine(system);
    }
    static void drawSupplyLine(Bf2MenuMapNode* map, const Rect* adjusted, const Rect* bounds,
                               System* system, RenderContext* context)
    {
        map->drawSupplyLine(adjusted, bounds, system, context);
    }
};
} }

void addSquadRequestIconForFixture(dice::meme::Bf2MenuMapNode* map, unsigned int type,
                                   dice::hfe::world::IPlayer* player,
                                   const dice::meme::MapIconRecord* icon)
{
    dice::meme::Bf2MenuMapNodeFixture::addSquadRequestIcon(map, type, player, icon);
}

void collectSquadRequestIconsForFixture(dice::meme::Bf2MenuMapNode* map)
{
    dice::meme::Bf2MenuMapNodeFixture::collectSquadRequestIcons(map);
}

bool pickSquadRequestForFixture(dice::meme::Bf2MenuMapNode* map, float scale,
                                dice::hfe::Vec2 position, float maxDistance)
{
    return dice::meme::Bf2MenuMapNodeFixture::pickSquadRequest(map, scale, position, maxDistance);
}

unsigned short pickEnemyVehicleForFixture(dice::meme::Bf2MenuMapNode* map, float scale,
                                          dice::hfe::Vec2 position, float maxDistance)
{
    return dice::meme::Bf2MenuMapNodeFixture::pickEnemyVehicle(map, scale, position, maxDistance);
}

int pickPlayerForFixture(dice::meme::Bf2MenuMapNode* map, float scale, float x, float y,
                         float maxDistance)
{
    return dice::meme::Bf2MenuMapNodeFixture::pickPlayer(map, scale, x, y, maxDistance);
}

void updateSupplyLineForFixture(dice::meme::Bf2MenuMapNode* map, dice::meme::System* system)
{
    dice::meme::Bf2MenuMapNodeFixture::updateSupplyLine(map, system);
}

void drawSupplyLineForFixture(dice::meme::Bf2MenuMapNode* map, const dice::meme::Rect* adjusted,
                              const dice::meme::Rect* bounds, dice::meme::System* system,
                              dice::meme::RenderContext* context)
{
    dice::meme::Bf2MenuMapNodeFixture::drawSupplyLine(map, adjusted, bounds, system, context);
}

void completeLineRecordForFixture(dice::meme::Bf2MenuMapNode* map, float scale,
                                  dice::meme::MapLineRecord* line)
{
    dice::meme::Bf2MenuMapNodeFixture::completeLineRecord(map, scale, line);
}

void collectPlayerIconsForFixture(dice::meme::Bf2MenuMapNode* map)
{
    dice::meme::Bf2MenuMapNodeFixture::collectPlayerIcons(map);
}

void collectStrategicIconsForFixture(dice::meme::Bf2MenuMapNode* map)
{
    dice::meme::Bf2MenuMapNodeFixture::collectStrategicIcons(map);
}

void collectTimedPositionIconsForFixture(dice::meme::Bf2MenuMapNode* map)
{
    dice::meme::Bf2MenuMapNodeFixture::collectTimedPositionIcons(map);
}

void collectUavIconsForFixture(dice::meme::Bf2MenuMapNode* map)
{
    dice::meme::Bf2MenuMapNodeFixture::collectUavIcons(map);
}

dice::meme::Bf2MenuMapNode* createTimedIconFallbackNodeForFixture(
    dice::meme::System* system, const char* name)
{
    return new dice::meme::TimedIconFallbackNode(system, name);
}

void destroyTimedIconFallbackNodeForFixture(dice::meme::Bf2MenuMapNode* map)
{
    delete dice::meme::asTimedIconFallbackNode(map);
}

int timedIconFallbackPopulateCallsForFixture(dice::meme::Bf2MenuMapNode* map)
{
    return dice::meme::asTimedIconFallbackNode(map)->populateCalls;
}

std::size_t timedIconFallbackRequestedCountForFixture(
    dice::meme::Bf2MenuMapNode* map)
{
    return dice::meme::asTimedIconFallbackNode(map)->requestedIds.size();
}

int timedIconFallbackRequestedIdForFixture(dice::meme::Bf2MenuMapNode* map,
                                            std::size_t index)
{
    dice::meme::TimedIconFallbackNode* const timed =
        dice::meme::asTimedIconFallbackNode(map);
    if (index >= timed->requestedIds.size())
        dice::meme::timedIconFixtureUnexpected(
            "TimedIconFallbackNode requested-id index");
    return timed->requestedIds[index];
}
