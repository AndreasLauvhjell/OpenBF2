// [TEMP] Friend access to actual private helpers, preserving MSVC mangling.
#include "../BF2/Menu/BF2HudManager.h"
namespace dice { namespace hfe { namespace hud {
struct BF2HudOrdersFixture {
    static void reply(BF2HudManager& hud,int squad,std::string message,bool announce) {
        hud.sendSquadReply(squad,message,announce);
    }
    static void response(BF2HudManager& hud) { hud.respondToRequest(true,8,2,1); }
};
} } }
void testSquadReply(dice::hfe::hud::BF2HudManager& hud,int squad,std::string message,bool announce) {
    dice::hfe::hud::BF2HudOrdersFixture::reply(hud,squad,message,announce);
}
void testNoPlayerResponse(dice::hfe::hud::BF2HudManager& hud) {
    dice::hfe::hud::BF2HudOrdersFixture::response(hud);
}
