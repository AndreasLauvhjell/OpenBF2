@echo off
setlocal
rem ===========================================================================
rem SCAFFOLDING - builds and runs the RendDX9 replacement host harness.
rem See RendDX9Host.cpp for what it can and cannot prove.
rem
rem The compatibility exe is built INTO Code\Build\bin_renddx9 and the native
rem exe into Code\Build\bin_renddx9_native so that LoadLibraryA finds the matching
rem replacement DLL in the executable's own directory - the same search rule that
rem makes BF2 pick up dice_py.dll beside bf2_w32ded.exe.
rem
rem   usage:  build.cmd                 build and run the VC7.1-compatible host
rem           build.cmd nobuild         run that host only
rem           build.cmd native          build and run the native-ABI host
rem           build.cmd native nobuild  run the native-ABI host only
rem ===========================================================================
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1

set HERE=%~dp0
set ABI=%HERE%..\..\RendDX9Abi
set BF2=%HERE%..\..\BF2
set NATIVE=0
set NOBUILD=0
if /i "%~1"=="native" set NATIVE=1
if /i "%~2"=="native" set NATIVE=1
if /i "%~1"=="nobuild" set NOBUILD=1
if /i "%~2"=="nobuild" set NOBUILD=1

set OUT=%HERE%..\..\Build\obj_renddx9host
set BIN=%HERE%..\..\Build\bin_renddx9
if "%NATIVE%"=="1" (
    set OUT=%HERE%..\..\Build\obj_renddx9host_native
    set BIN=%HERE%..\..\Build\bin_renddx9_native
)

if not exist "%OUT%" mkdir "%OUT%"

if "%NOBUILD%"=="1" goto run
if "%NATIVE%"=="1" goto build_native

rem /MT for the same reason the DLL uses it: the harness replaces global operator new/delete with
rem Memory.dll's allocator so that a vector buffer allocated inside the DLL can be freed here.
rem That cross-image round trip is the single most important thing this harness tests.
cl /nologo /MT /std:c++14 /permissive /EHsc /W3 /Zi /D_CRT_SECURE_NO_WARNINGS ^
   /Fo"%OUT%\\" /Fd"%OUT%\host.pdb" /Fe"%BIN%\RendDX9Host.exe" ^
   "%HERE%RendDX9Host.cpp" "%ABI%\Vc71String.cpp" "%ABI%\Msvcr71Heap.cpp" "%ABI%\BfMemory.cpp" ^
   /link /MACHINE:X86 "%~dp0..\..\Build\obj_renddx9\Memory.lib" kernel32.lib
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)
goto run

:build_native
rem /MD and BF2_PLUGIN_ABI_NATIVE select the ABI shared by the rebuilt client executable. The native
rem host uses its own CRT/STL directly, so it neither compiles the VC7.1 heap/string adapters nor
rem links Memory.lib. Its directory is shared only with the matching native RendDX9.dll.
cl /nologo /MD /std:c++14 /permissive /EHsc /W3 /Zi /D_CRT_SECURE_NO_WARNINGS ^
   /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0 /FI"%BF2%\Core\BuildConfig.h" ^
   /Fo"%OUT%\\" /Fd"%OUT%\host.pdb" /Fe"%BIN%\RendDX9Host.exe" ^
   "%HERE%RendDX9Host.cpp" ^
   /link /MACHINE:X86 kernel32.lib
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

:run
echo.
rem ⚠ THE GAME DIRECTORY MUST BE ON PATH BEFORE THE EXE STARTS, not merely before main() runs.
rem The compatibility harness links Memory.lib, so Memory.dll is a STATIC import resolved before
rem main. Both flavours also load the reference D3DX/Bink redistributables through the DLL. Calling
rem SetDllDirectory inside main is too late for a static import.
set PATH=D:\B2BF Launcher\Battlefield2;%PATH%
if "%NATIVE%"=="1" if not exist "%BIN%\RendDX9.dll" (
    echo RUN FAILED - native RendDX9.dll is missing from "%BIN%"
    echo Build it first with Code\Build\renddx9.cmd native.
    exit /b 1
)
pushd "%BIN%"
rem .\ IS REQUIRED. Windows only searches the current directory for an executable when
rem NoDefaultCurrentDirectoryInExePath is unset, and it is set on this machine.
.\RendDX9Host.exe
set RC=%ERRORLEVEL%
popd
exit /b %RC%
