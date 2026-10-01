// [CHOSEN, not recovered] Win32 console backend modeled on the POSIX status display.
// Replace it when the Windows implementation is established. Preserve StatusMonitor's screen layout.
// This backend temporarily widens narrow consoles and paints relative to the visible window;
// the POSIX backend adopts the existing terminal size.

#include "StatusMonitorImpl.h"

#if defined(_WIN32)

#include <windows.h>
#include <conio.h>
#include <vector>

#include "../../IO/Console/Console.h"    // OldConsole::updateAsciiKey, and the mainConsole singleton

namespace dice {
namespace hfe {

namespace {

// Matches the posix backend's use of System::getWallClockTime (vptr+0x1C). Using the performance
// counter directly rather than routing through the System singleton keeps this file linkable on its
// own, which is what lets the status screen be exercised before the engine stands up.
double wallClockSeconds()
{
    static LARGE_INTEGER freq = { { 0, 0 } };
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart / (double)freq.QuadPart;
}

} // anonymous namespace

class StatusMonitorImpl_win32 : public StatusMonitorImpl
{
public:
    StatusMonitorImpl_win32();
    virtual ~StatusMonitorImpl_win32();

    virtual bool shouldUpdate(bool duringLoad);
    virtual void clearScreen();
    virtual void getDimensions(int& width, int& height);
    virtual void outputString(int x, int y, const std::string& text);
    virtual void flushToScreen();

private:
    void setPreferredSize(int width, int height);
    void readDimensions();

    HANDLE            m_out;
    int               m_originY;        // the visible window's TOP row in buffer coordinates
    int               m_refreshCount;
    double            m_lastUpdateTime;
    int               m_width;
    int               m_height;
    std::vector<char> m_cells;          // the back buffer: m_width * m_height characters
    CONSOLE_SCREEN_BUFFER_INFO m_saved;
    bool              m_restoreSize;
};

StatusMonitorImpl_win32::StatusMonitorImpl_win32()
    : m_out(INVALID_HANDLE_VALUE)
    , m_originY(0)
    , m_refreshCount(0)
    , m_lastUpdateTime(-1000.0)     // same sentinel as posix - forces the first paint through
    , m_width(-1)
    , m_height(-1)
    , m_restoreSize(false)
{
    m_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (m_out == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(m_out, &m_saved))
    {
        m_out = INVALID_HANDLE_VALUE;   // no console -> every method below no-ops, as posix does
        return;
    }

    setPreferredSize(80, 25);
    readDimensions();

    CONSOLE_CURSOR_INFO cursor;
    if (GetConsoleCursorInfo(m_out, &cursor))
    {
        cursor.bVisible = FALSE;        // ncurses hides the cursor for us; here it must be asked
        SetConsoleCursorInfo(m_out, &cursor);
    }
}

StatusMonitorImpl_win32::~StatusMonitorImpl_win32()
{
    if (m_out == INVALID_HANDLE_VALUE)
        return;

    CONSOLE_CURSOR_INFO cursor;
    if (GetConsoleCursorInfo(m_out, &cursor))
    {
        cursor.bVisible = TRUE;
        SetConsoleCursorInfo(m_out, &cursor);
    }

    if (m_restoreSize)
    {
        // Shrinking: window first, then buffer. The reverse order fails when the window would be
        // larger than the buffer.
        SMALL_RECT window = m_saved.srWindow;
        SetConsoleWindowInfo(m_out, TRUE, &window);
        SetConsoleScreenBufferSize(m_out, m_saved.dwSize);
    }
}

// [CHOSEN]. No posix counterpart - see the header block.
void StatusMonitorImpl_win32::setPreferredSize(int width, int height)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(m_out, &info))
        return;

    const int haveW = info.srWindow.Right - info.srWindow.Left + 1;
    const int haveH = info.srWindow.Bottom - info.srWindow.Top + 1;
    if (haveW >= width && haveH >= height)
        return;                         // already big enough; leave the user's console alone

    const SHORT wantW = (SHORT)(haveW > width  ? haveW : width);
    const SHORT wantH = (SHORT)(haveH > height ? haveH : height);

    // Growing: buffer first, then window - the window may never exceed the buffer.
    COORD size;
    size.X = wantW;
    size.Y = info.dwSize.Y > wantH ? info.dwSize.Y : wantH;
    if (!SetConsoleScreenBufferSize(m_out, size))
        return;

    SMALL_RECT window;
    window.Left   = 0;
    window.Top    = info.srWindow.Top;
    window.Right  = (SHORT)(wantW - 1);
    window.Bottom = (SHORT)(info.srWindow.Top + wantH - 1);
    if (SetConsoleWindowInfo(m_out, TRUE, &window))
        m_restoreSize = true;
}

void StatusMonitorImpl_win32::readDimensions()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (!GetConsoleScreenBufferInfo(m_out, &info))
        return;

    m_width  = info.srWindow.Right  - info.srWindow.Left + 1;
    m_height = info.srWindow.Bottom - info.srWindow.Top  + 1;
    m_originY = info.srWindow.Top;      // see flushToScreen - this is what makes (0,0) mean the
                                        // top-left of the visible window rather than of the buffer

    const size_t cells = (size_t)m_width * (size_t)m_height;
    if (m_cells.size() != cells)
        m_cells.assign(cells, ' ');
}

// Mirrors StatusMonitorImpl_posix::shouldUpdate exactly, including the inverted-looking parameter
// and the fact that a keystroke both forwards AND forces a repaint. The key table is the console
// equivalent of the ncurses one: Windows delivers 0x00/0xE0 followed by a scan code for the keys
// ncurses reports as KEY_*.
bool StatusMonitorImpl_win32::shouldUpdate(bool duringLoad)
{
    if (m_out == INVALID_HANDLE_VALUE)
        return false;

    const double now = wallClockSeconds();

    if (!duringLoad)
    {
        if (_kbhit())
        {
            int c = _getch();
            int key = c;

            if (c == 0x00 || c == 0xE0)             // extended key: a second byte follows
            {
                switch (_getch())
                {
                case 72: key = 16; break;           // Up       -> ^P
                case 80: key = 14; break;           // Down     -> ^N
                case 73: key =  2; break;           // PageUp   -> ^B
                case 81: key =  6; break;           // PageDown -> ^F
                default: key =  0; break;
                }
            }
            else if (c == 8 || c == 127)
            {
                key = 8;
            }
            else if (c == 10 || c == 13)
            {
                key = 13;
            }

            // [CHOSEN] Allow standalone status-display tests before the console exists.
            // The POSIX implementation retains its original unconditional console access.
            if (key && io::mainConsole)
                io::mainConsole->updateAsciiKey((char)key);

            m_lastUpdateTime = now;
            return true;
        }
    }

    if (now - m_lastUpdateTime <= 0.5)              // the same 0.5 s floor as the ELF
        return false;

    m_lastUpdateTime = now;
    return true;
}

void StatusMonitorImpl_win32::clearScreen()
{
    if (m_out == INVALID_HANDLE_VALUE)
        return;
    readDimensions();                               // a resized console must be picked up
    m_cells.assign(m_cells.size(), ' ');
}

void StatusMonitorImpl_win32::getDimensions(int& width, int& height)
{
    width  = m_out == INVALID_HANDLE_VALUE ? -1 : m_width;
    height = m_out == INVALID_HANDLE_VALUE ? -1 : m_height;
}

void StatusMonitorImpl_win32::outputString(int x, int y, const std::string& text)
{
    if (m_out == INVALID_HANDLE_VALUE || m_cells.empty())
        return;
    // wmove returns ERR for an off-screen position and the posix backend then drops the write
    // entirely rather than clamping. Same here.
    if (x < 0 || y < 0 || x >= m_width || y >= m_height)
        return;

    const size_t row = (size_t)y * (size_t)m_width;
    size_t at = row + (size_t)x;
    const size_t rowEnd = row + (size_t)m_width;

    for (size_t i = 0; i < text.size() && at < rowEnd; ++i, ++at)
    {
        const char ch = text[i];
        m_cells[at] = (ch == '\t' || ch == '\n' || ch == '\r') ? ' ' : ch;
    }
}

void StatusMonitorImpl_win32::flushToScreen()
{
    if (m_out == INVALID_HANDLE_VALUE || m_cells.empty())
        return;

    // The posix backend does nine cheap refreshes then one that repaints everything. Win32 has no
    // damage tracking to skip, so every flush writes every cell; the counter is kept so the two
    // backends stay behaviourally identical if damage tracking is ever added.
    if (++m_refreshCount > 9)
        m_refreshCount = 0;

    // Address the visible window's origin within the console buffer so drawing
    // continues to target the screen after the buffer scrolls.
    COORD at;
    at.X = 0;
    for (int y = 0; y < m_height; ++y)
    {
        at.Y = (SHORT)(m_originY + y);
        DWORD written = 0;
        WriteConsoleOutputCharacterA(m_out, &m_cells[(size_t)y * (size_t)m_width],
                                     (DWORD)m_width, at, &written);
    }
}

StatusMonitorImpl* StatusMonitorImpl::createInstance()
{
    return new StatusMonitorImpl_win32();
}

} // namespace hfe
} // namespace dice

#endif // _WIN32
