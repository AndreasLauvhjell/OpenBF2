@echo off
setlocal
rem [TEMP] Real EventQueue/visitors; typed Node recorders isolate known dispatch+6C.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\event_dispatch_20260905\runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%s in (Object TypeDescriptor Stream Node System EventQueue InputNodeVisitors) do (
  cl %TASK_FLAGS% /d1reportSingleClassLayoutEvent1NodeVisitor /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\External\BFMeme\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0event_dispatch_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /OUT:"%BF2_BUILD_OUT%\event_dispatch_runtime.exe" /PDB:"%BF2_BUILD_OUT%\event_dispatch_runtime.pdb" /MAP:"%BF2_BUILD_OUT%\event_dispatch_runtime.map" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\EventQueue.obj" "%BF2_BUILD_OUT%\InputNodeVisitors.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
set "TASK_LINK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\link_force.log"
if not "%TASK_LINK_RESULT%"=="0" exit /b %TASK_LINK_RESULT%
"%BF2_BUILD_OUT%\event_dispatch_runtime.exe"
exit /b %errorlevel%
