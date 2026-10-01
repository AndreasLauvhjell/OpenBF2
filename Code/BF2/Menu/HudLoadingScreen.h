// [INFERRED] class/path/member names: loading-screen record owned at HUD+134.
// BF2.exe allocates28 bytes at757128 and calls7AA5A0 with the UI System pointer.
// Complete observed nonvirtual family; no vptr, descriptor or owned allocation.
#ifndef MENU_HUDLOADINGSCREEN_H
#define MENU_HUDLOADINGSCREEN_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
namespace dice { namespace meme { class System; } }
namespace dice { namespace hfe { namespace hud {
class HudLoadingScreen
{
public:
    explicit HudLoadingScreen(meme::System*); // 7AA5A0; borrowed pointer
    ~HudLoadingScreen();                     // 83D270, called at756C0C
    void registerData();
    void setProgress(float percent);
    void setDebug(const char* text);
private:
    meme::System* m_system;                   // +00; no virtual table
    bool m_show;                             // +04; padding remains untouched
    float m_progress;                        // +08
    const char* m_picture;                    // +0C; untouched by constructor
    const char* m_mapName;                    // +10
    const char* m_debug;                      // +14; points to shared storage after setDebug
    const char* m_buildNumber;                // +18; shared storage
};
} } }
#endif
#endif
