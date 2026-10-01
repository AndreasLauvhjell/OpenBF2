// Code/Tools/BF2Editor/App/EditorWindow.h

// The editor's main window (WP4). It is the editor's own Win32 window, not the game's BF2Window: the
// game window blocks resizing (WM_NCHITTEST always answers htclient), allows one window per process
// and persists SPI_SETFOREGROUNDLOCKTIMEOUT when it creates itself. EngineHost hands this window's
// hwnd to a BF2Window object through BF2Window::setHwnd, so the engine's renderer draws into it.

// The client size is fixed for now: resizing needs a device Reset (IRenderer +0x10), and the
// rebuilt RendDX9's Reset path has never run live.
// Editor-only code (D3).
#ifndef BF2EDITOR_APP_EDITORWINDOW_H
#define BF2EDITOR_APP_EDITORWINDOW_H

#include "Framework/EditorConfig.h"

#include <windows.h>

#include <string>

namespace bf2editor
{

class ImGuiHost;

class EditorWindow
{
public:
    EditorWindow();
    ~EditorWindow();

    // width/height: the client size; clamped to the work area. visible = false keeps the window hidden
    // (automated runs: nothing appears on the desktop, and D3D9 still renders into it).
    bool create(const std::string& title, int width, int height, bool visible);
    void destroy();

    // Routes window messages to ImGui first (set once ImGui is up; 0 before and after).
    void setImGui(ImGuiHost* imgui) { m_imgui = imgui; }

    // Drains the message queue. Returns false once the user asked to close the window (or WM_QUIT
    // arrived); the caller then leaves its frame loop and shuts down in order.
    bool pumpMessages();
    // The caller decided not to close after all (unsaved changes: the user chose Cancel).
    void cancelClose() { m_closeRequested = false; }

    HWND hwnd() const { return m_hwnd; }
    int  clientWidth() const { return m_width; }
    int  clientHeight() const { return m_height; }
    bool isMinimized() const { return m_minimized; }
    void setTitle(const std::string& title);

    // Soldier mode's mouse (App/PlayTest.h): captured, the cursor is hidden and held inside the window and the raw
    // movement (WM_INPUT) and buttons go to the play-test. Losing the focus releases it.
    void setMouseCaptured(bool captured);
    bool mouseCaptured() const { return m_mouseCaptured; }
    // The movement since the last call (mouse counts) and the buttons held.
    void takeRawMouse(float& dx, float& dy, bool buttons[2]);

private:
    EditorWindow(const EditorWindow&);
    EditorWindow& operator=(const EditorWindow&);

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT handle(UINT message, WPARAM wParam, LPARAM lParam);
    void    readRawInput(LPARAM lParam);
    void    holdCursor();

    HWND       m_hwnd;
    ImGuiHost* m_imgui;
    int        m_width;
    int        m_height;
    bool       m_closeRequested;
    bool       m_minimized;
    bool       m_mouseCaptured;
    long       m_rawDx;
    long       m_rawDy;
    bool       m_rawButtons[2];
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_EDITORWINDOW_H
