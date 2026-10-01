// [TEMP]
// Friend access to private squad helpers preserves MSVC decoration; no production body is replaced.
#include "../BF2/Menu/HudSquad.h"

namespace dice { namespace hfe { namespace hud {
class HudSquadFixture {
public:
    static void refreshCounts(HudSquad* squad, int team) { squad->refreshCounts(team); }
    static void scrollToSquad(HudSquad* squad) { squad->scrollToSquad(); }
    static void refreshRows(HudSquad* squad, int team) { squad->refreshRows(team); }
};
} } }

void hudSquadRefreshRowsForFixture(dice::hfe::hud::HudSquad* squad, int team)
{
    dice::hfe::hud::HudSquadFixture::refreshRows(squad, team);
}

void hudSquadRefreshCountsForFixture(dice::hfe::hud::HudSquad* squad, int team)
{
    dice::hfe::hud::HudSquadFixture::refreshCounts(squad, team);
}

void hudSquadScrollToSquadForFixture(dice::hfe::hud::HudSquad* squad)
{
    dice::hfe::hud::HudSquadFixture::scrollToSquad(squad);
}
