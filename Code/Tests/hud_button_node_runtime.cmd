@echo off
setlocal
rem [TEMP] Native x86 fixture for Bf2ButtonNode, its three action classes and HUD
rem builder rows 116, 117, 118, 120, 121 and 123.
rem /FORCE:UNRESOLVED only covers the families the builder header pulls in but
rem these never call.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_button_node_20260915"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc %BF2_FIXTURE_OPT% /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0"
set "TASK_OBJECTS="
for %%S in (Object TypeDescriptor Node System NamedObjectRegistry Data IntData FloatData Quad Stream Effect ColorEffect EffectNode Style TransformNode PictureNode Action ActionListAction ObjectList ObjectCollection CallFunctionAction Function) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Debug StringUtil EventManager) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Core\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
for %%S in (Bf2ButtonNode Bf2PictureNode Bf2CallFunctionAction Bf2ActionListAction Bf2TransformNode BfTransformNode HudBlinker HudColors) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\HudInformationLayer.cpp" /Fo"%BF2_BUILD_OUT%\HudInformationLayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\HudInformationLayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_button_node_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_button_node.exe" /MAP:"%BF2_BUILD_OUT%\hud_button_node.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if not exist "%BF2_BUILD_OUT%\hud_button_node.exe" (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_button_node.exe"
if not "%errorlevel%"=="0" exit /b 1
call :death create-death   "FIXTURE_DEBUG line=1418 Node: Warn, already exists!!"          FIXTURE_CREATE_RETURNED
if errorlevel 1 exit /b 1
call :death texture-death  "FIXTURE_DEBUG line=1434 Failed setting Picture on node: Absent" FIXTURE_TEXTURE_RETURNED
if errorlevel 1 exit /b 1
call :death mousearea-death "FIXTURE_DEBUG line=1446 Failed setting mouse area on node: Absent" FIXTURE_MOUSEAREA_RETURNED
if errorlevel 1 exit /b 1
call :death debug-death    "FIXTURE_DEBUG line=1470 Failed setting debug on node: Absent"   FIXTURE_DEBUG_RETURNED
if errorlevel 1 exit /b 1
call :death concmd-death   "FIXTURE_DEBUG line=1535 Failed setting console command on node: Absent" FIXTURE_CONCMD_RETURNED
if errorlevel 1 exit /b 1
call :death altconcmd-death "FIXTURE_DEBUG line=1568 Failed setting alternative console command on node: Absent" FIXTURE_ALTCONCMD_RETURNED
if errorlevel 1 exit /b 1
call :frontier updateself-death "[FRONTIER] dice::meme::Bf2ButtonNode::updateSelf" FIXTURE_UPDATESELF_RETURNED
if errorlevel 1 exit /b 1
call :frontier paintself-death  "[FRONTIER] dice::meme::Bf2ButtonNode::paintSelf"  FIXTURE_PAINTSELF_RETURNED
if errorlevel 1 exit /b 1
echo HUD button node, its three action classes and its six rows passed
exit /b 0

:death
"%BF2_BUILD_OUT%\hud_button_node.exe" %1 >"%BF2_BUILD_OUT%\%1.log" 2>&1
if "%errorlevel%"=="0" (echo %1 did not stop & exit /b 1)
findstr /c:%2 "%BF2_BUILD_OUT%\%1.log" >nul
if errorlevel 1 (echo %1 missing its diagnostic & type "%BF2_BUILD_OUT%\%1.log" & exit /b 1)
findstr /c:"%3" "%BF2_BUILD_OUT%\%1.log" >nul
if not errorlevel 1 (echo %1 returned past its stop & exit /b 1)
echo %1: named diagnostic confirmed
exit /b 0

:frontier
"%BF2_BUILD_OUT%\hud_button_node.exe" %1 >"%BF2_BUILD_OUT%\%1.log" 2>&1
if "%errorlevel%"=="0" (echo %1 did not stop & exit /b 1)
findstr /c:%2 "%BF2_BUILD_OUT%\%1.log" >nul
if errorlevel 1 (echo %1 stopped without its named frontier & type "%BF2_BUILD_OUT%\%1.log" & exit /b 1)
findstr /c:"%3" "%BF2_BUILD_OUT%\%1.log" >nul
if not errorlevel 1 (echo %1 returned past its frontier & exit /b 1)
echo %1: named frontier confirmed
exit /b 0
