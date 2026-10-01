@echo off
setlocal
rem [TEMP] Native client chat send/spawn fixture; wider /FORCE dependencies stay uncalled.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_primary_continue_20260906\actions_runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC" /O2 %HUD_SEND_EXTRA_FLAGS%"
set "TASK_OBJECTS="
for %%F in (Menu\BF2HudManager Menu\BF2HudManagerChatInput Menu\BF2HudManagerState Menu\HudKitSelection Menu\HudCommander Menu\HudSquad Menu\BfListBoxData Menu\HudBlinker Core\EventManager Core\StringUtil Core\System Core\Debug External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor Scene\Object\PlayerManager Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager Game\GameServer\ServerInfo Game\GameLogic\ScoreSystem\TeamScore) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%~nF.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_primary_actions_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_primary_actions_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_primary_actions_runtime.pdb" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
rem LINK may return1 for the expected /FORCE diagnostic; execution is the acceptance check.
if not exist "%BF2_BUILD_OUT%\hud_primary_actions_runtime.exe" exit /b 1
"%BF2_BUILD_OUT%\hud_primary_actions_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
