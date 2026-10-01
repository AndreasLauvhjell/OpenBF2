// [INFERRED] class, payload and path names for BF2.exe's nonvirtual HUD+14C
// award catalogue. Constructor7BF910 and all three population bodies read in
// full. This is not IPlayerAwards/PlayerAwards.
#ifndef MENU_HUDAWARDCATALOGUE_H
#define MENU_HUDAWARDCATALOGUE_H
#if !DEDICATED
#include <cstdint>
#include <map>
#include <string>

namespace dice { namespace hfe { namespace hud {

struct HudAwardDescription {
    std::wstring message;
    std::string iconPath;
    std::string notification;
};

class HudAwardCatalogue {
public:
    HudAwardCatalogue(); // 7BF910; native map construction, then91/45/2 entries
    // 7B4600, called by ECHud39 at75BC0B. [INFERRED] name and two-argument
    // source spelling; caller prints both words as unsigned int. Returns the
    // stored payload or null without inserting a missing key.
    HudAwardDescription* findAward(unsigned int awardId, unsigned int level);
private:
    friend class BF2HudManager;
    friend struct HudAwardCatalogueFixture;
    void populateFirstGroup();
    void populateSecondGroup();
    void populateThirdGroup();
    // [INFERRED] factorization of the repeated straight-line population blocks.
    void addAward(unsigned int awardId, unsigned int level);
    // PE7B4230/455F60 compare the unsigned high word first, then low word.
    // PE7B48E0 is map::operator[], with wstring/string/string mapped storage.
    // Keep the current compiler's map and strings; retail12/84-byte sizes are
    // evidence, never overlays or required native sizes.
    std::map<std::uint64_t, HudAwardDescription> m_awards;
};

} } }
#endif
#endif
