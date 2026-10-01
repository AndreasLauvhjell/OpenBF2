// Core class server registered first by engine setup, supplying foundational classes and singletons.
// Original x86 state is a vptr plus reference count. Source placement is [INFERRED].

#ifndef CORE_MODULE_H               // [INFERRED] guard style
#define CORE_MODULE_H

#include "IModule.h"                 // IModule -> IClassServer -> the IBase quartet

#include <vector>

namespace dice {
namespace hfe {

// Core module providing class, singleton and console registration.
class Module : public IModule
{
public:
    // Starts with one reference; registering a server adds a separate manager-owned reference.
    Module() : m_refCount(1) {}                                 // inlined at 0x813B13A

    // IBase
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // IClassServer
    virtual void getClassList(std::vector<ClassRegInfo>&);
    virtual void getSingletonList(std::vector<SingletonRegInfo>&);

    // IModule
    virtual bool init();
    // Unregisters the core console bindings and reports completion.
    virtual bool close();

private:
    // The compiler supplies the vptr; the reference count is the only explicit member.
    /* 4 */ int m_refCount;         // initModules writes 1 into this word at construction
};

} // namespace hfe
} // namespace dice

#endif // CORE_MODULE_H
