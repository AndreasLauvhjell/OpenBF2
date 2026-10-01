// Code/Tools/BF2Editor/App/EditorWindow.cpp - see EditorWindow.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/EditorWindow.h"

#include "App/ImGuiHost.h"
#include "Framework/EditorLog.h"

namespace bf2editor
{

namespace
{

const char kWindowClass[] = "BF2EditorMainWindow";
const DWORD kStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;   // fixed size, see header

} // namespace

EditorWindow::EditorWindow()
    : m_hwnd(0)
    , m_imgui(0)
    , m_width(0)
    , m_height(0)
    , m_closeRequested(false)
    , m_minimized(false)
    , m_mouseCaptured(false)
    , m_rawDx(0)
    , m_rawDy(0)
{
    m_rawButtons[0] = m_rawButtons[1] = false;
}

EditorWindow::~EditorWindow()
{
    destroy();
}

bool EditorWindow::create(const std::string& title, int width, int height, bool visible)
{
    if (m_hwnd)
        return true;
    const HINSTANCE instance = GetModuleHandleA(0);

    WNDCLASSEXA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = &EditorWindow::windowProc;
    wc.hInstance = instance;
    wc.hIcon = LoadIconA(0, IDI_APPLICATION);
    wc.hCursor = LoadCursorA(0, IDC_ARROW);
    wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = kWindowClass;
    if (!RegisterClassExA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        logLine(LogLevel::Error, "window: RegisterClassEx failed (%lu)", GetLastError());
        return false;
    }

    // Clamp the client area so the whole window fits the primary work area.
    RECT work;
    if (!SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0))
        SetRect(&work, 0, 0, 1280, 720);
    RECT frame = { 0, 0, 0, 0 };
    AdjustWindowRectEx(&frame, kStyle, FALSE, 0);
    const int frameW = frame.right - frame.left;
    const int frameH = frame.bottom - frame.top;
    const int maxW = (work.right - work.left) - frameW;
    const int maxH = (work.bottom - work.top) - frameH;
    if (width > maxW)  width = maxW;
    if (height > maxH) height = maxH;
    if (width < 640)   width = 640;
    if (height < 480)  height = 480;

    RECT outer = { 0, 0, width, height };
    AdjustWindowRectEx(&outer, kStyle, FALSE, 0);
    const int outerW = outer.right - outer.left;
    const int outerH = outer.bottom - outer.top;
    const int x = work.left + ((work.right - work.left) - outerW) / 2;
    const int y = work.top + ((work.bottom - work.top) - outerH) / 2;

    m_hwnd = CreateWindowExA(0, kWindowClass, title.c_str(), kStyle, x, y, outerW, outerH,
                             0, 0, instance, this);
    if (!m_hwnd)
    {
        logLine(LogLevel::Error, "window: CreateWindowEx failed (%lu)", GetLastError());
        return false;
    }

    RECT client;
    GetClientRect(m_hwnd, &client);
    m_width = client.right - client.left;
    m_height = client.bottom - client.top;
    if (visible)
    {
        ShowWindow(m_hwnd, SW_SHOWNORMAL);
        UpdateWindow(m_hwnd);
    }
    logLine(LogLevel::Info, "window: %dx%d client area%s", m_width, m_height, visible ? "" : " (hidden)");
    return true;
}

void EditorWindow::destroy()
{
    if (!m_hwnd)
        return;
    setMouseCaptured(false);
    const HWND hwnd = m_hwnd;
    m_hwnd = 0;
    DestroyWindow(hwnd);
    UnregisterClassA(kWindowClass, GetModuleHandleA(0));
}

void EditorWindow::setTitle(const std::string& title)
{
    if (m_hwnd)
        SetWindowTextA(m_hwnd, title.c_str());
}

bool EditorWindow::pumpMessages()
{
    MSG msg;
    while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
            m_closeRequested = true;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return !m_closeRequested;
}

LRESULT CALLBACK EditorWindow::windowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        const CREATESTRUCTA* cs = reinterpret_cast<const CREATESTRUCTA*>(lParam);
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
    }
    EditorWindow* self = reinterpret_cast<EditorWindow*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (self && self->m_hwnd == hwnd)
        return self->handle(message, wParam, lParam);
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

void EditorWindow::setMouseCaptured(bool captured)
{
    if (captured == m_mouseCaptured || !m_hwnd)
        return;
    RAWINPUTDEVICE device;
    device.usUsagePage = 0x01;                          // generic desktop
    device.usUsage = 0x02;                              // mouse
    device.dwFlags = captured ? 0 : RIDEV_REMOVE;
    device.hwndTarget = captured ? m_hwnd : 0;
    if (!RegisterRawInputDevices(&device, 1, sizeof(device)))
    {
        logLine(LogLevel::Warning, "window: the raw mouse could not be %s (%lu)", captured ? "taken" : "released",
                GetLastError());
        if (captured)
            return;
    }
    m_mouseCaptured = captured;
    m_rawDx = m_rawDy = 0;
    m_rawButtons[0] = m_rawButtons[1] = false;
    if (captured)
    {
        holdCursor();
        while (ShowCursor(FALSE) >= 0) {}
    }
    else
    {
        ClipCursor(0);
        while (ShowCursor(TRUE) < 0) {}
    }
}

void EditorWindow::holdCursor()
{
    // The cursor stays at the client area's centre, so a click never lands outside the window.
    RECT client;
    GetClientRect(m_hwnd, &client);
    POINT centre = { (client.left + client.right) / 2, (client.top + client.bottom) / 2 };
    ClientToScreen(m_hwnd, &centre);
    RECT pin = { centre.x, centre.y, centre.x + 1, centre.y + 1 };
    ClipCursor(&pin);
    SetCursorPos(centre.x, centre.y);
}

void EditorWindow::takeRawMouse(float& dx, float& dy, bool buttons[2])
{
    dx = static_cast<float>(m_rawDx);
    dy = static_cast<float>(m_rawDy);
    m_rawDx = m_rawDy = 0;
    buttons[0] = m_mouseCaptured && m_rawButtons[0];
    buttons[1] = m_mouseCaptured && m_rawButtons[1];
    if (m_mouseCaptured)
        holdCursor();                                   // the system drops a cursor clip on some focus changes
}

void EditorWindow::readRawInput(LPARAM lParam)
{
    RAWINPUT input;
    UINT size = sizeof(input);
    if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER))
            == static_cast<UINT>(-1)
        || input.header.dwType != RIM_TYPEMOUSE)
        return;
    const RAWMOUSE& mouse = input.data.mouse;
    if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) == 0)
    {
        m_rawDx += mouse.lLastX;
        m_rawDy += mouse.lLastY;
    }
    if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)  m_rawButtons[0] = true;
    if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)    m_rawButtons[0] = false;
    if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) m_rawButtons[1] = true;
    if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)   m_rawButtons[1] = false;
}

LRESULT EditorWindow::handle(UINT message, WPARAM wParam, LPARAM lParam)
{
    if (m_mouseCaptured)
    {
        switch (message)
        {
        case WM_INPUT:
            readRawInput(lParam);
            return DefWindowProcA(m_hwnd, message, wParam, lParam);
        case WM_SETCURSOR:
            SetCursor(0);
            return TRUE;
        case WM_KILLFOCUS:
            setMouseCaptured(false);
            break;
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
                setMouseCaptured(false);
            break;
        default:
            break;
        }
    }
    if (m_imgui)
    {
        LRESULT result = 0;
        if (m_imgui->handleMessage(m_hwnd, message, wParam, lParam, result))
            return result;
    }
    switch (message)
    {
    case WM_CLOSE:
        // Close is a request: the frame loop ends, then everything shuts down in order and the
        // window is destroyed last (EditorApp), never from inside its own message.
        m_closeRequested = true;
        return 0;
    case WM_SIZE:
        m_minimized = wParam == SIZE_MINIMIZED;
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU)        // Alt alone would open the system menu over ImGui
            return 0;
        break;
    default:
        break;
    }
    return DefWindowProcA(m_hwnd, message, wParam, lParam);
}

} // namespace bf2editor
