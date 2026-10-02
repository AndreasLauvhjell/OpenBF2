@echo off
setlocal
rem [TEMP] Real state virtuals; complete unresolved HUD family is not published.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_state_20260906"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /O2 /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%F in (BF2HudManager BF2HudManagerState HudBlinker) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Menu\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_state_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_state_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_state_runtime.pdb" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\BF2HudManagerState.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_state_runtime.exe"
exit /b %errorlevel%
