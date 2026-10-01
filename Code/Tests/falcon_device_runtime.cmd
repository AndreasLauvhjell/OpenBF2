@echo off
setlocal
rem [TEMP] Bounded Falcon fixture; original Novint library, no SDK replacements.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_falcon_device_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
lib /nologo /machine:x86 /def:"%TASK_ROOT%\Code\BF2\External\NovintHFX\NovintHFX.def" /out:"%BF2_BUILD_OUT%\NovintHFX.lib"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\IO\Input\FalconDevice.cpp" /Fo"%BF2_BUILD_OUT%\FalconDevice.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0falcon_device_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\falcon_device_runtime.exe" /PDB:"%BF2_BUILD_OUT%\falcon_device_runtime.pdb" "%BF2_BUILD_OUT%\FalconDevice.obj" "%BF2_BUILD_OUT%\fixture.obj" "%BF2_BUILD_OUT%\NovintHFX.lib" user32.lib
if errorlevel 1 exit /b 1
rem Loader searches the unchanged original DLL directory through PATH; no original file is written.
set "PATH=D:\B2BF Launcher\Battlefield2;%PATH%"
"%BF2_BUILD_OUT%\falcon_device_runtime.exe"
exit /b %errorlevel%
