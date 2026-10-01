@echo off
setlocal
rem [TEMP] Bounded GameLogic::load archive-prefix regression linked to an isolated x86 client build.
rem Usage: gamelogic_archive_mount_runtime.cmd <absolute CMake client build directory>
if "%~1"=="" exit /b 2
if not exist "%~1\CMakeFiles\bf2_client.dir\objects1.rsp" exit /b 2
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_gamelogic_archive_mount_runtime"
set "TESTOUT=%BF2_BUILD_OUT%\gamelogic_archive_mount_runtime"
if not exist "%TESTOUT%" mkdir "%TESTOUT%"

cl /nologo /EHsc /std:c++14 /permissive /MD /Zi /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 ^
 /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0gamelogic_archive_mount_runtime.cpp" ^
 /Fo"%TESTOUT%\runtime.obj" /Fd"%TESTOUT%\compile.pdb" > "%TESTOUT%\compile.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\compile.log" & exit /b 1 )

if exist "%TESTOUT%\runtime.exe" del /q "%TESTOUT%\runtime.exe"
pushd "%~1"
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /NODEFAULTLIB:python23.lib ^
 /IGNORE:4006,4088,4098 /OUT:"%TESTOUT%\runtime.exe" /PDB:"%TESTOUT%\runtime.pdb" ^
 "%TESTOUT%\runtime.obj" @CMakeFiles\bf2_client.dir\objects1.rsp ^
 "%TASK_ROOT%\Code\BF2\External\python-embedded\OBJ\python\release\dice_py.lib" ^
 advapi32.lib crypt32.lib dbghelp.lib dinput8.lib dsound.lib dxguid.lib gdi32.lib ole32.lib ^
 psapi.lib shell32.lib user32.lib winmm.lib ws2_32.lib > "%TESTOUT%\link.log" 2>&1
set "TASK_LINK_RESULT=%errorlevel%"
popd
if not "%TASK_LINK_RESULT%"=="0" ( type "%TESTOUT%\link.log" & exit /b %TASK_LINK_RESULT% )
copy /y "%TASK_ROOT%\Code\BF2\External\python-embedded\OBJ\python\release\dice_py.dll" "%TESTOUT%\dice_py.dll" >nul
if errorlevel 1 exit /b 1
echo Diagnostic /FORCE fixture; only GameLogic::load's archive-selection prefix is attested.
"%TESTOUT%\runtime.exe"
exit /b %errorlevel%
