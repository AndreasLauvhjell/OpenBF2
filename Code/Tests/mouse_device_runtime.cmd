@echo off
setlocal
rem [TEMP] Real MouseDevice, controlled typed COM, real Debug filter. Strict link.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_mouse_device_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%s in (IO\Input\MouseDevice Core\Debug Core\StringUtil) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0mouse_device_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\mouse_device_runtime.exe" /PDB:"%BF2_BUILD_OUT%\mouse_device_runtime.pdb" "%BF2_BUILD_OUT%\MouseDevice.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\fixture.obj" user32.lib dxguid.lib dinput8.lib
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\mouse_device_runtime.exe"
exit /b %errorlevel%
