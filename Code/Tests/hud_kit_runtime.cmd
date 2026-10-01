@echo off
setlocal
rem [TEMP] Real kit bodies; uncalled map/question and wider HUD dependencies use /FORCE.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_kit_hud_owned_20260906"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /O2 /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\Include /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\PC %BF2_RUNTIME_OPT%"
for %%F in (Menu\HudKitSelection Menu\HudInformationAdapter Menu\BF2HudManager Menu\BF2HudManagerConfiguration Game\HudInformationLayer Menu\HudBlinker Core\EventManager Core\StringUtil Core\Debug Scene\Object\PlayerManager Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_kit_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_kit_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_kit_runtime.pdb" "%BF2_BUILD_OUT%\HudKitSelection.obj" "%BF2_BUILD_OUT%\HudInformationAdapter.obj" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\BF2HudManagerConfiguration.obj" "%BF2_BUILD_OUT%\HudInformationLayer.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\EventManager.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\PlayerManager.obj" "%BF2_BUILD_OUT%\IPlayer.obj" "%BF2_BUILD_OUT%\PersistentExplosionEffectManager.obj" "%BF2_BUILD_OUT%\fixture.obj" user32.lib
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_kit_runtime.exe"
exit /b %errorlevel%
