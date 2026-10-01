// Class/path/member names [INFERRED]. BF2.exe ctor 0x7A1DE0, allocation
// 0x24 at 0x7570CB; the HUD owns +0x11C. No vptr or owned allocation.
#ifndef MENU_HUDKITSELECTION_H
#define MENU_HUDKITSELECTION_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
namespace dice { namespace hfe { class HudInformationLayer; } }
namespace dice { namespace hfe { namespace hud {
class BF2HudManager;
class HudInformationAdapter;
class HudKitSelection {
public:
    HudKitSelection();
    ~HudKitSelection();                           // HUD 756B70 -> nullsub_3
    void reset();
    void suicide();
    void setSpawnGroup(int group);
    void selectKit(int kit);
    void showSpawnMenu(bool show);
    void setDisplayedTeam(int team);
    void cycleUnlock(int kit);
    void selectTeam(int team);                     // 7A2180, calls the named question frontier
private:
    int m_pendingTeam;                             // +00, constructor -1
    bool m_teamChangePending;                      // +04, constructor false
    bool m_field005;                               // +05, constructor false; identity unresolved
    // +06/+07 padding is intentionally untouched by the constructor.
    int m_selectedTeam;                            // +08, constructor 2
    int m_selectedKit[3];                          // +0C, constructor {2,2,2}
    int m_selectedSpawnGroup[3];                   // +18, constructor {0,0,0}
    // Direct retail field accesses in HUD/adapter, with no invented ABI getters.
    friend class BF2HudManager;
    friend class ::dice::hfe::HudInformationLayer;
    friend class HudInformationAdapter;
    friend struct HudKitSelectionFixture;
};
#if defined(_M_IX86)
static_assert(sizeof(HudKitSelection) == 0x24, "HudKitSelection retail x86 layout");
#endif
} } }
#endif
#endif
