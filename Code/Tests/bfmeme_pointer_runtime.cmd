@echo off
setlocal
rem [TEMP] Strict actual queue, then actual Pointer/System/Node callbacks and close.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_bfmeme_pointer_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\EventQueue.cpp" /Fo"%BF2_BUILD_OUT%\EventQueue.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /DBFMEME_QUEUE_ONLY=1 /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0bfmeme_pointer_runtime.cpp" /Fo"%BF2_BUILD_OUT%\queue_fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\queue_strict.exe" /PDB:"%BF2_BUILD_OUT%\queue_strict.pdb" "%BF2_BUILD_OUT%\EventQueue.obj" "%BF2_BUILD_OUT%\queue_fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\queue_strict.exe"
if errorlevel 1 exit /b 1
for %%s in (Object TypeDescriptor Stream Node Pointer System NamedObjectRegistry) do (
  cl %TASK_FLAGS% /d1reportSingleClassLayoutPointer /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0bfmeme_pointer_runtime.cpp" /Fo"%BF2_BUILD_OUT%\pointer_fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\pointer_runtime.exe" /PDB:"%BF2_BUILD_OUT%\pointer_runtime.pdb" /MAP:"%BF2_BUILD_OUT%\pointer_runtime.map" "%BF2_BUILD_OUT%\EventQueue.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\Pointer.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\pointer_fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\pointer_runtime.exe"
exit /b %errorlevel%
