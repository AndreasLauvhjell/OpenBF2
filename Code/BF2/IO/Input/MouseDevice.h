// Mouse device with a 28-slot interface and no virtual destructor. [INFERRED] Header placement.
#ifndef IO_INPUT_MOUSEDEVICE_H
#define IO_INPUT_MOUSEDEVICE_H

#include "../../Core/BuildConfig.h"
#if !DEDICATED
#include "Input.h"
#include <windows.h>
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
#include <list>

namespace dice { namespace hfe { namespace io {

class MouseDevice : public IAdvancedInputDevice
{
public:
    MouseDevice();
    ~MouseDevice();                                     // 0x6D80E0; nonvirtual
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int iid) const override; // +0C, 0x6D6A40
    InputDeviceTypes getType() const override;
    bool setCooperativeMode(InputDeviceMode mode) override;
    InputDeviceMode getCooperativeMode() const override;
    bool activate() override;                           // +1C, 0x6D7980
    bool deActivate() override;
    void update(float deltaTime) override;
    bool isActive() const override;
    bool getNextInput(InputDeviceAxes&, InputDeviceButtons&,
                      InputDeviceKeys&, bool&) override; // +2C, 0x6D7AB0
    int getNumAxes() const override;
    float readAxis(InputDeviceAxes axis, bool) const override;
    void peakXYAxis(float& x, float& y) override;
    void setInvertAxis(InputDeviceAxes axis, bool invert) override; // +3C, 0x6D7A10
    bool getInvertAxis(InputDeviceAxes axis) const override;
    void setAxisScale(InputDeviceAxes axis, float scale) override;
    float getAxisScale(InputDeviceAxes axis) const override;
    void setAxisAdd(InputDeviceAxes axis, float add) override; // +4C, 0x5A59F0
    float getAxisAdd(InputDeviceAxes axis) const override;
    int getNumButtons() const override;
    bool buttonDown(InputDeviceButtons button) const override;
    bool buttonPressed(InputDeviceButtons button) const override; // +5C, 0x6D6B60
    bool buttonReleased(InputDeviceButtons button) const override;
    float buttonPressedSampleLatency(InputDeviceButtons button) const override;
    virtual bool init();
    // Name [INFERRED]; distinct name keeps MSVC's virtual-overload grouping
    // from moving this appended hwnd overload in front of the generic +14.
    virtual bool setCooperativeModeForWindow(HWND, InputDeviceMode); // +6C, 0x6D7940

private:
    bool initDirectInput();                             // 0x6D7BA0; name [INFERRED]
    void updateSamples(float deltaTime);                // 0x6D8300; name [INFERRED]

    // Retail offsets below are evidence, not native STL layout assertions.
    int m_refCount;                                     // +04
    InputDeviceMode m_mode;                             // +08, untouched by ctor
    unsigned int m_numButtons;                          // +0C, assigned by init
    unsigned int m_buttons, m_previousButtons;          // +10/+14
    float m_axes[4];                                    // +18
    bool m_axisArmed[4];                                // +28
    int m_numAxes;                                      // +2C, assigned by init
    bool m_active;                                     // +30
    float m_axisScale[4];                               // +34
    bool m_invertAxis[4];                               // +44
    float m_buttonLatency[32];                          // +48, untouched by ctor
    DWORD m_pollTime, m_sampleTime;                      // +C8/+CC; CC untouched
    IDirectInputDevice8A* m_device;                      // +D0
    IDirectInput8A* m_directInput;                       // +D4, untouched by ctor
    std::list<DIDEVICEOBJECTDATA> m_samples;             // +D8
    std::list<DIDEVICEOBJECTDATA> m_peakSamples;         // +E4
    DWORD m_field0f0, m_field0f4, m_field0f8;             // [UNRESOLVED] initialized zero
    bool m_field0fc;                                    // [UNRESOLVED] initialized false
    DWORD m_lastSequence;                               // +100
};

} } }
#endif
#endif
