@echo off
setlocal
rem [TEMP] Nonvirtual loading record with actual information storage and event ownership.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_loading_screen_runtime_20260905"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy %BF2_RUNTIME_OPT% /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0"
for %%s in (Menu\HudLoadingScreen Game\HudInformationLayer Menu\HudBlinker Core\EventManager Core\StringUtil Core\Debug) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%s.cpp" /Fo"%BF2_BUILD_OUT%\\" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (
    type "%BF2_BUILD_OUT%\compile.log"
    exit /b 1
  )
)
cl %TASK_FLAGS% /d1reportSingleClassLayoutHudLoadingScreen /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_loading_screen_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\layout.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\layout.log"
  exit /b 1
)
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_loading_screen_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_loading_screen_runtime.pdb" /MAP:"%BF2_BUILD_OUT%\hud_loading_screen_runtime.map" "%BF2_BUILD_OUT%\HudLoadingScreen.obj" "%BF2_BUILD_OUT%\HudInformationLayer.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\EventManager.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\link_force.log" 2>&1
if errorlevel 1 (
  type "%BF2_BUILD_OUT%\link_force.log"
  exit /b 1
)
"%BF2_BUILD_OUT%\hud_loading_screen_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
