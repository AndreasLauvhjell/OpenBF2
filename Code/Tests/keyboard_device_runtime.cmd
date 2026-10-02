@echo off
setlocal
call "%~dp0..\Build\vsdevcmd.cmd" x86
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_keyboard_device_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_ROOT=%~dp0..\.."
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\IO\Input\KeyboardDevice.cpp" /Fo"%BF2_BUILD_OUT%\KeyboardDevice.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Core\Debug.cpp" /Fo"%BF2_BUILD_OUT%\Debug.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Core\StringUtil.cpp" /Fo"%BF2_BUILD_OUT%\StringUtil.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0keyboard_device_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\keyboard_device_runtime.exe" /PDB:"%BF2_BUILD_OUT%\keyboard_device_runtime.pdb" "%BF2_BUILD_OUT%\KeyboardDevice.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\fixture.obj" dinput8.lib dxguid.lib user32.lib
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\keyboard_device_runtime.exe"
exit /b %errorlevel%
