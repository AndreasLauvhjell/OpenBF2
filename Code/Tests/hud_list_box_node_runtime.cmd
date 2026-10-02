@echo off
setlocal
rem [TEMP] Native x86 fixture for Bf2NewListBoxNode and HUD builder rows 43..55.
rem /FORCE:UNRESOLVED only covers the families the builder and HUD manager headers
rem pull in but these never call.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_list_box_node_20260915"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc %BF2_FIXTURE_OPT% /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0"
set "TASK_OBJECTS="
for %%S in (Object TypeDescriptor Node System NamedObjectRegistry Data IntData FloatData Quad Stream OStream Effect ColorEffect EffectNode Style TransformNode Action ActionListAction ObjectList ObjectCollection CallFunctionAction Function) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Debug StringUtil EventManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Core\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Bf2NewListBoxNode BfNewListBoxNode BfListBoxData Bf2CallFunctionAction Bf2ActionListAction Bf2TransformNode BfTransformNode BF2HudManager HudBlinker HudColors) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\HudInformationLayer.cpp" /Fo"%BF2_BUILD_OUT%\HudInformationLayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\HudInformationLayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_list_box_node_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_list_box_node.exe" /MAP:"%BF2_BUILD_OUT%\hud_list_box_node.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if not exist "%BF2_BUILD_OUT%\hud_list_box_node.exe" (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_list_box_node.exe"
if not "%errorlevel%"=="0" exit /b 1
call :death data-death    "FIXTURE_DEBUG line=530 Failed setting Data!"    FIXTURE_DATA_RETURNED
if errorlevel 1 exit /b 1
call :death concmd-death  "FIXTURE_DEBUG line=617 Failed setting Action!"  FIXTURE_CONCMD_RETURNED
if errorlevel 1 exit /b 1
call :death outline-death "FIXTURE_DEBUG line=548 Failed setting outline on node: Absent" FIXTURE_OUTLINE_RETURNED
if errorlevel 1 exit /b 1
echo HUD list box node and its thirteen rows passed
exit /b 0

:death
"%BF2_BUILD_OUT%\hud_list_box_node.exe" %1 >"%BF2_BUILD_OUT%\%1.log" 2>&1
if "%errorlevel%"=="0" (echo %1 did not stop & exit /b 1)
findstr /c:%2 "%BF2_BUILD_OUT%\%1.log" >nul
if errorlevel 1 (echo %1 missing its diagnostic & type "%BF2_BUILD_OUT%\%1.log" & exit /b 1)
findstr /c:"%3" "%BF2_BUILD_OUT%\%1.log" >nul
if not errorlevel 1 (echo %1 returned past its stop & exit /b 1)
echo %1: named diagnostic confirmed
exit /b 0

:frontier
"%BF2_BUILD_OUT%\hud_list_box_node.exe" %1 >"%BF2_BUILD_OUT%\%1.log" 2>&1
if "%errorlevel%"=="0" (echo %1 did not stop & exit /b 1)
findstr /c:%2 "%BF2_BUILD_OUT%\%1.log" >nul
if errorlevel 1 (echo %1 stopped without its named frontier & type "%BF2_BUILD_OUT%\%1.log" & exit /b 1)
findstr /c:"%3" "%BF2_BUILD_OUT%\%1.log" >nul
if not errorlevel 1 (echo %1 returned past its frontier & exit /b 1)
echo %1: named frontier confirmed
exit /b 0
