// Null input service for images without devices. Header path [INFERRED].
// Initialization and activation succeed without creating devices; counts are zero and lookups null.
// initDevices leaves its output argument unchanged. The live device map remains empty.
// There is no virtual destructor; release destroys the concrete object and its map.

#ifndef IO_INPUT_NULLINPUTDEVICEMANAGER_H       // [INFERRED] guard style, per Core/IBase.h
#define IO_INPUT_NULLINPUTDEVICEMANAGER_H

#include <map>

#include "Input.h"
#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace io {

// Headless input-device manager used by the dedicated server.
class NullInputDeviceManager : public IInputDeviceManager
{
public:
    // Creates an empty device map with one reference.
    NullInputDeviceManager();                                   // [inlined in ELF] - see create()

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;       // 0x84A1830  - _ZNK..., const

    // IInputDeviceManager, +0x10..+0x2C
    virtual bool          initDevices(unsigned int* deviceMask);
    virtual IInputDevice* getInputDevice(InputDeviceFlags flags) const;
    virtual unsigned int  getInputDevices() const;
    virtual bool          activateDevices(unsigned int deviceMask);
    virtual bool          deActivateDevices(unsigned int deviceMask);
    virtual void          update(float deltaTime);
    virtual unsigned int  getActiveInputDevices() const;
    virtual void*         getAllDevices();

    // The factory ClassManager::createInstance calls.
    // _ZN4dice3hfe2io22NullInputDeviceManager6createEjPNS0_5IBaseE.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    /* 0x04 */ int                                        m_refCount;
    /* 0x08 */ std::map<InputDeviceFlags, IInputDevice*>  m_devices;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_INPUT_NULLINPUTDEVICEMANAGER_H
