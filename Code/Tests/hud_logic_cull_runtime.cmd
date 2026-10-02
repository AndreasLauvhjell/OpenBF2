@echo off
setlocal
rem [TEMP] Native x86 fixture for Bf2HudBuilder::setNodeLogicShowVariable, IntData,
rem IntRefData and LogicalData's29th slot. /FORCE:UNRESOLVED only covers the split
rem node, map node and transform node the command never calls.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_logic_cull_20260914"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc %BF2_FIXTURE_OPT% /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0"
set "TASK_OBJECTS="
for %%S in (Object TypeDescriptor Node System NamedObjectRegistry Data LogicalData EqualData OrData AndData NotData IntData CullNode) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Debug StringUtil EventManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Core\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Bf2CullNode HudBlinker HudColors) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\HudInformationLayer.cpp" /Fo"%BF2_BUILD_OUT%\HudInformationLayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\HudInformationLayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_logic_cull_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_logic_cull.exe" /MAP:"%BF2_BUILD_OUT%\hud_logic_cull.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_logic_cull.exe"
if not "%errorlevel%"=="0" exit /b 1
"%BF2_BUILD_OUT%\hud_logic_cull.exe" operation-death >"%BF2_BUILD_OUT%\operation_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=2034 Logical operation XOR not found!" "%BF2_BUILD_OUT%\operation_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\operation_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_OPERATION_RETURNED" "%BF2_BUILD_OUT%\operation_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_logic_cull.exe" cull-death >"%BF2_BUILD_OUT%\cull_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=2048 Failed setting logical cull on node: Sel" "%BF2_BUILD_OUT%\cull_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\cull_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_CULL_RETURNED" "%BF2_BUILD_OUT%\cull_death.log" >nul
if not errorlevel 1 exit /b 1
echo HUD logical-cull command, IntData/IntRefData and both Debug deaths passed
exit /b 0
