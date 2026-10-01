@echo off
setlocal
rem [TEMP] Real Ammo ability update and native owners plus binding/action regression.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_ammo_frame_20260906\runtime_o2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
type nul >"%BF2_BUILD_OUT%\compile.log"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\Include" /I"%TASK_ROOT%\External\python-embedded\py-src-2.3.4\PC" %BF2_TEST_OPT%"
set "TASK_OBJECTS="
rem Set BF2_TEST_REUSE_OBJS only for local fixture iteration after a full compile.
for %%F in (Menu\HudAmmo Menu\BF2HudManager Menu\HudVehicleInformation Menu\HudInformationAdapter Scene\Object\PlayerManager Game\Objects\IPlayer Game\Objects\Fx\PersistentExplosionEffectManager Game\HudInformationLayer Menu\HudBlinker Core\EventManager Core\StringUtil Core\Debug External\BFMeme\Data External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\System External\BFMeme\NamedObjectRegistry External\BFMeme\Stream External\BFMeme\Pointer Game\Objects\IObjectComponent Game\Objects\IObject Scene\Object\Object Math\Mat4 Math\Vec3 Core\System Game\Utility\Vars Game\Objects\IObjectTemplate Game\Objects\ObjectTemplate Game\Objects\Base\SimpleObjectTemplate Game\Objects\Base\BundleTemplate Game\Objects\ItemTemplate Game\Objects\PerClassTemplateBodies Game\Objects\GenericFireArmTemplate Game\Objects\Game\KitTemplate Game\Objects\Game\Kit Game\Objects\Item Game\Objects\Base\Bundle Game\Objects\Base\SimpleObject Game\Objects\Game\AnimatedBundle Game\Objects\Weapons\GenericFireArm Game\Objects\Weapons\AmmoComponents\ReplenishingAmmoComp Game\Objects\Weapons\AmmoComponents\AmmoComp Game\Objects\GameLogicComp\AbilityGLComp Game\Objects\GameLogicComp\AbilityGLCompTemplate) do (
  for %%D in ("%BF2_BUILD_OUT%\%%F.obj") do if not exist "%%~dpD" mkdir "%%~dpD"
  if not defined BF2_TEST_REUSE_OBJS cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%F.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0hud_ammo_frame_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_ammo_frame_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" user32.lib >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link_force.log" & exit /b 1)
"%BF2_BUILD_OUT%\hud_ammo_frame_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%




