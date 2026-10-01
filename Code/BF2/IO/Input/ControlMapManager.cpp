// Creates and manages named control maps. [INFERRED] Source placement.

#include "ControlMapManager.h"
#include "../../Core/ClassManager.h"       // create() builds a ControlMap through the class manager
#include "../../Core/Ids.h"                // [INFERRED] header name; the constants are attested

#include <utility>                         // std::make_pair - create()'s m_byId insert

namespace dice {
namespace hfe {

extern ClassManager* classManager;         // ds:0x8AC85A0, Core/Globals.cpp

namespace io {

// Keep this declaration's type consistent with the singleton definition; MSVC mangles variable types.
extern IBase* controlMapManager;        // ds:0x8AC8C8C - type must match Core/Globals.cpp

// constructor / destructor

// Initializes empty id/name registries with no active map and one owned reference.
ControlMapManager::ControlMapManager()
    : m_refCount(1)
    , m_byId()                  // 0x84970CC + header node alloc at 0x84970EE
    , m_nextId(0)
    , m_byName(3011)            // 0x849712B (m_maxSize) / 0x8497141 (operator new[](0x2F0C))
    , m_active(0)
{
}

// [CHOSEN-STUB] Container storage is released, but retained control maps are not.
// Complete the original map-release lifecycle before relying on destruction to free maps.
ControlMapManager::~ControlMapManager()
{
    // [CHOSEN-STUB] - the real body is at 0x8497340.
}

// IBase quartet - all four

// Core/IBase.h declares addRef() void.
void ControlMapManager::addRef()
{
    ++m_refCount;
}


int ControlMapManager::getRef()
{
    return m_refCount;
}

// Dispatches `(*(*this + 72))(this)` - vtable slot +0x48, the deleting
// destructor.
int ControlMapManager::release()
{
    const int count = m_refCount - 1;
    const bool dead = (m_refCount == 1);
    m_refCount = count;

    if (dead)
        delete this;                    // virtual, slot +0x48

    return count;
}

// The uniform two-id shape.
void* ControlMapManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<ControlMapManager*>(this);
    if (iid == IID_IControlMapManager)
        return const_cast<ControlMapManager*>(this);
    return 0;
}

// IControlMapManager

// Updates every registered control map by deltaTime, including inactive maps.
void ControlMapManager::update(float deltaTime)
{
    for (std::map<int, IControlMap*>::iterator it = m_byId.begin(); it != m_byId.end(); ++it)
        it->second->update(deltaTime);          // 0x84974A2, virtual slot +0x7C
}

// Creates and activates a named map, or activates the existing map with that name.
// Creation failure returns null and clears the active map.
IControlMap* ControlMapManager::create(const std::string& name)
{
    if (m_byName.exists(name, m_byName.bucketOf(name)))         // 0x84978C6, inlined
    {
        const IControlMap* existing = 0;
        m_byName.get(name, existing);                           // 0x8499720, out of line here
        setActive(existing);                                    // slot +0x30, virtual
        return const_cast<IControlMap*>(existing);
    }

    IControlMap* map = static_cast<IControlMap*>(
        classManager->createInstance(CID_ControlMap, IID_IControlMap, 0));   // slot +0x34
    if (!map)
    {
        setActive(static_cast<const IControlMap*>(0));           // still dispatched on failure
        return 0;
    }

    map->setName(name);                                          // slot +0x10

    const unsigned int bucket = m_byName.bucketOf(name);         // 0x8497927, recomputed
    if (!m_byName.exists(name, bucket))                          // asked again; cannot be true
        m_byName.insert(name, map, bucket);

    map->setID(m_nextId++);                                      // slot +0x1C, +0x14 post-increment
    m_byId.insert(std::make_pair(map->getID(), map));            // slot +0x18, the id read back
    setActive(map);                                              // slot +0x30
    return map;
}

// Returns the named control map, or null for an empty or unknown name.
IControlMap* ControlMapManager::getControlMap(const std::string& name) const
{
    const IControlMap* map = 0;                 // 0x84974D6, zeroed before the guard
    if (name.empty())
        return 0;
    m_byName.get(name, map);                    // inlined at 0x84974F0..0x849759D
    return const_cast<IControlMap*>(map);
}

// getControlMap(int) - A plain map find; miss returns 0.
IControlMap* ControlMapManager::getControlMap(int id) const
{
    std::map<int, IControlMap*>::const_iterator it = m_byId.find(id);
    if (it == m_byId.end())
        return 0;
    return it->second;
}

// [CHOSEN-STUB] - by the shape of its 46 bytes it is the same
// find-then-forward as the int overload below, via slot +0x18; not read, so not asserted.
void ControlMapManager::deleteControlMap(const std::string& /*name*/)
{
}

// Finds and deletes the map by id through virtual dispatch so overrides participate in both steps.
void ControlMapManager::deleteControlMap(int id)
{
    IControlMap* map = this->getControlMap(id);         // slot +0x1C, virtual
    this->deleteControlMap(map);                        // slot +0x28, virtual
}

// [CHOSEN-STUB] - the real removal: unlink from both containers and
// release the map.
void ControlMapManager::deleteControlMap(const IControlMap* /*map*/)
{
}

// setActive(const std::string&) - Same two-virtual-call
// shape as deleteControlMap(int):
//     v2 = (*(*this + 24))(this, name);    slot +0x18 = getControlMap(const std::string&)
//     return (*(*this + 48))(this, v2);    slot +0x30 = setActive(const IControlMap*)
void ControlMapManager::setActive(const std::string& name)
{
    IControlMap* map = this->getControlMap(name);       // slot +0x18, virtual
    this->setActive(map);                               // slot +0x30, virtual
}

// Selects map and immediately applies its tuning settings.
void ControlMapManager::setActive(const IControlMap* map)
{
    m_active = const_cast<IControlMap*>(map);           // const discarded, as in the ELF
    this->applyActive();                                // 0x84977E6, virtual slot +0x40
}


IControlMap* ControlMapManager::getActive() const
{
    return m_active;
}

// [CHOSEN-STUB] Input capture is unimplemented and always reports no captured input.
bool ControlMapManager::mapNextInput(IControlMap* /*map*/, int /*id*/, bool /*a*/, bool /*b*/,
                                     bool /*c*/, bool /*d*/, std::string& /*out*/)
{
    return false;
}

// Inputs held continuously rather than auto-repeated. Each table ends with its own enum's None sentinel.
namespace {
const int playerInputPushAndHold[] = {
    PIFire, PIUse, PIMouseLook, PIAltSprint, PISprint, PIAltFire, PILie, PICrouch,
    PIFlareFire, PIShowScoreBoard, PINone
};
const int gameInputPushAndHold[] = {
    GIVoipPushToTalk, GIVoipUseLeaderChannel, GIOk, GIAltOk, GIShowScoreboard,
    GIRightShift, GILeftShift, GILeftCtrl, GIRightAlt, GITacticalComm, GIRadioComm, GINone
};
}

// Returns the default trigger threshold: zero for held input or 10000 for an edge-triggered input.
// The last matching entry wins.
float ControlMapManager::getRepeatRate(int id, bool playerMap)
{
    float rate = 10000.0f;                                  // ds:0x888BBE0

    if (playerMap)
    {
        for (int i = 0; playerInputPushAndHold[i] != PINone; ++i)
            if (id == playerInputPushAndHold[i])
                rate = 0.0f;
    }
    else
    {
        for (int i = 0; gameInputPushAndHold[i] != GINone; ++i)
            if (id == gameInputPushAndHold[i])
                rate = 0.0f;
    }
    return rate;
}

// Copies active-map tuning into defaultPlayerInputControlMap through the global manager.
// Bindings are not copied. Requires the control-map manager singleton.
void ControlMapManager::applyActive()
{
    IControlMap* active = m_active;

    IControlMapManager* self =
        static_cast<IControlMapManager*>(static_cast<void*>(controlMapManager));  // ds:0x8AC8C8C
    IControlMap* target = self ? self->getControlMap("defaultPlayerInputControlMap")   // slot +0x18
                               : 0;

    if (active != 0 && target != 0)
    {
        target->setInvertMouse(active->getInvertMouse());                       // +0x34
        target->setMouseSensitivity(active->getMouseSensitivity());             // +0x38
        target->setKeyboardSensitivity(active->getKeyboardSensitivity());       // +0x3C
        target->setYawFactor(active->getYawFactor());                           // +0x50
        target->setPitchFactor(active->getPitchFactor());                       // +0x54

        for (int i = 0; i < IDA_COUNT; ++i)                             // 0x8497870, four passes
            target->setAxisScale(static_cast<InputDeviceAxes>(i),
                                 active->getAxisScale(static_cast<InputDeviceAxes>(i)));  // +0x40
    }
}

// Creates the requested manager interface with one reference. Aggregation is unsupported.
IBase* ControlMapManager::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    ControlMapManager* self = new ControlMapManager();      // 0x850BC50, operator new(0x28u)
    void* itf = self->queryInterface(iid);                  // virtual, slot +0x0C
    if (itf == 0)
        self->release();                                    // virtual, slot +0x08
    return static_cast<IBase*>(itf);
}

// The registry factory pointer selects the static create overload; named-map creation is a separate virtual method.

} // namespace io
} // namespace hfe
} // namespace dice
