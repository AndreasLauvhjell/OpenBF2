// Device manager used when no physical input devices are available. [INFERRED] Source placement.

#include "NullInputDeviceManager.h"
#include "../../Core/Ids.h"                // [INFERRED] header name; the constants are attested

namespace dice {
namespace hfe {
namespace io {

// Initializes an empty device registry and one owned reference.
NullInputDeviceManager::NullInputDeviceManager()
    : m_refCount(1)             // 0x850BB05, inlined into create
{
    // m_devices default-constructs: the header-node allocation at 0x850BB16 and the four stores
    // that follow are std::map's, not statements dice wrote.
}

// IBase quartet

// Core/IBase.h declares addRef() void.
void NullInputDeviceManager::addRef()
{
    ++m_refCount;
}


int NullInputDeviceManager::getRef()
{
    return m_refCount;
}

// Releases a reference and destroys the registry at zero. Returns the remaining count.
int NullInputDeviceManager::release()
{
    const int count = m_refCount - 1;
    m_refCount = count;

    if (count == 0)
        delete this;            // non-virtual; the map teardown is inlined above it

    return count;
}

// The uniform two-id shape.
void* NullInputDeviceManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<NullInputDeviceManager*>(this);
    if (iid == IID_IInputDeviceManager)
        return const_cast<NullInputDeviceManager*>(this);
    return 0;
}

// IInputDeviceManager - eight slots. Three say "yes", four say "nothing", one hands out the map.

// Returns true and leaves deviceMask unchanged; the headless manager opens no devices.
bool NullInputDeviceManager::initDevices(unsigned int* /*deviceMask*/)
{
    return true;
}

// - `return 0`. Never consults m_devices.
IInputDevice* NullInputDeviceManager::getInputDevice(InputDeviceFlags /*flags*/) const
{
    return 0;
}

// - `return 0`.
unsigned int NullInputDeviceManager::getInputDevices() const
{
    return 0;
}

// - `mov eax, 1 / ret`. Succeeds and does nothing.
bool NullInputDeviceManager::activateDevices(unsigned int /*deviceMask*/)
{
    return true;
}

// - `mov eax, 1 / ret`. Succeeds and does nothing.
bool NullInputDeviceManager::deActivateDevices(unsigned int /*deviceMask*/)
{
    return true;
}

// - empty. The server ticks this every frame and nothing happens.
void NullInputDeviceManager::update(float /*deltaTime*/)
{
}

// - `return 0`.
unsigned int NullInputDeviceManager::getActiveInputDevices() const
{
    return 0;
}

// Returns a borrowed pointer to the empty device map.
void* NullInputDeviceManager::getAllDevices()
{
    return &m_devices;
}

// Creates the requested null-device-manager interface with one reference. Aggregation is unsupported.
IBase* NullInputDeviceManager::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    NullInputDeviceManager* self = new NullInputDeviceManager();    // 0x850BAF0, operator new(0x14u)
    void* itf = self->queryInterface(iid);                          // virtual, slot +0x0C
    if (itf == 0)
        self->release();                                            // virtual, slot +0x08
    return static_cast<IBase*>(itf);
}

} // namespace io
} // namespace hfe
} // namespace dice
