// Reference-owning Python object handle. Construction and copying increment the reference
// count rather than stealing the caller's reference; destruction decrements it. Null is supported.

#ifndef GAME_PYTHON_PYREF_H         // [INFERRED] guard style
#define GAME_PYTHON_PYREF_H

#include <Python.h>                 // External/python-embedded/py-src-2.3.4/Include

namespace dice {
namespace hfe {
namespace python {

class PyRef
{
public:
    // inline, and deliberately so — the binary has no symbol for any constructor because GCC
    // expanded them at every call site. Written with the Py_X* macros because that is what the
    // inlined instruction sequences ARE; see the header block.
    PyRef()                    : m_obj(0)       {}
    PyRef(PyObject* o)         : m_obj(o)       { Py_XINCREF(m_obj); }
    PyRef(const PyRef& other)  : m_obj(other.m_obj) { Py_XINCREF(m_obj); }

    ~PyRef();
    PyRef& operator=(const PyRef&);

    // [INFERRED] accessors. No symbol exists for either — every use is inlined — so the names are
    // chosen. The member they read is not: it is the sole 4-byte field the recovered bodies touch.
    PyObject* get() const      { return m_obj; }
    operator PyObject*() const { return m_obj; }
    bool operator!() const     { return m_obj == 0; }

private:
    PyObject* m_obj;                            // +0x00, and the whole object
};

} // namespace python
} // namespace hfe
} // namespace dice

#endif // GAME_PYTHON_PYREF_H
