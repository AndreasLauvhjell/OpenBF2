@echo off
setlocal
rem [TEMP] Actual four HUD camera methods and native dependency lifetimes; wider uncalled code uses /FORCE.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_camera_20260906"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
set "TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\Include /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\PC"
for %%F in (Menu\BF2HudManager Menu\BF2HudManagerCamera Menu\HudBlinker Core\Debug Core\StringUtil Math\Mat4 Math\Vec3 Scene\Object\PlayerManager Scene\Object\Object Game\Objects\IObject Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager Game\Objects\IObjectTemplate Game\Objects\ObjectTemplate Game\Objects\Base\SimpleObjectTemplate Game\Objects\Base\BundleTemplate Game\Objects\PerClassTemplateBodies Game\Common\GameEvent Game\GameEvents\CommanderCamEvent) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_camera_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_camera_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_camera_runtime.pdb" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\BF2HudManagerCamera.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\Mat4.obj" "%BF2_BUILD_OUT%\Vec3.obj" "%BF2_BUILD_OUT%\PlayerManager.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\IObject.obj" "%BF2_BUILD_OUT%\IPlayer.obj" "%BF2_BUILD_OUT%\PersistentExplosionEffectManager.obj" "%BF2_BUILD_OUT%\IObjectTemplate.obj" "%BF2_BUILD_OUT%\ObjectTemplate.obj" "%BF2_BUILD_OUT%\SimpleObjectTemplate.obj" "%BF2_BUILD_OUT%\BundleTemplate.obj" "%BF2_BUILD_OUT%\PerClassTemplateBodies.obj" "%BF2_BUILD_OUT%\GameEvent.obj" "%BF2_BUILD_OUT%\CommanderCamEvent.obj" "%BF2_BUILD_OUT%\fixture.obj" user32.lib
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_camera_runtime.exe"
exit /b %errorlevel%
