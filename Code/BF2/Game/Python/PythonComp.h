// Caches a counted Python wrapper on a world object so repeated wrapObject calls
// preserve Python identity and attributes. Lookup uses queryInterface, not
// getInterfaceID. destroy releases the Python reference; release only frees storage.
// [UNRESOLVED] Callers must establish destroy-before-release ordering or leak
// the wrapper reference.

#ifndef GAME_PYTHON_PYTHONCOMP_H     // [INFERRED] guard style
#define GAME_PYTHON_PYTHONCOMP_H

#include <Python.h>                          // External/python-embedded/py-src-2.3.4 - PyObject
#include "../Objects/IObjectComponent.h"

namespace dice {
namespace hfe {
namespace python {

// Python wrapper component interface.
// [UNRESOLVED] Only getPythonObject is currently identified beyond IObjectComponent.
class IPythonComp : public world::IObjectComponent
{
public:
    virtual PyObject* getPythonObject() const = 0;   // +0x38
};

class PythonComp : public IPythonComp
{
public:
    explicit PythonComp(PyObject* object);

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                         // see the header block
    virtual void* queryInterface(unsigned int iid) const;

    // IObjectComponent
    // getInterfaceID (+0x10), init (+0x14), hasUpdate (+0x24), handleUpdate (+0x28),
    // hasFrameUpdate (+0x2C), handleFrameUpdate (+0x30) and handleMessage (+0x34) are NOT overridden
    // the vtable points at IObjectComponent's own defaults for all seven.
    virtual void destroy();                          // drops the Python reference

    // both ARE stubs. setObject ignores its argument (empty body, 5 B) and getObject returns 0
    // unconditionally (7 B). The component is attached TO an object and never learns which one.
    // Anything needing the object must already have it - which is why addObjectMethods takes the
    // IObject* separately rather than asking the component for it.
    virtual void     setObject(world::IObject*);     // empty
    virtual world::IObject* getObject() const;       // returns 0

    // IPythonComp
    virtual PyObject* getPythonObject() const;

private:
    int       m_refCount;                            // +0x04, 1 on construction
    PyObject* m_object;                              // +0x08
};

} // namespace python
} // namespace hfe
} // namespace dice

#endif // GAME_PYTHON_PYTHONCOMP_H
