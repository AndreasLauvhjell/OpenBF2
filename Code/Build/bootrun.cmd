@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem THE BOOT HARNESS - force-links the recovered engine into a RUNNABLE exe and
rem executes it. Written 2026-07-29.
rem
rem   Code\Tests\syntaxcheck.cmd   parses    (/Zs)   - does it read as C++?
rem   Code\Build\build.cmd         compiles + links  - is every symbol written?
rem   Code\Build\bootrun.cmd       THIS. RUNS IT     - does the frame execute?
rem
rem ** WHY THIS CAN WORK WITH GAPS STILL OPEN. ** /FORCE:UNRESOLVED tells the
rem linker to emit the image anyway. Every unwritten function becomes an address
rem that faults when called - so the program runs correctly right up to the
rem moment the boot path reaches something we have not recovered, and then dies
rem AT THAT SYMBOL. That is the measurement: not "416 gaps remain", but "which
rem gap does a booting server actually hit FIRST".
rem
rem A clean exit or a fault deep in the frame both beat a link error, because
rem both are evidence about the code we have written rather than the code we
rem have not.
rem
rem ** THIS IS SCAFFOLDING, NOT RECOVERED SOURCE. ** It links the real objects
rem from build.cmd - no stand-ins, no stubs. Run build.cmd first.
rem
rem   usage:  build.cmd  &&  bootrun.cmd
rem ===========================================================================
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set OUT=%~dp0obj
if defined BF2_BUILD_OUT set OUT=%BF2_BUILD_OUT%

rem NOTE: numbered names, not a fixed object name. build.cmd records them in engine_objects.rsp;
rem the wildcard here is only the cheap "was a full build run?" guard, never link membership.
if not exist "%OUT%\*_BF2.obj" (
    echo   no objects found - run Code\Build\build.cmd first
    exit /b 2
)
if not exist "%OUT%\engine_objects.rsp" (
    echo   no engine object manifest found - run Code\Build\build.cmd first
    exit /b 2
)

rem dice_py.lib IS on this line, unlike build.cmd's diagnostic link. There the
rem library was deliberately withheld so CPython's own internals would not drown
rem out the engine's gap list. Here we are not counting gaps, we are trying to
rem EXECUTE - and PythonHost's calls into CPython have to resolve for real.
rem (Name and location ATTESTED by dice_py.dll's PDB path - see build_python.cmd.)
set PYLIB=%~dp0..\BF2\External\python-embedded\OBJ\python\release\dice_py.lib
if not exist "%PYLIB%" (
    echo   WARNING: %PYLIB% missing - CPython calls will fault. Run python\build_python.cmd.
    set PYLIB=
)

rem bootmain.cpp supplies the entry point AND a fault handler armed via #pragma init_seg(compiler),
rem so a crash prints an image-relative RVA instead of a silent 0xC0000005. It is needed because
rem Game/Main/BF2.cpp compiles its main() inside `#else` of `#if defined(_WIN32)` - DICE's WinMain
rem is still `#if 0` pseudocode - so THE WINDOWS BUILD HAS NO ENTRY POINT OF ITS OWN. It reproduces
rem the ELF main (BF2.cpp:213) statement for statement, except the frame loop is bounded.
rem
rem âš  /FORCE, not /FORCE:UNRESOLVED. The latter alone is not enough once python23.lib joins the
rem link: CPython drags in OLDNAMES.lib and needs /FORCE:MULTIPLE too. Plain /FORCE is both.
rem ---------------------------------------------------------------------------------------------
rem THE OUTPUT LAYOUT MIRRORS THE REAL INSTALL (2026-08-18).
rem
rem The shipped game is FLAT - there is no bin\ directory in it. bf2_w32ded.exe, dice_py.dll,
rem AIDLL_w32ded.dll, RendDX9.dll, Memory_w32ded.dll and the rest all sit in the GAME ROOT beside
rem mods\, python\, pb\, Admin\ and Profiles\. So BIN below is a STAGED INSTALL ROOT: it holds the
rem shipped-shaped artifacts and nothing else, which is what makes "what would ship" legible at a
rem glance. Intermediates (.obj, .pdb, .map, link logs) stay in OBJ - and that split is DICE's own:
rem the PDB path embedded in the shipped dice_py.dll is
rem   ...\External\python-embedded\OBJ\python\release\dice_py.pdb
rem so their intermediates went to OBJ\ too.
rem
rem THE STAGED TREE IS NOT THE GAME INSTALL and nothing is ever written into the user's copy: the
rem drives run this exe with the GAME DIRECTORY as the working directory, which is what makes
rem mods\ and python\ resolve. Do not copy it over the shipped bf2_w32ded.exe.
rem ---------------------------------------------------------------------------------------------
set BIN=%~dp0bin
if not exist "%BIN%" mkdir "%BIN%"

echo == force-linking ==
rem /MD - the harness has to agree with the engine and with dice_py.dll. Without it this one
rem translation unit stayed on the default static CRT and the linker refused all 238 others with
rem LNK2038 "RuntimeLibrary MD_DynamicRelease doesn't match MT_StaticRelease in bootmain.obj",
rem which is the one-CRT-per-image rule being enforced rather than discovered at run time.
cl /nologo /c /MD /std:c++14 /EHsc /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%OUT%\bootmain.obj" "%~dp0bootmain.cpp" >nul 2>&1
rem advapi32/user32/shell32: the three system libraries pythoncore pulls in (getpathp.c reads the
rem registry, dl_nt.c LoadStringA's, posixmodule ShellExecuteA's) - build_python.cmd's own note.
rem Without them /FORCE made the Reg* imports RVA-0 and Py_Initialize died in calculate_path.
rem /PDB and /IMPLIB are explicit because the linker otherwise drops them BESIDE THE OUTPUT, i.e.
rem into the staged install - and a .pdb, .lib and .exp are not shipped artifacts. Only the exe
rem belongs in BIN; everything else is an intermediate and goes to OBJ with the rest.
link /nologo /FORCE /DEBUG /MAP:"%OUT%\bf2_w32ded.map" /PDB:"%OUT%\bf2_w32ded.pdb" ^
     /IMPLIB:"%OUT%\bf2_w32ded.lib" /out:"%BIN%\bf2_w32ded.exe" /subsystem:console ^
     /NODEFAULTLIB:python23.lib ^
     "%OUT%\bootmain.obj" @"%OUT%\engine_objects.rsp" %PYLIB% advapi32.lib user32.lib shell32.lib Ws2_32.lib > "%OUT%\bootlink.log" 2>&1
if not exist "%BIN%\bf2_w32ded.exe" (
    echo   LINK PRODUCED NO IMAGE - see %OUT%\bootlink.log
    exit /b 1
)
for %%A in ("%BIN%\bf2_w32ded.exe") do echo    image: %%~zA bytes  ^(bin\bf2_w32ded.exe^)

rem STAGE dice_py.dll BESIDE THE EXE. That is how the shipped game is laid out, and it is also how
rem the DLL gets FOUND at run time: Windows searches the executable's own directory before the
rem working directory, and the drives run with the GAME directory as cwd.
set PYDLL=%~dp0..\BF2\External\python-embedded\OBJ\python\release\dice_py.dll
if exist "%PYDLL%" (
    copy /y "%PYDLL%" "%BIN%\dice_py.dll" >nul
    for %%A in ("%BIN%\dice_py.dll") do echo    staged: dice_py.dll %%~zA bytes
) else (
    echo   WARNING: %PYDLL% missing - run Code\Build\python\build_python.cmd
)
echo.
echo == running (the caller applies the timeout) ==
"%BIN%\bf2_w32ded.exe"
echo.
echo == exit code: %errorlevel% ==
exit /b %errorlevel%
