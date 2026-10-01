// Client input sampling and filtering. The dedicated-server registry does not construct this manager.

#include "InputManager.h"
#include "../../IO/IInputFilter.h"
#include "../../Core/Ids.h"                // [INFERRED] header name; the constants are attested
#include "../../Core/Debug.h"              // the three constructor failure paths

#include <cstring>                         // memset - the constructor's explicit zero-fill

namespace dice {
namespace hfe {
namespace io {

// Global declarations must match their definitions for MSVC linkage.
// Input operations require the corresponding device and control-map singletons to be initialized.
extern IBase* inputDeviceManager;       // ds:0x8AC8C88 - type must match Core/Globals.cpp
extern IBase* controlMapManager;        // ds:0x8AC8C8C - type must match Core/Globals.cpp

namespace {

// The two casts, kept in one place so the type mismatch with Core/Globals.cpp is visible once.
inline IInputDeviceManager* deviceManager()
{
    return static_cast<IInputDeviceManager*>(static_cast<void*>(inputDeviceManager));
}

inline IControlMapManager* mapManager()
{
    return static_cast<IControlMapManager*>(static_cast<void*>(controlMapManager));
}

// The PE gives these classes 320/280-byte strides, while the ELF gives them 316/276-byte strides.
// Both binaries initialise only the logical fields below, not the PE's final alignment padding.
const unsigned int GAME_INPUT_LOGICAL_SIZE = 316;
const unsigned int PLAYER_INPUT_LOGICAL_SIZE = 276;

inline void clearGameInput(GameInput& input)
{
    std::memset(&input, 0, GAME_INPUT_LOGICAL_SIZE);
}

inline void clearPlayerInput(PlayerInput& input)
{
    std::memset(&input, 0, PLAYER_INPUT_LOGICAL_SIZE);
}

template <class InputType>
inline void setAnalogueBit(InputType& input, unsigned int index, bool analogue)
{
    // Variable 64-bit shifts on both shipped x86 compilers mask the count to six bits. Game input
    // ids 64..73 consequently alias tail bits 0..9, just as their mapping bits do.
    const unsigned int shift = index & 0x3Fu;
    unsigned int& word = (shift & 0x20u) ? input.dword1 : input.dword0;
    const unsigned int bit = 1u << (shift & 0x1Fu);
    if (analogue)
        word |= bit;
    else
        word &= ~bit;
}

} // anonymous namespace

// [INFERRED] Shared latency measurement written by player-input sampling and read by latency compensation.
float inputSampleLatency = 0.0f;

#if defined(_M_IX86)
static_assert(sizeof(GameInput) == 320, "BF2.exe GameInput stride must be 320 bytes");
static_assert(sizeof(PlayerInput) == 280, "BF2.exe PlayerInput stride must be 280 bytes");
static_assert(sizeof(InputManager) == 60, "BF2.exe InputManager must be 60 bytes");
#endif

// constructor / destructor

// Allocates the input-history arrays and scratch samples, then explicitly clears every sample byte.
// Sample sizes differ by image; preserve the layout-dependent allocation sizes and failure diagnostics.
// The allocation null checks remain even though the selected new form throws.
InputManager::InputManager()
    : m_refCount(1)
    , m_spareGameSample(0)              // set below - 0x849FB7A
    , m_sparePlayerSample(0)            // set below - 0x849FC2E
    , m_defaultGameControlMap(0)
    , m_defaultPlayerControlMap(0)
    , m_gameControlMap(0)
    , m_playerControlMap(0)
    , m_filterHead(0)
    , m_writeIndex(0)
    , m_nextUnread(0)
    , m_maxEntries(16)
    , m_gameSamples(0)                  // set below - 0x849FA34
    , m_playerSamples(0)                // set below - 0x849FAF0
    , m_playerInputDelay(0)
{
    // the game sample ring, 16 * platform stride -------------------------- 0x849F8F9
    m_gameSamples = new GameInput[16];
    for (int i = 0; i < 16; ++i)
        clearGameInput(m_gameSamples[i]);

    if (m_gameSamples == 0)             // dead - operator new throws. dice's statement, kept.
        Debug(DebugTypeError, "IO/Input/InputManager.cpp", 38, "")
            << "Unable to allocate memory to Input";

    // Allocate the player sample ring using the configured entry count.
    m_playerSamples = new PlayerInput[m_maxEntries];
    for (int i = 0; i < m_maxEntries; ++i)
        clearPlayerInput(m_playerSamples[i]);

    if (m_playerSamples == 0)           // dead. Kept.
        Debug(DebugTypeError, "IO/Input/InputManager.cpp", 44, "")
            << "Unable to allocate memory to Input";

    // the two single spare samples ---------------------------------------- 0x849FB0A / 0x849FBC0
    m_spareGameSample = new GameInput;
    clearGameInput(*m_spareGameSample);

    m_sparePlayerSample = new PlayerInput;
    clearPlayerInput(*m_sparePlayerSample);

    if (m_spareGameSample == 0 || m_sparePlayerSample == 0)         // dead. Kept.
        Debug(DebugTypeError, "IO/Input/InputManager.cpp", 52, "")
            << "unabled to alloc memory";                           // dice'S typo. Kept.
}

// Deletes sample rings and spare samples. Registered filter nodes are not freed here.
InputManager::~InputManager()
{
    delete[] m_gameSamples;
    delete[] m_playerSamples;
    delete m_spareGameSample;
    delete m_sparePlayerSample;
}

// IBase quartet - all four

// Core/IBase.h declares addRef() void.
void InputManager::addRef()
{
    ++m_refCount;
}


int InputManager::getRef()
{
    return m_refCount;
}

// Dispatches `(*(*this + 80))(this)` - vtable slot +0x50, the deleting
// destructor.
int InputManager::release()
{
    const int count = m_refCount - 1;
    const bool dead = (m_refCount == 1);
    m_refCount = count;

    if (dead)
        delete this;                    // virtual, slot +0x50

    return count;
}

// The uniform two-id shape.
void* InputManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<InputManager*>(this);
    if (iid == IID_IInputManager)
        return const_cast<InputManager*>(this);
    return 0;
}

// Initializes devices using deviceMask in place and returns whether the resulting mask is nonzero.
// Requires the device manager; its Boolean return is ignored.
bool InputManager::init(unsigned int deviceMask)
{
    deviceManager()->initDevices(&deviceMask);      // 0x84A0299, virtual slot +0x10
    return deviceMask != 0;
}

// Releases the device-manager singleton when present and returns true.
// The global is not cleared and may dangle after its final reference is released.
bool InputManager::shutdown()
{
    if (inputDeviceManager != 0)
        deviceManager()->release();                 // 0x84A02D8, virtual slot +0x08 - see above

    return true;
}

// Activates or deactivates selected devices and returns the device manager's result. Requires initialization.
bool InputManager::setHasFocus(bool hasFocus, unsigned int deviceMask)
{
    if (hasFocus)
        return deviceManager()->activateDevices(deviceMask);        // +0x1C

    return deviceManager()->deActivateDevices(deviceMask);          // +0x20
}

// Populates player input from controlMap. Returns false for a null map, otherwise true.
// Records analog-axis flags and the firing trigger's sample latency.
bool getPlayerInput(PlayerInput& input, IControlMap* controlMap)
{
    if (controlMap == 0)
        return false;

    bool allAnalogue = true;
    for (int i = 0; i < 64; ++i)
    {
        const PlayerInputMap id = static_cast<PlayerInputMap>(i);

        if (controlMap->triggerExists(i))
        {
            const bool pressed = controlMap->getTriggerState(i) != 0;
            input.setInputIsMapped(id, true);
            setAnalogueBit(input, static_cast<unsigned int>(i), false);
            // Player input calls the float overload after spelling the digital state as 0.0/1.0.
            input.setInput(id, pressed ? 1.0f : 0.0f);

            if (pressed && i == PIFire)
                inputSampleLatency = controlMap->getTriggerSampleLatency(PIFire);
        }
        else
        {
            // Shipped asymmetry: every player id is marked mapped even when no trigger exists.
            input.setInputIsMapped(id, true);
            setAnalogueBit(input, static_cast<unsigned int>(i), false);
        }

        if (controlMap->axisExists(i))
        {
            input.setInputIsMapped(id, true);
            input.setInput(id, controlMap->getAxisValue(i));

            const bool analogue = controlMap->isAxisValueAnalogue(i);
            setAnalogueBit(input, static_cast<unsigned int>(i), analogue);

            if (i == PIPitch || i == PIRoll)
            {
                // The binary deliberately asks the virtual a second time instead of reusing the
                // value above.
                allAnalogue = controlMap->isAxisValueAnalogue(i) && allAnalogue;
            }
            else if (i == PIThrottle)
            {
                float throttle = (input.mappingFlags & (1ULL << PIThrottle))
                    ? input.inputs[PIThrottle]
                    : 0.0f;
                if (throttle < -1.0f)
                    throttle = -1.0f;
                else if (throttle > 1.0f)
                    throttle = 1.0f;
                input.setInput(PIThrottle, throttle);
            }
        }
    }

    input.flag = allAnalogue ? 1 : 0;
    return true;
}

// Samples game input through controlMap. Returns false when no map is supplied.
bool getGameInput(GameInput& input, IControlMap* controlMap)
{
    if (controlMap == 0)
        return false;

    clearGameInput(input);

    IInputDevice* device = deviceManager()->getInputDevice(static_cast<InputDeviceFlags>(2));
    IKeyboardDevice* keyboard = static_cast<IKeyboardDevice*>(device->queryInterface(IID_IKeyboardDevice));
    if (keyboard != 0 && keyboard->keyDown(static_cast<InputDeviceKeys>(28))
                      && keyboard->keyDown(static_cast<InputDeviceKeys>(56)))
    {
        input.setInputIsMapped(GIAltEnter, true);
        input.setInput(GIAltEnter, true);
    }

    for (int i = 0; i < 74; ++i)
    {
        const GameInputMap id = static_cast<GameInputMap>(i);

        if (controlMap->triggerExists(i))
        {
            input.setInputIsMapped(id, true);
            setAnalogueBit(input, static_cast<unsigned int>(i), false);
            input.setInput(id, controlMap->getTriggerState(i) != 0);
        }

        if (controlMap->axisExists(i))
        {
            input.setInputIsMapped(id, true);
            input.setInput(id, controlMap->getAxisValue(i));
            setAnalogueBit(input, static_cast<unsigned int>(i), controlMap->isAxisValueAnalogue(i));
        }
    }

    return true;
}

// Samples input for tick and updates control maps.
// Device updates use -1 initially and 10 on the final pass; control maps always receive deltaTime.
void InputManager::update(int tick, float deltaTime)
{
    if (m_playerInputDelay > 0)
        --m_playerInputDelay;

    if (m_defaultPlayerControlMap == 0)
        m_defaultPlayerControlMap = mapManager()->getControlMap("defaultPlayerInputControlMap");

    if (m_defaultGameControlMap == 0)
        m_defaultGameControlMap = mapManager()->getControlMap("defaultGameControlMap");
    deviceManager()->update(-1.0f);

    int writeIndex = m_writeIndex;
    for (int pass = 0; pass < tick; ++pass)
    {
        deviceManager()->update(pass + 1 == tick ? 10.0f : deltaTime);
        mapManager()->update(deltaTime);

        const int ringIndex = writeIndex % m_maxEntries;
        PlayerInput& playerInput = m_playerSamples[ringIndex];
        clearPlayerInput(playerInput);

        if (m_defaultPlayerControlMap != 0)
            getPlayerInput(playerInput, m_defaultPlayerControlMap);
        if (m_playerControlMap != 0)
            getPlayerInput(playerInput, m_playerControlMap);

        IControlMap* gameControlMap = m_gameControlMap;
        if (gameControlMap == 0)
            gameControlMap = m_defaultGameControlMap;
        getGameInput(m_gameSamples[ringIndex], gameControlMap);

        ++writeIndex;
    }

    m_writeIndex = writeIndex;
}

// Appends a borrowed filter. Duplicate pointers are allowed; unregistration removes the first match.

bool InputManager::registerInputFilter(IInputFilter* filter)
{
    if (filter == 0)
        return false;

    InputFilterNode* node = new InputFilterNode;
    node->filter = filter;
    node->next = 0;

    if (m_filterHead == 0)
    {
        m_filterHead = node;
    }
    else
    {
        InputFilterNode* tail = m_filterHead;
        while (tail->next != 0)
            tail = tail->next;
        tail->next = node;
    }

    return true;
}

bool InputManager::unregisterInputFilter(IInputFilter* filter)
{
    if (filter == 0 || m_filterHead == 0)
        return false;

    InputFilterNode* previous = m_filterHead;
    InputFilterNode* node = m_filterHead;
    while (node->filter != filter)
    {
        previous = node;
        node = node->next;
        if (node == 0)
            return false;
    }

    if (node == m_filterHead)
        m_filterHead = node->next;
    else
        previous->next = node->next;

    delete node;
    return true;
}

// Selects the named game control map. Returns false if no map is found.
bool InputManager::setGameControlMap(std::string name)
{
    IControlMap* map = mapManager()->getControlMap(name);   // +0x18
    m_gameControlMap = map;
    return map != 0;
}

// Selects and activates the named player control map, applying its tuning. Returns whether it exists.
bool InputManager::setPlayerControlMap(std::string name)
{
    IControlMap* map = mapManager()->getControlMap(name);   // +0x18
    m_playerControlMap = map;
    mapManager()->setActive(map);                           // 0x84A05B6, slot +0x30 - see above
    return m_playerControlMap != 0;                         // re-reads the member
}

// Returns the selected player map's name, or an empty string when none is selected.
std::string InputManager::getPlayerControlMap()
{
    if (m_playerControlMap != 0)
        return m_playerControlMap->getName();               // 0x84A05EB, virtual slot +0x14

    return std::string("");                                 // the empty string
}

// Returns the next unread player sample without checking queue availability.
// Filters run in registration order until one refuses; input delay clears values but preserves masks and tail data.
PlayerInput* InputManager::getNewPlayerInputSample()
{
    if (m_playerSamples == 0)
        Debug(DebugTypeError, "IO/Input/InputManager.cpp", 225, "") << "no buffer";

    PlayerInput* sample = &m_playerSamples[m_nextUnread % m_maxEntries];
    InputFilterNode* node = m_filterHead;
    while (node != 0 && node->filter->filterPlayerInput(*sample))
        node = node->next;

    if (m_playerInputDelay > 0)
        std::memset(sample->inputs, 0, sizeof(sample->inputs));

    return sample;
}

// Returns a new game input sample without clearing the player-input delay.
GameInput* InputManager::getNewGameInputSample()
{
    if (m_gameSamples == 0)
        Debug(DebugTypeError, "IO/Input/InputManager.cpp", 247, "") << "no buffer";

    GameInput* sample = &m_gameSamples[m_nextUnread % m_maxEntries];
    InputFilterNode* node = m_filterHead;
    while (node != 0 && node->filter->filterGameInput(*sample))
        node = node->next;

    return sample;
}


// no modulo. See the header block.
bool InputManager::advanceNextUnread()
{
    if (m_nextUnread >= m_writeIndex)
        return false;

    ++m_nextUnread;
    return true;
}


// Discards unread samples so simulation can resume from the current write position.
void InputManager::catchUp()
{
    m_nextUnread = m_writeIndex;
}

// See the header block on latency tuning.
void InputManager::setPlayerInputDelay(int delay)
{
    m_playerInputDelay = delay;
}

// A plain subtraction - no wrap handling.
int InputManager::getNumEntries()
{
    return m_writeIndex - m_nextUnread;
}

// Creates the client input manager for iid with one owned reference. Rejects aggregation and unsupported interfaces.
IBase* InputManager::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    InputManager* self = new InputManager();
    void* itf = self->queryInterface(iid);
    if (itf == 0)
        self->release();
    return static_cast<IBase*>(itf);
}

// The dedicated-server class list omits InputManager and AsyncMultiStreamer.
// Missing singleton factories must leave their globals null; do not add client factories to the server list.

} // namespace io
} // namespace hfe
} // namespace dice
