@echo off
setlocal
rem [TEMP] Isolated x86 client construction fixture; /FORCE leaves the unwritten HUD table visible.
rem No missing virtual method is called or supplied by a fixture override.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_manager_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Zi /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Menu\BF2HudManager.cpp" /Fo"%BF2_BUILD_OUT%\BF2HudManager.obj" /Fd"%BF2_BUILD_OUT%\compile.pdb"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Menu\HudBlinker.cpp" /Fo"%BF2_BUILD_OUT%\HudBlinker.obj" /Fd"%BF2_BUILD_OUT%\compile.pdb"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_manager_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" /Fd"%BF2_BUILD_OUT%\compile.pdb"
if errorlevel 1 exit /b 1
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_manager_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_manager_runtime.pdb" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_manager_runtime.exe"
if errorlevel 1 exit /b 1
rem The controller family has complete bodies: prove it also links and runs WITHOUT /FORCE.
cl %TASK_FLAGS% /DHUD_BLINK_ONLY=1 /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_manager_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture_blink.obj" /Fd"%BF2_BUILD_OUT%\compile.pdb"
if errorlevel 1 exit /b 1
link /nologo /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_blink_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_blink_runtime.pdb" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\fixture_blink.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_blink_runtime.exe"
exit /b %errorlevel%
