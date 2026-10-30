// DirectInput mouse initialization, sampling and state accumulation.

#include "MouseDevice.h"
#if !DEDICATED
#include "../../Core/Debug.h"
#include "../../Core/Ids.h"
#include "../../Game/Main/BF2ClientSeams.h"
#include "../../Core/SettingsRepostitory.h"
#include "../../Game/Main/BF2Window.h"
#include <cmath>
#include <cstdio>

namespace dice { namespace hfe { namespace io {
extern IBase* inputDeviceManager;
extern IBase* controlMapManager;

namespace {
// The retail polling scratch buffer is global (0xA054E0), shared by instances.
DIDEVICEOBJECTDATA mouseEvents[128];
const char mouseSource[] = "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\IO\\Input\\MouseDevice.cpp";

// [CHOSEN, not recovered] Raw Input mouse backend, enabled with the +rawMouse 1 launch option
// (setting "GSRawMouse"). Retail reads the mouse through a 128-event DirectInput buffer and drops
// the whole poll on overflow, which loses movement with high polling-rate mice. In raw mode the
// DirectInput device is still created (capabilities, cooperative mode bookkeeping) but never
// acquired; WM_INPUT reports are converted into the same DIDEVICEOBJECTDATA samples, so
// updateSamples, peakXYAxis, sensitivity and inversion are unchanged. All state lives here rather
// than in MouseDevice so the retail class layout stays untouched; there is one system mouse.
// Messages are pumped on the thread that calls MouseDevice::update, so no locking is needed.
struct RawMouseState
{
    bool enabled;                                   // read once in MouseDevice::init
    bool handlerInstalled;
    bool registered;                                // RegisterRawInputDevices is in effect
    bool exclusive;                                 // registered with NOLEGACY | CAPTUREMOUSE
    bool lost;                                      // focus lost: reset like DIERR_INPUTLOST
    bool cursorHidden;                              // one ShowCursor(FALSE) is outstanding
    HWND window;
    DWORD sequence;
    std::list<DIDEVICEOBJECTDATA> pending;          // reports since the last poll
};
RawMouseState rawMouse = { false, false, false, false, false, false, 0, 0, std::list<DIDEVICEOBJECTDATA>() };

// Upper bound on queued reports between two polls; mirrors the retail 256-sample guard.
const size_t rawMousePendingLimit = 4096;

// Confines and hides the cursor while the mouse is captured, as an exclusive DirectInput
// acquisition does. NOLEGACY suppresses WM_SETCURSOR, so the window's own hide never runs.
void clipRawMouseCursor(bool clip)
{
    if (clip != rawMouse.cursorHidden) {
        ShowCursor(clip ? FALSE : TRUE);
        rawMouse.cursorHidden = clip;
    }
    if (!clip || !rawMouse.window) {
        ClipCursor(0);
        return;
    }
    RECT rect;
    if (!GetClientRect(rawMouse.window, &rect))
        return;
    MapWindowPoints(rawMouse.window, 0, reinterpret_cast<POINT*>(&rect), 2);
    ClipCursor(&rect);
}

// Millisecond clock for raw reports and raw-mode poll times. GetTickCount and GetMessageTime
// advance in roughly 16 ms steps, which lumps a fast mouse into one tick; the performance counter
// stamps each report when the frame pumps it. Only compared within the raw path.
DWORD rawMouseClock()
{
    static LONGLONG frequency = 0;
    if (!frequency) {
        LARGE_INTEGER value;
        QueryPerformanceFrequency(&value);
        frequency = value.QuadPart;
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return static_cast<DWORD>(counter.QuadPart * 1000 / frequency);
}

// Appends one report. Axis motion with the same millisecond timestamp is summed into the newest
// matching sample, which keeps the queue near two axis samples per millisecond at any polling rate.
void pushRawMouseEvent(DWORD offset, DWORD data, DWORD time)
{
    if (offset == DIMOFS_X || offset == DIMOFS_Y || offset == DIMOFS_Z) {
        for (std::list<DIDEVICEOBJECTDATA>::reverse_iterator it = rawMouse.pending.rbegin();
             it != rawMouse.pending.rend() && it->dwTimeStamp == time; ++it) {
            if (it->dwOfs == offset) {
                it->dwData = static_cast<DWORD>(static_cast<LONG>(it->dwData) + static_cast<LONG>(data));
                return;
            }
        }
    }
    if (rawMouse.pending.size() >= rawMousePendingLimit)
        rawMouse.pending.pop_front();
    DIDEVICEOBJECTDATA event = {};
    event.dwOfs = offset;
    event.dwData = data;
    event.dwTimeStamp = time;
    event.dwSequence = ++rawMouse.sequence;
    rawMouse.pending.push_back(event);
}

void handleRawMouseInput(HRAWINPUT handle)
{
    RAWINPUT input;
    UINT size = sizeof(input);
    if (GetRawInputData(handle, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER)) == static_cast<UINT>(-1))
        return;
    if (input.header.dwType != RIM_TYPEMOUSE)
        return;
    const RAWMOUSE& mouse = input.data.mouse;
    const DWORD time = rawMouseClock();
    // Absolute reports (remote desktop, tablets) have no relative meaning for aiming; ignored.
    if (!(mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
        if (mouse.lLastX)
            pushRawMouseEvent(DIMOFS_X, static_cast<DWORD>(mouse.lLastX), time);
        if (mouse.lLastY)
            pushRawMouseEvent(DIMOFS_Y, static_cast<DWORD>(mouse.lLastY), time);
    }
    const USHORT flags = mouse.usButtonFlags;
    if (flags & RI_MOUSE_WHEEL)
        pushRawMouseEvent(DIMOFS_Z, static_cast<DWORD>(static_cast<LONG>(static_cast<SHORT>(mouse.usButtonData))), time);
    static const USHORT downFlags[5] = { RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_2_DOWN,
        RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_5_DOWN };
    static const USHORT upFlags[5] = { RI_MOUSE_BUTTON_1_UP, RI_MOUSE_BUTTON_2_UP,
        RI_MOUSE_BUTTON_3_UP, RI_MOUSE_BUTTON_4_UP, RI_MOUSE_BUTTON_5_UP };
    for (DWORD i = 0; i < 5; ++i) {
        if (flags & downFlags[i])
            pushRawMouseEvent(DIMOFS_BUTTON0 + i, 0x80, time);
        if (flags & upFlags[i])
            pushRawMouseEvent(DIMOFS_BUTTON0 + i, 0, time);
    }
}

// Window message handler, installed on the game window the first time raw mode activates.
int rawMouseMessageHandler(HWND, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!rawMouse.registered)
        return 0;
    if (message == WM_INPUT) {
        handleRawMouseInput(reinterpret_cast<HRAWINPUT>(lParam));
    } else if (message == WM_ACTIVATEAPP) {
        if (wParam) {
            clipRawMouseCursor(rawMouse.exclusive);
        } else {
            clipRawMouseCursor(false);
            rawMouse.lost = true;
        }
    }
    return 0;
}

bool registerRawMouse(HWND window, bool exclusive)
{
    RAWINPUTDEVICE device;
    device.usUsagePage = 0x01;                      // generic desktop
    device.usUsage = 0x02;                          // mouse
    device.dwFlags = exclusive ? RIDEV_NOLEGACY | RIDEV_CAPTUREMOUSE : 0;
    device.hwndTarget = window;
    if (!RegisterRawInputDevices(&device, 1, sizeof(device)))
        return false;
    rawMouse.window = window;
    rawMouse.registered = true;
    rawMouse.exclusive = exclusive;
    clipRawMouseCursor(exclusive && GetForegroundWindow() == window);
    return true;
}

void unregisterRawMouse()
{
    if (!rawMouse.registered)
        return;
    RAWINPUTDEVICE device;
    device.usUsagePage = 0x01;
    device.usUsage = 0x02;
    device.dwFlags = RIDEV_REMOVE;
    device.hwndTarget = 0;
    RegisterRawInputDevices(&device, 1, sizeof(device));
    clipRawMouseCursor(false);
    rawMouse.registered = false;
    rawMouse.exclusive = false;
    rawMouse.pending.clear();
}

HWND rawMouseWindow()
{
    if (!g_bf2Render || !g_bf2Render->getWindow())
        return 0;
    return g_bf2Render->getWindow()->getHwnd();
}
}

MouseDevice::MouseDevice()
    : m_refCount(1), m_buttons(0), m_previousButtons(0), m_active(false),
      m_pollTime(0), m_device(0), m_samples(), m_peakSamples(),
      m_field0f0(0), m_field0f4(0), m_field0f8(0), m_field0fc(false), m_lastSequence(0)
{
    for (int i = 0; i < 4; ++i) {
        m_axes[i] = 0.0f;
        m_axisScale[i] = 0.03f;
        m_invertAxis[i] = false;
        m_axisArmed[i] = false;
    }
    // Preserve the retail uninitialized mode/counts/latencies/sample-time and
    // DirectInput pointer. In particular, destruction before init is not safe.
}

MouseDevice::~MouseDevice()
{
    unregisterRawMouse();                               // [CHOSEN, not recovered]; no-op in DI mode
    if (rawMouse.handlerInstalled && g_bf2Render && g_bf2Render->getWindow()) {
        g_bf2Render->getWindow()->removeMessageHandler(rawMouseMessageHandler);
        rawMouse.handlerInstalled = false;
    }
    if (m_device) {
        m_device->Unacquire();
        m_device->Release();
        m_device = 0;
    }
    if (m_directInput) {
        m_directInput->Release();
        m_directInput = 0;
    }
    // Native list destruction: peak queue first, then sample queue.
}

void MouseDevice::addRef() { ++m_refCount; }
int MouseDevice::getRef() { return m_refCount; }
int MouseDevice::release()
{
    const int count = --m_refCount;
    if (count == 0)
        delete this;
    return count;
}
void* MouseDevice::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IInputDevice || iid == IID_IAdvancedInputDevice)
        return const_cast<MouseDevice*>(this);
    return 0;
}
InputDeviceTypes MouseDevice::getType() const { return static_cast<InputDeviceTypes>(1); }
InputDeviceMode MouseDevice::getCooperativeMode() const { return m_mode; }

bool MouseDevice::setCooperativeMode(InputDeviceMode mode)
{
    const DWORD flags = mode ? DISCL_NONEXCLUSIVE | DISCL_BACKGROUND
                             : DISCL_EXCLUSIVE | DISCL_FOREGROUND;
    if (m_device->SetCooperativeLevel(GetFocus(), flags) != DI_OK)
        return setCooperativeModeForWindow(g_bf2Render->getWindow()->getHwnd(), mode);
    m_mode = mode;
    // [CHOSEN, not recovered] A live raw registration follows the new exclusivity.
    if (rawMouse.registered && rawMouse.exclusive != (mode == 0))
        registerRawMouse(rawMouse.window, mode == 0);
    return true;
}

bool MouseDevice::setCooperativeModeForWindow(HWND window, InputDeviceMode mode)
{
    const DWORD flags = mode ? DISCL_NONEXCLUSIVE | DISCL_FOREGROUND
                             : DISCL_EXCLUSIVE | DISCL_FOREGROUND;
    if (m_device->SetCooperativeLevel(window, flags) != DI_OK)
        return false;
    m_mode = mode;
    return true;
}

bool MouseDevice::activate()
{
    if (!m_device)
        return false;
    if (rawMouse.enabled) {
        // [CHOSEN, not recovered] Raw mode registers for WM_INPUT instead of acquiring.
        if (!m_active) {
            const HWND window = rawMouseWindow();
            if (window && !rawMouse.handlerInstalled) {
                g_bf2Render->getWindow()->addMessageHandler(rawMouseMessageHandler);
                rawMouse.handlerInstalled = true;
            }
            m_active = window && registerRawMouse(window, m_mode == 0);
            rawMouse.lost = false;
        }
        return true;                                    // also when registration fails, as retail
    }
    if (!m_active)
        m_active = m_device->Acquire() == DI_OK;
    return true;                                        // also when Acquire fails
}

bool MouseDevice::deActivate()
{
    if (!m_device)
        return false;
    if (m_active) {
        if (rawMouse.enabled)
            unregisterRawMouse();                       // [CHOSEN, not recovered]
        else
            m_device->Unacquire();
        m_active = false;
        m_samples.clear();                              // peak queue is retained
        m_buttons = m_previousButtons = 0;
        m_axes[0] = m_axes[1] = m_axes[2] = m_axes[3] = 0.0f;
    }
    return true;
}

bool MouseDevice::isActive() const { return m_device && m_active; }
int MouseDevice::getNumAxes() const { return m_numAxes; }
int MouseDevice::getNumButtons() const { return m_numButtons; }
float MouseDevice::readAxis(InputDeviceAxes axis, bool) const
{
    return m_invertAxis[axis] ? -m_axes[axis] : m_axes[axis];
}
void MouseDevice::setInvertAxis(InputDeviceAxes axis, bool invert)
{
    if (axis <= m_numAxes) m_invertAxis[axis] = invert;
}
bool MouseDevice::getInvertAxis(InputDeviceAxes axis) const
{
    return axis <= m_numAxes ? m_invertAxis[axis] : false;
}
void MouseDevice::setAxisScale(InputDeviceAxes axis, float scale)
{
    if (axis <= m_numAxes) m_axisScale[axis] = scale;
}
float MouseDevice::getAxisScale(InputDeviceAxes axis) const
{
    return axis <= m_numAxes ? m_axisScale[axis] : 0.0f;
}
// These two entire shipped bodies are a ret8 and an x87 constant-zero return.
void MouseDevice::setAxisAdd(InputDeviceAxes, float) {}
float MouseDevice::getAxisAdd(InputDeviceAxes) const { return 0.0f; }

bool MouseDevice::buttonDown(InputDeviceButtons button) const
{
    return (m_buttons & button) != 0;
}
bool MouseDevice::buttonPressed(InputDeviceButtons button) const
{
    return (m_previousButtons & button) == 0 && (m_buttons & button) != 0;
}
bool MouseDevice::buttonReleased(InputDeviceButtons button) const
{
    return (m_previousButtons & button) != 0 && (m_buttons & button) == 0;
}
float MouseDevice::buttonPressedSampleLatency(InputDeviceButtons button) const
{
    for (unsigned int i = 0; i < 32; ++i)
        if ((1u << i) & button)
            return m_buttonLatency[i];
    return 0.0f;
}

bool MouseDevice::getNextInput(InputDeviceAxes& axis, InputDeviceButtons& button,
                               InputDeviceKeys& key, bool& negative)
{
    for (unsigned int i = 0; i < m_numButtons; ++i) {
        const InputDeviceButtons candidate = static_cast<InputDeviceButtons>(1u << (i & 31));
        if (buttonPressed(candidate)) {
            axis = static_cast<InputDeviceAxes>(15);
            button = candidate;
            key = static_cast<InputDeviceKeys>(0);
            return true;                               // negative remains untouched
        }
    }
    for (int i = 0; i < m_numAxes; ++i) {
        const float magnitude = std::fabs(m_axes[i]);
        if (magnitude < 0.1f) {
            m_axisArmed[i] = true;
        } else if (m_axisArmed[i] && magnitude >= 0.45f) {
            m_axisArmed[i] = false;
            axis = static_cast<InputDeviceAxes>(i);
            button = static_cast<InputDeviceButtons>(0);
            key = static_cast<InputDeviceKeys>(0);
            negative = m_axes[i] < 0.0f;
            return true;
        }
    }
    return false;
}

void MouseDevice::peakXYAxis(float& x, float& y)
{
    y = 0.0f;
    x = 0.0f;
    while (!m_peakSamples.empty()) {
        const DIDEVICEOBJECTDATA& event = m_peakSamples.front();
        if (event.dwSequence > m_lastSequence) {
            if (event.dwOfs == DIMOFS_X)
                x += static_cast<LONG>(event.dwData);
            else if (event.dwOfs == DIMOFS_Y)
                y += static_cast<LONG>(event.dwData);
        }
        m_peakSamples.pop_front();
    }
    IControlMap* map = static_cast<IControlMapManager*>(controlMapManager)->getActive();
    x = map->getAxisScale(static_cast<InputDeviceAxes>(0)) * x;
    x = map->getMouseSensitivity() * x;
    y = map->getAxisScale(static_cast<InputDeviceAxes>(1)) * y;
    y = map->getMouseSensitivity() * y;
    if (map->getInvertMouse())
        y = -y;
}

void MouseDevice::updateSamples(float deltaTime)
{
    IControlMap* map = static_cast<IControlMapManager*>(controlMapManager)->getActive();
    m_axes[0] = m_axes[1] = m_axes[2] = 0.0f;             // fourth axis retained
    m_previousButtons = m_buttons;
    // PE retains the x87 product until fistp truncation. A float temporary
    // would round 0.01f * 1000 up to 10; the shipped result truncates to 9.
    m_sampleTime += static_cast<int>(static_cast<double>(deltaTime) * 1000.0);
    while (!m_samples.empty()) {
        const DIDEVICEOBJECTDATA& event = m_samples.front();
        if (event.dwTimeStamp >= m_sampleTime)
            break;                                     // equality waits for next sample
        const float age = static_cast<float>(m_pollTime - event.dwTimeStamp) * 0.001f;
        const float latency = age <= 0.033333335f ? age : 0.033333335f;
        switch (event.dwOfs) {
        case DIMOFS_X: m_axes[0] += static_cast<LONG>(event.dwData); break;
        case DIMOFS_Y: m_axes[1] += static_cast<LONG>(event.dwData); break;
        case DIMOFS_Z: m_axes[2] += static_cast<LONG>(event.dwData); break;
        case DIMOFS_BUTTON0: case DIMOFS_BUTTON1: case DIMOFS_BUTTON2: case DIMOFS_BUTTON3:
        case DIMOFS_BUTTON4: case DIMOFS_BUTTON5: case DIMOFS_BUTTON6: case DIMOFS_BUTTON7: {
            const unsigned int index = event.dwOfs - DIMOFS_BUTTON0;
            const unsigned int mask = 1u << index;
            if (event.dwData & 0x80) {
                m_buttons |= mask;
                m_buttonLatency[index] = latency;
            } else {
                m_buttons &= ~mask;
            }
            break;
        }
        default: break;
        }
        m_lastSequence = event.dwSequence;              // including ignored offsets
        m_samples.pop_front();
    }
    for (int i = 0; i < 4; ++i)
        m_axes[i] = map->getAxisScale(static_cast<InputDeviceAxes>(i)) * m_axes[i];
}

void MouseDevice::update(float deltaTime)
{
    if (!m_active || !m_device)
        return;
    if (deltaTime > 0.0f) {
        updateSamples(deltaTime);
        return;
    }
    if (m_peakSamples.size() > 256)
        m_peakSamples.clear();
    if (m_samples.size() > 256) {
        m_samples.clear();
        m_peakSamples.clear();
    }
    if (rawMouse.enabled) {
        // [CHOSEN, not recovered] Drain WM_INPUT reports. A focus loss resets state the way
        // DIERR_INPUTLOST does; nothing is dropped on volume.
        // The cursor capture follows the foreground state every poll: activation messages can
        // arrive before the device is registered again, so they alone cannot restore it.
        const bool capture = rawMouse.registered && rawMouse.exclusive
                          && GetForegroundWindow() == rawMouse.window;
        if (capture != rawMouse.cursorHidden)
            clipRawMouseCursor(capture);
        // Another application can leave its cursor shape behind, and NOLEGACY suppresses the
        // WM_SETCURSOR through which the window would clear it again.
        if (capture && GetCursor())
            SetCursor(0);
        if (rawMouse.lost) {
            rawMouse.lost = false;
            rawMouse.pending.clear();
            m_samples.clear();
            m_peakSamples.clear();
            m_buttons = m_previousButtons = 0;
            m_axes[0] = m_axes[1] = m_axes[2] = m_axes[3] = 0.0f;
        }
        m_sampleTime = m_pollTime;
        m_pollTime = rawMouseClock();                   // same clock as the report stamps
        for (std::list<DIDEVICEOBJECTDATA>::const_iterator it = rawMouse.pending.begin();
             it != rawMouse.pending.end(); ++it) {
            m_samples.push_back(*it);
            m_peakSamples.push_back(*it);
        }
        rawMouse.pending.clear();
        return;
    }
    DWORD count;
    HRESULT result;
    for (;;) {
        count = 128;
        result = m_device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA), mouseEvents, &count, 0);
        if (result != DIERR_INPUTLOST)
            break;
        m_samples.clear();
        m_peakSamples.clear();
        m_buttons = m_previousButtons = 0;
        m_axes[0] = m_axes[1] = m_axes[2] = m_axes[3] = 0.0f;
        if (m_device->Acquire() != DI_OK)
            return;
    }
    if (result != DI_OK) {                              // buffer overflow also clears
        m_samples.clear();
        m_peakSamples.clear();
        m_buttons = m_previousButtons = 0;
        m_axes[0] = m_axes[1] = m_axes[2] = m_axes[3] = 0.0f;
    } else {
        m_sampleTime = m_pollTime;
        m_pollTime = GetTickCount();
        for (DWORD i = 0; i < count; ++i) {
            m_samples.push_back(mouseEvents[i]);
            m_peakSamples.push_back(mouseEvents[i]);
        }
    }
}

bool MouseDevice::initDirectInput()
{
    const HRESULT result = DirectInput8Create(GetModuleHandleA(0), 0x800,
        IID_IDirectInput8A, reinterpret_cast<void**>(&m_directInput), 0);
    if (result == DI_OK)
        return true;
    switch (result) {
    case DIERR_BETADIRECTINPUTVERSION:
        Debug(DebugTypeError, mouseSource, 46, "") << "InputDeviceBase error betaversion";
        break;
    case DIERR_INVALIDPARAM:
        Debug(DebugTypeError, mouseSource, 49, "") << "InputDeviceBase error invalid param";
        break;
    case DIERR_OLDDIRECTINPUTVERSION:
        Debug(DebugTypeError, mouseSource, 52, "") << "InputDeviceBase error oldversion";
        break;
    case DIERR_OUTOFMEMORY:
        Debug(DebugTypeError, mouseSource, 55, "") << "InputDeviceBase error out of memory";
        break;
    default:
        Debug(DebugTypeError, mouseSource, 59, "") << "InputDeviceBase error other error " << result;
        break;
    }
    return false;
}

bool MouseDevice::init()
{
    if (!inputDeviceManager)
        return false;
    if (!initDirectInput()) {
        Debug(DebugTypeError, mouseSource, 127, "") << "init failed";
        return false;
    }
    if (m_directInput->CreateDevice(GUID_SysMouse, &m_device, 0) != DI_OK)
        return false;
    DIDEVCAPS capabilities;
    capabilities.dwSize = sizeof(capabilities);
    m_device->GetCapabilities(&capabilities);            // return deliberately ignored
    m_numButtons = capabilities.dwButtons;
    m_numAxes = capabilities.dwAxes;
    if (m_device->SetDataFormat(&c_dfDIMouse2) != DI_OK)
        return false;
    DIPROPDWORD bufferSize;
    bufferSize.diph.dwSize = sizeof(bufferSize);
    bufferSize.diph.dwHeaderSize = sizeof(bufferSize.diph);
    bufferSize.diph.dwObj = 0;
    bufferSize.diph.dwHow = DIPH_DEVICE;
    bufferSize.dwData = 128;
    if (m_device->SetProperty(DIPROP_BUFFERSIZE, &bufferSize.diph) < 0)
        Debug(DebugTypeError, mouseSource, 172, "") << "Failed to init buffered mode";
    // [CHOSEN, not recovered] Backend choice for this run; see RawMouseState.
    rawMouse.enabled = settings && settings->boolGet("GSRawMouse", false);
    if (rawMouse.enabled) {
        std::fprintf(stderr, "[CHOSEN] MouseDevice: Raw Input backend enabled (+rawMouse 1)\n");
        std::fflush(stderr);
    }
    return setCooperativeMode(static_cast<InputDeviceMode>(0)) != false;
}

} } }
#endif
