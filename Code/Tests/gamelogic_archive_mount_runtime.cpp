// [TEMP] [CHOSEN, not recovered] Client-image regression for the recovered
// GameLogic::load archive-selection prefix. This invokes the production body and
// observes only its typed FileManager/Console calls. It deliberately stops at the
// first map-size content probe; it does not parse a map, start networking, or claim
// that a level loaded.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdio>
#include <cstdlib>
#include <list>
#include <string>
#include <vector>

#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Core/System.h"
#include "../BF2/Game/GameClient/GameClient.h"
#include "../BF2/Game/GameLogic/GameLogic.h"
#include "../BF2/Game/GameLogic/TicketHistory.h"
#include "../BF2/IO/Console/Console.h"
#include "../BF2/IO/File/FileManager.h"

using namespace dice::hfe;
using namespace dice::hfe::world;

namespace dice { namespace hfe {
extern ClassManager* classManager;
} }

namespace {

int checks = 0;
int failures = 0;

void check(bool ok, const char* label)
{
    ++checks;
    if (!ok)
        ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    std::fflush(stdout);
}

void dumpTrace(const std::vector<std::string>& trace)
{
    for (std::size_t i = 0; i < trace.size(); ++i)
        std::printf("  [%u] %s\n", static_cast<unsigned int>(i), trace[i].c_str());
    std::fflush(stdout);
}

struct MapProbeReached {};

// [TEMP] The real load body dispatches through this observer. Returning a null archive is
// faithful to this bounded call witness because GameLogic::load discards mountArchive's result;
// no archive object is ever used. Any attempt to continue into map discovery throws visibly.
struct Files : io::FileManager
{
    std::vector<std::string> trace;

    bool unmountArchive(const std::string& archive) override
    {
        trace.push_back(std::string("unmount:") + archive);
        return true;
    }

    void permitMountArchives(bool permit) override
    {
        trace.push_back(std::string("permit:") + (permit ? "1" : "0"));
    }

    io::IArchive* mountArchive(const std::string& archive,
                               const std::string& mountPath,
                               bool contentCheck) override
    {
        trace.push_back(std::string("mount:") + archive + "|" + mountPath + "|"
                        + (contentCheck ? "1" : "0"));
        return 0;
    }

    bool fileExists(const std::string& path) override
    {
        trace.push_back(std::string("probe:") + path);
        throw MapProbeReached();
    }
};

// [TEMP] GameLogic must use the shipped privileged runner for both archive scripts. The
// arguments are observed as well so the fixture cannot accidentally accept a different call.
struct Console : io::OldConsole
{
    explicit Console(Files* files) : m_files(files) {}

    bool runFullAccess(const std::string& file,
                       const std::string& a1, const std::string& a2,
                       const std::string& a3, const std::string& a4,
                       const std::string& a5, const std::string& a6,
                       const std::string& a7, const std::string& a8,
                       std::string& out) override
    {
        if (!a1.empty() || !a2.empty() || !a3.empty() || !a4.empty()
            || !a5.empty() || !a6.empty() || !a7.empty() || !a8.empty()
            || !out.empty())
        {
            std::fputs("Unexpected non-empty GameLogic archive-script argument\n", stderr);
            std::fflush(stderr);
            std::abort();
        }
        m_files->trace.push_back(std::string("run:") + file);
        return true;
    }

private:
    Files* m_files;
};

// Use the recovered concrete client Game rather than fabricating the many client-policy slots
// that intentionally remain pure on the shared Game base. Only its inherited MapInfo is seeded.
struct FixtureGame : GameClient
{
    FixtureGame()
    {
        m_mapInfo.setGameMode("gpm_cq");
        m_mapInfo.setMaxNumberOfPlayers(64);
    }
};

// This is the established fail-loud GameLogic fixture shape used by the focused runtime tests.
// Only isServer/getKitTemplate are needed to make the shared recovered base concrete; every
// unrelated concrete-policy virtual aborts if the bounded prefix unexpectedly reaches it.
struct Logic : GameLogic
{
    [[noreturn]] static void unused()
    {
        std::fputs("Unexpected fixture GameLogic operation\n", stderr);
        std::fflush(stderr);
        std::abort();
    }

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

    bool isServer() const override { return false; }
    IObjectTemplate* getKitTemplate(int, int) override { return 0; }
};

void expectTrace(const std::vector<std::string>& actual,
                 const std::vector<std::string>& expected,
                 const char* label)
{
    const bool matches = actual == expected;
    check(matches, label);
    if (!matches)
    {
        std::puts("Expected:");
        dumpTrace(expected);
        std::puts("Actual:");
        dumpTrace(actual);
    }
}

void runLoadPrefix(Logic* logic, Files* files, const std::string& levelPath,
                   const std::vector<std::string>& expected, const char* label)
{
    files->trace.clear();
    bool stoppedAtProbe = false;
    try
    {
        logic->load(levelPath);
    }
    catch (const MapProbeReached&)
    {
        stoppedAtProbe = true;
    }
    catch (...)
    {
        check(false, "archive prefix raised an unexpected C++ exception");
    }

    check(stoppedAtProbe, "real load body reached the first map-size probe");
    expectTrace(files->trace, expected, label);
}

} // anonymous namespace

int main()
{
    static_assert(sizeof(void*) == 4, "x86 fixture only");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _CALL_REPORTFAULT);

    // These bounded fixtures intentionally live until process exit. Their production destructors
    // cover a much wider engine shutdown path than this regression claims to exercise.
    g_system = new System;
    settings = new SettingsRepostitory;
    classManager = new ClassManager;
    eventManager = new EventManager;

    Files* files = new Files;
    Console* console = new Console(files);
    io::fileManager = files;
    io::mainConsole = console;

    FixtureGame* fixtureGame = new FixtureGame;
    game = fixtureGame;
    Logic* logic = new Logic;
    gameLogic = logic;

    setDedicated(false);
    runLoadPrefix(logic, files, "Levels/dalian_plant/",
        {
            "unmount:*levels",
            "unmount:Booster_server.zip",
            "unmount:Booster_client.zip",
            "permit:1",
            "mount:Levels/dalian_plant/server.zip|Levels/dalian_plant|1",
            "run:Levels/dalian_plant/ServerArchives.con",
            "mount:Levels/dalian_plant/client.zip|Levels/dalian_plant|1",
            "run:Levels/dalian_plant/ClientArchives.con",
            "permit:0",
            "probe:Levels/dalian_plant/GameModes/gpm_cq/16/GamePlayObjects.con"
        },
        "client non-booster load mounts and runs both level archive halves in retail order");

    runLoadPrefix(logic, files, "Levels/Midnight_Sun/",
        {
            "unmount:*levels",
            "permit:1",
            "mount:Booster_server.zip|Objects|1",
            "mount:Booster_client.zip|Objects|1",
            "permit:0",
            "permit:1",
            "mount:Levels/Midnight_Sun/server.zip|Levels/Midnight_Sun|1",
            "run:Levels/Midnight_Sun/ServerArchives.con",
            "mount:Levels/Midnight_Sun/client.zip|Levels/Midnight_Sun|1",
            "run:Levels/Midnight_Sun/ClientArchives.con",
            "permit:0",
            "probe:Levels/Midnight_Sun/GameModes/gpm_cq/16/GamePlayObjects.con"
        },
        "client booster load mounts server and client booster and level archives in retail order");

    // BF2.exe keeps this runtime branch for its shipped +dedicated mode. The server-only control
    // proves the new client calls did not leak out of the PE's `if (!isDedicated())` arm.
    setDedicated(true);
    runLoadPrefix(logic, files, "Levels/Midnight_Sun/",
        {
            "unmount:*levels",
            "permit:1",
            "mount:Booster_server.zip|Objects|1",
            "permit:0",
            "permit:1",
            "mount:Levels/Midnight_Sun/server.zip|Levels/Midnight_Sun|1",
            "run:Levels/Midnight_Sun/ServerArchives.con",
            "permit:0",
            "probe:Levels/Midnight_Sun/GameModes/gpm_cq/16/GamePlayObjects.con"
        },
        "client executable dedicated mode keeps the server-only archive sequence");
    setDedicated(false);

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
