@echo off
setlocal
rem [TEMP] Native client chat send/spawn fixture; wider /FORCE dependencies stay uncalled.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_events_map_20260906\runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC" /O2 %HUD_EVENTS_EXTRA_FLAGS%"
set "TASK_OBJECTS="
for %%F in (Menu\BF2HudManager Menu\BF2HudManagerChatInput Menu\BF2HudManagerEvents Menu\Bf2MenuMapNode Menu\HudMessages Menu\HudQuestionManager Menu\HudInformationAdapter Game\HudInformationLayer Menu\HudKitSelection Menu\HudBlinker Core\EventManager Core\StringUtil Core\System Core\Debug External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor Scene\Object\PlayerManager Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager Game\GameServer\ServerInfo Game\GameLogic\ScoreSystem\TeamScore) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%~nF.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_events_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_events_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_events_runtime.pdb" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_events_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
