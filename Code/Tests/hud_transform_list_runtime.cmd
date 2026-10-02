@echo off
setlocal
rem [TEMP] Native x86 fixture for Bf2TransformListNode and HUD builder rows 61..64.
rem /FORCE:UNRESOLVED only covers the families the builder header pulls in but
rem these never call. ONE :death label is used for every check - cmd's label
rem lookup gets unreliable when a script alternates between two of them.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_transform_list_20260915"
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
for %%S in (Bf2TransformListNode Bf2TransformNode BfTransformNode HudBlinker HudColors) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Menu\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
  call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\%%S.obj""
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Game\HudInformationLayer.cpp" /Fo"%BF2_BUILD_OUT%\HudInformationLayer.obj"
if errorlevel 1 exit /b 1
call set "TASK_OBJECTS=%%TASK_OBJECTS%% "%BF2_BUILD_OUT%\HudInformationLayer.obj""
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0hud_transform_list_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE:UNRESOLVED /OPT:NOREF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_transform_list.exe" /MAP:"%BF2_BUILD_OUT%\hud_transform_list.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if not exist "%BF2_BUILD_OUT%\hud_transform_list.exe" (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_transform_list.exe"
if not "%errorlevel%"=="0" exit /b 1
call :death create-death "FIXTURE_DEBUG line=682 Node: Warn, already exists!!"                    FIXTURE_CREATE_RETURNED
if errorlevel 1 exit /b 1
call :death add-death    "FIXTURE_DEBUG line=697 Failed adding node to node: Absent"              FIXTURE_ADD_RETURNED
if errorlevel 1 exit /b 1
call :death posvar-death "FIXTURE_DEBUG line=713 Failed setting position variable on node: Absent" FIXTURE_POSVAR_RETURNED
if errorlevel 1 exit /b 1
call :death offset-death "FIXTURE_DEBUG line=723 Failed setting offset on node: Absent"           FIXTURE_OFFSET_RETURNED
if errorlevel 1 exit /b 1
echo HUD transform list node and its four rows passed
exit /b 0

:death
"%BF2_BUILD_OUT%\hud_transform_list.exe" %1 >"%BF2_BUILD_OUT%\%1.log" 2>&1
if "%errorlevel%"=="0" (echo %1 did not stop & exit /b 1)
findstr /c:%2 "%BF2_BUILD_OUT%\%1.log" >nul
if errorlevel 1 (echo %1 missing its diagnostic & type "%BF2_BUILD_OUT%\%1.log" & exit /b 1)
findstr /c:"%3" "%BF2_BUILD_OUT%\%1.log" >nul
if not errorlevel 1 (echo %1 returned past its stop & exit /b 1)
echo %1: named diagnostic confirmed
exit /b 0
