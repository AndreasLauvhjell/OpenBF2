@echo off
setlocal
rem [TEMP] Native x86 fixture for Bf2ObjectMarkerNode and the nine HUD builder
rem rows 76..84. /FORCE:UNRESOLVED only covers the split, picture, bar, compass,
rem text and blend families the builder header pulls in but these never call.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_object_marker_20260915"
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
for %%S in (Bf2ObjectMarkerNode Bf2BlendFuncEffect Bf2ColorEffect Bf2EffectNode Bf2TransformNode BfTransformNode HudBlinker HudColors) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\HudInformationLayer.cpp" /Fo"%BF2_BUILD_OUT%\HudInformationLayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\HudInformationLayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_object_marker_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_object_marker.exe" /MAP:"%BF2_BUILD_OUT%\hud_object_marker.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_object_marker.exe"
if not "%errorlevel%"=="0" exit /b 1
"%BF2_BUILD_OUT%\hud_object_marker.exe" duplicate-death >"%BF2_BUILD_OUT%\duplicate_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=956 Node: Igla, already exists!!" "%BF2_BUILD_OUT%\duplicate_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\duplicate_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_CREATE_RETURNED" "%BF2_BUILD_OUT%\duplicate_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_object_marker.exe" texture-death >"%BF2_BUILD_OUT%\texture_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=968 Failed setting texture on node: Absent" "%BF2_BUILD_OUT%\texture_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\texture_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_TEXTURE_RETURNED" "%BF2_BUILD_OUT%\texture_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_object_marker.exe" size-death >"%BF2_BUILD_OUT%\size_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=978 Failed setting texture 2 size on node: Absent" "%BF2_BUILD_OUT%\size_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\size_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_SIZE_RETURNED" "%BF2_BUILD_OUT%\size_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_object_marker.exe" weapon-death >"%BF2_BUILD_OUT%\weapon_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=1022 Failed setting active weapon on node: Absent" "%BF2_BUILD_OUT%\weapon_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\weapon_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_WEAPON_RETURNED" "%BF2_BUILD_OUT%\weapon_death.log" >nul
if not errorlevel 1 exit /b 1
echo HUD object marker node and its nine rows passed
exit /b 0
