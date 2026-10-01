// Client HUD information provider. Names without matching strings are [INFERRED].
#ifndef GAME_HUDINFORMATIONLAYER_H
#define GAME_HUDINFORMATIONLAYER_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include "../Core/IBase.h"
#include "../Core/WeakPtr.h"
#include "../Math/Vec3.h"
#include "IEventListener.h"
#include <map>
#include <string>
namespace dice { namespace hfe {
namespace hud { class HudBlinkManager; }
namespace world { class IObject; class KitTemplate; class WeaponHudCompTemplate; }
class HudInformationLayer : public IBase, public IEventListener
{
public:
    HudInformationLayer(); // retail allocation 0x5A0
    static IBase* create(unsigned int iid, IBase* outer);
    // Primary HUD interface; unfinished nested update calls retain diagnostic stops.
    void addRef() override;
    int getRef() override;
    int release() override; // virtual destructor dispatch +0x94
    void* queryInterface(unsigned int) const override;

    virtual int registerBool(const std::string&, bool*); // +0x10
    virtual bool* getBoolById(int);                      // +0x14
    virtual bool* getBoolByName(const std::string&);      // +0x18
    virtual int registerInt(const std::string&, int*);    // +0x1C
    virtual int* getIntById(int);                         // +0x20
    virtual int* getIntByName(const std::string&);         // +0x24
    virtual int registerFloat(const std::string&, float*);// +0x28
    virtual float* getFloatById(int);                    // +0x2C
    virtual float* getFloatByName(const std::string&);     // +0x30
    virtual int registerString(const std::string&, const char**); // +0x34
    virtual const char** getStringById(int);             // +0x38
    virtual const char** getStringByName(const std::string&); // +0x3C
    virtual int registerWideString(const std::string&, const wchar_t**); // +0x40
    virtual const wchar_t** getWideStringById(int);        // +0x44
    virtual const wchar_t** getWideStringByName(const std::string&); // +0x48

    virtual void activateHud();                          // +0x4C
    virtual void deactivateHud();                        // +0x50
    virtual void update(float);                          // +0x54
    // [UNRESOLVED] source names; exact retail bodies are both xor al,al / ret.
    virtual bool unknown58();                            // +0x58
    virtual bool unknown5C();                            // +0x5C
    virtual void setDeathMessage(std::wstring);           // +0x60
    virtual void setDisconnectMessage(std::wstring);      // +0x64
    virtual void updateString(const char**, const char*);  // +0x68
    virtual void updateWideString(const wchar_t**, const wchar_t*); // +0x6C
    virtual bool isWithinRadius(Math::Vec3, Math::Vec3, float); // +0x70
    using Function = bool (*)();
    using StringFunction = bool (__fastcall *)(const std::string&);
    virtual bool registerFunction(const std::string&, Function); // +0x74
    virtual Function getFunction(const std::string&);     // +0x78
    virtual bool registerStringFunction(const std::string&, StringFunction); // +0x7C
    virtual StringFunction getStringFunction(const std::string&); // +0x80
    virtual void refreshColors();                        // +0x84
    virtual void updateKits(int team);                    // +0x88
    virtual void setServerInfo(std::string, std::string, int); // +0x8C
    virtual void setServerFavorite(bool);                 // +0x90
    virtual ~HudInformationLayer();                      // +0x94
    void handleEvent(EventCategory, unsigned int, EventNode*, void*) override;

private:
    template<class T> struct Storage {
        std::map<int, T*> values;
        typename std::map<int, T*>::iterator current; // retail cached iterator
        Storage() : current() {}
    };
    template<class T> int registerValue(Storage<T>&, const std::string&, T*, int);
    template<class T> T* findValue(Storage<T>&, int);
    void handleHudEvent(unsigned int, EventNode*, void*);
    void updateHitDirection(bool, float);
    void findClosestControlPoint();
    void setCloseToControlPointTeam(int);
    world::WeaponHudCompTemplate* getKitWeaponHud(world::KitTemplate*, int);
    void updateControlPointCounts();
    void updateLevelTime();
    void updateTickets();
    void updateKitUnlockBlink(float);
    void updateControlPointProgress();
    void refreshServerFavorite();
    int m_refCount; // PE +0x08
    Storage<bool> m_bools;
    Storage<int> m_ints;
    Storage<float> m_floats;
    Storage<const char*> m_strings;
    Storage<const wchar_t*> m_wideStrings;
    std::map<std::string, int> m_names;
    // Callback invocation witnesses: 0x82E470 (no arguments), 0x82E480
    // (const std::string& in ECX), registration 0x785A95 -> 0x785940.
    std::map<std::string, Function> m_functions;            // PE +0x68
    std::map<std::string, StringFunction> m_stringFunctions;// PE +0x74
    int m_nextId;
    hud::HudBlinkManager* m_blinkers; // PE +0x84

    // This suffix has no STL. Named scalar/array members retain its measured
    // x86 layout; the native maps above are never padded to VC7.1 sizes.
    struct State {
        bool field88; // PE +0x88
        unsigned char unknown89[3];
        float FriendlyTicketBleed; // PE +0x8c
        bool field90; // PE +0x90
        unsigned char unknown91[3];
        float EnemyTicketBleed; // PE +0x94
        bool field98; // PE +0x98
        unsigned char unknown99[3];
        WeakPtr<world::IObject> weakTarget; // PE +0x9c; 0x4659FF calls IObject::getWeakPtr
        const wchar_t* DeathMessage; // PE +0xa0
        const wchar_t* DisconnectMessage; // PE +0xa4
        bool CPInterfaceEnabled; // PE +0xa8
        unsigned char unknowna9[3];
        float TimePreCPIsNeutral; // PE +0xac
        float TimePreCPIsTaken; // PE +0xb0
        float FriendlyCPs; // PE +0xb4
        float EnemyCPs; // PE +0xb8
        float EnemyNeutralCPs; // PE +0xbc
        float FriendlyNeutralCPs; // PE +0xc0
        float EnemyFriendlyCPs; // PE +0xc4
        float FriendlyEnemyCPs; // PE +0xc8
        const char* FriendlyCPString; // PE +0xcc
        const char* EnemyCPString; // PE +0xd0
        const char* NeutralCPString; // PE +0xd4
        float FriendlyVictoryAlpha; // PE +0xd8
        float EnemyVictoryAlpha; // PE +0xdc
        float MenuMapAlpha; // PE +0xe0
        float MenuMapIconAlpha; // PE +0xe4
        float MenuBackgroundRed; // PE +0xe8
        float MenuBackgroundGreen; // PE +0xec
        float MenuBackgroundBlue; // PE +0xf0
        float MenuBackgroundAlpha; // PE +0xf4
        float FriendlyTextRed; // PE +0xf8
        float FriendlyTextGreen; // PE +0xfc
        float FriendlyTextBlue; // PE +0x100
        float FriendlyTextAlpha; // PE +0x104
        float EnemyTextRed; // PE +0x108
        float EnemyTextGreen; // PE +0x10c
        float EnemyTextBlue; // PE +0x110
        float EnemyTextAlpha; // PE +0x114
        float FriendlyBrightRed; // PE +0x118
        float FriendlyBrightGreen; // PE +0x11c
        float FriendlyBrightBlue; // PE +0x120
        float FriendlyBrightAlpha; // PE +0x124
        float EnemyBrightRed; // PE +0x128
        float EnemyBrightGreen; // PE +0x12c
        float EnemyBrightBlue; // PE +0x130
        float EnemyBrightAlpha; // PE +0x134
        float FriendlyTeamTopRed; // PE +0x138
        float FriendlyTeamTopGreen; // PE +0x13c
        float FriendlyTeamTopBlue; // PE +0x140
        float FriendlyTeamTopAlpha; // PE +0x144
        float EnemyTeamTopRed; // PE +0x148
        float EnemyTeamTopGreen; // PE +0x14c
        float EnemyTeamTopBlue; // PE +0x150
        float EnemyTeamTopAlpha; // PE +0x154
        float FriendlyTicketRed; // PE +0x158
        float FriendlyTicketGreen; // PE +0x15c
        float FriendlyTicketBlue; // PE +0x160
        float FriendlyTicketAlpha; // PE +0x164
        float EnemyTicketRed; // PE +0x168
        float EnemyTicketGreen; // PE +0x16c
        float EnemyTicketBlue; // PE +0x170
        float EnemyTicketAlpha; // PE +0x174
        float CrossHairColorRed; // PE +0x178
        float CrossHairColorGreen; // PE +0x17c
        float CrossHairColorBlue; // PE +0x180
        float CrossHairColorAlpha; // PE +0x184
        int OrderMenuPosX; // PE +0x188
        int OrderMenuPosY; // PE +0x18c
        unsigned char unknown190[4];
        int MapZoom; // PE +0x194
        bool ShowIngameHud; // PE +0x198
        unsigned char unknown199[3];
        float field19c; // PE +0x19c
        float HitDirectionAngle; // PE +0x1a0
        float field1a4; // PE +0x1a4
        bool HitDirectionShow; // PE +0x1a8
        bool TimeInfoShow; // PE +0x1a9
        bool SetupShow; // PE +0x1aa
        bool SayAllShow; // PE +0x1ab
        bool SaySquadShow; // PE +0x1ac
        bool SayTeamShow; // PE +0x1ad
        bool ChatListShow; // PE +0x1ae
        bool CloseToFlag; // PE +0x1af
        bool CloseToAlliedCP; // PE +0x1b0
        bool CloseToAxisCP; // PE +0x1b1
        bool CloseToNeutralCP; // PE +0x1b2
        bool MedicInRadius; // PE +0x1b3
        bool RepairInRadius; // PE +0x1b4
        bool AmmoInRadius; // PE +0x1b5
        bool CloseToMine; // PE +0x1b6
        bool ShowRankUpIcon; // PE +0x1b7
        bool ShowMedalIcon; // PE +0x1b8
        bool PlayerRankIconShow; // PE +0x1b9
        bool PlayerKitIconShow; // PE +0x1ba
        unsigned char unknown1bb[1];
        const char* RankUpIconPathString; // PE +0x1bc
        const char* MedalIconPathString; // PE +0x1c0
        bool IngameHelpShow; // PE +0x1c4
        unsigned char unknown1c5[3];
        const wchar_t* IngameHelpString; // PE +0x1c8
        bool VictoryShow; // PE +0x1cc
        bool VictoryRankShow; // PE +0x1cd
        bool MapShow; // PE +0x1ce
        bool MapBorderShow; // PE +0x1cf
        bool MapBorderAlternateShow; // PE +0x1d0
        bool MapStatic; // PE +0x1d1
        bool SpawnShow; // PE +0x1d2
        bool SpawnInfoShow; // PE +0x1d3
        bool ScoreboardShow; // PE +0x1d4
        bool VoipListShow; // PE +0x1d5
        bool LevelsListShow; // PE +0x1d6
        bool MapFullSize; // PE +0x1d7
        bool MapMinSize; // PE +0x1d8
        bool MapFullSizeAndNotSpawnShow; // PE +0x1d9
        bool MapFullSizeAndSpawnShow; // PE +0x1da
        bool SuicideButtonShow; // PE +0x1db
        bool ObjectiveIconShow; // PE +0x1dc
        bool SquadOptionsShow; // PE +0x1dd
        bool JoinOptionShow; // PE +0x1de
        bool KickOptionShow; // PE +0x1df
        bool LeaveOptionShow; // PE +0x1e0
        bool InviteOptionShow; // PE +0x1e1
        bool MapMenuShow; // PE +0x1e2
        bool SquadLeaderMenuShow; // PE +0x1e3
        bool CommanderMenuShow; // PE +0x1e4
        bool ChoiceMenuShow; // PE +0x1e5
        bool CommanderShow; // PE +0x1e6
        bool SquadInterfaceShow; // PE +0x1e7
        bool RadioInterfaceShow; // PE +0x1e8
        bool RadioVehicleInterfaceShow; // PE +0x1e9
        bool SquadLeaderInterfaceShow; // PE +0x1ea
        bool CommanderInterfaceShow; // PE +0x1eb
        bool SpottedInterfaceShow; // PE +0x1ec
        bool DemoShow; // PE +0x1ed
        bool DemoCameraInterfaceShow; // PE +0x1ee
        bool DemoPlayerNameShow; // PE +0x1ef
        bool DemoRecInterfaceShow; // PE +0x1f0
        unsigned char unknown1f1[3];
        const char* DemoPlayerNameString; // PE +0x1f4
        const wchar_t* DemoPausePlayString; // PE +0x1f8
        const wchar_t* DemoCameraString; // PE +0x1fc
        bool MembersShow; // PE +0x200
        bool KitsShow; // PE +0x201
        bool kitShow[7]; // PE +0x202
        unsigned char unknown209[3];
        const char* optionIcons[10]; // PE +0x20c
        bool EnemyTicketsShow; // PE +0x234
        bool FriendlyTicketsShow; // PE +0x235
        bool BarTopLeftShow; // PE +0x236
        bool BarTopRightShow; // PE +0x237
        bool PlayerScoreShow; // PE +0x238
        bool Team1Selected; // PE +0x239
        bool Team2Selected; // PE +0x23a
        unsigned char unknown23b[1];
        int PlayerTeam; // PE +0x23c
        bool kitSelected[7]; // PE +0x240
        bool kitAbilityShow[35]; // PE +0x247
        bool kitUnlockArrowShow[7]; // PE +0x26a
        bool kitUnlockShow[7]; // PE +0x271
        float kitSprint[7]; // PE +0x278
        float kitUnlockAlpha[7]; // PE +0x294
        bool field2b0; // PE +0x2b0
        unsigned char unknown2b1[3];
        int field2b4; // PE +0x2b4; signed blink counter, 0x463D30
        bool weaponFlags[32]; // PE +0x2b8
        const char* LocalScore; // PE +0x2d8
        const char* kitWeapon[7]; // PE +0x2dc
        const char* kitAltWeapon[7]; // PE +0x2f8
        const char* kitAbilityIcons[35]; // PE +0x314
        const wchar_t* kitNames[7]; // PE +0x3a0
        const char* kitIcons[7]; // PE +0x3bc
        unsigned char unknown3d8[56];
        const wchar_t* squadNames[12]; // PE +0x410
        const wchar_t* squadLeaderNames[12]; // PE +0x440
        const wchar_t* radioNames[12]; // PE +0x470
        const wchar_t* radioVehicleNames[12]; // PE +0x4a0
        const wchar_t* commanderNames[12]; // PE +0x4d0
        const char* EnemyTicketsString; // PE +0x500
        const char* FriendlyTicketsString; // PE +0x504
        const char* ScoreString; // PE +0x508
        const wchar_t* TimeToSpawnString; // PE +0x50c
        const wchar_t* SpawnInfoString; // PE +0x510
        const char* LevelTime; // PE +0x514
        const wchar_t* ActiveLevel; // PE +0x518
        const char* FriendlyFlagIconPathString; // PE +0x51c
        const char* EnemyFlagIconPathString; // PE +0x520
        const char* Team1FlagIconPathString; // PE +0x524
        const char* Team2FlagIconPathString; // PE +0x528
        const char* FriendlyTeamNameString; // PE +0x52c
        const char* EnemyTeamNameString; // PE +0x530
        const wchar_t* Team1NameString; // PE +0x534
        const wchar_t* Team2NameString; // PE +0x538
        const char* RoundRestartString; // PE +0x53c
        const char* FriendlyTeamWinsString; // PE +0x540
        const char* EnemyTeamWinsString; // PE +0x544
        const wchar_t* WinHeadlineString; // PE +0x548
        const char* weaponIcons[12]; // PE +0x54c
        const char* ServerNameString; // PE +0x57c
        const char* ServerIPString; // PE +0x580
        const char* ServerPortString; // PE +0x584
        int field588; // PE +0x588, cached server port
        bool ServerIsFavourite; // PE +0x58c
        unsigned char unknown58d[3];
        const char* TotalNoOfPlayers; // PE +0x590
        const char* YesVotes; // PE +0x594
        const char* NoVotes; // PE +0x598
        bool ShowNightVisionGauge; // PE +0x59c
        unsigned char unknown59d[3];
    } m_state;
    friend struct HudInformationLayerFixture;
};
} }
#endif
#endif
