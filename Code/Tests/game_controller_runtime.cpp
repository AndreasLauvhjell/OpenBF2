// [TEMP] [CHOSEN, not recovered] Bounded COM fixture; no hardware acquisition.
// Real GameControllerDevice/Debug/StringUtil bodies are linked. Unsupported SDK
// calls abort instead of supplying success values.
#include <string>
#include <vector>
#include <Windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <cstdint>
#define private public
#include "../BF2/IO/Input/GameControllerDevice.h"
#undef private
#include "../BF2/Core/Debug.h"
#include "../BF2/Core/Ids.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <new>

namespace dice { namespace hfe { namespace io { IBase* inputDeviceManager = nullptr; } } }
using namespace dice::hfe;
using namespace dice::hfe::io;

namespace {
void* deviceSlots[32];
void* inputSlots[11];
struct FakeCom { void** slots; } deviceCom = {deviceSlots}, inputCom = {inputSlots};
HRESULT directResult = DI_OK, enumResult = DI_OK, deviceResult = DI_OK, formatResult = DI_OK;
HRESULT rangeResult = DI_OK, bufferResult = DI_OK, capsResult = DI_OK, objectsResult = DI_OK;
HRESULT cooperativeResult = DI_OK, acquireResult = DI_OK, pollResult = DI_OK;
HRESULT stateResult = DI_OK, dataResult = DI_OK;
int sdkCreates, deviceCreates, rangeCalls, acquireCalls, inputLostCount, pollCalls;
int unacquireCalls, deviceReleases, inputReleases, reads, dataReads, enumerated;
DWORD cooperativeFlags;
bool enumVerified, createVerified, formatVerified, bufferVerified, rangesVerified = true;
DIJOYSTATE2 immediate;
std::vector<DIDEVICEOBJECTDATA> events;
std::vector<int> lifecycle;

void __stdcall unsupported() { std::fprintf(stderr, "unexpected COM fixture call\n"); std::abort(); }
ULONG __stdcall releaseDevice(void*) { ++deviceReleases; lifecycle.push_back(8); return 0; }
ULONG __stdcall releaseInput(void*) { ++inputReleases; lifecycle.push_back(9); return 0; }
HRESULT __stdcall acquire(void*)
{ ++acquireCalls; if (inputLostCount > 0) { --inputLostCount; return DIERR_INPUTLOST; } return acquireResult; }
HRESULT __stdcall unacquire(void*) { ++unacquireCalls; lifecycle.push_back(7); return DI_OK; }
HRESULT __stdcall setCoop(void*, HWND, DWORD flags) { cooperativeFlags = flags; lifecycle.push_back(4); return cooperativeResult; }
HRESULT __stdcall setFormat(void*, const DIDATAFORMAT* f)
{ formatVerified = f == &c_dfDIJoystick2 && f->dwDataSize == 272 && f->dwNumObjs == 164; lifecycle.push_back(2); return formatResult; }
HRESULT __stdcall setProperty(void*, REFGUID property, const DIPROPHEADER* h)
{
    if (&property == &DIPROP_BUFFERSIZE)
    {
        const DIPROPDWORD& p = *reinterpret_cast<const DIPROPDWORD*>(h);
        bufferVerified = h->dwSize == 20 && h->dwHeaderSize == 16 && h->dwObj == 0 && h->dwHow == 0 && p.dwData == 128;
        lifecycle.push_back(3);
        return bufferResult;
    }
    if (&property != &DIPROP_RANGE) std::abort();
    const DIPROPRANGE& r = *reinterpret_cast<const DIPROPRANGE*>(h);
    rangesVerified &= h->dwSize == 24 && h->dwHeaderSize == 16 && h->dwHow == DIPH_BYID &&
        h->dwObj == (DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(rangeCalls)) && r.lMin == -1000 && r.lMax == 1000;
    ++rangeCalls;
    return rangeResult;
}
HRESULT __stdcall getCaps(void*, DIDEVCAPS* caps)
{
    if (caps->dwSize != 44) std::abort();
    caps->dwAxes = 5; caps->dwButtons = 3; caps->dwPOVs = 2;
    lifecycle.push_back(5);
    return capsResult;
}
HRESULT __stdcall enumObjects(void*, LPDIENUMDEVICEOBJECTSCALLBACKA callback, void* context, DWORD flags)
{
    if (flags != DIDFT_AXIS) std::abort();
    lifecycle.push_back(6);
    const GUID* guids[] = {&GUID_XAxis, &GUID_ZAxis, &GUID_RyAxis, &GUID_Slider, &GUID_Slider};
    for (unsigned int i = 0; i != 5; ++i)
    {
        DIDEVICEOBJECTINSTANCEA object = {};
        object.dwSize = sizeof(object); object.guidType = *guids[i]; object.dwType = DIDFT_ABSAXIS | DIDFT_MAKEINSTANCE(i);
        if (callback(&object, context) == DIENUM_STOP) break;
    }
    return objectsResult;
}
HRESULT __stdcall createDevice(void*, REFGUID guid, IDirectInputDevice8A** out, IUnknown* outer)
{
    ++deviceCreates; createVerified = guid == GUID_YAxis && !outer;
    if (SUCCEEDED(deviceResult)) *out = reinterpret_cast<IDirectInputDevice8A*>(&deviceCom);
    return deviceResult;
}
HRESULT __stdcall enumDevices(void*, DWORD kind, LPDIENUMDEVICESCALLBACKA callback, void* context, DWORD flags)
{
    enumVerified = kind == DI8DEVCLASS_GAMECTRL && flags == DIEDFL_ATTACHEDONLY;
    if (FAILED(enumResult)) return enumResult;
    const GUID* guids[] = {&GUID_XAxis, &GUID_YAxis, &GUID_ZAxis};
    for (unsigned int i = 0; i != 3; ++i)
    {
        DIDEVICEINSTANCEA instance = {};
        instance.dwSize = sizeof(instance); instance.guidInstance = *guids[i];
        ++enumerated;
        if (callback(&instance, context) == DIENUM_STOP) break;
    }
    return enumResult;
}
HRESULT __stdcall poll(void*) { ++pollCalls; return pollResult; }
HRESULT __stdcall readState(void*, DWORD size, void* out)
{ ++reads; if (size != sizeof(immediate)) std::abort(); if (SUCCEEDED(stateResult)) std::memcpy(out, &immediate, size); return stateResult; }
HRESULT __stdcall readData(void*, DWORD size, DIDEVICEOBJECTDATA* out, DWORD* count, DWORD flags)
{
    ++dataReads;
    if (size != 20 || *count != 128 || flags != 0) std::abort();
    if (FAILED(dataResult)) return dataResult;
    *count = DWORD(events.size());
    for (std::size_t i = 0; i != events.size(); ++i) out[i] = events[i];
    return dataResult;
}
DIDEVICEOBJECTDATA record(DWORD offset, DWORD value)
{ DIDEVICEOBJECTDATA r = {offset, value, 0, 0, 0}; return r; }
bool bytesAre(const void* pointer, std::size_t size, unsigned char value)
{
    const unsigned char* p = static_cast<const unsigned char*>(pointer);
    for (std::size_t i = 0; i != size; ++i) if (p[i] != value) return false;
    return true;
}
bool nearValue(float a, float b) { return std::fabs(a-b) < 0.000001f; }
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version, REFIID iid, void** out, IUnknown* outer)
{
    if (instance != GetModuleHandleA(nullptr) || version != 0x800 || iid != IID_IDirectInput8A || outer) std::abort();
    ++sdkCreates; lifecycle.push_back(1);
    if (SUCCEEDED(directResult)) *out = &inputCom;
    return directResult;
}

int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    for (void*& slot : deviceSlots) slot = reinterpret_cast<void*>(&unsupported);
    for (void*& slot : inputSlots) slot = reinterpret_cast<void*>(&unsupported);
    deviceSlots[2] = reinterpret_cast<void*>(&releaseDevice);
    deviceSlots[3] = reinterpret_cast<void*>(&getCaps);
    deviceSlots[4] = reinterpret_cast<void*>(&enumObjects);
    deviceSlots[6] = reinterpret_cast<void*>(&setProperty);
    deviceSlots[7] = reinterpret_cast<void*>(&acquire);
    deviceSlots[8] = reinterpret_cast<void*>(&unacquire);
    deviceSlots[9] = reinterpret_cast<void*>(&readState);
    deviceSlots[10] = reinterpret_cast<void*>(&readData);
    deviceSlots[11] = reinterpret_cast<void*>(&setFormat);
    deviceSlots[13] = reinterpret_cast<void*>(&setCoop);
    deviceSlots[25] = reinterpret_cast<void*>(&poll);
    inputSlots[2] = reinterpret_cast<void*>(&releaseInput);
    inputSlots[3] = reinterpret_cast<void*>(&createDevice);
    inputSlots[4] = reinterpret_cast<void*>(&enumDevices);
    unsigned int checks = 0, failures = 0;
    auto check = [&](bool condition, const char* name) { ++checks; if (!condition) { ++failures; std::printf("FAIL: %s\n", name); } };
    alignas(GameControllerDevice) unsigned char storage[sizeof(GameControllerDevice)];
    std::memset(storage, 0xA5, sizeof(storage));
    GameControllerDevice* controller = new(storage) GameControllerDevice;
    check(sizeof(*controller) == 188 && controller->getRef() == 1, "size and reference defaults");
    check(bytesAre(&controller->m_mode, 4, 0xA5) && bytesAre(&controller->m_numAxes, 4, 0xA5), "mode and axis count untouched");
    check(bytesAre(controller->m_povs, sizeof(controller->m_povs), 0xA5) && bytesAre(&controller->m_directInput, 4, 0xA5), "POV and DirectInput storage untouched");
    check(!controller->isActive() && !controller->m_device && !controller->m_capabilities && controller->getDeviceIndex() == -1, "owned/default state");
    check(controller->getNumButtons() == 0 && controller->getNumPOVs() == 0 && !controller->slot_0x70(), "tail virtual returns");
    check(controller->queryInterface(IID_IBase) == controller && controller->queryInterface(IID_IInputDevice) == controller && controller->queryInterface(IID_IAdvancedInputDevice) == controller && !controller->queryInterface(0), "interface identities");
    check(controller->getType() == IDTGameController, "controller type");
    controller->addRef(); check(controller->release() == 1, "reference round trip");
    check(controller->init(1) && sdkCreates == 0, "null manager returns true without SDK call");
    check(!controller->activate() && !controller->deActivate(), "null device activate/deactivate");
    inputDeviceManager = controller;
    check(controller->init(1), "full init through enumerations");
    check(enumVerified && enumerated == 2 && deviceCreates == 1 && createVerified, "select requested controller index");
    check(formatVerified && bufferVerified && rangesVerified && rangeCalls == 5, "SDK format/buffer/axis property contracts");
    check(lifecycle == std::vector<int>({1,2,3,4,5,6}), "init call order");
    check(controller->getNumAxes() == 5 && controller->getNumButtons() == 11 && controller->getNumPOVs() == 2, "capability counts plus POV buttons");
    check(controller->m_axisMask == (1u|4u|16u|64u) && controller->m_numSliders == 2, "sparse axes and sliders");
    check(controller->getCooperativeMode() == IDM_EXCLUSIVE && cooperativeFlags == 5 && controller->getDeviceIndex() == -1, "mode store and untouched device index");
    check(controller->setCooperativeMode(IDM_NONEXCLUSIVE) && cooperativeFlags == 6, "nonexclusive foreground mode");
    cooperativeResult = E_FAIL;
    check(!controller->setCooperativeMode(IDM_EXCLUSIVE) && controller->getCooperativeMode() == IDM_NONEXCLUSIVE, "failed mode preserves old value");
    cooperativeResult = DI_OK;
    acquireResult = E_FAIL;
    check(controller->activate() && !controller->isActive(), "failed acquisition still returns true");
    acquireResult = DI_OK;
    check(controller->activate() && controller->isActive(), "successful acquisition");
    int before = acquireCalls;
    check(controller->activate() && acquireCalls == before + 1, "active controller reacquires on every activate");

    controller->setAxisScale(static_cast<InputDeviceAxes>(5), 7);
    controller->setAxisAdd(static_cast<InputDeviceAxes>(5), 8);
    controller->setInvertAxis(static_cast<InputDeviceAxes>(5), true);
    check(controller->getAxisScale(static_cast<InputDeviceAxes>(5)) == 7 && controller->getAxisAdd(static_cast<InputDeviceAxes>(5)) == 8 && controller->getInvertAxis(static_cast<InputDeviceAxes>(5)), "equal-to-count axis accepted");
    check(controller->getAxisScale(static_cast<InputDeviceAxes>(6)) == 0 && controller->getAxisAdd(static_cast<InputDeviceAxes>(6)) == 0 && !controller->getInvertAxis(static_cast<InputDeviceAxes>(6)), "greater-than-count axis rejected");
    controller->m_axes[0] = 0.25f; controller->setAxisAdd(IDA_X, 2); controller->setInvertAxis(IDA_X, true);
    check(controller->readAxis(IDA_X, true) == 1.75f && controller->readAxis(IDA_X, false) == 1.75f, "readAxis ignores bool and adds after invert");
    float x = 1, y = 2; controller->peakXYAxis(x, y);
    check(x == 0 && y == 0 && controller->buttonPressedSampleLatency(IDB_NONE) == 0, "real zero peak and latency defaults");

    std::memset(&immediate, 0, sizeof(immediate));
    for (DWORD& pov : immediate.rgdwPOV) pov = 0xFFFF;
    immediate.lX = 500; immediate.lZ = -250; immediate.lRy = 1000;
    immediate.rglSlider[0] = -500; immediate.rglSlider[1] = 100;
    immediate.rgbButtons[1] = 0x80; immediate.rgdwPOV[0] = 9000; immediate.rgdwPOV[1] = 27000;
    controller->m_axisScale[0] = 2; controller->m_axisScale[1] = 3; controller->m_axisScale[2] = 0.5f; controller->m_axisScale[3] = 1; controller->m_axisScale[4] = -2;
    events = {record(DIJOFS_BUTTON(0),0x80), record(DIJOFS_BUTTON(0),0), record(DIJOFS_POV(0),0), record(DIJOFS_POV(1),4500)};
    controller->update(0.0f);
    check(nearValue(controller->m_axes[0],1) && nearValue(controller->m_axes[1],-0.75f) && nearValue(controller->m_axes[2],0.5f) && nearValue(controller->m_axes[3],-0.5f) && nearValue(controller->m_axes[4],-0.2f), "sparse axis packing and slider scale");
    check(controller->m_buttons == ((1u<<0)|(1u<<1)|(1u<<3)|(1u<<4)|(1u<<10)), "buffered quick press merges with state/cardinal POV");
    check(controller->buttonPressed(static_cast<InputDeviceButtons>(1u<<0)) && controller->readPOV(static_cast<InputDevicePOVs>(1)) == 27000 && controller->readPOV(static_cast<InputDevicePOVs>(2)) == -1, "press transition and POV read bounds");
    before = pollCalls; controller->update(-0.1f); check(pollCalls == before, "negative delta skips polling");
    controller->update(std::numeric_limits<float>::quiet_NaN()); check(pollCalls == before + 1, "NaN delta still polls");
    pollResult = E_FAIL; inputLostCount = 2; before = acquireCalls;
    controller->update(0.01f); check(acquireCalls == before + 3 && inputLostCount == 0, "reacquire loops only while input lost");
    pollResult = DI_OK; stateResult = E_FAIL; const auto savedButtons = controller->m_buttons; before = dataReads;
    controller->update(0.01f); check(controller->m_buttons == savedButtons && dataReads == before, "failed state leaves sample untouched");
    stateResult = DI_OK;
    std::memset(immediate.rgbButtons, 0, sizeof(immediate.rgbButtons));
    immediate.rgdwPOV[0] = 0x1234FFFF; immediate.rgdwPOV[1] = 4500; events.clear();
    controller->update(0.01f);
    check(controller->m_buttons == 0 && controller->m_povs[0] == -1 && controller->m_povs[1] == 4500, "POV low-word neutral and diagonal exclusion");
    check(controller->buttonReleased(static_cast<InputDeviceButtons>(1)), "button release transition");
    dataResult = DI_BUFFEROVERFLOW; events = {record(DIJOFS_BUTTON(2), 0x80)};
    controller->update(0.01f); check(controller->m_buttons == 4, "positive buffer-overflow result still consumes records");
    dataResult = DI_OK; events.clear();
    controller->m_numButtons = 35; immediate.rgbButtons[34] = 0x80;
    controller->update(0.01f); check(controller->m_buttons == 4, "x86 button mask wraps beyond bit31");
    controller->m_numButtons = 3; immediate.rgbButtons[34] = 0;
    controller->m_previousButtons = 0;

    InputDeviceAxes axis = IDA_X; InputDeviceButtons button = IDB_NONE; InputDeviceKeys key = IDK_NONE; bool negative = true;
    check(controller->getNextInput(axis,button,key,negative) && int(axis) == 15 && int(button) == 4 && negative, "button precedence leaves direction output alone");
    controller->m_buttons = controller->m_previousButtons = 0;
    for (float& value : controller->m_axes) value = 0;
    check(!controller->getNextInput(axis,button,key,negative) && controller->m_axisReady[0], "neutral arms axis capture");
    controller->m_axes[0] = 0.95f;
    check(!controller->getNextInput(axis,button,key,negative), "capture threshold is strict");
    controller->m_axes[0] = -0.96f;
    check(controller->getNextInput(axis,button,key,negative) && axis == IDA_X && negative && !int(button) && !int(key), "negative full excursion captures axis");
    check(!controller->getNextInput(axis,button,key,negative), "capture requires rearming");
    controller->m_axes[0] = std::numeric_limits<float>::quiet_NaN();
    check(!controller->getNextInput(axis,button,key,negative) && !controller->m_axisReady[0], "NaN axis neither arms nor captures");
    controller->m_povsAsButtons = false;
    check(controller->getNumButtons() == 3, "POV reporting toggle affects count");
    check(controller->deActivate() && !controller->isActive(), "deactivate clears active state");
    lifecycle.clear(); controller->~GameControllerDevice();
    check(lifecycle == std::vector<int>({7,8,9}) && deviceReleases == 1 && inputReleases == 1, "destructor unacquires then releases device and input");

    // Failure coverage uses the real Debug filter to permit the retail post-log
    // branches; no diagnostic or controller method is replaced.
    turnOffAllDebug(DebugTypeError);
    GameControllerDevice* failed = new GameControllerDevice;
    failed->m_directInput = nullptr; // [TEMP] model zeroed pre-init ownership for failure cleanup
    directResult = DIERR_INVALIDPARAM;
    check(!failed->init(1), "failed DirectInput creation returns false after filtered diagnostics");
    directResult = DI_OK; deviceResult = E_FAIL; enumerated = 0;
    check(!failed->init(1) && enumerated == 2, "failed selected device creation still stops enumeration");
    failed->release();
    std::printf("GameControllerDevice: %u/%u checks passed; strict x86 link, complete31-slot table.\n", checks-failures, checks);
    return failures ? 1 : 0;
}
