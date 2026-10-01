// Separate game/UI and replicated-player sample rings, initially 16 entries deep. [INFERRED] Header placement.
// catchUp discards unread samples; indices wrap when selecting slots. The dedicated image leaves inputManager null.

#ifndef IO_INPUT_INPUTMANAGER_H         // [INFERRED] guard style, per Core/IBase.h
#define IO_INPUT_INPUTMANAGER_H

#include <string>

#include "Input.h"
#include "GameInput.h"
#include "PlayerInput.h"
#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace io {

// Buffers input samples and applies the registered filters.
// Spare samples are separately owned objects; the player-input delay is configurable at runtime.
class InputManager : public IInputManager
{
public:
    InputManager();

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;       // 0x84A0A50  - _ZNK..., const

    // IInputManager, +0x10..+0x48. order is frozen - see IO/Input/Input.h.
    virtual bool  init(unsigned int deviceMask);                // 0x84A0280,  55   recovered
    virtual bool  shutdown();                                   // 0x84A02C0,  42   recovered
    virtual bool  setHasFocus(bool hasFocus, unsigned int deviceMask);  // 0x84A02F0, 55 recovered
    virtual void  update(int tick, float deltaTime);            // 0x84A0330, 533   recovered
    virtual bool  registerInputFilter(IInputFilter* filter);    // 0x84A0900, 110   recovered
    virtual bool  unregisterInputFilter(IInputFilter* filter);  // 0x84A0970, 120   recovered
    virtual bool  setGameControlMap(std::string name);          // 0x84A0550,  45   recovered
    virtual bool  setPlayerControlMap(std::string name);        // 0x84A0580,  70   recovered
    virtual std::string getPlayerControlMap();                  // 0x84A05D0,  79   recovered
    virtual PlayerInput* getNewPlayerInputSample();             // 0x84A0620, 342   recovered
    virtual GameInput*   getNewGameInputSample();               // 0x84A0780, 314   recovered
    virtual bool  advanceNextUnread();                          // 0x84A08C0,  29   recovered
    virtual void  catchUp();                                    // 0x84A08E0,  14   recovered
    virtual void  setPlayerInputDelay(int delay);               // 0x84A0A70,  14   recovered
    virtual int   getNumEntries();                              // 0x84A08F0,  16   recovered

    // +0x4C / +0x50
    virtual ~InputManager();

    // BF2.exe 0x6AC3A7. Absent from the dedicated ELF; register client-side only.
    static IBase* create(unsigned int iid, IBase* outer);       // [PLATFORM] client factory

private:
    struct InputFilterNode
    {
        IInputFilter* filter;
        InputFilterNode* next;
    };

    /* 0x04 */ int                m_refCount;
    /* 0x08 */ GameInput*         m_spareGameSample;        // [INFERRED] name
    /* 0x0C */ PlayerInput*       m_sparePlayerSample;      // [INFERRED] name
    /* 0x10 */ IControlMap*       m_defaultGameControlMap;
    /* 0x14 */ IControlMap*       m_defaultPlayerControlMap;
    /* 0x18 */ IControlMap*       m_gameControlMap;
    /* 0x1C */ IControlMap*       m_playerControlMap;
    /* 0x20 */ InputFilterNode*   m_filterHead;
    /* 0x24 */ int                m_writeIndex;             // [INFERRED] name
    /* 0x28 */ int                m_nextUnread;             // [INFERRED] name
    /* 0x2C */ int                m_maxEntries;             // ctor 16
    /* 0x30 */ GameInput*         m_gameSamples;
    /* 0x34 */ PlayerInput*       m_playerSamples;
    /* 0x38 */ int                m_playerInputDelay;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_INPUT_INPUTMANAGER_H
