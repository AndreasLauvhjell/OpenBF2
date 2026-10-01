/* Module configuration */

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

extern void initarray(void);
#ifndef MS_WIN64
extern void initaudioop(void);
#endif
extern void initbinascii(void);
/* BF2Rebuilt 2026-08-21: zlib is a BUILTIN in DICE's dice_py, not a .pyd - the shipped
   dice_py.dll carries zlib 1.2.2 inside it (its version string and both inflate and deflate are
   in the bytes). zipimport needs it to read pylib-2.3.4.zip; without it every stdlib import
   fails with "zipimport.ZipImportError: can't decompress data; zlib not available".

   BF2_HAVE_ZLIB is defined by build_python.cmd ONLY when the deflate half of zlib is present in
   External/zlib-1.2.3. The engine vendors the inflate half only (ZipArchive reads .zip, never
   writes one), so deflate.c and trees.c have to be added before this can be switched on. */
#ifdef BF2_HAVE_ZLIB
extern void initzlib(void);
#endif
extern void initcmath(void);
extern void initerrno(void);
extern void initgc(void);
#ifndef MS_WIN64
extern void initimageop(void);
#endif
extern void initmath(void);
extern void initmd5(void);
extern void initnt(void);
extern void initoperator(void);
extern void initregex(void);
#ifndef MS_WIN64
extern void initrgbimg(void);
#endif
extern void initrotor(void);
extern void initsignal(void);
extern void initsha(void);
extern void initstrop(void);
extern void initstruct(void);
extern void inittime(void);
extern void initthread(void);
extern void initcStringIO(void);
extern void initcPickle(void);
extern void initpcre(void);
#ifdef WIN32
extern void initmsvcrt(void);
extern void init_locale(void);
#endif
extern void init_codecs(void);
extern void initxreadlines(void);
extern void init_weakref(void);
extern void init_hotshot(void);
extern void initxxsubtype(void);
extern void initzipimport(void);
extern void init_random(void);
extern void inititertools(void);

/* BF2Rebuilt 2026-09-07: the four builtins the shipped dice_py.dll carries in its inittab and this
   table lacked. READ FROM THE ARTIFACT: dice_py.dll.i64, PyImport_Inittab (0x100DEEAC) points at
   _PyImport_Inittab at 0x10175F88, whose 27 live entries are, in order:
       signal errno _sre _codecs zipimport _symtable xxsubtype marshal imp
       gc zlib array cPickle cStringIO itertools md5 operator sha strop struct time
       _weakref _socket math datetime _random
   (plus the four NULL sys.builtin_module_names entries). Nothing is REMOVED here to match that
   list - DICE's table also drops several stock entries this one keeps - only the missing four
   are added.

   WHY IT MATTERS: without _sre, `import re` fails. python/bf2/stats/stats.py dies at its line-3
   `import fpformat` (fpformat imports re), Python 2.3 leaves the half-initialised module in
   sys.modules (no remove_module in 2.3.4), bf2.init_module() catches the ImportError and prints
   "Official stats module not found." into host.log - which has no visible sink on the dedicated
   build - and the stats / endofround / medals / snapshot init() calls are all skipped. The
   observable results: no end-of-round data block 8 (so a retail client never opens the EOR
   screen), no ranked snapshot, and admin/default.py cannot bind its rcon socket (_socket). */
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
#ifdef MS_WINDOWS
#ifndef MS_WIN64
        {"audioop", initaudioop},
#endif
#endif
        {"binascii", initbinascii},
#ifdef BF2_HAVE_ZLIB
        {"zlib", initzlib},        /* BF2Rebuilt 2026-08-21 - see the extern above */
#endif
        {"cmath", initcmath},
        {"errno", initerrno},
        {"gc", initgc},
#ifndef MS_WIN64
        {"imageop", initimageop},
#endif
        {"math", initmath},
        {"md5", initmd5},
        {"nt", initnt}, /* Use the NT os functions, not posix */
        {"operator", initoperator},
        {"regex", initregex},
#ifndef MS_WIN64
        {"rgbimg", initrgbimg},
#endif
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
#ifdef WIN32
        {"msvcrt", initmsvcrt},
        {"_locale", init_locale},
#endif

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
