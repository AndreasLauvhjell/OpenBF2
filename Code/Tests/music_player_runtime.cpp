// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: music-player-runtime]
// Exercise the actual MusicPlayer and real event/registry/null-sound/Debug bodies.
// Typed observers cover only MusicPlayer's collaborators; unobserved host/mod slots abort.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <algorithm>
#include <string>
#include <vector>
#include "../BF2/Sound/BfSound/MusicPlayer.h"
#include "../BF2/Sound/BfSound.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/Game/IModInfo.h"
#include "../BF2/Game/Main/Game.h"
#include "SwiffHostStandIn.h"

using namespace dice::hfe;
namespace dice { namespace hfe {
// Only module globals normally supplied by Core/Globals.cpp are fixture-owned.
ClassManager* classManager = nullptr;
EventManager* eventManager = nullptr;
// [TEMP] Only the runtime dedicated flag is modeled: linking all Game.cpp also pulls
// its static network event makers (271 unresolved dependencies in the strict attempt).
// This is a fixture switch, not validation of Game.cpp's two already-recovered bodies.
static bool fixtureDedicated = false;
bool isDedicated() { return fixtureDedicated; }
void setDedicated(bool value) { fixtureDedicated = value; }
} }

static unsigned checks, failures;
static MusicPlayer* observedPlayer;
static std::vector<std::string> calls;
static std::vector<int> callbackStates;
// [TEMP] Inspect the attested sole data word (+4) without adding a production friend/accessor.
// The private-state corruption below is used only for adversarial diagnostic cases.
static int state(const MusicPlayer& player)
{
    static_assert(sizeof(MusicPlayer) == 8, "MusicPlayer is the x86 vptr plus one state word");
    int value;
    std::memcpy(&value, reinterpret_cast<const unsigned char*>(&player) + 4, sizeof(value));
    return value;
}
static void corruptState(MusicPlayer& player, int value)
{ std::memcpy(reinterpret_cast<unsigned char*>(&player) + 4, &value, sizeof(value)); }
static void record(const std::string& call)
{ calls.push_back(call); callbackStates.push_back(observedPlayer ? state(*observedPlayer) : -1); }
static void check(bool passed, const char* label)
{ ++checks; if (!passed) { ++failures; std::printf("FAIL %s\n", label); } }
static void expect(std::initializer_list<const char*> expected, const char* label)
{
    std::vector<std::string> wanted;
    for (const char* call : expected) wanted.push_back(call);
    check(calls == wanted, label);
    if (calls != wanted) for (const auto& call : calls) std::printf("  saw %s\n", call.c_str());
}

struct ModObserver : IModInfo {
    Mod mod;
    Map map;
    bool hasMod = true, hasMap = true;
    mutable std::string requestedMap;
    ModObserver() { mod.music = "menu/theme.ogg"; map.music = "level/theme.ogg"; }
    void addRef() override { std::abort(); }
    int getRef() override { std::abort(); }
    int release() override { std::abort(); }
    void* queryInterface(unsigned int) const override { std::abort(); }
    bool init() override { std::abort(); }
    std::vector<Mod>* getModList() override { std::abort(); }
    const Map* getMap(const std::string& name) const override
    { requestedMap = name; record("map:" + name); return hasMap ? &map : nullptr; }
    const Mod* getCurrentMod() const override
    { record("currentMod"); return hasMod ? &mod : nullptr; }
    std::string getCurrentModPath() override { std::abort(); }
    const Map* findMapFromCurrentMod(const std::string&) override { std::abort(); }
    const Map* findMapFromMod(const Mod*, const std::string&) override { std::abort(); }
    const Mod* getModFromName(const std::string&) override { std::abort(); }
    const Mod* getModFromPath(const std::string&) override { std::abort(); }
    const Map* getMapFromGameSpyId(int) override { std::abort(); }
    std::string getMapNameFromMap(const std::string&) override { std::abort(); }
    std::string getMapPathFromMap(const std::string&) override { std::abort(); }
    std::string getMapNameFromMap(const Mod*, const std::string&) override { std::abort(); }
    std::string getMapPathFromMap(const Mod*, const std::string&) override { std::abort(); }
    void addNameForGameMode(const std::string&, const std::string&) override { std::abort(); }
    std::string getNameFromGameMode(const std::string&) override { std::abort(); }
};
struct HostObserver : SwiffHostStandIn {
    ModObserver mod;
    IBase* getModInfo() override { record("modInfo"); return &mod; }
};
struct RegistryObserver : ClassManager {
    IBase* published = nullptr;
    unsigned queries = 0, absentAt = 0;
    std::string* swapAtLookup = nullptr;
    std::string replacement;
    IBase* getSingleton(const std::string& name) override
    {
        record("lookup:" + name);
        ++queries;
        if (swapAtLookup) {
            swapAtLookup->swap(replacement); // retains the earlier c_str allocation in replacement
            swapAtLookup = nullptr;
        }
        IBase* result = ClassManager::getSingleton(name);
        return absentAt == queries ? nullptr : result;
    }
};
struct EventsObserver : EventManager {
    unsigned registrations = 0;
    int category = -1, priority = -1;
    IEventListener* listener = nullptr;
    bool registerEventHandler(EventCategory cat, IEventListener* handler, int pri) override
    {
        ++registrations; category = cat; listener = handler; priority = pri;
        return EventManager::registerEventHandler(cat, handler, pri);
    }
};
struct EngineObserver : NullSoundEngine {
    bool loadResult = true, playResult = false, stopResult = false;
    unsigned playCalls = 0;
    std::string lastMusic;
    bool loadMusic(const char* path) override
    { lastMusic = path; record("coreLoad:" + lastMusic); return loadResult; }
    bool playMusic() override { ++playCalls; record("play"); return playResult; }
    bool stopMusic(float seconds) override
    { record(seconds == 0.0f ? "stop:0" : seconds == 5.0f ? "stop:5" : "stop:OTHER"); return stopResult; }
};
struct SoundObserver : BfSoundNull {
    EngineObserver engine0, engine1;
    bool loadResult = true;
    mutable unsigned engineQueries = 0;
    bool alternateEngines = false;
    std::string* mutateMusic = nullptr;
    std::string replacementMusic;
    ISoundEngine* engine() const override
    {
        record("engine");
        const bool second = alternateEngines && engineQueries != 0;
        if (mutateMusic && engineQueries == 0) *mutateMusic = replacementMusic;
        ++engineQueries;
        return const_cast<EngineObserver*>(second ? &engine1 : &engine0);
    }
    bool loadMusic(const std::string& path) override
    { record("facadeLoad:" + path); return loadResult; }
};
struct Fixture {
    EventsObserver events;
    RegistryObserver registry;
    HostObserver host;
    SoundObserver sound;
    std::unique_ptr<MusicPlayer> player;
    Fixture()
    {
        eventManager = &events; classManager = &registry; g_bfSound = &sound;
        setDedicated(false);
        registry.published = &host;
        SingletonRegInfo reg = {"SwiffHost", &registry.published, 0, 0, 4};
        registry.registerSingleton(reg);
        player.reset(new MusicPlayer);
        observedPlayer = player.get();
        clear();
    }
    ~Fixture()
    {
        events.unregisterEventHandler(static_cast<EventCategory>(10), player.get());
        player.reset(); observedPlayer = nullptr;
        registry.unregisterSingleton("SwiffHost");
        classManager = nullptr; eventManager = nullptr; g_bfSound = nullptr;
        setDedicated(false);
    }
    void clear() { calls.clear(); callbackStates.clear(); registry.queries = 0; sound.engineQueries = 0; }
    void seed(int wanted)
    {
        if (wanted == 1) player->setState(0, nullptr);
        if (wanted == 2) player->setState(2, "levels/oman/");
        check(state(*player) == wanted, "setup reaches requested state through recovered methods");
        clear();
    }
    void event(unsigned id, void* data = nullptr)
    { events.postEvent(static_cast<EventCategory>(10), id, data, 0.0f); }
};

static void gates()
{
    Fixture f;
    check(f.events.registrations == 1 && f.events.category == 10 && f.events.priority == 100
        && f.events.listener == f.player.get() && state(*f.player) == 0,
        "constructor registers actual listener once at menu priority100 and state0");
    f.event(1, reinterpret_cast<void*>(1));
    expect({}, "actual synchronous menu-enable (10,1) dispatch ignores opaque payload");
    for (unsigned id : {0u, 1u, 2u, 4u, 6u, 0xffffffffu}) f.event(id, reinterpret_cast<void*>(1));
    expect({}, "all non3/non5 menu IDs return without touching payload or host");
    for (int category : {-1, 0, 2, 9, 11, 99})
        f.player->handleEvent(static_cast<EventCategory>(category), 3, nullptr, reinterpret_cast<void*>(1));
    expect({}, "category gate precedes payload dereference");
    setDedicated(true);
    f.event(3, reinterpret_cast<void*>(1)); f.event(5, reinterpret_cast<void*>(1));
    expect({}, "fixture runtime dedicated flag gates both supported IDs");
    setDedicated(true);
    f.player->setState(0, nullptr);
    check(state(*f.player) == 1 && f.registry.queries == 2,
        "direct setState deliberately has no runtime dedicated gate");
    setDedicated(false);
    f.player->setState(1, nullptr); f.clear();
    f.registry.published = nullptr;
    for (int mode : {-1, 0, 1, 2, 3, 4}) f.player->setState(mode, nullptr);
    check(f.registry.queries == 6 && calls.size() == 6 && state(*f.player) == 0,
        "missing outer SwiffHost suppresses every mode before state or path use");
    for (int initial : {1, 2}) {
        Fixture active; active.seed(initial);
        active.event(1, reinterpret_cast<void*>(1));
        expect({}, "menu-enable also leaves active music alone");
        active.registry.published = nullptr;
        for (int mode : {-1, 0, 1, 2, 3, 4}) active.player->setState(mode, nullptr);
        check(active.registry.queries == 6 && calls.size() == 6 && state(*active.player) == initial,
            "missing outer host also preserves active state for every direct mode");
    }
}

static void menuBranches()
{
    for (int mode : {1, 3}) { Fixture f; f.player->setState(mode, nullptr);
        expect({"lookup:SwiffHost"}, "idle stop modes only query host"); check(state(*f.player) == 0, "idle stop preserves0"); }
    for (int failure : {0, 1, 2, 3}) {
        Fixture f;
        if (failure == 1) f.registry.absentAt = 2;
        if (failure == 2) f.host.mod.hasMod = false;
        if (failure == 3) f.sound.loadResult = false;
        f.player->setState(0, nullptr);
        if (!failure) {
            expect({"lookup:SwiffHost", "lookup:SwiffHost", "modInfo", "currentMod",
                "facadeLoad:menu/theme.ogg", "engine", "play"}, "menu loads through facade then core play");
            check(state(*f.player) == 1, "play false return still advances menu state1");
            check(callbackStates.back() == 0, "menu state store occurs after play callback");
        } else {
            check(state(*f.player) == 0, "failed menu helper preserves idle state0");
            if (failure == 1) expect({"lookup:SwiffHost", "lookup:SwiffHost"}, "second host lookup is independently guarded");
            if (failure == 2) expect({"lookup:SwiffHost", "lookup:SwiffHost", "modInfo", "currentMod"}, "missing current mod returns");
            if (failure == 3) expect({"lookup:SwiffHost", "lookup:SwiffHost", "modInfo", "currentMod", "facadeLoad:menu/theme.ogg"}, "failed facade music load does not play");
        }
    }
    for (int initial : {1, 2}) for (int mode : {1, 3}) for (bool stopResult : {false, true}) {
        Fixture f; f.seed(initial); f.sound.engine0.stopResult = stopResult;
        f.player->setState(mode, nullptr);
        expect({"lookup:SwiffHost", "engine", "stop:5"}, "active direct stop uses five-second fade");
        check(state(*f.player) == 0 && callbackStates.back() == initial, "stop result ignored and state0 written after callback");
    }
    for (int failure : {0, 1, 2, 3}) {
        Fixture f; f.seed(2);
        if (failure == 1) f.registry.absentAt = 2;
        if (failure == 2) f.host.mod.hasMod = false;
        if (failure == 3) f.sound.loadResult = false;
        f.player->setState(0, nullptr);
        check(state(*f.player) == 1, "map-to-menu preassigns1 even if replacement cannot load");
        check(calls.size() >= 4 && calls[1] == "engine" && calls[2] == "stop:0"
            && callbackStates[2] == 2 && calls[3] == "lookup:SwiffHost" && callbackStates[3] == 1,
            "map-to-menu stops immediately then preassigns1 before helper lookup");
    }
}

static void mapBranches()
{
    for (int initial : {0, 1, 2}) for (bool viaEvent : {false, true}) {
        Fixture f; f.seed(initial); std::string path = "levels/oman/";
        if (viaEvent) f.event(3, &path); else f.player->setState(2, path.c_str());
        if (!initial) expect({"lookup:SwiffHost", "lookup:SwiffHost", "modInfo", "map:oman", "engine", "coreLoad:level/theme.ogg", "engine", "play"}, "idle map start uses core load and repeated engine query");
        else expect({"lookup:SwiffHost", "engine", "stop:0", "lookup:SwiffHost", "modInfo", "map:oman", "engine", "coreLoad:level/theme.ogg", "engine", "play"}, "active map start stops immediately before loading");
        check(state(*f.player) == 2 && callbackStates.back() == initial, "map play false still stores2 after callback");
    }
    for (int initial : {0, 1, 2}) for (int failure : {1, 2, 3}) {
        Fixture f; f.seed(initial);
        if (failure == 1) f.registry.absentAt = 2;
        if (failure == 2) f.host.mod.hasMap = false;
        if (failure == 3) f.sound.engine0.loadResult = false;
        std::string path = "levels/oman/"; f.event(3, &path);
        check(state(*f.player) == initial, "map load failures preserve preceding state even after stop");
        check(std::find(calls.begin(), calls.end(), "play") == calls.end(), "failed map helper never plays");
        if (failure == 1) check(calls.back() == "lookup:SwiffHost", "missing second map host returns before map lookup");
        if (failure == 2) check(calls.back() == "map:oman", "missing map returns before sound query");
        if (failure == 3) check(calls.back() == "coreLoad:level/theme.ogg", "failed core load returns before second engine query");
    }
    struct PathCase { const char* input; const char* result; };
    for (const auto& item : {PathCase{"", ""}, {"short", ""}, {"1234567", ""}, {"1234567x", ""},
        {"levels/oman", "oma"}, {"abcdefgOTHER!", "OTHER"}, {"levels/Case Mixed/", "Case Mixed"}}) {
        Fixture f; f.player->setState(2, item.input);
        check(f.host.mod.requestedMap == item.result, "map path unconditionally erases first7 and remaining final character");
    }
    {
        Fixture f; f.sound.alternateEngines = true; f.sound.engine1.playResult = false;
        f.host.mod.map.music = std::string("prefix\0suffix", 13);
        f.player->setState(2, "levels/oman/");
        check(f.sound.engine0.lastMusic == "prefix" && f.sound.engine1.lastMusic.empty()
            && f.sound.engine0.playCalls == 0 && f.sound.engine1.playCalls == 1
            && f.sound.engineQueries == 2 && state(*f.player) == 2, "map music uses c_str and refreshes engine for playback");
    }
    for (bool shortToHeap : {false, true}) {
        Fixture f;
        f.host.mod.map.music = shortToHeap ? "short.ogg" : std::string(64, 'a') + ".ogg";
        f.sound.replacementMusic = shortToHeap ? std::string(80, 'b') + ".ogg" : "new.ogg";
        f.sound.mutateMusic = &f.host.mod.map.music;
        f.player->setState(2, "levels/oman/");
        check(f.sound.engine0.lastMusic == f.sound.replacementMusic,
            "engine query precedes map music c_str across short/heap storage changes");
    }
    {
        Fixture f;
        std::string payload = "levels/original_long_heap_map_name/";
        f.registry.replacement = "levels/replacement_long_heap_map_name/";
        f.registry.swapAtLookup = &payload;
        f.event(3, &payload);
        check(f.host.mod.requestedMap == "original_long_heap_map_name", "event3 materializes payload c_str before first host lookup");
        check(payload == "levels/replacement_long_heap_map_name/", "lookup witness swapped the payload while retaining old storage");
    }
    for (int initial : {0, 1, 2}) {
        Fixture f; f.seed(initial); f.event(5);
        if (initial) expect({"lookup:SwiffHost", "engine", "stop:5"}, "event5 fades active music for five seconds");
        else expect({"lookup:SwiffHost"}, "event5 in idle queries host then returns");
        check(state(*f.player) == 0, "event5 reaches idle");
    }
    for (int initial : {1, 2}) {
        Fixture f; f.seed(initial); f.registry.published = nullptr; f.event(5);
        expect({"lookup:SwiffHost"}, "event5 missing host leaves sound untouched");
        check(state(*f.player) == initial, "event5 missing host preserves state");
    }
    for (int initial : {0, 1, 2}) {
        Fixture f; f.seed(initial); f.registry.published = nullptr;
        std::string path = "levels/oman/"; f.event(3, &path);
        expect({"lookup:SwiffHost"}, "event3 missing outer host leaves sound and map lookup alone");
        check(state(*f.player) == initial, "event3 missing outer host preserves prior state");
    }
}

static void recoveredNullSound()
{
    Fixture f;
    BfSoundNull nullFacade;
    g_bfSound = &nullFacade;
    check(nullFacade.engine() == g_nullSoundEngine && nullFacade.loadMusic("unused.ogg")
        && nullFacade.engine()->loadMusic("unused.ogg") && nullFacade.engine()->playMusic()
        && !nullFacade.engine()->stopMusic(5.0f), "actual recovered null-sound return values");
    f.player->setState(0, nullptr);
    check(state(*f.player) == 1, "menu starts through actual recovered null facade/core");
    std::string path = "levels/oman/"; f.event(3, &path);
    check(state(*f.player) == 2, "event3 transitions through actual null stop/load/play");
    f.event(5);
    check(state(*f.player) == 0, "event5 ignores actual null stop false and reaches idle");
    g_bfSound = &f.sound;
}

static int expectedErrorMode, expectedErrorState;
static void diagnosticObserver(DebugType type, const std::string&, const std::string&, int line,
    const std::string& function, const std::string& message, const std::string& file, void*)
{
    const std::string wanted = "Event " + std::to_string(expectedErrorMode)
        + " is not a valid event in state " + std::to_string(expectedErrorState);
    if (type != DebugTypeError || line != 155 || !function.empty() || message != wanted
        || file.find("Sound\\BfSound\\MusicPlayer.cpp") == std::string::npos) std::_Exit(42);
    std::printf("EXPECTED_DIAGNOSTIC %s\n", message.c_str()); std::fflush(stdout);
    // Return into the real Debug::~Debug; its fatal path must terminate with abort exit3.
}
static int fatalCase(const char* name)
{
    Fixture f;
    if (!std::strcmp(name, "state1-mode0")) { f.seed(1); expectedErrorMode = 0; expectedErrorState = 1; }
    else if (!std::strcmp(name, "idle-mode4")) { expectedErrorMode = 4; expectedErrorState = 0; }
    else if (!std::strcmp(name, "idle-mode-minus1")) { expectedErrorMode = -1; expectedErrorState = 0; }
    else { corruptState(*f.player, 99); expectedErrorState = 99; expectedErrorMode = !std::strcmp(name, "corrupt-event3") ? 2 : 3; }
    setDebugCallback(DebugTypeError, &diagnosticObserver, nullptr);
    if (!std::strcmp(name, "corrupt-event3")) { std::string path = "levels/oman/"; f.event(3, &path); }
    else if (!std::strcmp(name, "corrupt-event5")) f.event(5);
    else f.player->setState(expectedErrorMode, nullptr);
    return 43; // An unexpectedly suppressed diagnostic must fail the runner.
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (argc == 2) return fatalCase(argv[1]);
    gates(); menuBranches(); mapBranches(); recoveredNullSound();
    std::printf("MusicPlayer runtime: %u/%u passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
