@echo off
setlocal
rem [TEMP] Native x86 fixture for BfOccupiedVehicleNode and HUD builder rows
rem 73/74/75. /FORCE:UNRESOLVED only covers the split, picture, bar, compass,
rem text, blend, marker and move families the builder header pulls in but these
rem never call - and the BF2HudManager surface the fixture subclass does not use.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_occupied_node_20260915"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc %BF2_FIXTURE_OPT% /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0"
set "TASK_OBJECTS="
for %%S in (Object TypeDescriptor Node System NamedObjectRegistry Data IntData FloatData Quad Stream Effect ColorEffect EffectNode Style TransformNode) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Debug StringUtil EventManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Core\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (BfOccupiedVehicleNode BfOccupiedVehicleData Bf2TransformNode BfTransformNode HudBlinker HudColors) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\HudInformationLayer.cpp" /Fo"%BF2_BUILD_OUT%\HudInformationLayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\HudInformationLayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_occupied_node_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_occupied_node.exe" /MAP:"%BF2_BUILD_OUT%\hud_occupied_node.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_occupied_node.exe"
if not "%errorlevel%"=="0" exit /b 1
"%BF2_BUILD_OUT%\hud_occupied_node.exe" create-death >"%BF2_BUILD_OUT%\create_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=1368 Node: Dot1, already exists!!" "%BF2_BUILD_OUT%\create_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\create_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_CREATE_RETURNED" "%BF2_BUILD_OUT%\create_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_occupied_node.exe" pos-death >"%BF2_BUILD_OUT%\pos_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=1385 Failed setting position variable on node: Absent" "%BF2_BUILD_OUT%\pos_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\pos_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_POS_RETURNED" "%BF2_BUILD_OUT%\pos_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_occupied_node.exe" data-death >"%BF2_BUILD_OUT%\data_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=1402 Failed setting data on node: Absent" "%BF2_BUILD_OUT%\data_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\data_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_DATA_RETURNED" "%BF2_BUILD_OUT%\data_death.log" >nul
if not errorlevel 1 exit /b 1
echo HUD occupied vehicle node and its three rows passed
exit /b 0
