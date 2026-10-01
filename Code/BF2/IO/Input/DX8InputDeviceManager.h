// [INFERRED] class/file spelling from the attested registry name
// "dice.hfe.io.InputDeviceManager.DX8"; BF2.exe primary table 0x9146FC.
#ifndef IO_INPUT_DX8INPUTDEVICEMANAGER_H
#define IO_INPUT_DX8INPUTDEVICEMANAGER_H
#include "../../Core/BuildConfig.h"
#if !DEDICATED
#include "Input.h"
#include <map>
namespace dice { namespace hfe { namespace io {
class DX8InputDeviceManager : public IInputDeviceManager
{
public:
    DX8InputDeviceManager();
    ~DX8InputDeviceManager();
    void addRef() override;
    int getRef() override;
    int release() override;
    void* queryInterface(unsigned int iid) const override;
    bool initDevices(unsigned int* deviceMask) override;
    IInputDevice* getInputDevice(InputDeviceFlags flags) const override;
    unsigned int getInputDevices() const override;
    bool activateDevices(unsigned int mask) override;
    bool deActivateDevices(unsigned int mask) override;
    void update(float dt) override;
    unsigned int getActiveInputDevices() const override;
    void* getAllDevices() override;
    static IBase* create(unsigned int iid, IBase* outer);
private:
    int m_refCount;
    std::map<InputDeviceFlags, IInputDevice*> m_devices;
    unsigned int m_activeDevices;
};
} } }
#endif
#endif
