@echo off
setlocal
rem [TEMP] Native x86 fixture for HudVoip and the three BfListBoxData entries
rem its row painter needs. /FORCE:UNRESOLVED only covers the HUD families the
rem manager header pulls in but this test never calls.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_voip_20260915"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "PYINC=/DPy_NO_ENABLE_SHARED /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\Include /I%TASK_ROOT%\Code\BF2\External\python-embedded\py-src-2.3.4\PC"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc %BF2_FIXTURE_OPT% /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0 %PYINC%"
set "TASK_OBJECTS="
for %%S in (Object TypeDescriptor Node System NamedObjectRegistry Data IntData FloatData Quad Stream OStream Effect ColorEffect EffectNode Style TransformNode ObjectList ObjectCollection) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Debug StringUtil EventManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Core\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (HudVoip BfListBoxData HudColors BF2HudManager HudBlinker) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
rem PlayerManager's constructor builds a CPersistentExplosionEffectManager; without
rem that object the /FORCE link resolves it to RVA 0 and the fixture dies silently.
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Scene\Object\PlayerManager.cpp" /Fo"%BF2_BUILD_OUT%\PlayerManager.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\PlayerManager.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\Objects\Fx\PersistentExplosionEffectManager.cpp" /Fo"%BF2_BUILD_OUT%\PersistentExplosionEffectManager.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\PersistentExplosionEffectManager.obj""
rem IPlayer.cpp carries the interface's own out-of-line pieces; without it the
rem player boundary's construction resolves to RVA 0 the same silent way.
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\Objects\IPlayer.cpp" /Fo"%BF2_BUILD_OUT%\IPlayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\IPlayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_voip_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_voip.exe" /MAP:"%BF2_BUILD_OUT%\hud_voip.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if not exist "%BF2_BUILD_OUT%\hud_voip.exe" (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_voip.exe"
exit /b %errorlevel%
