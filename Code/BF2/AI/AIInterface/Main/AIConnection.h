// Owns the AI library connection and its registered AI interface.
// init loads the library; getAI borrows the resulting interface; clear shuts it down and unloads it.
// A nonempty library name that fails to load causes abort.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_MAIN_AICONNECTION_H     // [INFERRED] guard style
#define AI_AIINTERFACE_MAIN_AICONNECTION_H

#include <string>

#include "../../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace ai {

// Reference-counted interface for loading, querying and closing an AI library connection.
class IAIConnection : public IBase
{
public:
    virtual ~IAIConnection();                          // +0x10 / +0x14

    // The ABI requires a mutable string reference even though init does not modify it.
    virtual bool  init(std::string& libraryName) = 0;  // +0x18
    virtual bool  clear()                        = 0;  // +0x1C
    // [UNRESOLVED] return type. Itanium does not mangle it and the pointee's class lives in
    // aidll.so. `void*` is chosen to match ClassManager::getSimpleSingleton, which is where the
    // value comes from and which is itself declared `void*`.
    virtual void* getAI()                        = 0;  // +0x20
};

// Opaque. The object handed back by getSimpleSingleton("AI") is constructed inside aidll.so; its
// class is not in this binary and never will be. Declared only so the .cpp has something to name.
// Nothing here dispatches through a shape-declared version of it - see clear in the .cpp.
struct AIConnectionData;

// AIConnection
class AIConnection : public IAIConnection
{
public:
    AIConnection();
    virtual ~AIConnection();

    virtual void  addRef();                                      // +0x00
    virtual int   getRef();                                      // +0x04
    virtual int   release();                                     // +0x08
    virtual void* queryInterface(unsigned int iid) const;        // +0x0C

    virtual bool  init(std::string& libraryName);                // +0x18
    virtual bool  clear();                                       // +0x1C
    virtual void* getAI();                                       // +0x20

private:
    int                m_refCount;                               // +0x04
    AIConnectionData*  m_data;                                   // +0x08
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_MAIN_AICONNECTION_H
