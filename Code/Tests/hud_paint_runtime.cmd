@echo off
setlocal
rem [TEMP] Actual HUD body with typed external callback observers; uncalled /FORCE frontiers.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_paint_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy %BF2_RUNTIME_OPT% /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DPy_NO_ENABLE_SHARED /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\Include /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\PC"
for %%s in (Menu\BF2HudManager Menu\HudBlinker Menu\Ref2System Core\StringUtil Scene\Object\PlayerManager Game\Objects\Fx\PersistentExplosionEffectManager External\BFMeme\Object External\BFMeme\TypeDescriptor External\BFMeme\Stream External\BFMeme\OStream External\BFMeme\System Math\Mat4 Math\Vec3 Core\ClassManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (
    type "%BF2_BUILD_OUT%\compile.log"
    exit /b 1
  )
)
cl %TASK_FLAGS% /d1reportSingleClassLayoutBF2HudManager /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_paint_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\layout.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\layout.log"
  exit /b 1
)
link /nologo /FORCE /INCREMENTAL:NO /DEBUG /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_paint_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_paint_runtime.pdb" /MAP:"%BF2_BUILD_OUT%\hud_paint_runtime.map" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\Ref2System.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\PlayerManager.obj" "%BF2_BUILD_OUT%\PersistentExplosionEffectManager.obj" "%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Stream.obj" "%BF2_BUILD_OUT%\OStream.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\Mat4.obj" "%BF2_BUILD_OUT%\Vec3.obj" "%BF2_BUILD_OUT%\ClassManager.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link_force.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_paint_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
