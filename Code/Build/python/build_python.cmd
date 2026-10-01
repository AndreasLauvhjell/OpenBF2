@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem SCAFFOLDING - NOT RECOVERED SOURCE.
rem
rem Builds Code\BF2\External\python-embedded\py-src-2.3.4 into dice_py.lib, then
rem builds and runs pycheck.exe to prove the interpreter actually starts.
rem Usage: build_python.cmd [x86^|x64]. Default x86 retains the attested output.
rem x64 uses OBJ\python\x64\release (CHOSEN native-port path, not retail layout).
rem BF2_PYTHON_OUT optionally selects an isolated output for regression checks.
rem
rem LAYOUT AND NAME ARE ATTESTED (2026-08-16, dice_py.dll.i64): the Windows DLL's
rem embedded PDB path is
rem     D:\bf2\Projects\BF2\Code\BF2\External\python-embedded\OBJ\python\release\dice_py.pdb
rem and its __FILE__ strings are relative "py-src-2.3.4\Objects\...". So DICE's
rem project dir is External\python-embedded\, the CPython sources sit in its
rem py-src-2.3.4\ subdirectory, the build output goes to OBJ\python\release\
rem INSIDE the project dir, and the artifact is named dice_py (dice_py.dll on
rem Windows, libdice_py.so on Linux). The old note here calling dice_py "an
rem output name only" and the old External\Python-2.3.4 placement are RETIRED
rem by that evidence. This script now mirrors all three.
rem
rem /MD ON BOTH SIDES (2026-08-18, was /MT + a static archive). dice_py is a real DLL again,
rem matching the shipped game. The 2026-08-15 crash that forced /MT was a /MD dice_py DLL
rem inside a /MT ENGINE - two CRTs in one image - so Py_Initialize died in PyFile_FromFile the
rem moment _PySys_Init wrapped a FILE* the other CRT owned. The rule was never "link
rem statically", it was ONE CRT PER IMAGE: build.cmd compiles the engine /MD too now, so both
rem share the one CRT DLL and the stdio state is single. (DICE shipped the dynamic runtime
rem beside their exe as well - MFC71.dll and mfc70.dll sit in the game root.)
rem
rem WHY BUILD IT OURSELVES rather than use PCbuild\pcbuild.sln: that solution is
rem a VC6/VC7 .dsp set from 2004. Driving the source list out of pythoncore.dsp
rem (see pythoncore.files) and compiling it with the current toolchain is both
rem simpler and reproducible. See docs\building.md for interpreter setup.
rem
rem BUILTIN MODULE TABLE (2026-09-07). PC\config.c now also carries _sre, _symtable, datetime and
rem _socket - the four entries the shipped dice_py.dll's inittab has and this build lacked. The
rem list was READ FROM THE ARTIFACT (dice_py.dll.i64: PyImport_Inittab at 0x100DEEAC points at
rem _PyImport_Inittab at 0x10175F88 - signal errno _sre _codecs zipimport _symtable xxsubtype
rem marshal imp gc zlib array cPickle cStringIO itertools md5 operator sha strop struct time
rem _weakref _socket math datetime _random). Their four sources are in pythoncore.files, ws2_32.lib
rem joins the DLL link for _socket, and pycheck imports all four so the gap cannot come back
rem silently. Without _sre `import re` fails, python\bf2\stats\stats.py dies at `import fpformat`,
rem and bf2.init_module() swallows that ImportError - no end-of-round data block, no ranked
rem snapshot, no rcon socket. Nothing was REMOVED to match DICE's shorter table (it has no nt, so
rem `import os` would not even work there); only the missing four were added.
rem
rem NOTE: keep this file PLAIN ASCII. A non-ASCII character in a .cmd breaks
rem cmd.exe's parser in a way that reports as garbled command names, which is
rem exactly how this script first failed.
rem ===========================================================================
set "PYARCH=%~1"
if not defined PYARCH set "PYARCH=x86"
if /i not "%PYARCH%"=="x86" if /i not "%PYARCH%"=="x64" (
    echo Usage: build_python.cmd [x86^|x64]
    exit /b 2
)
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=%PYARCH% -no_logo >nul 2>&1
if errorlevel 1 ( echo Visual Studio %PYARCH% environment failed & exit /b 1 )

set PYSRC=%~dp0..\..\BF2\External\python-embedded\py-src-2.3.4
rem The ATTESTED output location - OBJ\python\release inside the project dir.
for %%P in ("%~dp0..\..\BF2\External\python-embedded\OBJ\python\release") do set "PYX86OUT=%%~fP"
for %%P in ("%~dp0..\..\BF2\External\python-embedded\OBJ\python\x64\release") do set "PYX64OUT=%%~fP"
set "OUT=%PYX86OUT%"
if /i "%PYARCH%"=="x64" set "OUT=%PYX64OUT%"
if defined BF2_PYTHON_OUT set "OUT=%BF2_PYTHON_OUT%"
for %%P in ("%OUT%") do set "OUT=%%~fP"
if /i "%PYARCH%"=="x64" if /i "%OUT%"=="%PYX86OUT%" (
    echo Refusing to overwrite the x86 release with x64 artifacts.
    exit /b 2
)
if /i "%PYARCH%"=="x86" if /i "%OUT%"=="%PYX64OUT%" (
    echo Refusing to overwrite the x64 release with x86 artifacts.
    exit /b 2
)
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%" exit /b 1
rem Link exactly this invocation's compiled manifest; never consume stale/globbed objects.
type nul > "%OUT%\dice_py_objects.rsp"
type nul > "%OUT%\dice_py_compile.log"

rem 2003-era C against a 2026 compiler. These are era gaps, not sloppiness:
rem   /wd4996                    POSIX/CRT names deprecated long after this shipped
rem   /wd4244 /wd4267            32/64 narrowing warnings predating size_t discipline
rem   /D_CRT_SECURE_NO_WARNINGS  same era, same reason
rem   /DPy_BUILD_CORE            required - it is what exposes the internal headers
rem   (Py_NO_ENABLE_SHARED is NOT defined - 2026-08-18.) Leaving it OUT is the whole
rem       mechanism of the DLL split: PC\pyconfig.h then defines Py_ENABLE_SHARED and
rem       MS_COREDLL, and Include\pyport.h turns every PyAPI_FUNC into
rem       __declspec(dllexport) HERE (Py_BUILD_CORE is defined) and __declspec(dllimport)
rem       in every consumer (it is not). Nothing else about the sources changes.
rem   /I %PYSRC%\Python          PC\import_nt.c includes "importdl.h", which lives
rem       in Python\ rather than Include\. It is the only file that needs it.
rem   /DHAVE_INET_PTON (2026-09-07)  socketmodule.c carries its own inet_pton/inet_ntop for
rem       pre-Vista SDKs; the current ws2tcpip.h declares both (WSAAPI), so the copies clash
rem       (C2373). The define makes the module use the system pair, exported by ws2_32.dll.
rem ZLIB, added 2026-08-21. ATTESTED BY THE SHIPPED ARTIFACT: the retail dice_py.dll carries
rem zlib 1.2.2 inside it (the version string and both inflate and deflate are in its bytes), so
rem DICE built zlib INTO the interpreter rather than shipping a zlib.pyd. Without it Python
rem cannot read pylib-2.3.4.zip and every stdlib import fails with
rem     zipimport.ZipImportError: can't decompress data; zlib not available
rem which is exactly where the production boot stopped before this line existed.
rem
rem The engine already vendors zlib at External\zlib-1.2.3 for IO/File/ZipArchive, and this
rem points at the same tree rather than adding a second copy. Note the VERSION DIFFERENCE from
rem retail (1.2.3 here, 1.2.2 there) - a [PLATFORM]-class substitution, recorded not hidden.
set ZLIB=%~dp0..\..\BF2\External\zlib-1.2.3

set IMAGEFLAGS=/DDEDICATED=1 /FI"%~dp0..\..\BF2\Core\BuildConfig.h"
set CFLAGS=/nologo /c /MD /O2 /DWIN32 /D_WINDOWS /DPy_BUILD_CORE /DNDEBUG /DHAVE_INET_PTON %IMAGEFLAGS% ^
 /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE ^
 /wd4996 /wd4244 /wd4267 /wd4018 /wd4101 /wd4102 /wd4113 /wd4133 ^
 /I"%PYSRC%\Include" /I"%PYSRC%\PC" /I"%PYSRC%\Python" /I"%ZLIB%"
if /i "%PYARCH%"=="x64" set "CFLAGS=%CFLAGS% /we4311 /we4312"

echo == compiling pythoncore %PYARCH% into %OUT% ==
set OK=0
set BAD=0
for /f "usebackq delims=" %%F in ("%~dp0pythoncore.files") do (
    cl %CFLAGS% /Fo"%OUT%\\" "%PYSRC%\%%F" >> "%OUT%\dice_py_compile.log" 2>&1
    if errorlevel 1 ( set /a BAD+=1 & echo   FAIL %%F ) else ( set /a OK+=1 )
    for %%N in ("%%F") do echo "%OUT%\%%~nN.obj">> "%OUT%\dice_py_objects.rsp"
)
echo compiled: !OK!   failed: !BAD!
if not "!BAD!"=="0" ( echo See %OUT%\dice_py_compile.log & exit /b 1 )

echo == zlib ==
rem THE DEFLATE HALF DECIDES WHETHER zlib GOES IN. External/zlib-1.2.3 vendors only the INFLATE
rem side - adler32 crc32 inffast inflate inftrees zutil - because that is all IO/File/ZipArchive
rem ever needed (it reads .zip, never writes one). zlibmodule.c also exposes compress/compressobj,
rem so building it against that subset leaves EXACTLY FOUR unresolved symbols, all from deflate.c:
rem     _deflate  _deflateEnd  _deflateInit_  _deflateInit2_
rem
rem RESOLVED 2026-08-21: the full zlib 1.2.3 source is present and zlib IS built in. The nine
rem files compiled below are the transitive closure, established by LINKING, not by reading:
rem   * the six inflate files were already here
rem   * deflate.c supplies the four symbols above, and trees.c its _tr_* helpers
rem   * compress.c is needed too, which was NOT obvious - an earlier note in this file said it
rem     was not. deflate.c's deflateBound() calls compressBound(), which lives there. The
rem     linker found it after the first four were satisfied.
rem uncompr.c, infback.c and gzio.c are genuinely unused. example.c and minigzip.c are stock
rem zlib PROGRAMS with their own main() and must never be compiled in.
rem
rem The block still detects deflate.c/trees.c and skips cleanly if they are ever absent, so the
rem script stays runnable on a tree that only has the ZipArchive subset.
rem
rem WHY IT MATTERS: without zlib, Python cannot read pylib-2.3.4.zip and every stdlib import dies
rem with "zipimport.ZipImportError: can't decompress data; zlib not available" - which is exactly
rem where the production boot stops. The SHIPPED dice_py.dll has zlib 1.2.2 built in, so this is a
rem reconstruction gap, not a design choice. (Note the version difference: 1.2.3 here, 1.2.2 there.)
set ZLIBOK=0
if exist "%ZLIB%\deflate.c" if exist "%ZLIB%\trees.c" set ZLIBOK=1
if "!ZLIBOK!"=="1" (
    echo    deflate half found - building zlib in
    for %%Z in (adler32 crc32 inffast inflate inftrees zutil deflate trees compress) do (
        cl %CFLAGS% /Fo"%OUT%\zlib_%%Z.obj" "%ZLIB%\%%Z.c" >> "%OUT%\dice_py_compile.log" 2>&1
        if errorlevel 1 ( echo   FAIL zlib/%%Z.c & exit /b 1 )
        echo "%OUT%\zlib_%%Z.obj">> "%OUT%\dice_py_objects.rsp"
    )
    cl %CFLAGS% /DBF2_HAVE_ZLIB /Fo"%OUT%\zlibmodule.obj" "%PYSRC%\Modules\zlibmodule.c" >> "%OUT%\dice_py_compile.log" 2>&1
    if errorlevel 1 ( echo   FAIL Modules/zlibmodule.c & exit /b 1 )
    echo "%OUT%\zlibmodule.obj">> "%OUT%\dice_py_objects.rsp"
    cl %CFLAGS% /DBF2_HAVE_ZLIB /Fo"%OUT%\config.obj" "%PYSRC%\PC\config.c" >> "%OUT%\dice_py_compile.log" 2>&1
    if errorlevel 1 ( echo   FAIL PC/config.c & exit /b 1 )
) else (
    echo    SKIPPED - %ZLIB%\deflate.c and trees.c are missing.
    echo    zipimport will NOT be able to read pylib-2.3.4.zip. See the note above this line.
    if exist "%OUT%\zlibmodule.obj" del /q "%OUT%\zlibmodule.obj"
    for %%Z in (adler32 crc32 inffast inflate inftrees zutil deflate trees compress) do (
        if exist "%OUT%\zlib_%%Z.obj" del /q "%OUT%\zlib_%%Z.obj"
    )
)

echo == linking dice_py.dll ==
rem A REAL DLL, as the shipped game has. /IMPLIB names the import library dice_py.lib - the
rem same filename the old static archive had, so nothing downstream needed renaming, but its
rem contents are import thunks now. The four system libraries are pythoncore's OWN
rem dependencies and belong on this line rather than the engine's:
rem   advapi32  getpathp.c reads the interpreter path from the registry (Reg*)
rem   user32    dl_nt.c's DllMain calls LoadStringA
rem   shell32   posixmodule.c's os.startfile is ShellExecuteA
rem   ws2_32    socketmodule.c (_socket, added 2026-09-07) - WSAStartup and the BSD socket calls
link /nologo /DLL /MACHINE:%PYARCH% /out:"%OUT%\dice_py.dll" /IMPLIB:"%OUT%\dice_py.lib" ^
     @"%OUT%\dice_py_objects.rsp" advapi32.lib user32.lib shell32.lib ws2_32.lib > "%OUT%\dice_py_link.log" 2>&1
if errorlevel 1 ( echo dll link failed - see %OUT%\dice_py_link.log & exit /b 1 )
for %%A in ("%OUT%\dice_py.dll") do echo    dice_py.dll: %%~zA bytes
echo == building the check ==
rem THE CHECK'S OBJECT GOES IN A SUBDIRECTORY. It used to be
rem written straight into %OUT% - the directory the old DLL link globbed with "%OUT%\*.obj" - so
rem the SECOND run of this script swept pycheck.obj into dice_py.dll. pycheck.cpp is a consumer:
rem it does NOT define Py_BUILD_CORE, so PC\pyconfig.h plants #pragma comment(lib,"python23.lib")
rem in it, and the link then died with LNK1104 on a library that does not exist here. The first
rem run always worked because the object did not exist yet - the script was not idempotent.
if not exist "%OUT%\check" mkdir "%OUT%\check"
rem cPickle imports these two upstream library modules. Stage them for pycheck so Python 2.3
rem writes its .pyc files under the selected output, never over the vendored reference sources.
if not exist "%OUT%\check\lib" mkdir "%OUT%\check\lib"
for %%P in (copy_reg types) do (
    copy /y "%PYSRC%\Lib\%%P.py" "%OUT%\check\lib\%%P.py" >nul
    if errorlevel 1 exit /b 1
)

rem /NODEFAULTLIB:python23.lib - PC\pyconfig.h emits `#pragma comment(lib,"python23.lib")`
rem into every consumer that does NOT define Py_BUILD_CORE (it is being helpful to extension
rem authors). There is no python23.lib here; the import library is dice_py.lib, named below.
cl /nologo /MD %IMAGEFLAGS% /I"%PYSRC%\Include" /I"%PYSRC%\PC" "%~dp0pycheck.cpp" /Fo"%OUT%\check\pycheck.obj" /Fe:"%OUT%\pycheck.exe" /link /MACHINE:%PYARCH% /NODEFAULTLIB:python23.lib "%OUT%\dice_py.lib" > "%OUT%\pycheck_build.log" 2>&1
if errorlevel 1 ( echo check link failed - see %OUT%\pycheck_build.log & exit /b 1 )

echo.
"%OUT%\pycheck.exe" "%OUT%\check\lib" > "%OUT%\pycheck_runtime.log" 2>&1
set "PYRESULT=%errorlevel%"
type "%OUT%\pycheck_runtime.log"
exit /b %PYRESULT%
