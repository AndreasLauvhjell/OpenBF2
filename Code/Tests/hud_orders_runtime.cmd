@echo off
setlocal
rem [TEMP] Native client order/request fixture; wider /FORCE dependencies stay uncalled.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_order_continue_20260906\runtime_o2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC" /O2 %HUD_SEND_EXTRA_FLAGS%"
set "TASK_OBJECTS="
for %%F in (Menu\BF2HudManagerOrders Menu\HudInformationAdapter Menu\BF2HudManager Menu\BF2HudManagerChatInput Menu\BF2HudManagerState Menu\HudKitSelection Menu\HudCommander Menu\HudSquad Menu\BfListBoxData Menu\HudBlinker Core\EventManager Core\StringUtil Core\System Core\Debug External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor Scene\Object\PlayerManager Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager Game\GameServer\ServerInfo Game\GameLogic\ScoreSystem\TeamScore) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%~nF.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_order_private_access.cpp" /Fo"%BF2_BUILD_OUT%\access.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 exit /b 1
set TASK_OBJECTS=%TASK_OBJECTS% "%BF2_BUILD_OUT%\access.obj"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_orders_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_orders_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_orders_runtime.pdb" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
rem LINK may return1 for the expected /FORCE diagnostic; execution is the acceptance check.
if not exist "%BF2_BUILD_OUT%\hud_orders_runtime.exe" exit /b 1
"%BF2_BUILD_OUT%\hud_orders_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
