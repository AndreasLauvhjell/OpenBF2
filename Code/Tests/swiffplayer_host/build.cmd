@echo off
setlocal
rem ===========================================================================
rem SCAFFOLDING - builds and runs the SwiffPlayer replacement host harness.
rem See SwiffPlayerHost.cpp for what it can and cannot prove.
rem
rem The exe is built INTO Code\Build\bin_swiffplayer so LoadLibraryA finds the
rem replacement DLL in the executable's own directory.
rem
rem   usage:  build.cmd          build and run
rem           build.cmd nobuild  run only
rem ===========================================================================
call "%~dp0..\..\Build\vsdevcmd.cmd" x86

set HERE=%~dp0
set ABI=%HERE%..\..\RendDX9Abi
set OUT=%HERE%..\..\Build\obj_swiffplayerhost
set BIN=%HERE%..\..\Build\bin_swiffplayer

if not exist "%OUT%" mkdir "%OUT%"

if /i "%~1"=="nobuild" goto run

rem /MT for the same reason the DLL uses it: the harness replaces global operator new/delete with
rem Memory.dll's allocator so a vector buffer allocated inside the DLL can be freed here. That
rem cross-image round trip is the single most important thing this harness tests. Reuses the shared
rem RendDX9Abi kit (Vc71String/Msvcr71Heap/BfMemory) verbatim.
cl /nologo /MT /std:c++14 /permissive /EHsc /W3 /Zi /D_CRT_SECURE_NO_WARNINGS ^
   /Fo"%OUT%\\" /Fd"%OUT%\host.pdb" /Fe"%BIN%\SwiffPlayerHost.exe" ^
   "%HERE%SwiffPlayerHost.cpp" "%ABI%\Vc71String.cpp" "%ABI%\Msvcr71Heap.cpp" "%ABI%\BfMemory.cpp" ^
   /link /MACHINE:X86 "%HERE%..\..\Build\obj_swiffplayer\Memory.lib" kernel32.lib
if errorlevel 1 (
    echo BUILD FAILED
    exit /b 1
)

:run
echo.
rem THE GAME DIRECTORY MUST BE ON PATH BEFORE THE EXE STARTS. The harness links Memory.lib, so
rem Memory.dll is a STATIC import the loader resolves before a single instruction of ours runs -
rem SetDllDirectory inside main is far too late.
set PATH=D:\B2BF Launcher\Battlefield2;%PATH%
pushd "%BIN%"
rem .\ IS REQUIRED - NoDefaultCurrentDirectoryInExePath is set on this machine.
.\SwiffPlayerHost.exe
set RC=%ERRORLEVEL%
popd
exit /b %RC%
