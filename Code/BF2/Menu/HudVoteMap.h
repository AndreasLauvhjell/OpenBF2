// Map-voting HUD state and events. [INFERRED] Class, header and field names.
#ifndef MENU_HUDVOTEMAP_H
#define MENU_HUDVOTEMAP_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "../Game/IEventListener.h"
#include <cstdint>
#include <string>
namespace dice { namespace meme { class System; class BfListBoxData; } }
namespace dice { namespace hfe { class HudInformationLayer; } }
namespace dice { namespace hfe { namespace hud {
class IHudVoteMap {
public:
    virtual void setSystem(meme::System*) = 0;
    virtual bool registerData() = 0;
    virtual bool registerFunctions() = 0;
};
class HudVoteMap : public IHudVoteMap, public IEventListener {
public:
    explicit HudVoteMap(meme::System*);
    ~HudVoteMap(); // Nonvirtual; the HUD explicitly destroys this owner.
    void setSystem(meme::System*) override;
    bool registerData() override;
    bool registerFunctions() override;
    void handleEvent(EventCategory, unsigned int, EventNode*, void*) override;
    std::string getSelectedMap(); // 7856B0, copies selected hidden-column text
    // Map-vote console commands. [INFERRED] Method names follow the command names.
    void setVoteMapShow(int show); // 785300 -> +24; also HudInformationLayer 466CF9 with 0
    void singleClick(int);         // 785700; the argument is never read
private:
    friend class BF2HudManager;
    friend class ::dice::hfe::HudInformationLayer;
    // PE 7A458C (HudScoreboard::singleClick) tests m_selected (+0x24) straight off getVoteMap():
    // a scoreboard click is ignored while the vote map is selected.
    friend class HudScoreboard;
    // Changes visibility only when a map is selected; otherwise retains the previous state.
    void show(bool);
    // 785350, [INFERRED] name. Rebuilds the map rows from MapList, one row per entry, with the
    // ballot icon in column 1 and the tick box in column 2.
    // it dereferences A null m_data on its OWN failure path - see the .cpp.
    // m_rowCount is incremented per row and never reset, which the class comment already noted.
    void refresh();
    meme::System* m_system; // retail +08, borrowed
    std::uint32_t m_field00c; // +0C, initialized zero
    std::uint32_t m_field010, m_field014; // untouched; interpretation unresolved
    int m_rowCount;          // +18, initially zero; refresh increments without reset
    bool m_visible;          // +1C false
    int m_visibleRows;       // +20 15
    bool m_selected;         // +24 false, bound to VoteMapSelected
    int m_field028;          // +28 12, initialized after event registration
    int m_columnWidth;       // +2C 160, initialized after event registration
    float m_rowOffset;       // +30 2
    std::string m_field034;  // +34 empty; native STL layout
    meme::BfListBoxData* m_data; // +50 untouched by ctor, not deleted by dtor
};
} } }
#endif
#endif
