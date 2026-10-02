@echo off
setlocal
rem [TEMP] Native x86 real builder/helper fixture. /FORCE wider UI paths are uncalled.
rem Production Node factory/defaults are linked; events7/9/13/14/17 return, event10 stops.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_bf2_hud_builder_commands_20260908"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_FIXTURE_OPT set "BF2_FIXTURE_OPT=/Od"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc %BF2_FIXTURE_OPT% /Z7 /Gy /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0"
for %%S in (Object TypeDescriptor Node System NamedObjectRegistry) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\External\BFMeme\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
)
for %%S in (Debug StringUtil) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%TASK_ROOT%\Code\BF2\Core\%%S.cpp" /Fo"%BF2_BUILD_OUT%\%%S.obj"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" "%~dp0bf2_hud_builder_commands_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
set "TASK_OBJECTS="%BF2_BUILD_OUT%\Object.obj" "%BF2_BUILD_OUT%\TypeDescriptor.obj" "%BF2_BUILD_OUT%\Node.obj" "%BF2_BUILD_OUT%\System.obj" "%BF2_BUILD_OUT%\NamedObjectRegistry.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\StringUtil.obj""
link /nologo /FORCE:UNRESOLVED /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\builder_commands.exe" /MAP:"%BF2_BUILD_OUT%\builder_commands.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\builder_commands.exe"
if not "%errorlevel%"=="0" exit /b 1
"%BF2_BUILD_OUT%\builder_commands.exe" debug-death >"%BF2_BUILD_OUT%\debug_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"FIXTURE_DEBUG line=2384 Absent doesn't exist..." "%BF2_BUILD_OUT%\debug_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\debug_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_DEBUG_RETURNED" "%BF2_BUILD_OUT%\debug_death.log" >nul
if not errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\builder_commands.exe" node-event10-death >"%BF2_BUILD_OUT%\node_death.log" 2>&1
if "%errorlevel%"=="0" exit /b 1
findstr /c:"[FRONTIER] dice::meme::dispatchNodeEvent10 reached" "%BF2_BUILD_OUT%\node_death.log" >nul
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\node_death.log"
  exit /b 1
)
findstr /c:"FIXTURE_NODE_EVENT10_RETURNED" "%BF2_BUILD_OUT%\node_death.log" >nul
if not errorlevel 1 exit /b 1
echo BF2 HUD builder default Debug and actual Node event10 death checks passed
exit /b 0
