// Scaffolding, not recovered source. Checks interpreter startup, Python execution,
// required builtins, and shutdown through the shared dice_py library.
// Build: Code\Build\python\build_python.cmd [x86|x64]
#include <Python.h>
#include <cstdio>

static_assert(sizeof(void*) == SIZEOF_VOID_P, "Python headers and consumer pointer width disagree");

static void* destroyedPointer;
static void* destroyedDescription;

static void destroyPointer(void* pointer)
{
    destroyedPointer = pointer;
}

static void destroyDescribedPointer(void* pointer, void* description)
{
    destroyedPointer = pointer;
    destroyedDescription = description;
}

static bool checkPointersAndData()
{
    // Opaque high-bit values deliberately catch a round-trip through a 32-bit long.
    // They are never dereferenced. Also round-trip an actual stack address.
    int local = 7;
    Py_uintptr_t bits = 0x87654321u;
#if SIZEOF_VOID_P == 8
    bits |= static_cast<Py_uintptr_t>(0x1234u) << 32;
#endif
    void* pointer = reinterpret_cast<void*>(bits);
    void* description = &local;
    PyObject* object = PyCObject_FromVoidPtr(pointer, destroyPointer);
    if (!object)
        return false;
    bool ok = PyCObject_AsVoidPtr(object) == pointer;
    Py_DECREF(object);
    ok = ok && destroyedPointer == pointer;

    destroyedPointer = 0;
    object = PyCObject_FromVoidPtrAndDesc(description, pointer, destroyDescribedPointer);
    if (!object)
        return false;
    ok = ok && PyCObject_AsVoidPtr(object) == description && PyCObject_GetDesc(object) == pointer;
    Py_DECREF(object);
    ok = ok && destroyedPointer == description && destroyedDescription == pointer;

    // Force the consumer to import and use both load-bearing Python data symbols.
    Py_INCREF(Py_None);
    ok = ok && Py_None->ob_type != 0;
    Py_DECREF(Py_None);
    PyErr_SetString(PyExc_RuntimeError, "pycheck data-import probe");
    ok = ok && PyErr_ExceptionMatches(PyExc_RuntimeError);
    PyErr_Clear();
    printf("PyCObject roundtrip %s (%u-bit pointer, high bits and destructor preserved)\n",
        ok ? "ok" : "FAILED", static_cast<unsigned>(sizeof(void*) * 8));
    return ok;
}

int main(int argc, char** argv)
{
    printf("=== BF2 Python runtime check ===\n\n");
    printf("PY_VERSION          %s\n", PY_VERSION);
    printf("PYTHON_API_VERSION  %d\n", PYTHON_API_VERSION);
    printf("pointer bytes       %u\n", static_cast<unsigned>(sizeof(void*)));

    // This isolated check has no standard-library tree, so suppress automatic site import.
    // The game supplies its own Python library paths.
    Py_NoSiteFlag = 1;

    Py_Initialize();
    printf("Py_IsInitialized    %d\n", Py_IsInitialized());
    if (!Py_IsInitialized() || !checkPointersAndData())
        return 1;
    // The build supplies staged upstream library modules for builtin cPickle's copy_reg import.
    // A standalone invocation retains the original no-stdlib smoke coverage.
    if (argc > 1)
    {
        PySys_SetPath(argv[1]);
        if (PyRun_SimpleString("import sys\nfor name in sys.builtin_module_names: __import__(name)\n") != 0)
            return 2;
    }

    // PyRun_SimpleString is the call PythonHost::execute is built on, and also the one
    // initializeGameMode uses to inject generated import source - the path the game-mode-name
    // whitelist exists to protect.
    if (PyRun_SimpleString("print 'hello from the BF2 interpreter'\n") != 0 ||
        PyRun_SimpleString("import sys; print 'sys.version =', sys.version.split()[0]\n") != 0)
        return 1;

    // Import required builtins explicitly so a missing module fails this check instead of
    // being hidden by script-level ImportError handling.
    if (PyRun_SimpleString(
            "import _sre, _symtable, datetime, _socket, zlib\n"
            "assert datetime.date(2026, 9, 7).isoformat() == '2026-09-07'\n"
            "assert _socket.gethostname()\n"
            "payload = 'BF2 native interpreter\\x00\\xff' * 1000\n"
            "assert zlib.decompress(zlib.compress(payload)) == payload\n"
            "sock = _socket.socket(_socket.AF_INET, _socket.SOCK_DGRAM)\n"
            "sock.bind(('127.0.0.1', 0))\n"
            "sock.settimeout(2.0)\n"
            "assert sock.getsockname()[1] != 16567\n"
            "assert sock.sendto('pycheck', sock.getsockname()) == 7\n"
            "assert sock.recvfrom(32)[0] == 'pycheck'\n"
            "sock.close()\n"
            "print 'builtin modules ok:', len(sys.builtin_module_names)\n"
            "print 'zlib roundtrip and loopback UDP send/receive ok'\n") != 0)
    {
        printf("\nbuiltin module check FAILED\n");
        return 2;
    }

    Py_Finalize();
    printf("\npython runtime check complete.\n");
    return 0;
}
