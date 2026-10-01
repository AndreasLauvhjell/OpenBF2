// Class and path [INFERRED]. The PE has no RTTI or source string for this ordinary
// adapter. HUD +0x104 owns it; ctor 0x7870B0, primary table 0x930EF8 (11 slots).
#ifndef MENU_HUDINFORMATIONADAPTER_H
#define MENU_HUDINFORMATIONADAPTER_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "../Game/IEventListener.h"
#include <string>
namespace dice { namespace meme { class System; } }
namespace dice { namespace hfe { namespace hud {
// Primary-interface name [INFERRED]; the second vptr is the event listener.
class IHudInformationAdapter {
public:
    virtual void setSystem(meme::System*) = 0;                     // +00
    virtual bool registerData() = 0;                              // +04
    virtual bool registerFunctions() = 0;                         // +08
    virtual bool setBool(std::string, bool) = 0;                  // +0C
    virtual bool getBool(std::string) = 0;                         // +10
    virtual bool setInt(std::string, int) = 0;                    // +14
    virtual int getInt(std::string) = 0;                           // +18
    virtual bool setString(std::string, std::string) = 0;          // +1C
    virtual bool setWideString(std::string, std::wstring) = 0;     // +20
    virtual float getFloat(std::string) = 0;                       // +24
    virtual bool setFloat(std::string, float) = 0;                // +28
};
class HudInformationAdapter : public IHudInformationAdapter, public IEventListener {
public:
    HudInformationAdapter();
    ~HudInformationAdapter(); // Nonvirtual, explicit HUD-owned destruction
    void setSystem(meme::System*) override;
    bool registerData() override;
    bool registerFunctions() override;
    bool setBool(std::string, bool) override;
    bool getBool(std::string) override;
    bool setInt(std::string, int) override;
    int getInt(std::string) override;
    bool setString(std::string, std::string) override;
    bool setWideString(std::string, std::wstring) override;
    float getFloat(std::string) override;
    bool setFloat(std::string, float) override;
    void handleEvent(EventCategory, unsigned int, EventNode*, void*) override;
    // 0x785AB0, typed native map state and existing HUD mode virtuals.
    void setMapZoom(int);
    int getMapZoom(); // 0x785B40; same four native stores as setMapZoom
    void setMapStatic(bool); // 0x785BA0; posts ECHud 10 synchronously
    void setCloseToMine(bool); // 0x7893D0; forwards to setBool and discards its result
    void setTeam(int team);  // 0x787260; team selector, not a Boolean
    void setHudMode(int mode); // 0x786260, nonvirtual; [INFERRED] source name/void return
    void setSelectedTeam(int team); // 0x785F80, nonvirtual; both Boolean results discarded
    bool setSelectedKit(int kit); // 0x786030; indices 0..7 inclusive, stops on false
    bool setKitIconCount(int count); // 0x785FD0; stops at first failed Boolean update
    void updateVictory(); // 0x786190; [INFERRED] source name/void return
    void setField014(int value); // 5BB600, scalar identity unresolved
private:
    meme::System* m_system; // PE +08
    int m_mapZoom;          // +0C, initialized 1
    int m_field010;         // +10, initialized 0
    int m_field014;         // +14, initialized 0
    int m_field018;         // +18, initialized 0
};
} } }
#endif
#endif
