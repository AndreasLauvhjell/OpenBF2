/* Module configuration - the builtin-module table of libdice_py.so, the LINUX build of the
   embedded CPython 2.3.4.

   [CHOSEN, not recovered]. Upstream generates Modules/config.c from Modules/Setup with makesetup;
   DICE's Windows table is the hand-kept PC/config.c. This file is that Windows table transposed
   to Unix, so the two ports differ only where the platform forces it:

     * nt      -> posix        Modules/posixmodule.c builds `posix` (initposix) outside MS_WINDOWS
     * msvcrt  dropped          PC/msvcrtmodule.c is Windows-only and is not compiled here
     * audioop, imageop, rgbimg dropped
                                upstream's own 64-bit rule: setup.py 2.3.4 lines 395-405 builds the
                                three only when sys.maxint is 32-bit ("These don't work for 64-bit
                                platforms!!!"), and PC/config.c registers them #ifndef MS_WIN64. The
                                retail dice_py.dll's inittab has none of them either.
     * the MS_WINDOWS / MS_WIN64 / WIN32 conditionals are gone: this table is only ever compiled
       for Linux x86-64, so every entry below is unconditional except the two that were already
       switches on Windows (zlib under BF2_HAVE_ZLIB, thread under WITH_THREAD).

   Everything else is kept as PC/config.c has it, in its order: _locale, thread, zlib, and the
   four 2026-09-07 additions (_sre, _symtable, datetime, _socket) that the shipped dice_py.dll's
   inittab carries - read from the artifact (dice_py.dll.i64, PyImport_Inittab 0x100DEEAC ->
   _PyImport_Inittab 0x10175F88), 27 live entries:
       signal errno _sre _codecs zipimport _symtable xxsubtype marshal imp
       gc zlib array cPickle cStringIO itertools md5 operator sha strop struct time
       _weakref _socket math datetime _random
   As on Windows, nothing is REMOVED to match that shorter list - it has no os-module backend at
   all, so `import os` could not work from it - and only the missing four were added. The retail
   libdice_py.so's own table has not been read; when it is, reconcile this file against it.

   Every module named here is compiled into the library by ../linux/CMakeLists.txt, which derives
   its source list from Code/Build/python/pythoncore.files; dice_py_check imports every entry of
   sys.builtin_module_names, so an entry without its object fails the build, not the game.

   See init_builtin() in import.c. */

#include "Python.h"

extern void initarray(void);
extern void initbinascii(void);
/* BF2Rebuilt 2026-08-21: zlib is a BUILTIN in DICE's dice_py, not a .pyd/.so - the shipped
   dice_py.dll carries zlib 1.2.2 inside it. zipimport needs it to read pylib-2.3.4.zip; without it
   every stdlib import fails with "zipimport.ZipImportError: can't decompress data; zlib not
   available". BF2_HAVE_ZLIB is defined by CMakeLists.txt, which always builds the nine zlib 1.2.3
   units and Modules/zlibmodule.c in. */
#ifdef BF2_HAVE_ZLIB
extern void initzlib(void);
#endif
extern void initcmath(void);
extern void initerrno(void);
extern void initgc(void);
extern void initmath(void);
extern void initmd5(void);
extern void initposix(void);
extern void initoperator(void);
extern void initregex(void);
extern void initrotor(void);
extern void initsignal(void);
extern void initsha(void);
extern void initstrop(void);
extern void initstruct(void);
extern void inittime(void);
#ifdef WITH_THREAD
extern void initthread(void);
#endif
extern void initcStringIO(void);
extern void initcPickle(void);
extern void initpcre(void);
extern void init_locale(void);
extern void init_codecs(void);
extern void initxreadlines(void);
extern void init_weakref(void);
extern void init_hotshot(void);
extern void initxxsubtype(void);
extern void initzipimport(void);
extern void init_random(void);
extern void inititertools(void);

/* BF2Rebuilt 2026-09-07: the four builtins the shipped dice_py.dll's inittab carries (see the
   header comment). Without _sre, `import re` fails: python/bf2/stats/stats.py dies at its line-3
   `import fpformat` (fpformat imports re), bf2.init_module() swallows the ImportError, and the
   stats / endofround / medals / snapshot init() calls are all skipped - no end-of-round data
   block, no ranked snapshot, and admin/default.py cannot bind its rcon socket (_socket). */
extern void init_sre(void);
extern void init_symtable(void);
extern void initdatetime(void);
extern void init_socket(void);

/* tools/freeze/makeconfig.py marker for additional "extern" */
/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init(void);
extern void initimp(void);

struct _inittab _PyImport_Inittab[] = {

        {"array", initarray},
        {"binascii", initbinascii},
#ifdef BF2_HAVE_ZLIB
        {"zlib", initzlib},        /* BF2Rebuilt 2026-08-21 - see the extern above */
#endif
        {"cmath", initcmath},
        {"errno", initerrno},
        {"gc", initgc},
        {"math", initmath},
        {"md5", initmd5},
        {"posix", initposix}, /* Use the posix os functions, not nt */
        {"operator", initoperator},
        {"regex", initregex},
        {"rotor", initrotor},
        {"signal", initsignal},
        {"sha", initsha},
        {"strop", initstrop},
        {"struct", initstruct},
        {"time", inittime},
#ifdef WITH_THREAD
        {"thread", initthread},
#endif
        {"cStringIO", initcStringIO},
        {"cPickle", initcPickle},
        {"pcre", initpcre},
        {"_locale", init_locale},

        {"_codecs", init_codecs},
        {"xreadlines", initxreadlines},
        {"_weakref", init_weakref},
        {"_hotshot", init_hotshot},
        {"_random", init_random},
        {"itertools", inititertools},
        /* BF2Rebuilt 2026-09-07 - the shipped inittab's four missing entries, see the extern block */
        {"_sre", init_sre},
        {"_symtable", init_symtable},
        {"datetime", initdatetime},
        {"_socket", init_socket},

        {"xxsubtype", initxxsubtype},
        {"zipimport", initzipimport},

/* tools/freeze/makeconfig.py marker for additional "_inittab" entries */
/* -- ADDMODULE MARKER 2 -- */

        /* This module "lives in" with marshal.c */
        {"marshal", PyMarshal_Init},

        /* This lives it with import.c */
        {"imp", initimp},

        /* These entries are here for sys.builtin_module_names */
        {"__main__", NULL},
        {"__builtin__", NULL},
        {"sys", NULL},
        {"exceptions", NULL},

        /* Sentinel */
        {0, 0}
};
