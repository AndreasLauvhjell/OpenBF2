@echo off
setlocal
rem [TEMP] Actual HUD query, PlayerManager and console component checks.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_queries_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /d1reportSingleClassLayoutBF2HudManager /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\Include /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\PC"
for %%s in (Menu\BF2HudManager Menu\HudBlinker Core\StringUtil IO\Console\Console Scene\Object\PlayerManager Game\Objects\Fx\PersistentExplosionEffectManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_queries_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_queries_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_queries_runtime.pdb" /MAP:"%BF2_BUILD_OUT%\hud_queries_runtime.map" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\Console.obj" "%BF2_BUILD_OUT%\PlayerManager.obj" "%BF2_BUILD_OUT%\PersistentExplosionEffectManager.obj" "%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_queries_runtime.exe"
exit /b %errorlevel%
