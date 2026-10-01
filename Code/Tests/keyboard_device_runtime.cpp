// [TEMP] [CHOSEN, not recovered] COM fixture for the real KeyboardDevice. No
// hardware is acquired. Every unsupported COM operation aborts the fixture.
#include <string>
#include <vector>
#include <list>
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#define private public
#include "../BF2/IO/Input/KeyboardDevice.h"
#undef private
#include "../BF2/Core/Ids.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

namespace dice { namespace hfe {
class BF2Render;
BF2Render* g_bf2Render = 0;
namespace io { IBase* inputDeviceManager = 0; }
} }
using namespace dice::hfe;
using namespace dice::hfe::io;

namespace {
void* deviceSlots[32];
void* inputSlots[11];
struct FakeCom { void** slots; } deviceCom = {deviceSlots}, inputCom = {inputSlots};
HRESULT acquireResult = DI_OK, readResult = DI_OK, cooperativeResult = DI_OK;
bool loseInputOnce = false;
int acquireCalls = 0, unacquireCalls = 0, deviceReleases = 0, inputReleases = 0;
int createCalls = 0, readCalls = 0;
DWORD cooperativeFlags = 0;
bool formatVerified = false, bufferVerified = false, guidVerified = false;
std::vector<DIDEVICEOBJECTDATA> records;
void __stdcall unsupported() { std::fprintf(stderr, "unsupported fixture COM call\n"); std::abort(); }
ULONG __stdcall releaseDevice(void*) { ++deviceReleases; return 0; }
ULONG __stdcall releaseInput(void*) { ++inputReleases; return 0; }
HRESULT __stdcall acquire(void*) { ++acquireCalls; return acquireResult; }
HRESULT __stdcall unacquire(void*) { ++unacquireCalls; return DI_OK; }
HRESULT __stdcall setCoop(void*, HWND, DWORD flags) { cooperativeFlags = flags; return cooperativeResult; }
HRESULT __stdcall setFormat(void*, const DIDATAFORMAT* format)
{ formatVerified = format == &c_dfDIKeyboard; return DI_OK; }
HRESULT __stdcall setProperty(void*, REFGUID property, const DIPROPHEADER* header)
{
    const DIPROPDWORD* p = reinterpret_cast<const DIPROPDWORD*>(header);
    bufferVerified = &property == &DIPROP_BUFFERSIZE && p->dwData == 100 &&
        header->dwSize == 20 && header->dwHeaderSize == 16 && header->dwObj == 0 && header->dwHow == 0;
    return DI_OK;
}
HRESULT __stdcall getProperty(void*, REFGUID property, DIPROPHEADER* header)
{
    if (&property != &DIPROP_KEYNAME || header->dwObj != 28 || header->dwHow != DIPH_BYOFFSET)
        std::abort();
    std::wcscpy(reinterpret_cast<DIPROPSTRING*>(header)->wsz, L"Return");
    return DI_OK;
}
HRESULT __stdcall createDevice(void*, REFGUID guid, IDirectInputDevice8A** out, IUnknown* outer)
{ guidVerified = guid == GUID_SysKeyboard && !outer; *out = reinterpret_cast<IDirectInputDevice8A*>(&deviceCom); return DI_OK; }
HRESULT __stdcall readData(void*, DWORD size, DIDEVICEOBJECTDATA* out, DWORD* count, DWORD flags)
{
    ++readCalls;
    if (size != 20 || *count != 100 || flags != 0) std::abort();
    if (loseInputOnce) { loseInputOnce = false; return DIERR_INPUTLOST; }
    *count = static_cast<DWORD>(records.size());
    for (size_t i = 0; i < records.size(); ++i) out[i] = records[i];
    return readResult;
}
DIDEVICEOBJECTDATA record(DWORD key, DWORD data, DWORD stamp)
{ DIDEVICEOBJECTDATA r = {key, data, stamp, 0, 0}; return r; }
}

extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE instance, DWORD version, REFIID iid, void** out, IUnknown* outer)
{
    if (instance != GetModuleHandleA(0) || version != 0x800 || iid != IID_IDirectInput8A || outer) std::abort();
    ++createCalls;
    *out = &inputCom;
    return DI_OK;
}

int main()
{
    for (void*& entry : deviceSlots) entry = reinterpret_cast<void*>(&unsupported);
    for (void*& entry : inputSlots) entry = reinterpret_cast<void*>(&unsupported);
    deviceSlots[2] = reinterpret_cast<void*>(&releaseDevice);
    deviceSlots[5] = reinterpret_cast<void*>(&getProperty);
    deviceSlots[6] = reinterpret_cast<void*>(&setProperty);
    deviceSlots[7] = reinterpret_cast<void*>(&acquire);
    deviceSlots[8] = reinterpret_cast<void*>(&unacquire);
    deviceSlots[10] = reinterpret_cast<void*>(&readData);
    deviceSlots[11] = reinterpret_cast<void*>(&setFormat);
    deviceSlots[13] = reinterpret_cast<void*>(&setCoop);
    inputSlots[2] = reinterpret_cast<void*>(&releaseInput);
    inputSlots[3] = reinterpret_cast<void*>(&createDevice);
    unsigned checks = 0, failures = 0;
    auto check = [&](bool condition, const char* name) {
        ++checks; if (!condition) { ++failures; std::printf("FAIL: %s\n", name); }
    };
    alignas(KeyboardDevice) unsigned char storage[sizeof(KeyboardDevice)];
    std::memset(storage, 0xA5, sizeof(storage));
    KeyboardDevice* keyboard = new(storage) KeyboardDevice;
    check(keyboard->m_directInput == reinterpret_cast<IDirectInput8A*>(0xA5A5A5A5u), "constructor retains untouched DirectInput pointer");
    check(keyboard->m_keys[28] == 0xA5 && keyboard->m_previousKeys[28] == 0xA5, "constructor leaves both key arrays untouched");
    check(keyboard->getRef() == 1 && keyboard->m_device == 0 && !keyboard->isActive(), "initial owned state");
    check(!keyboard->init() && createCalls == 0, "null global blocks init before SDK");
    check(!keyboard->activate() && !keyboard->deActivate(), "null device activation results");
    check(keyboard->queryInterface(IID_IBase) == keyboard && keyboard->queryInterface(IID_IInputDevice) == keyboard &&
          keyboard->queryInterface(IID_IKeyboardDevice) == keyboard && !keyboard->queryInterface(0), "queryInterface quartet");
    check(keyboard->getType() == IDTKeyboard && IDF_KEYBOARD == 2 && IDF_MOUSE == 1, "retail input types and mask bits");
    keyboard->addRef(); check(keyboard->release() == 1, "reference round trip");
    inputDeviceManager = keyboard;
    check(keyboard->init() && createCalls == 1, "real initialization sequence");
    check(guidVerified && formatVerified && bufferVerified, "keyboard GUID format and buffering");
    check(cooperativeFlags == 22 && keyboard->getCooperativeMode() == IDM_NONEXCLUSIVE, "init cooperative mode");
    check(keyboard->m_keys[28] == 0 && keyboard->m_previousKeys[28] == 0xA5, "init clears only current state");
    check(keyboard->getKeyName(static_cast<InputDeviceKeys>(28)) == L"Return", "wide key name property");
    check(keyboard->setCooperativeModeForWindow(0, IDM_EXCLUSIVE) && cooperativeFlags == 21, "exclusive window flags");
    cooperativeResult = E_FAIL;
    check(!keyboard->setCooperativeModeForWindow(0, IDM_NONEXCLUSIVE) && keyboard->getCooperativeMode() == IDM_EXCLUSIVE, "failed cooperative mode does not store");
    cooperativeResult = DI_OK;
    acquireResult = E_FAIL;
    check(keyboard->activate() && !keyboard->isActive(), "failed Acquire still returns true");
    acquireResult = DI_OK;
    check(keyboard->activate() && keyboard->isActive(), "successful acquisition");
    const int beforeAcquire = acquireCalls;
    check(keyboard->activate() && acquireCalls == beforeAcquire, "active device does not reacquire");
    records = {record(28, 0x80, 1000), record(255, 0x80, 1001), record(56, 0x80, 1002)};
    keyboard->update(0.0f);
    check(keyboard->m_events.size() == 2 && !keyboard->keyDown(static_cast<InputDeviceKeys>(28)), "poll queues valid offsets without sampling");
    keyboard->update(0.01f);
    check(keyboard->keyDown(static_cast<InputDeviceKeys>(28)) && keyboard->keyPressed(static_cast<InputDeviceKeys>(28)), "positive tick folds press");
    check(keyboard->m_events.empty() && keyboard->keyDown(static_cast<InputDeviceKeys>(56)), "sample boundary consumes both accepted events");
    check(keyboard->keyDown(static_cast<InputDeviceKeys>(284)), "key index truncates to byte");
    InputDeviceAxes axis = IDA_X; InputDeviceButtons button = static_cast<InputDeviceButtons>(99);
    InputDeviceKeys key = static_cast<InputDeviceKeys>(99); bool analogue = true;
    check(keyboard->getNextInput(axis, button, key, analogue) && axis == 15 && button == 0 && key == 28 && analogue,
          "capture writes first key and leaves bool untouched");
    keyboard->update(0.01f);
    check(!keyboard->keyPressed(static_cast<InputDeviceKeys>(28)), "empty sample tick advances previous state");
    records = {record(28, 0, 2000)}; keyboard->update(-1.0f); keyboard->update(0.01f);
    check(keyboard->keyReleased(static_cast<InputDeviceKeys>(28)), "queued release transition");
    const int beforeRead = readCalls;
    keyboard->update(std::numeric_limits<float>::quiet_NaN());
    check(readCalls == beforeRead + 1, "NaN polls rather than sampling");
    keyboard->m_events.clear();
    keyboard->m_events.push_back(record(30, 0x80, 3000));
    keyboard->m_events.push_back(record(31, 0x80, 3001));
    keyboard->update(0.001f);
    check(keyboard->keyDown(static_cast<InputDeviceKeys>(30)) && !keyboard->keyDown(static_cast<InputDeviceKeys>(31)) && keyboard->m_events.size() == 1,
          "timestamp equality remains queued");
    keyboard->m_events.clear();
    keyboard->m_events.push_back(record(32, 0x80, 0xfffffffdu)); keyboard->update(0.01f);
    check(keyboard->m_events.size() == 1 && !keyboard->keyDown(static_cast<InputDeviceKeys>(32)), "DWORD wrap preserves unsigned comparison");
    readResult = DI_BUFFEROVERFLOW; keyboard->update(0.0f);
    check(keyboard->m_events.empty() && !keyboard->keyDown(static_cast<InputDeviceKeys>(56)), "overflow clears queue and current keys");
    readResult = DI_OK; loseInputOnce = true; records = {record(40, 0x80, 4000)};
    const int beforeRetry = readCalls; keyboard->update(0.0f);
    check(readCalls == beforeRetry + 2 && keyboard->m_events.size() == 1, "input loss clears then reacquires and retries");
    acquireResult = E_FAIL; loseInputOnce = true; keyboard->update(0.0f);
    check(keyboard->m_events.empty() && keyboard->isActive(), "failed retry Acquire leaves active flag unchanged");
    keyboard->m_keys[40] = 1;
    check(keyboard->deActivate() && !keyboard->isActive() && !keyboard->m_keys[40], "deactivation clears current keys");
    const int beforeUnacquire = unacquireCalls;
    keyboard->m_keys[40] = 1;
    check(keyboard->deActivate() && unacquireCalls == beforeUnacquire && keyboard->m_keys[40] == 1, "inactive deactivation preserves state");
    keyboard->~KeyboardDevice();
    check(deviceReleases == 1 && inputReleases == 1 && unacquireCalls == beforeUnacquire + 1, "destructor independently unacquires and releases both COM objects");
    inputDeviceManager = 0;
    std::printf("KeyboardDevice: %u/%u checks passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
