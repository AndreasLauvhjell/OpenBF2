// Header [INFERRED] beside the attested IO/Input/KeyboardDevice.cpp.
#ifndef IO_INPUT_KEYBOARDDEVICE_H
#define IO_INPUT_KEYBOARDDEVICE_H
#include "../../Core/BuildConfig.h"
#if !DEDICATED
#include "Input.h"
#include <list>
#include <windows.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>

namespace dice { namespace hfe { namespace io {
class KeyboardDevice : public IKeyboardDevice
{
public:
    KeyboardDevice();
    ~KeyboardDevice();                                     // 0x6AE400, nonvirtual
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int iid) const override;
    InputDeviceTypes getType() const override;
    bool setCooperativeMode(InputDeviceMode mode) override;
    InputDeviceMode getCooperativeMode() const override;
    bool activate() override;
    bool deActivate() override;
    void update(float dt) override;
    bool isActive() const override;
    bool getNextInput(InputDeviceAxes&, InputDeviceButtons&, InputDeviceKeys&, bool&) override;
    bool keyDown(InputDeviceKeys key) const override;
    bool keyPressed(InputDeviceKeys key) const override;
    bool keyReleased(InputDeviceKeys key) const override;
    std::wstring getKeyName(InputDeviceKeys key) const override;
    virtual bool init();                                    // +0x40
    // Distinct name preserves MSVC slot order (overloads otherwise group).
    virtual bool setCooperativeModeForWindow(HWND, InputDeviceMode); // +0x44

private:
    bool initDirectInput();
    void updateCurrentState(float dt);
    int m_refCount;
    InputDeviceMode m_mode;                                 // constructor leaves untouched
    std::list<DIDEVICEOBJECTDATA> m_events;
    unsigned char m_keys[256], m_previousKeys[256];          // untouched until sampled/cleared
    IDirectInputDevice8A* m_device;
    IDirectInput8A* m_directInput;                           // constructor leaves untouched
    bool m_active;
};
} } }
#endif
#endif
