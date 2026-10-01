// Engine-facing AI interface. Use typed virtual calls; this is a root interface, not an IBase.
// The DLL uses its own timer and ignores the engine frame delta passed to update.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_AIINSTANCE_H     // [INFERRED] guard style
#define AI_AIINTERFACE_AIINSTANCE_H

#include "../../Core/IBase.h"           // g_aiInstance is declared IBase* in Core/Globals.cpp

namespace dice {
namespace hfe {
namespace ai {

// Engine view of the AI DLL interface. Preserve its virtual declaration order.
class IAIInstance
{
public:
    virtual ~IAIInstance() = 0;                      // ELF / +0/+4; PE scalar dtor +0
    virtual bool init()                       = 0;   // ELF +0x08 / PE +0x04
    virtual bool clear()                      = 0;   // ELF +0x0C / PE +0x08
    virtual bool shutdown()                   = 0;   // ELF +0x10 / PE +0x0C
    // Returns the menu-setting result; engine callers may discard it.
    virtual int  getSettingsFromMenu(bool)    = 0;   // ELF +0x14 / PE +0x10
    virtual int  update(double dt)             = 0;   // ELF +0x18 / PE +0x14; dt is discarded
    virtual bool isInitialized() const         = 0;   // ELF +0x1C / PE +0x18
    virtual bool isShuttingDown() const        = 0;   // ELF +0x20 / PE +0x1C

    // Compatibility aliases used by existing engine callers.
    // [RETIRE-WHEN: ai-instance-names] Remove after callers use the interface method names directly.
    void setFlag14(bool b)        { getSettingsFromMenu(b); }   // result is discarded by the caller
    bool isBotsGame()             { return isInitialized(); }
};

// This engine-side view is never instantiated or used for ownership. The definition merely makes
// the pure destructor a valid C++ interface without importing the aidll's registration teardown.
inline IAIInstance::~IAIInstance() {}

extern IBase* g_aiInstance;                          // ds: - Core/Globals.cpp

// IAIInstance does not derive from IBase. Singleton lookup returns the interface address
// without an inheritance adjustment.
inline IAIInstance* aiInstance() { return reinterpret_cast<IAIInstance*>(g_aiInstance); }

class AIInstanceServer
{
public:
    static void updateInstance();
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_AIINSTANCE_H
