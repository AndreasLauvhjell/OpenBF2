// Scaffolding, not recovered source. Demonstrates the status-monitor interface with a
// text-grid backend by default or the Win32 backend with --live. Width precedes height;
// output coordinates are x then y. The engine's StatusMonitor::update remains authoritative.
// Build: Code\Build\demo\statusscreen.cmd
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "StatusMonitorImpl.h"

using namespace dice::hfe;

// The demo has no console singleton. The Win32 backend guards this null pointer before
// forwarding queued keystrokes.
namespace dice { namespace hfe { namespace io { class OldConsole; OldConsole* mainConsole = 0; } } }

// Text-grid backend implementing the status-monitor interface.
class StatusMonitorImpl_text : public StatusMonitorImpl
{
public:
    StatusMonitorImpl_text(int w, int h) : m_w(w), m_h(h), m_cells((size_t)w * h, ' ') {}

    virtual bool shouldUpdate(bool) { return true; }
    virtual void clearScreen()      { m_cells.assign(m_cells.size(), ' '); }

    virtual void getDimensions(int& width, int& height) { width = m_w; height = m_h; }

    virtual void outputString(int x, int y, const std::string& text)
    {
        // Off-screen writes are dropped whole, not clamped - that is what wmove()==ERR does.
        if (x < 0 || y < 0 || x >= m_w || y >= m_h) return;
        size_t at = (size_t)y * m_w + x, rowEnd = (size_t)(y + 1) * m_w;
        for (size_t i = 0; i < text.size() && at < rowEnd; ++i, ++at) m_cells[at] = text[i];
    }

    virtual void flushToScreen()
    {
        printf("    +%.*s+\n", m_w, std::string(m_w, '-').c_str());
        for (int y = 0; y < m_h; ++y)
            printf("    |%.*s|\n", m_w, &m_cells[(size_t)y * m_w]);
        printf("    +%.*s+\n", m_w, std::string(m_w, '-').c_str());
    }

private:
    int m_w, m_h;
    std::vector<char> m_cells;
};

// Fixture snapshot of values read by StatusMonitor::update.
struct Snapshot
{
    std::string version, serverName, ip, mapName, gameMode, modDir;
    int   port, mapSize, players, maxPlayers, reserved, connecting, round, roundsPerMap;
    int   timeLeftSeconds, dropped, overshot, gameStatus;
    float fps;
    bool  ranked, lan, punkbuster, noVehicles;
    std::vector<std::string> consoleLines;
};

static std::string getTimeString(int seconds)          // free function, ELF-attested
{
    char buf[32];
    sprintf(buf, "%d:%02d", seconds / 60, seconds % 60);
    return buf;
}

static const char* statusText(int gameStatus)          // GameLogic::getGameStatus
{
    switch (gameStatus)
    {
    case 1: return "[playing]";      case 2: return "[endgame]";
    case 3: return "[pregame]";      case 4: return "[paused]";
    case 5: return "[restart]";      case 6: return "[disconnected]";
    default: return "[unknown]";
    }
}

// Mirrors StatusMonitor::update's normal (non-net-graph) path. Columns 0/41/61 are the literal
// immediates from the ELF.
static void draw(StatusMonitorImpl* impl, const Snapshot& s)
{
    if (!impl->shouldUpdate(false))
        return;

    int width = 0, height = 0;
    impl->getDimensions(width, height);        // width is fetched and never used, as in the ELF

    std::string title = "Battlefield 2 Dedicated Server " + s.version;
    if (s.ranked)
        title.append(" - Official rankings enabled");

    impl->clearScreen();
    impl->outputString(0, 0, title);

    std::ostringstream o;

    o.str(""); o << '"' << s.serverName << '"';
    impl->outputString(0, 1, o.str());

    o.str(""); o << "Average FPS:" << s.fps << " [d:" << s.dropped << ", o:" << s.overshot << "]";
    impl->outputString(41, 1, o.str());

    o.str(""); o << "IP: " << s.ip << " Port: " << s.port << ' ';
    if (s.lan)        o << "(LAN)";
    if (s.punkbuster) o << " (PB)";
    impl->outputString(0, 2, o.str());

    impl->outputString(41, 2, "Map: " + s.mapName);

    o.str(""); o << "Game mode: " << s.gameMode << "/" << s.mapSize;
    if (s.noVehicles) o << " (No Vehicles)";
    impl->outputString(0, 3, o.str());

    impl->outputString(41, 3, "Mod: " + s.modDir);
    impl->outputString(61, 3, "TimeLeft: " + getTimeString(s.timeLeftSeconds));

    o.str(""); o << "Players: " << s.players << "/" << s.maxPlayers
                 << " (" << s.reserved << " r)" << " (" << s.connecting << " connecting)";
    impl->outputString(0, 4, o.str());

    o.str("");
    if (s.round < 0) o << "Round: N/A";
    else             o << "Round: " << s.round << "/" << s.roundsPerMap;
    impl->outputString(41, 4, o.str());

    impl->outputString(61, 4, std::string("Status: ") + statusText(s.gameStatus));

    const int nextRow  = 4;
    const int sepRow   = nextRow + 1;
    impl->outputString(0, sepRow, std::string(79, '-'));   // the 79-dash string at 0x8AACC4C

    // Bottom-aligned: the newest line always lands on the last row of the terminal.
    const int firstRow = sepRow + 1;
    const int avail    = height - firstRow;
    int n = (int)s.consoleLines.size();
    if (n > avail) n = avail;
    for (int i = 0; i < n; ++i)
        impl->outputString(0, height - n + i, s.consoleLines[(size_t)s.consoleLines.size() - n + i]);

    impl->flushToScreen();
}

int main(int argc, char** argv)
{
    const bool live = (argc > 1 && strcmp(argv[1], "--live") == 0);

    Snapshot s;
    s.version      = "v1.5.3153-802.0(IA-32)";
    s.serverName   = "BF2 Rebuilt - recovered engine";
    s.ip           = "192.168.1.40";
    s.port         = 16567;
    s.mapName      = "strike_at_karkand";
    s.gameMode     = "gpm_cq";
    s.modDir       = "bf2";                 // GSModDirectory "mods/bf2", lowercased, prefix erased
    s.mapSize      = 64;
    s.players      = 23;
    s.maxPlayers   = 64;
    s.reserved     = 4;
    s.connecting   = 2;
    s.round        = 1;
    s.roundsPerMap = 3;
    s.timeLeftSeconds = 17 * 60 + 42;
    s.dropped      = 0;
    s.overshot     = 3;
    s.gameStatus   = 1;                     // [playing]
    s.fps          = 29.94f;
    s.ranked       = true;
    s.lan          = false;
    s.punkbuster   = true;
    s.noVehicles   = false;

    s.consoleLines.push_back("Loading level strike_at_karkand");
    s.consoleLines.push_back("Python: bf2.gameLogic initialised");
    s.consoleLines.push_back("Level loaded in 4.21 seconds");
    s.consoleLines.push_back("Player 'Ripley' connected from 10.0.0.7");
    s.consoleLines.push_back("Round started");

    if (live)
    {
        printf("live mode: painting through StatusMonitorImpl_win32...\n");
        StatusMonitorImpl* impl = StatusMonitorImpl::createInstance();
        draw(impl, s);
        delete impl;
        return 0;
    }

#if defined(_WIN32)
    // --scrolltest scrolls the console, paints, and checks that the title appears at the
    // visible window origin rather than at buffer row zero.
    if (argc > 1 && strcmp(argv[1], "--scrolltest") == 0)
    {
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO before;
        if (out == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(out, &before))
        {
            printf("scrolltest: SKIPPED - no console (stdout is redirected)\n");
            return 0;
        }

        // Create scrollback first so the window can move within the buffer. Without it, buffer
        // and window origins coincide and this test cannot detect an origin error.
        {
            COORD grow;
            grow.X = before.dwSize.X;
            grow.Y = (SHORT)(before.srWindow.Bottom - before.srWindow.Top + 1 + 200);
            SetConsoleScreenBufferSize(out, grow);
        }

        printf("scrolltest: scrolling the console so the window leaves the top of the buffer...\n");
        for (int i = 0; i < 120; ++i)
            printf("scrolltest: filler line %d\n", i);
        fflush(stdout);

        CONSOLE_SCREEN_BUFFER_INFO mid;
        GetConsoleScreenBufferInfo(out, &mid);
        printf("scrolltest: window top is now buffer row %d\n", (int)mid.srWindow.Top);
        fflush(stdout);

        StatusMonitorImpl* impl = StatusMonitorImpl::createInstance();
        draw(impl, s);

        // Read back the first visible ROW and look for the title.
        CONSOLE_SCREEN_BUFFER_INFO after;
        GetConsoleScreenBufferInfo(out, &after);
        const int w = after.srWindow.Right - after.srWindow.Left + 1;
        std::vector<char> row((size_t)w + 1, 0);
        COORD at;
        at.X = 0;
        at.Y = after.srWindow.Top;
        DWORD got = 0;
        ReadConsoleOutputCharacterA(out, &row[0], (DWORD)w, at, &got);
        delete impl;

        std::string firstVisible(&row[0], (size_t)got);
        const bool ok = firstVisible.find("Battlefield 2 Dedicated Server") != std::string::npos;

        // Also read buffer row 0 - if the title is there and not in the window, the origin is wrong.
        std::vector<char> row0((size_t)w + 1, 0);
        COORD zero; zero.X = 0; zero.Y = 0;
        DWORD got0 = 0;
        ReadConsoleOutputCharacterA(out, &row0[0], (DWORD)w, zero, &got0);
        std::string bufferRow0(&row0[0], (size_t)got0);

        // The console verdict scrolls away with everything else, so record it where a script can
        // read it: the demo directory, beside the exe.
        if (FILE* f = fopen("scrolltest.log", "w"))
        {
            fprintf(f, "buffer size      = %d x %d\n", (int)after.dwSize.X, (int)after.dwSize.Y);
            fprintf(f, "window rows      = %d..%d (top = %d)\n",
                    (int)after.srWindow.Top, (int)after.srWindow.Bottom, (int)after.srWindow.Top);
            fprintf(f, "first VISIBLE row= \"%s\"\n", firstVisible.c_str());
            fprintf(f, "BUFFER row 0     = \"%s\"\n", bufferRow0.c_str());
            fprintf(f, "verdict          = %s\n",
                    after.srWindow.Top == 0 ? "INCONCLUSIVE (window never left buffer row 0)"
                                            : (ok ? "PASS" : "FAIL"));
            fclose(f);
        }

        // Park the window back at the bottom so the verdict is readable.
        printf("\n\n");
        printf("scrolltest: window top at paint time = %d\n", (int)after.srWindow.Top);
        printf("scrolltest: first visible row = \"%s\"\n", firstVisible.c_str());
        printf("scrolltest: %s\n", ok ? "PASS - the screen painted at the WINDOW origin"
                                       : "FAIL - the screen painted somewhere else (buffer origin?)");
        fflush(stdout);
        if (after.srWindow.Top == 0)
            return 2;                 // inconclusive - the console gave us no scrollback to test with
        return ok ? 0 : 1;
    }
#endif

    printf("=== BF2 dedicated-server status screen ===\n");
    printf("layout driven through the recovered StatusMonitorImpl interface\n\n");
    StatusMonitorImpl_text impl(80, 24);
    draw(&impl, s);
    printf("\nRerun with --live to paint it through the real Win32 console backend,\n"
           "or --scrolltest to verify it paints at the WINDOW origin after scrolling.\n");
    return 0;
}
