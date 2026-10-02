@echo off
setlocal
rem [TEMP] Strict x86 client COM fixture; no actual device acquisition.
call "%~dp0..\Build\vsdevcmd.cmd" x86
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_game_controller_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_ROOT=%~dp0..\.."
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
cl %TASK_FLAGS% /d1reportSingleClassLayoutGameControllerDevice /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\IO\Input\GameControllerDevice.cpp" /Fo"%BF2_BUILD_OUT%\GameControllerDevice.obj" > "%BF2_BUILD_OUT%\class_layout.log" 2>&1
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Core\Debug.cpp" /Fo"%BF2_BUILD_OUT%\Debug.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Core\StringUtil.cpp" /Fo"%BF2_BUILD_OUT%\StringUtil.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0game_controller_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\game_controller_runtime.exe" /PDB:"%BF2_BUILD_OUT%\game_controller_runtime.pdb" "%BF2_BUILD_OUT%\GameControllerDevice.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\fixture.obj" dinput8.lib dxguid.lib user32.lib
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\game_controller_runtime.exe"
exit /b %errorlevel%
