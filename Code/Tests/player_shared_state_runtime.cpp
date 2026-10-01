// [TEMP] [CHOSEN, not recovered] component fixture, never part of production.
// Real constructors and recovered Player/KitTemplate bodies; collaborator overrides observe calls.
// Objects live until process exit. This does not boot, bind a socket, or connect a retail client.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../BF2/Game/GameLogic/Player.h"
#include "../BF2/Game/GameLogic/GameLogic.h"
#include "../BF2/Game/Objects/Game/KitTemplate.h"
#include "../BF2/Game/Objects/ItemContainerTemplate.h"
#include "../BF2/Game/Objects/Game/PlayerControlObject.h"
#include "../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../BF2/Game/Objects/GenericFireArmTemplate.h"
#include "../BF2/Game/Objects/Networkables/Networkables.h"
#undef protected
#undef private
#include "../BF2/IO/BitStream.h"
#include "../BF2/IO/Network/NetworkManager.h"
#include "../BF2/Game/Objects/ICameraObject.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Scene/Object/BinTreeCuller.h"
#include "../BF2/Scene/Object/QuadTreeCuller.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/System.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Game/SingletonServer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../BF2/IO/Stream/MemStream.h"

using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe { extern ClassManager* classManager; } }
namespace {
int checks = 0, failures = 0;
void check(bool ok, const char* name)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    std::fflush(stdout);
}
LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, 0, TRUE);
    std::fprintf(stderr, "Fixture exception %08lx at %p\n", fault->ExceptionRecord->ExceptionCode,
                 fault->ExceptionRecord->ExceptionAddress);
    CONTEXT context = *fault->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip; frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 16; ++i)
    {
        char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
            std::fprintf(stderr, "  %08llx %s+%llu\n", frame.AddrPC.Offset, symbol->Name, displacement);
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &frame, &context,
                        0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) break;
    }
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
std::vector<int> scriptCalls;
struct Classes : ClassManager
{
    IBase* createInstance(unsigned int cid, unsigned int iid, IBase* outer) override
    {
        if (cid == CID_BinTreeCuller) return BinTreeCuller::create(iid, outer);
        if (cid == CID_QuadTreeCuller) return QuadTreeCuller::create(iid, outer);
        std::fprintf(stderr, "Unexpected fixture factory %u\n", cid);
        std::abort();
    }
};
struct ScriptPlayer : Player
{
    ScriptPlayer() : Player(-1, 0) { m_playerName = std::string("A\0 B", 4); }
    mutable int aiReads = 0, networkReads = 0;
    bool flagsOn = true;
    const std::string& getName() const override { scriptCalls.push_back(1); return m_playerName; }
    unsigned int getClassID() const override { scriptCalls.push_back(2); return CID_Player; }
    bool testFlags(const Flag32Bits<IPlayer>& flags) const override
    {
        if (&flags == &g_playerFlagBit0) { scriptCalls.push_back(4); return flagsOn && ++aiReads == 1; }
        scriptCalls.push_back(5); return flagsOn && ++networkReads == 1;
    }
};
struct Players : PlayerManager
{
    std::string type = "Player";
    const std::string& getClassNameFromPlayerClassID(unsigned int id) const override
    { scriptCalls.push_back(3); return type; }
};
struct Sink : io::MemStream
{
    std::string bytes;
    unsigned int count = 0;
    int writes = 0, flushes = 0;
    bool shortWrite = false;
    unsigned int write(const void* p, unsigned int n) override
    { ++writes; count = n; bytes.assign(static_cast<const char*>(p), n); return shortWrite ? n-1 : n; }
    void flushWrite() override { ++flushes; }
};
struct Logic : GameLogic
{
    [[noreturn]] static void unused()
    { std::fputs("Unexpected fixture GameLogic operation\n", stderr); std::abort(); }
#define UNUSED_METHOD(signature) signature override { unused(); }
    UNUSED_METHOD(void addRef())
    UNUSED_METHOD(int getRef())
    UNUSED_METHOD(int release())
    UNUSED_METHOD(void* queryInterface(unsigned int) const)
    UNUSED_METHOD(void runNextLevel())
    UNUSED_METHOD(void setNextLevel(int))
    UNUSED_METHOD(bool selectKitAndUnlockLevel(IPlayer*, int, int))
    UNUSED_METHOD(void setTicketState(unsigned int, int))
    UNUSED_METHOD(int getTicketState(unsigned int))
    UNUSED_METHOD(void setDefaultNumberOfTickets(unsigned int, unsigned int))
    UNUSED_METHOD(unsigned int getDefaultNumberOfTickets(unsigned int))
    UNUSED_METHOD(void setDefaultNumberOfTicketsEx(unsigned int, unsigned int, unsigned int))
    UNUSED_METHOD(unsigned int getDefaultNumberOfTicketsEx(unsigned int, unsigned int))
    UNUSED_METHOD(void setDefaultTimeToNextAIWave(float))
    UNUSED_METHOD(float getDefaultTimeToNextAIWave())
    UNUSED_METHOD(void setTicketLossAtEndPerMin(float))
    UNUSED_METHOD(float getTicketLossAtEndPerMin())
    UNUSED_METHOD(void setTicketLossPerMin(unsigned int, float))
    UNUSED_METHOD(float getTicketLossPerMin(unsigned int))
    UNUSED_METHOD(void playEffect(Vec3, const IObjectTemplate*, Vec3))
    UNUSED_METHOD(bool handleCollisionForProjectile(IObject*, IObject*, const Vec3&, const Vec3&,
                  const Vec3&, int, int, CollisionFaceInfo*, CollisionFaceInfo*))
    UNUSED_METHOD(bool handleCollisionForReplenisher(bool, IObject*, IObject*, const Vec3&,
                  const Vec3&, const Vec3&, int, int))
    UNUSED_METHOD(bool handleCollisionForResurrecter(IObject*, IObject*, float, float,
                  const Vec3&, const Vec3&, const Vec3&, int, int))
    UNUSED_METHOD(void handleCollision(IObject*, IObject*, const Vec3&, const Vec3&,
                  const Vec3&, int, int, CollisionFaceInfo*, CollisionFaceInfo*))
    UNUSED_METHOD(void spawnMultipleObjects(const IObjectTemplate*, int))
    UNUSED_METHOD(IObject* spawnObject(const IObjectTemplate*, Vec3, Vec3, int))
    UNUSED_METHOD(IObject* spawnObject(const std::string&, Vec3, Vec3, int))
    UNUSED_METHOD(void checkPlayerTriggers(IPlayer*, IObject*, io::PlayerInput, bool))
    UNUSED_METHOD(void toggleLocalFreeCamera())
    UNUSED_METHOD(TicketHistory& getTicketHistory())
    UNUSED_METHOD(void setTicketHistory(const TicketHistory&))
#undef UNUSED_METHOD
    bool server = true;
    int team = 0, kitNumber = 0;
    KitTemplate* kit = 0;
    bool isServer() const override { return server; }
    IObjectTemplate* getKitTemplate(int t, int k) override { team = t; kitNumber = k; return kit; }
};
struct Templates : ObjectTemplateManager
{
    std::map<std::string, IObjectTemplate*> entries;
    mutable std::vector<std::string> lookups;
    IObjectTemplate* getTemplate(const std::string& name) const override
    { lookups.push_back(name); const auto it = entries.find(name); return it == entries.end() ? 0 : it->second; }
};
struct Container : ItemContainerTemplate
{
    mutable std::vector<int> calls;
    explicit Container(int level) { m_unlockLevel = level; }
    void* queryInterface(unsigned int id) const override
    { calls.push_back(id == 0 ? 1 : 9); return ItemContainerTemplate::queryInterface(id); }
    unsigned int getClassID() const override { calls.push_back(2); return CID_ItemContainerTemplate; }
    void* queryImplementation(unsigned int id) override
    { calls.push_back(3); return ItemContainerTemplate::queryImplementation(id); }
};
struct PositionObject : Object
{
    PositionObject() : Object(0) {}
    int writes = 0;
    Vec3 position;
    void setAbsolutePosition(const Vec3& p) override { ++writes; position = p; }
};
struct InputPlayer : Player
{
    InputPlayer() : Player(-1, 0) {}
    IObject* vehicle = 0;
    ICameraObject* camera = 0;
    mutable int vehicleReads = 0;
    mutable int cameraReads = 0;
    IObject* getVehicle() const override { ++vehicleReads; return vehicle; }
    ICameraObject* getCamera() const override { ++cameraReads; return camera; }
};
struct CameraProbe : ICameraObject
{
    int explosions = 0;
    float lastAmount = 0.0f;
    [[noreturn]] static void unused()
    { std::fputs("Unexpected fixture camera operation\n", stderr); std::abort(); }
    void* queryInterface(unsigned int) const override { unused(); }
    void addRef() override { unused(); }
    int getRef() override { unused(); }
    int release() override { unused(); }
    const Mat4& getTransformation(float) override { unused(); }
    void setInterpolatedMode(bool) override { unused(); }
    bool setViewMode(CameraViewMode, IObject*, bool) override { unused(); }
    CameraViewMode getViewMode() const override { unused(); }
    bool isActive() const override { unused(); }
    void setIsActive(bool) override { unused(); }
    void setCameraId(int) override { unused(); }
    int getCameraId() override { unused(); }
    void prevViewMode() override { unused(); }
    void setTraceObjectId(unsigned int) override { unused(); }
    unsigned int getTraceObjectId() override { unused(); }
    void setExternCameraTrans(Mat4) override { unused(); }
    Mat4* getExternCameraTrans() override { unused(); }
    bool getToggleMouseLook() override { unused(); }
    int getPcoId() override { unused(); }
    void handleExplosion(float amount) override { ++explosions; lastAmount = amount; }
    void resetCameraView(bool) override { unused(); }
    void updateCameraView(bool) override { unused(); }
    float getActiveFOV() const override { unused(); }
    const Vec3& getStaticSunDirection() const override { unused(); }
    const Vec3& getStaticSunColor() const override { unused(); }
    const Vec3& getStaticSunDamageColor() const override { unused(); }
    const Vec3& getStaticSunAmbientColor() const override { unused(); }
};
struct HudSink : IEventListener
{
    int hitDirectionEvents = 0;
    void handleEvent(EventCategory category, unsigned int id, EventNode*, void*) override
    { if (category == ECHud && id == 52) ++hitDirectionEvents; }
};
struct Control : PlayerControlObject
{
    explicit Control(const PlayerControlObjectTemplate* t) : PlayerControlObject(t) {}
    IObject* weapons[3] = {};
    mutable std::vector<int> slots;
    std::vector<std::pair<IObject*, int> > activeWeaponWrites;
    std::vector<bool> duckingWrites;
    IObject* getActiveWeapon(int slot) const override { slots.push_back(slot); return weapons[slot]; }
    void setActiveWeapon(IObject* weapon, int slot) override
    { activeWeaponWrites.push_back(std::make_pair(weapon, slot)); weapons[slot] = weapon; }
    void setIsDucking(bool ducking) override
    { duckingWrites.push_back(ducking); m_isDucking = ducking; }
};
struct Weapon : GenericFireArm
{
    explicit Weapon(const GenericFireArmTemplate* t) : GenericFireArm(t) {}
    bool reload = false, needs = false, heat = false, reloading = false, missile = false;
    bool usesHeat = false;
    bool primaryProjectile = true, altProjectile = true;
    int ammoInMag = 0, maxMags = 0, currentMags = 0, magSize = 0, zoom = 0, wireFlags = 0;
    FireRate fireRate = static_cast<FireRate>(0);
    float heatAmount = 0.0f;
    WeaponState receivedState = {};
    int stateSyncWrites = 0;
    mutable int maxMagsReads = 0;
    mutable int currentMagsReads = 0;
    int magSizeReads = 0;
    io::PlayerInputMap fire = io::PIFire, alt = io::PIAltFire;
    mutable std::vector<int> calls;
    FireRate getFireRate() override { return fireRate; }
    float getHeat() const override { return heatAmount; }
    bool shouldReload() const override { calls.push_back(1); return reload; }
    bool needsReload() const override { calls.push_back(2); return needs; }
    bool getHasHeat() const override { calls.push_back(3); return heat; }
    bool getUsesHeat() const override { return usesHeat; }
    bool isReloading() const override { calls.push_back(4); return reloading; }
    bool isWaitingForMissileConfirm() const override { calls.push_back(5); return missile; }
    int getAmmoInMag(unsigned int) const override { return ammoInMag; }
    int getMaxNrOfMags() const override { ++maxMagsReads; return maxMags; }
    int getCurrentNrOfMags() const override { ++currentMagsReads; return currentMags; }
    int getMagSize() override { ++magSizeReads; return magSize; }
    int getZoom() override { return zoom; }
    int getFlags() const override { return wireFlags; }
    void setStateSync(const WeaponState& state) override
    { receivedState = state; ++stateSyncWrites; }
    io::PlayerInputMap getFireInput() const override { calls.push_back(6); return fire; }
    io::PlayerInputMap getAltFireInput() const override { calls.push_back(7); return alt; }
    bool launchesProjectile(int slot) const override
    { calls.push_back(8 + slot); return slot == 0 ? primaryProjectile : altProjectile; }
};
io::PlayerInput inputState(bool fire)
{
    io::PlayerInput input = {};
    input.mappingFlags = (1ULL << io::PIFire) | (1ULL << io::PIAltFire) |
                         (1ULL << io::PIFlareFire) | (1ULL << io::PIReload);
    input.inputs[io::PIFire] = fire ? 1.0f : 0.0f;
    input.inputs[io::PIAltFire] = fire ? 1.0f : 0.0f;
    input.inputs[io::PIFlareFire] = fire ? 1.0f : 0.0f;
    return input;
}
}

int main()
{
    static_assert(sizeof(void*) == 4, "x86 only");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _CALL_REPORTFAULT);
    SetUnhandledExceptionFilter(reportFault);
    g_system = new System;
    settings = new SettingsRepostitory;
    eventManager = new EventManager;
    classManager = new Classes;
    objectManager = new ObjectManager;
    playerManager = new Players;
    Logic* logic = new Logic; gameLogic = logic;
    Player* player = new Player(-1, 0);
    IPlayer* api = player;
    WorldPref::resetTickCount(100);
    api->setSpawnAtTick(130);
    check(std::fabs(api->getTimeToSpawn() - 1.0f) < 0.000001f, "spawn timer uses thirty ticks per second");
    api->setSpawnAtTick(99);
    check(api->getTimeToSpawn() == 0.0f, "expired spawn timer clamps to zero");
    WorldPref::resetTickCount(0xFFFFFFF0u); api->setSpawnAtTick(14);
    check(std::fabs(api->getTimeToSpawn() - 1.0f) < 0.000001f, "spawn timer wraps subtraction before signed conversion");
    WorldPref::resetTickCount(0); api->setSpawnAtTick(0x80000000u);
    check(api->getTimeToSpawn() == 0.0f, "half-range delta remains negative");
    api->setSpawnAtTick(60); player->m_isManDown = true;
    check(std::fabs(api->getManDownTimeLeft() - 2.0f) < 0.000001f, "man-down query now reaches real spawn timer");
    check(api->getViewDistance() == 500.0f, "view distance has global integer default");
    settings->intSet("GSViewDistance", -41);
    check(api->getViewDistance() == -41.0f, "view distance preserves negative settings");
    turnOffAllDebug(DEBUGTYPE_6); // fixture disables the retail Debug termination policy
    api->setViewDistance(777.0f);
    check(api->getViewDistance() == -41.0f, "view-distance setter writes no setting when Debug is disabled");
    api->setUpdateFrequencyType(static_cast<UpdateFreqencyType>(-123));
    api->setAutoReload(true); api->setKitTimeStamp(123456.125);
    api->handleUpdate(99.0f, 0xFFFFFFFFu);
    check(static_cast<int>(api->getUpdateFrequencyType()) == -123 && api->getAutoReload() &&
          api->getKitTimeStamp() == 123456.125, "update no-op leaves full-width shared state intact");
    api->setAutoReload(false);
    check(!api->getAutoReload() && !api->getIsUnderwater() && !api->isPlayerInNightMap(), "dedicated constant queries and reload clear");
    api->doAddTouchItem(0); // retail null-singleton guard
    check(true, "null nametag manager needs no object");
    std::memset(&player->m_playerInput, 0x5A, sizeof(player->m_playerInput));
    player->m_playerInput.inputs[47] = 2.5f;
    io::PlayerInput copy = api->getPlayerInput();
    check(std::memcmp(&copy, &player->m_playerInput, sizeof(copy)) == 0, "PE input snapshot includes whole native aggregate");
    copy.inputs[47] = 9.0f;
    check(player->m_playerInput.inputs[47] == 2.5f, "input snapshot has independent ownership");

    ScriptPlayer* scripted = new ScriptPlayer;
    Sink* sink = new Sink;
    scriptCalls.clear();
    io::IConsoleSaveable* saveable = scripted;
    check(saveable->makeScript(static_cast<io::IMemStream*>(sink)), "script writes through adjusted secondary face");
    const std::string expected = std::string("Player.create Player A\0 B", 25) +
                                 "\nPlayer.isAi 0\nPlayer.isNetwork 0\n";
    check(sink->bytes == expected && sink->count == expected.size(), "script preserves embedded NUL, spaces and repeated flag values");
    check(scriptCalls == std::vector<int>({1,2,3,4,4,5,5}) && sink->writes == 1 && sink->flushes == 0,
          "script preserves query ordering and makes one write without flushing target");
    scripted->flagsOn = false; sink->shortWrite = true; scriptCalls.clear();
    check(!saveable->makeScript(static_cast<io::IMemStream*>(sink)) && sink->writes == 2,
          "script returns false on short write without retry");
    check(scriptCalls == std::vector<int>({1,2,3,4,5}) && sink->bytes.find("Player.isAi") == std::string::npos,
          "unset script flags emit no extra lines");

    Templates* templates = new Templates; objectTemplateManager = templates;
    KitTemplate* kit = new KitTemplate; logic->kit = kit; kit->m_creationIndex = 21;
    Container* low = new Container(-7);
    Container* high = new Container(3);
    Container* nested = new Container(12);
    templates->entries["low"] = low; templates->entries["high"] = high;
    templates->entries["nested"] = nested;
    check(kit->getHighestUnlockLevel() == 0, "empty kit has maximum unlock level zero");
    kit->m_templateInfos.push_back(new BundleTemplate::TemplateInfo("low"));
    check(kit->getHighestUnlockLevel() == 0, "negative container level cannot lower zero floor");
    kit->m_templateInfos.push_back(new BundleTemplate::TemplateInfo("high"));
    high->m_templateInfos.push_back(new BundleTemplate::TemplateInfo("nested"));
    templates->lookups.clear(); high->calls.clear();
    check(kit->getHighestUnlockLevel() == 3 && templates->lookups == std::vector<std::string>({"low","high"}),
          "unlock maximum resolves direct names and does not recurse");
    check(high->calls == std::vector<int>({1,2,3}), "helper queries interface zero before class id, then implementation");
    templates->lookups.clear();
    check(!api->isUnlocked(6, 2, 0) && templates->lookups.empty() && logic->team == 2 && logic->kitNumber == 6,
          "absent profile key returns false before visiting kit children");
    player->m_unlockMaps[1][21] = 2;
    check(!api->isUnlocked(6, 2, 3) && templates->lookups.empty(), "insufficient profile level skips template walk");
    check(api->isUnlocked(6, 2, 2), "profile and direct-container level both permit unlock");
    player->m_unlockMaps[1][21] = 9;
    check(!api->isUnlocked(6, 2, 4), "kit maximum limits higher profile unlock");
    check(api->isUnlocked(6, 2, -1) && !api->isUnlocked(6, 1, -1), "signed requested level and separate team map preserved");

    InputPlayer* inputPlayer = new InputPlayer;
    logic->server = false;
    check(inputPlayer->moveTo(Vec3{1,2,3}) == "Cannot move player from client." && inputPlayer->vehicleReads == 0,
          "move rejects client before asking for vehicle");
    logic->server = true;
    check(inputPlayer->moveTo(Vec3{1,2,3}) == "Player has no vehicle.", "move reports absent root");
    PositionObject* child = new PositionObject; PositionObject* root = new PositionObject;
    child->m_parent = root; child->m_flags &= ~0x01000000u;
    inputPlayer->vehicle = child; inputPlayer->vehicleReads = 0;
    check(inputPlayer->moveTo(Vec3{-4,5,6}).empty() && inputPlayer->vehicleReads == 1 && root->writes == 1 &&
          child->writes == 0 && root->position.x == -4 && root->position.z == 6, "move writes root absolute position once");

    Control* control = new Control(new PlayerControlObjectTemplate);
    Weapon* weapon = new Weapon(new GenericFireArmTemplate);
    control->weapons[0] = weapon;
    inputPlayer->vehicle = control;
    WeakPtr<IObject>::Ctrl weak = {1, control}; inputPlayer->m_vehicleWeak = &weak;
    inputPlayer->setAutoReload(true); weapon->reload = true;
    io::PlayerInput input = inputState(false);
    inputPlayer->getAutoInput(input, false);
    check(input.inputs[io::PIReload] == 1.0f && control->slots == std::vector<int>({0}), "auto reload stops at first eligible weapon");
    input.mappingFlags &= ~(1ULL << io::PIReload); input.inputs[io::PIReload] = 7.0f;
    inputPlayer->getAutoInput(input, false);
    check(input.inputs[io::PIReload] == 7.0f, "auto reload respects unmapped input");
    input = inputState(true); weapon->calls.clear(); control->slots.clear(); inputPlayer->vehicleReads = 0;
    weapon->needs = true;
    inputPlayer->getAutoInput(input, true);
    check(weapon->calls == std::vector<int>({2,6,7,8,9}), "remote needs-reload short circuits other weapon state reads");
    check(input.inputs[io::PIFire] == 0 && input.inputs[io::PIAltFire] == 0 && input.inputs[io::PIFlareFire] == 1 &&
          input.inputs[io::PIReload] == 0, "remote input clears weapon fire buttons without auto reloading");
    check(inputPlayer->vehicleReads == 2 && control->slots == std::vector<int>({0,1,2}), "active-fire arm repeats vehicle query and scans all three slots");
    inputPlayer->setAutoReload(false); weapon->needs = false; weapon->missile = true;
    weapon->altProjectile = false; weapon->calls.clear(); input = inputState(true);
    inputPlayer->getAutoInput(input, false);
    check(weapon->calls == std::vector<int>({3,4,5,6,7,8,9}) && input.inputs[io::PIAltFire] == 1,
          "local missile wait checks heat and reload first; non-projectile alt fire remains");
    weapon->calls.clear(); input = inputState(true); inputPlayer->getAutoInput(input, true);
    check(weapon->calls == std::vector<int>({2,3,4}) && input.inputs[io::PIFire] == 1,
          "remote input ignores missile-confirm wait");
    weapon->heat = true; weapon->fire = static_cast<io::PlayerInputMap>(64);
    weapon->alt = static_cast<io::PlayerInputMap>(64); weapon->calls.clear(); input = inputState(true);
    inputPlayer->getAutoInput(input, true);
    check(weapon->calls == std::vector<int>({2,3,6,7}), "button sentinel skips projectile queries and writes");
    weapon->fire = io::PIFire; weapon->alt = io::PIAltFire; weapon->calls.clear();
    input = inputState(false); input.inputs[io::PIFire] = std::numeric_limits<float>::quiet_NaN();
    inputPlayer->getAutoInput(input, true);
    check(input.inputs[io::PIFire] == 0 && !weapon->calls.empty(), "mapped NaN fire follows nonzero branch");
    input = inputState(true); input.mappingFlags = 0; weapon->calls.clear();
    inputPlayer->getAutoInput(input, true);
    check(weapon->calls.empty() && input.inputs[io::PIFire] == 1, "unmapped nonzero fire never queries weapons");

    // Player state-sync sender/receiver pair. Both ends use real Player, PlayerControlObject,
    // GenericFireArm and BitStream objects; only the weapon accessors are fixture observers.
    InputPlayer* stateWriter = new InputPlayer;
    InputPlayer* stateReader = new InputPlayer;
    CameraProbe* readerCamera = new CameraProbe;
    HudSink* hudSink = new HudSink;
    check(eventManager->registerEventHandler(ECHud, hudSink, 0),
          "state-sync fixture registers HUD event observer");
    Control* writerControl = new Control(new PlayerControlObjectTemplate);
    Control* readerControl = new Control(new PlayerControlObjectTemplate);
    Weapon* writerWeapon = new Weapon(new GenericFireArmTemplate);
    Weapon* readerWeapon = new Weapon(new GenericFireArmTemplate);
    Weapon* readerLimitWeapon = new Weapon(new GenericFireArmTemplate);
    writerControl->weapons[0] = writerWeapon;
    readerControl->weapons[0] = readerWeapon;
    readerControl->weapons[1] = readerLimitWeapon;
    writerControl->m_weapons.push_back(writerWeapon);
    readerControl->m_weapons.push_back(readerWeapon); // getWeaponVehicle's non-empty gate
    stateWriter->vehicle = writerControl;
    stateReader->vehicle = readerControl;
    stateReader->camera = readerCamera;

    writerWeapon->m_flags &= ~1u; // updateFiringState accepts enabled weapons only
    readerWeapon->m_flags &= ~1u;
    writerWeapon->ammoInMag = 31;
    writerWeapon->maxMags = 9;
    writerWeapon->currentMags = 4;
    writerWeapon->magSize = 30;
    writerWeapon->fireRate = static_cast<FireRate>(2);
    writerWeapon->zoom = 3;
    writerWeapon->wireFlags = 1;
    writerWeapon->usesHeat = true;
    writerWeapon->heatAmount = 3.0f;
    readerWeapon->maxMags = 12;
    readerWeapon->currentMags = 7;
    readerWeapon->magSize = 40;
    readerWeapon->usesHeat = true;
    readerLimitWeapon->maxMags = 5;
    readerLimitWeapon->currentMags = 9;
    readerLimitWeapon->magSize = 50;

    stateWriter->m_hitIndicator = 1;
    stateWriter->m_isGivingReplenish = true;
    stateWriter->m_isGivingAbility = false;
    stateWriter->m_isRepairing = true;
    stateWriter->m_squadInviteMask = 0xA5;
    stateWriter->m_hitFromDirection = 1.75f;
    stateWriter->m_someHitCounter = 2;
    stateWriter->m_cameraShake = 12.5f;
    stateWriter->m_field194 = 2;
    stateWriter->m_spawnAtTick = 0x12345678u;
    stateWriter->m_isManDown = true;
    stateWriter->m_isInsideCP = true;
    stateWriter->m_field160[3] = true;
    stateWriter->m_field160[4] = false;

    io::BitStream stateStream(64);
    check(stateWriter->getStateSync(&stateStream, writerControl),
          "state-sync sender writes real control-object payload");
    const unsigned int stateBits = stateStream.getBitPosition();
    check(stateBits == 113, "state-sync fixture wire is 113 bits");
    stateStream.setBitSize(stateBits);
    stateStream.setReadMode();
    check(stateReader->setStateSync(&stateStream, readerControl, 9876.5),
          "state-sync receiver accepts complete payload");
    check(stateStream.getBitPosition() == stateBits,
          "state-sync receiver consumes the complete payload");
    check(stateReader->m_hitIndicator == 1 && stateReader->m_isGivingReplenish &&
          !stateReader->m_isGivingAbility && stateReader->m_isRepairing &&
          stateReader->m_squadInviteMask == 0xA5,
          "state-sync receiver applies indicator, support flags and invite mask");
    const float expectedHitDirection = static_cast<float>(
        (163.0 / 255.0 * 2.0 - 1.0) * 6.2831855f);
    check(stateReader->m_someHitCounter == 1 &&
          stateReader->m_hitFromDirection == expectedHitDirection,
          "state-sync receiver exactly dequantizes hit direction and decrements its counter");
    check(hudSink->hitDirectionEvents == 1 && stateReader->cameraReads == 2 &&
          readerCamera->explosions == 1 && stateReader->m_field194 == 1 &&
          std::fabs(readerCamera->lastAmount - 12.5f) < 0.06f,
          "state-sync receiver posts HUD hit and repeats camera lookup for one shake");
    check(stateReader->m_weaponSlotActive[0] && !stateReader->m_weaponSlotActive[1] &&
          !stateReader->m_weaponSlotActive[2] && readerWeapon->stateSyncWrites == 1,
          "state-sync receiver selects one slot and pushes it to the client weapon");
    check(readerWeapon->receivedState.ammoInMag == 31 &&
          static_cast<int>(readerWeapon->receivedState.fireRate) == 2 &&
          readerWeapon->receivedState.currentNrOfMags == 4 &&
          readerWeapon->receivedState.camera == 3 &&
          readerWeapon->receivedState.flags == 1 &&
          std::fabs(readerWeapon->receivedState.heat - 3.0f) < 0.03f,
          "state-sync weapon state preserves ammo, fire mode, zoom, flags and heat");
    check(readerWeapon->maxMagsReads == 2 && readerWeapon->currentMagsReads == 1 &&
          readerWeapon->magSizeReads == 1 && stateReader->m_maxNrOfMagsWeap1[0] == 12 &&
          stateReader->m_magSizeWeap1[0] == 40,
          "state-sync limit refresh repeats max read and caches active-slot limits");
    check(readerLimitWeapon->maxMagsReads == 2 && readerLimitWeapon->currentMagsReads == 1 &&
          readerLimitWeapon->magSizeReads == 1 && stateReader->m_maxNrOfMagsWeap1[1] == 5 &&
          stateReader->m_currNbMagsWeap1[1] == 5 && stateReader->m_magSizeWeap1[1] == 50,
          "state-sync limit refresh clamps an inactive slot before wire decoding");
    check(stateReader->vehicleReads == 2,
          "state-sync ammo handoff performs the shipped vehicle gate and weapon-vehicle lookup");
    check(stateReader->m_spawnAtTick == 0x12345678u && stateReader->m_isManDown &&
          stateReader->m_isInsideCP && stateReader->m_field160[3] &&
          !stateReader->m_field160[4],
          "state-sync receiver applies spawn, man-down, control-point and sprint tail");

    InputPlayer* noVehicleReader = new InputPlayer;
    noVehicleReader->m_someHitCounter = 1; // suppress the one-shot event while reusing the payload
    noVehicleReader->m_field194 = 1;
    for (int slot = 0; slot < 3; ++slot)
        noVehicleReader->m_ammoCountWeap1[slot] = 77;
    const int stateWritesBeforeNullVehicle = readerWeapon->stateSyncWrites;
    stateStream.resetPositions();
    stateStream.setReadMode();
    check(noVehicleReader->setStateSync(&stateStream, readerControl, 0.0) &&
          noVehicleReader->vehicleReads == 1 &&
          noVehicleReader->m_ammoCountWeap1[0] == 0 &&
          noVehicleReader->m_ammoCountWeap1[1] == 0 &&
          noVehicleReader->m_ammoCountWeap1[2] == 0 &&
          noVehicleReader->m_currNbMagsWeap1[0] == 4 &&
          readerWeapon->stateSyncWrites == stateWritesBeforeNullVehicle,
          "null vehicle clears only cached ammo and skips client-weapon state handoff");

    InputPlayer* quietWriter = new InputPlayer;
    io::BitStream quietState(16);
    check(quietWriter->getStateSync(&quietState, 0),
          "state-sync sender writes object-null payload");
    const unsigned int quietBits = quietState.getBitPosition();
    check(quietBits == 59, "object-null state-sync wire is 59 bits");
    quietState.setBitSize(quietBits);
    quietState.setReadMode();
    stateReader->m_hitIndicator = 1;
    stateReader->m_someHitCounter = 2;
    stateReader->m_field194 = 2;
    const int cameraReadsBeforeQuiet = stateReader->cameraReads;
    check(stateReader->setStateSync(&quietState, 0, 55.0) &&
          stateReader->m_hitIndicator == 1 && stateReader->m_someHitCounter == 1 &&
          stateReader->m_field194 == 1 && stateReader->cameraReads == cameraReadsBeforeQuiet,
          "absent state-sync pulses retain hit latch and age counters without camera lookup");

    unsigned char truncatedBytes[8] = {};
    std::memcpy(truncatedBytes, quietState.getBuffer(), sizeof(truncatedBytes));
    io::BitStream truncatedState(truncatedBytes, sizeof(truncatedBytes));
    truncatedState.setBitSize(quietBits - 1);
    truncatedState.setReadMode();
    stateReader->m_spawnAtTick = 99;
    stateReader->m_isManDown = true;
    stateReader->m_isInsideCP = true;
    stateReader->m_field160[3] = true;
    stateReader->m_field160[4] = true;
    check(!stateReader->setStateSync(&truncatedState, 0, 0.0) &&
          !truncatedState.hasError() && truncatedState.m_errorCode == 0 &&
          stateReader->m_spawnAtTick == 0 && !stateReader->m_isManDown &&
          !stateReader->m_isInsideCP && !stateReader->m_field160[3] &&
          !stateReader->m_field160[4],
          "truncated state sync keeps tail mutations, clears error code/latch and returns false");

    // PlayerControlObjectNetworkable receiver, own-camera arm. The concrete networkable, control
    // object, weapons and BitStream are production classes; Control's two narrow overrides record
    // the receiver's application calls without entering unrelated weapon/HUD machinery.
    readerControl->m_ownCameras[7] = readerCamera;
    readerControl->m_slotWeapons[0].clear();
    readerControl->m_slotWeapons[0].push_back(readerWeapon);
    readerControl->m_slotWeapons[0].push_back(readerLimitWeapon);
    readerControl->m_slotWeapons[1].clear();
    readerControl->m_slotWeapons[1].push_back(readerWeapon);
    readerControl->m_slotWeapons[2].clear();
    readerControl->activeWeaponWrites.clear();
    readerControl->duckingWrites.clear();
    readerControl->m_isDucking = false;

    PlayerControlObjectNetworkable pcoNetworkable;
    check(pcoNetworkable.init(readerControl),
          "PCO networkable receiver binds a real control object and allocates its ring");
    pcoNetworkable.m_records[0]._pad21[0] = 0x21;
    pcoNetworkable.m_records[0]._pad21[1] = 0x43;
    pcoNetworkable.m_records[0]._pad21[2] = 0x65;

    io::BitStream pcoFullStream(8);
    unsigned int pcoMask = 0x0Fu;
    unsigned int pcoSlot0 = 2u; // index 1 after the wire's value - 1 transform
    unsigned int pcoSlot1 = 1u; // index 0
    unsigned int pcoSlot2 = 0u; // -1 sentinel: decode, but do not apply
    unsigned int pcoDucking = 1u;
    pcoFullStream.writeBits(&pcoMask, 4);
    pcoFullStream.writeBits(&pcoSlot0, 4);
    pcoFullStream.writeBits(&pcoSlot1, 4);
    pcoFullStream.writeBits(&pcoSlot2, 4);
    pcoFullStream.writeBits(&pcoDucking, 1);
    const unsigned int pcoFullBits = pcoFullStream.getBitPosition();
    pcoFullStream.setBitSize(pcoFullBits);
    pcoFullStream.setReadMode();
    const int pcoReadBitsBefore = PlayerControlObjectNetworkable::m_stats.m_readBits;
    pcoNetworkable.setNetUpdate(pcoFullStream, 4321.5f, false, 0, io::StatePrecisionFull);

    const PlayerControlObjectNetworkable::Record& pcoFullRecord = pcoNetworkable.m_records[0];
    check(pcoFullBits == 17 && pcoFullStream.getBitPosition() == pcoFullBits &&
          PlayerControlObjectNetworkable::m_stats.m_readBits == pcoReadBitsBefore + 17,
          "PCO full receiver consumes and accounts for mask, three indices and duck bit");
    check(pcoFullRecord.time == 4321.5f && pcoFullRecord.weaponIndex[0] == 1 &&
          pcoFullRecord.weaponIndex[1] == 0 && pcoFullRecord.weaponIndex[2] == -1,
          "PCO full receiver timestamps the ring and decodes four-bit weapon values");
    check(sizeof(pcoFullRecord.isDucking) == 1 && pcoFullRecord.isDucking &&
          pcoFullRecord._pad21[0] == 0x21 && pcoFullRecord._pad21[1] == 0x43 &&
          pcoFullRecord._pad21[2] == 0x65,
          "PCO full receiver writes one ducking byte at +0x20 without touching its padding");
    check(readerControl->activeWeaponWrites.size() == 2 &&
          readerControl->activeWeaponWrites[0] == std::make_pair(static_cast<IObject*>(readerLimitWeapon), 0) &&
          readerControl->activeWeaponWrites[1] == std::make_pair(static_cast<IObject*>(readerWeapon), 1),
          "PCO full precision applies valid weapon indices in slot order and skips -1");
    check(readerControl->duckingWrites == std::vector<bool>(1, true) && readerControl->m_isDucking,
          "PCO full precision applies the decoded ducking byte after weapons");

    io::BitStream pcoCoarseStream(4);
    pcoMask = 0x09u;
    pcoSlot0 = 1u;
    pcoDucking = 0u;
    pcoCoarseStream.writeBits(&pcoMask, 4);
    pcoCoarseStream.writeBits(&pcoSlot0, 4);
    pcoCoarseStream.writeBits(&pcoDucking, 1);
    const unsigned int pcoCoarseBits = pcoCoarseStream.getBitPosition();
    pcoCoarseStream.setBitSize(pcoCoarseBits);
    pcoCoarseStream.setReadMode();
    const int pcoReadBitsAfterFull = PlayerControlObjectNetworkable::m_stats.m_readBits;
    pcoNetworkable.setNetUpdate(pcoCoarseStream, 8765.25f, false, 0, io::StatePrecisionGhost);

    const PlayerControlObjectNetworkable::Record& pcoCoarseRecord = pcoNetworkable.m_records[1];
    check(pcoCoarseBits == 9 && pcoCoarseStream.getBitPosition() == pcoCoarseBits &&
          PlayerControlObjectNetworkable::m_stats.m_readBits == pcoReadBitsAfterFull + 9 &&
          pcoCoarseRecord.time == 8765.25f && pcoCoarseRecord.weaponIndex[0] == 0 &&
          !pcoCoarseRecord.isDucking && pcoCoarseRecord._pad21[0] == 0x21 &&
          pcoCoarseRecord._pad21[1] == 0x43 && pcoCoarseRecord._pad21[2] == 0x65,
          "PCO coarse receiver still consumes, accounts and records its unoccupied update");
    check(readerControl->getOccupingPlayer() == 0 && readerControl->activeWeaponWrites.size() == 2 &&
          readerControl->duckingWrites == std::vector<bool>(1, true) && readerControl->m_isDucking,
          "PCO coarse unoccupied gate suppresses weapon and ducking application");

    inputPlayer->m_vehicleWeak = 0;
    std::printf("Player shared-state checks: %d, failures: %d\n", checks, failures);
    return failures ? 1 : 0;
}
