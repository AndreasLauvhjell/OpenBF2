@echo off
setlocal
rem [TEMP] Native x86 fixture for MoveEffect, Bf2MoveEffect and the two rows they
rem complete: 59 createTransformNode and 132 addNodeMoveShowEffect.
rem /FORCE:UNRESOLVED only covers the split, picture, bar, compass, text, blend
rem and marker families the builder header pulls in but these never call.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_move_effect_20260915"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc %BF2_FIXTURE_OPT% /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0"
set "TASK_OBJECTS="
for %%S in (Object TypeDescriptor Node System NamedObjectRegistry Data IntData FloatData Quad Stream Effect ColorEffect BlendFuncEffect MoveEffect EffectNode ShowEffectNode CullNode Style TransformNode) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Debug StringUtil EventManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Core\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Bf2MoveEffect Bf2ObjectMarkerNode Bf2BlendFuncEffect Bf2ColorEffect Bf2CullNode Bf2EffectNode Bf2TransformNode BfTransformNode HudBlinker HudColors) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\HudInformationLayer.cpp" /Fo"%BF2_BUILD_OUT%\HudInformationLayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\HudInformationLayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_move_effect_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_move_effect.exe" /MAP:"%BF2_BUILD_OUT%\hud_move_effect.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_move_effect.exe"
if not "%errorlevel%"=="0" exit /b 1
"%BF2_BUILD_OUT%\hud_move_effect.exe" create-death >"%BF2_BUILD_OUT%\create_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=655 Node: Panel, already exists!!" "%BF2_BUILD_OUT%\create_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\create_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_CREATE_RETURNED" "%BF2_BUILD_OUT%\create_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_move_effect.exe" move-death >"%BF2_BUILD_OUT%\move_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=2155 Failed setting MoveShowEffect on node: Absent" "%BF2_BUILD_OUT%\move_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\move_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_MOVE_RETURNED" "%BF2_BUILD_OUT%\move_death.log" >nul
if not errorlevel 1 exit /b 1
echo HUD move effect classes and their two rows passed
exit /b 0
