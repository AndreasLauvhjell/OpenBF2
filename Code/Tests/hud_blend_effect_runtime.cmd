@echo off
setlocal
rem [TEMP] Native x86 fixture for the HUD blend family: BlendFuncEffect,
rem Bf2BlendFuncEffect, the four Bf2TransformNode accessors and builder rows
rem 133/136/137/141. /FORCE:UNRESOLVED only covers the split, picture, bar,
rem compass and text families the builder header pulls in but these never call.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_blend_effect_20260915"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc %BF2_FIXTURE_OPT% /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0"
set "TASK_OBJECTS="
for %%S in (Object TypeDescriptor Node System NamedObjectRegistry Data IntData Quad Stream Effect ColorEffect BlendFuncEffect EffectNode Style TransformNode) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Debug StringUtil EventManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Core\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Bf2BlendFuncEffect Bf2ColorEffect Bf2EffectNode Bf2TransformNode BfTransformNode HudBlinker HudColors) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\HudInformationLayer.cpp" /Fo"%BF2_BUILD_OUT%\HudInformationLayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\HudInformationLayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_blend_effect_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_blend_effect.exe" /MAP:"%BF2_BUILD_OUT%\hud_blend_effect.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_blend_effect.exe"
if not "%errorlevel%"=="0" exit /b 1
rem The duplicate arm falls THROUGH to the second diagnostic in the source, but
rem ~Debug aborts on every emitted DebugTypeError, so only the first line can be
rem observed with the type enabled. The fall-through is proven by the main run.
"%BF2_BUILD_OUT%\hud_blend_effect.exe" duplicate-death >"%BF2_BUILD_OUT%\duplicate_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=2166 Framealready has a BlendEffect!" "%BF2_BUILD_OUT%\duplicate_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\duplicate_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_DUPLICATE_RETURNED" "%BF2_BUILD_OUT%\duplicate_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_blend_effect.exe" blend-death >"%BF2_BUILD_OUT%\blend_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=2186 Failed setting BlendEffect on node: Absent" "%BF2_BUILD_OUT%\blend_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\blend_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_BLEND_RETURNED" "%BF2_BUILD_OUT%\blend_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_blend_effect.exe" alpha-death >"%BF2_BUILD_OUT%\alpha_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=2249 Failed setting alpha variable on node: Frame" "%BF2_BUILD_OUT%\alpha_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\alpha_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_ALPHA_RETURNED" "%BF2_BUILD_OUT%\alpha_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_blend_effect.exe" rgb-death >"%BF2_BUILD_OUT%\rgb_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=2276 Failed setting RGB variable on node: Frame" "%BF2_BUILD_OUT%\rgb_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\rgb_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_RGB_RETURNED" "%BF2_BUILD_OUT%\rgb_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_blend_effect.exe" size-death >"%BF2_BUILD_OUT%\size_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=1808 Failed setting size on node: Absent" "%BF2_BUILD_OUT%\size_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\size_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_SIZE_RETURNED" "%BF2_BUILD_OUT%\size_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_blend_effect.exe" pos-death >"%BF2_BUILD_OUT%\pos_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=1795 Failed setting position on node: Absent" "%BF2_BUILD_OUT%\pos_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\pos_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_POS_RETURNED" "%BF2_BUILD_OUT%\pos_death.log" >nul
if not errorlevel 1 exit /b 1
echo HUD blend effect classes, transform accessors and their four rows passed
exit /b 0
