@echo off
setlocal
rem [TEMP] Bounded localization fixture against the actual client CMake objects.
rem Usage: client_localization_runtime.cmd <absolute CMake build> [stock English.utxt] [stock mod directory]
if "%~1"=="" exit /b 2
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_client_localization_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
cl /nologo /EHsc /std:c++14 /permissive /MD /Z7 /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0client_localization_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
pushd "%~1"
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /NODEFAULTLIB:python23.lib /IGNORE:4006,4088,4098 /OUT:"%BF2_BUILD_OUT%\client_localization_runtime.exe" /PDB:"%BF2_BUILD_OUT%\client_localization_runtime.pdb" "%BF2_BUILD_OUT%\fixture.obj" @CMakeFiles\bf2_client.dir\objects1.rsp "%TASK_ROOT%\Code\BF2\External\python-embedded\OBJ\python\release\dice_py.lib" advapi32.lib crypt32.lib dbghelp.lib dinput8.lib dsound.lib dxguid.lib gdi32.lib ole32.lib psapi.lib shell32.lib user32.lib winmm.lib ws2_32.lib
set "TASK_RESULT=%errorlevel%"
popd
if not "%TASK_RESULT%"=="0" exit /b %TASK_RESULT%
copy /y "%~1\bin_client\dice_py.dll" "%BF2_BUILD_OUT%\dice_py.dll" >nul
"%BF2_BUILD_OUT%\client_localization_runtime.exe" %2 %3
exit /b %errorlevel%
