// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: SERVER-SETTINGS-SAVE-FIXTURE]
// The recovered ServerSettings::save pair (ELF 0x80BC520 / 0x80BC940, PE 0x426A40 / 0x425E80).
// The real class is linked and constructed by its own dynamic initialiser; the only collaborator
// this fixture supplies is a FileManager observer, and every other engine global stays null and
// unreached (the link forces them, see the .cmd).
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "../BF2/Game/GameServer/ServerSettings.h"
#include "../BF2/IO/File/FileManager.h"
#include "../BF2/IO/Stream/IStream.h"

using namespace dice::hfe;
static unsigned checks, failures;
static void check(bool value, const char* label)
{ ++checks; if (!value) { ++failures; std::printf("FAIL %s\n", label); } }

namespace dice { namespace hfe {
// [TEMP] The engine globals ServerSettings.cpp names. save() reaches only io::fileManager; the
// constructor's setMaxPlayers(16) reads `game` and `settings` and takes the null arm of both.
// Pointers to incomplete types are enough, and they mangle to the same symbols the class expects.
class IGame; class IGameLogic; class SettingsRepostitory;
namespace io { class OldConsole; }
IGame*                game = nullptr;
IGameLogic*           gameLogic = nullptr;
SettingsRepostitory*  settings = nullptr;
namespace io { FileManager* fileManager = nullptr; OldConsole* mainConsole = nullptr; }

// [TEMP] The write end of one save(): it records the buffer it was handed, answers a configurable
// count and counts its own release. Every other IStream slot aborts, so a wrong offset cannot pass
// quietly.
struct SaveStream : io::IStream
{
    std::string  written;
    unsigned int writes = 0, releases = 0, answer = 0xFFFFFFFFu;   // 0xFFFFFFFF: answer the size
    void addRef() override { std::abort(); }
    int  getRef() override { std::abort(); }
    int  release() override { ++releases; return 0; }
    void* queryInterface(unsigned int) const override { std::abort(); }
    unsigned int read(void*, unsigned int) override { std::abort(); }
    unsigned int write(const void* buffer, unsigned int size) override
    {
        ++writes;
        written.assign(static_cast<const char*>(buffer), size);
        return answer == 0xFFFFFFFFu ? size : answer;
    }
    int skip(unsigned int) override { std::abort(); }
    int getAvailableBytes() override { std::abort(); }
    io::IStream* clone() override { std::abort(); }
    bool canRead() override { std::abort(); }
    bool canWrite() override { std::abort(); }
};

// [TEMP] The filesystem observer: it records the open() arguments and hands back the stream it was
// given, or null to model a refused open.
struct Files : io::FileManager
{
    SaveStream*  stream = nullptr;
    unsigned     opens = 0;
    std::string  lastName;
    int          lastMode = -1, lastAccess = -1;
    unsigned int lastFlags = 0xFFFFFFFFu;
    io::IStream* open(const std::string& name, io::FileMode mode, io::FileAccess access,
                      unsigned int flags) override
    {
        ++opens; lastName = name; lastMode = mode; lastAccess = access; lastFlags = flags;
        return stream;
    }
};
} }

// The constructor's defaults, written out by save(std::ostream&) - the whole expected script, in
// the shipped row order. Every value here is ServerSettings()'s own default, so this doubles as a
// check that neither the writer nor the constructor drifts.
static const char* const kDefaultScript =
    "sv.serverName \"\"\n"
    "sv.password \"\"\n"
    "sv.internet 0\n"
    "sv.bandwidthChoke 0\n"
    "sv.serverIP \"\"\n"
    "sv.serverPort 16567\n"
    "sv.welcomeMessage \"\"\n"
    "sv.punkBuster 0\n"
    "sv.allowFreeCam 1\n"
    "sv.allowExternalViews 1\n"
    "sv.allowNoseCam 1\n"
    "sv.hitIndicator 1\n"
    "sv.maxPlayers 16\n"
    "sv.numPlayersNeededToStart 2\n"
    "sv.notEnoughPlayersRestartDelay 5\n"
    "sv.startDelay 2\n"
    "sv.endDelay 10\n"
    "sv.spawnTime 5\n"
    "sv.manDownTime 10\n"
    "sv.endOfRoundDelay 15\n"
    "sv.ticketRatio 100\n"
    "sv.roundsPerMap 3\n"
    "sv.timeLimit 0\n"
    "sv.scoreLimit 0\n"
    "sv.soldierFriendlyFire 100\n"
    "sv.vehicleFriendlyFire 100\n"
    "sv.soldierSplashFriendlyFire 100\n"
    "sv.vehicleSplashFriendlyFire 100\n"
    "sv.tkPunishEnabled 0\n"
    "sv.tkNumPunishToKick 3\n"
    "sv.tkPunishByDefault 1\n"
    "sv.votingEnabled 1\n"
    "sv.voteTime 60\n"
    "sv.minPlayersForVoting 2\n"
    "sv.teamVoteOnly 1\n"
    "sv.gameSpyPort 29900\n"
    "sv.allowNATNegotiation 0\n"
    "sv.interfaceIP \n"
    "sv.autoRecord 0\n"
    "sv.demoIndexURL \"\"\n"
    "sv.demoDownloadURL \"\"\n"
    "sv.autoDemoHook \"\"\n"
    "sv.demoQuality 1\n"
    "sv.adminScript \"\"\n"
    "sv.timeBeforeRestartMap 30\n"
    "sv.autoBalanceTeam 0\n"
    "sv.teamRatioPercent 100\n"
    "sv.coopBotRatio 50\n"
    "sv.coopBotCount 16\n"
    "sv.coopBotDifficulty 50\n"
    "sv.voipEnabled 1\n"
    "sv.voipQuality 3\n"
    "sv.voipServerRemote 0\n"
    "sv.voipServerRemoteIP \n"
    "sv.voipServerPort 55125\n"
    "sv.voipBFClientPort 55123\n"
    "sv.voipBFServerPort 55124\n"
    "sv.voipSharedPassword \n"
    "sv.useGlobalRank 1\n"
    "sv.useGlobalUnlocks 1\n"
    "sv.noVehicles 0\n"
    "sv.sponsorText \"\"\n"
    "sv.sponsorLogoURL \"\"\n"
    "sv.communityLogoURL \"\"\n"
    "sv.radioSpamInterval 6\n"
    "sv.radioMaxSpamFlagCount 6\n"
    "sv.radioBlockedDurationTime 30\n"
    "sv.numReservedSlots 0\n"
    "sv.friendlyFireWithMines 0\n";

static std::string writeScript(ServerSettings& s)
{
    std::ostringstream out;
    s.save(out);
    return out.str();
}

static unsigned countLines(const std::string& text)
{
    unsigned n = 0;
    for (std::string::size_type i = 0; i < text.size(); ++i) if (text[i] == '\n') ++n;
    return n;
}

static bool hasLine(const std::string& text, const char* line)
{
    const std::string needle = std::string("\n") + line + "\n";
    return text.compare(0, std::strlen(line) + 1, std::string(line) + "\n") == 0
        || text.find(needle) != std::string::npos;
}

int main()
{
    ServerSettings& s = *serverSettings;   // the TU's own dynamic initialiser built it
    check(&s != nullptr, "the class's own dynamic initialiser published the singleton");

    // save(std::ostream&): the whole default script, row for row
    const std::string defaults = writeScript(s);
    check(defaults == kDefaultScript, "the default settings script is written row for row");
    if (defaults != kDefaultScript) {
        // Show the first differing line, which is what a drift here needs.
        std::string::size_type i = 0;
        while (i < defaults.size() && i < std::strlen(kDefaultScript) && defaults[i] == kDefaultScript[i]) ++i;
        const std::string::size_type from = defaults.rfind('\n', i == 0 ? 0 : i - 1);
        std::printf("  first difference near: %s\n",
                    defaults.substr(from == std::string::npos ? 0 : from + 1, 60).c_str());
    }
    check(countLines(defaults) == 69, "an unranked server writes sixty-nine rows");

    // sv.ranked is the only conditional row
    check(defaults.find("sv.ranked") == std::string::npos,
          "an unranked server writes no sv.ranked row at all");
    s.m_ranked = true;
    const std::string ranked = writeScript(s);
    check(countLines(ranked) == 70 && hasLine(ranked, "sv.ranked 1"),
          "a ranked server writes the row, between useGlobalUnlocks and noVehicles");
    check(ranked.find("sv.useGlobalUnlocks 1\nsv.ranked 1\nsv.noVehicles 0\n") != std::string::npos,
          "and writes it in the shipped position");
    s.m_ranked = false;

    // quoting: the three unquoted string rows, and one quoted one
    s.m_serverName = "a server with spaces";
    s.m_interfaceIP = "1.2.3.4";
    s.m_voipServerRemoteIP = "5.6.7.8";
    s.m_voipSharedPassword = "shared secret";
    s.m_serverIP = "9.8.7.6";
    const std::string strings = writeScript(s);
    check(hasLine(strings, "sv.serverName \"a server with spaces\""), "quoted rows keep their quotes");
    check(hasLine(strings, "sv.serverIP \"9.8.7.6\""), "serverIP is quoted too, string literal and all");
    check(hasLine(strings, "sv.interfaceIP 1.2.3.4"), "interfaceIP is written WITHOUT quotes");
    check(hasLine(strings, "sv.voipServerRemoteIP 5.6.7.8"), "voipServerRemoteIP is written without quotes");
    check(hasLine(strings, "sv.voipSharedPassword shared secret"),
          "voipSharedPassword is written without quotes, so a space in it does not round-trip");

    // number formatting: the flags print 0/1, the floats use the default precision
    s.m_allowFreeCam = true; s.m_hitIndicator = false; s.m_autoRecord = true;
    s.m_spawnTime = 2.5f; s.m_coopBotDifficulty = 33.75f; s.m_teamRatioPercent = 66.6f;
    s.m_maxPlayers = 64; s.m_bandwidthChoke = -1;
    const std::string numbers = writeScript(s);
    check(hasLine(numbers, "sv.allowFreeCam 1") && hasLine(numbers, "sv.hitIndicator 0")
          && hasLine(numbers, "sv.autoRecord 1"),
          "the five int-cast flags still print as 0/1");
    check(hasLine(numbers, "sv.spawnTime 2.5") && hasLine(numbers, "sv.coopBotDifficulty 33.75"),
          "floats print at the stream's default precision");
    check(hasLine(numbers, "sv.teamRatioPercent 66.6"), "and six significant digits are enough for 66.6f");
    check(hasLine(numbers, "sv.maxPlayers 64") && hasLine(numbers, "sv.bandwidthChoke -1"),
          "ints print as themselves, negative included");

    // save(): the file name gate
    Files files; io::fileManager = &files;
    SaveStream stream; files.stream = &stream;
    s.m_configFile = "";
    check(!s.save() && files.opens == 0, "an empty config file name refuses before opening anything");

    // save(): the whole script in one write, then the release
    s.m_configFile = "mods/bf2/settings/serversettings.con";
    check(s.save(), "a configured name and an accepting stream answer true");
    check(files.opens == 1 && files.lastName == "mods/bf2/settings/serversettings.con",
          "the configured name is what is opened");
    check(files.lastMode == 0 && files.lastAccess == 2 && files.lastFlags == 0,
          "the open uses FileMode(0), FileAccess(2) and no flags");
    check(stream.writes == 1 && stream.written == writeScript(s),
          "the script reaches the stream in ONE write and matches the ostream overload byte for byte");
    check(stream.releases == 1, "the stream is released once");

    // save(): a refused open, and a short write
    files.stream = nullptr;
    const unsigned before = stream.writes;
    check(!s.save() && stream.writes == before, "a refused open answers false and writes nothing");
    files.stream = &stream;
    stream.answer = 1;
    check(!s.save() && stream.releases == 2,
          "a short write answers false, and the stream is still released");
    stream.answer = 0xFFFFFFFFu;

    io::fileManager = nullptr;
    std::printf("ServerSettings save: %u/%u passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
