@echo off
setlocal
rem [TEMP] Actual client ability actions; unresolved wider tables remain uncalled.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_ability_actions_20260906\runtime"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC" /O2 %HUD_ABILITY_EXTRA_FLAGS%"
set "TASK_OBJECTS="
for %%F in (Menu\BF2HudManager Menu\BF2HudManagerAbilities Menu\BF2HudManagerState Menu\HudCommander Menu\HudSquad Menu\HudKitSelection Menu\BfListBoxData Menu\HudBlinker Core\ClassManager Core\EventManager Core\StringUtil Core\System Core\Debug External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor Scene\Object\PlayerManager Scene\Object\ObjectManager Scene\Object\ObjectCache Scene\Object\Object Scene\Object\ObjectPredicator Scene\Object\BinTreeCuller Scene\Object\QuadTreeCuller Game\Objects\ObjectTemplate Game\Objects\IObjectTemplate Game\Objects\IObject Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager Game\Objects\Game\SupplyDropManager Game\Common\GameEvent Game\GameEvents\UAVEvent Game\GameEvents\DropVehicleEvent Game\GameEvents\SupplyDropEvent Game\Utility\Vars Math\Vec3 Math\Mat4 IO\BitStream) do (
  for %%D in ("%BF2_BUILD_OUT%\%%F.obj") do if not exist "%%~dpD" mkdir "%%~dpD"
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%F.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_ability_actions_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_ability_actions_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_ability_actions_runtime.pdb" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if not exist "%BF2_BUILD_OUT%\hud_ability_actions_runtime.exe" exit /b 1
"%BF2_BUILD_OUT%\hud_ability_actions_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
