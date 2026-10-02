@echo off
setlocal EnableDelayedExpansion
rem [TEMP] Native filter and Squad regressions, typed observers, uncalled /FORCE seams.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_game_filter_final_20260906\runtime_o2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /O2 /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\Include /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\PC %BF2_RUNTIME_OPT%"
for %%F in (Menu\BF2HudManagerGameFilter Menu\HudAmmo Menu\HudQuestionManager Menu\HudInformationAdapter Menu\HudRadio Menu\HudSquad Menu\HudColors Menu\BF2HudManager Menu\BfListBoxData Menu\BfNewListBoxNode External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\Node External\BFMeme\System Game\HudInformationLayer Menu\HudBlinker Core\EventManager Core\StringUtil Core\System Core\Debug Scene\Object\PlayerManager Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager) do (
  set "TASK_OBJECT=%%F"
  set "TASK_OBJECT=!TASK_OBJECT:\=_!"
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%F.cpp" /Fo"%BF2_BUILD_OUT%\!TASK_OBJECT!.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_game_filter_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
rem 2026-09-26 (E2): squadRegressionMain now calls HudSquad's private row helpers through this clean
rem friend TU; without it those calls link to RVA 0 under /FORCE and crash with no output.
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_squad_private_access.cpp" /Fo"%BF2_BUILD_OUT%\private_access.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_game_filter_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" "%BF2_BUILD_OUT%\*.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if not exist "%BF2_BUILD_OUT%\hud_game_filter_runtime.exe" exit /b 1
"%BF2_BUILD_OUT%\hud_game_filter_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
