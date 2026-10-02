@echo off
setlocal
rem ===========================================================================
rem SCAFFOLDING - builds and runs the RendDX9 replacement host harness.
rem See RendDX9Host.cpp for what it can and cannot prove.
rem
rem The exe is built INTO Code\Build\bin_renddx9_native so LoadLibraryA finds the matching
rem replacement DLL in the executable's own directory - the same search rule that
rem makes BF2 pick up dice_py.dll beside bf2_w32ded.exe.
rem
rem   usage:  build.cmd                 build and run the native host
rem           build.cmd nobuild         run that host only
rem           build.cmd native          build and run the native-ABI host
rem           build.cmd native nobuild  run the native-ABI host only
rem ===========================================================================
if not "%~1"=="" if /i not "%~1"=="native" if /i not "%~1"=="nobuild" (echo Usage: build.cmd [native] [nobuild]& exit /b 2)
if not "%~2"=="" if /i not "%~2"=="native" if /i not "%~2"=="nobuild" (echo Usage: build.cmd [native] [nobuild]& exit /b 2)
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1

set HERE=%~dp0
set BF2=%HERE%..\..\BF2
set NOBUILD=0
if /i "%~1"=="nobuild" set NOBUILD=1
if /i "%~2"=="nobuild" set NOBUILD=1

set OUT=%HERE%..\..\Build\obj_renddx9host_native
set BIN=%HERE%..\..\Build\bin_renddx9_native
if defined BF2_RENDDX9_HOST_OUT set "OUT=%BF2_RENDDX9_HOST_OUT%"
if defined BF2_RENDDX9_BIN set "BIN=%BF2_RENDDX9_BIN%"

if not exist "%OUT%" mkdir "%OUT%"

if "%NOBUILD%"=="1" goto run
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
rem Resolve the renderer's D3DX/Bink redistributables before DLL loading.
set PATH=D:\B2BF Launcher\Battlefield2;%PATH%
if not exist "%BIN%\RendDX9.dll" (
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
