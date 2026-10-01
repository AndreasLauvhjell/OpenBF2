@echo off
setlocal EnableDelayedExpansion
rem [TEMP] Real client player-input filter; unresolved named dependencies stop.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_messages_finish_20260906\runtime_o2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
set "TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC""
set "TASK_SOURCES=Menu\BF2HudManager Menu\BF2HudManagerPlayerFilter Menu\HudVehicleInformation Menu\HudMessages Menu\HudInformationAdapter Menu\HudRadio Menu\HudBlinker Menu\Ref2System Core\EventManager Core\StringUtil Core\System Core\Debug External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\Node External\BFMeme\System External\BFMeme\Stream External\BFMeme\OStream Math\Vec3 Math\Mat4 Scene\Object\PlayerManager Scene\Object\Object Game\Objects\IObject Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager Game\GameServer\ServerInfo Game\GameLogic\ScoreSystem\TeamScore IO\Input\PlayerInput"
type nul >"%BF2_BUILD_OUT%\compile.log"
for %%F in (%TASK_SOURCES%) do (
  set "TASK_KEY=%%F"
  set "TASK_KEY=!TASK_KEY:\=_!"
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\!TASK_KEY!.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_messages_finish_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
type nul >"%BF2_BUILD_OUT%\objects.rsp"
for %%F in (%TASK_SOURCES%) do (
  set "TASK_KEY=%%F"
  set "TASK_KEY=!TASK_KEY:\=_!"
  echo "%BF2_BUILD_OUT%\!TASK_KEY!.obj" >>"%BF2_BUILD_OUT%\objects.rsp"
)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_messages_finish_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" @"%BF2_BUILD_OUT%\objects.rsp" "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link_force.log" & exit /b 1)
"%BF2_BUILD_OUT%\hud_messages_finish_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
