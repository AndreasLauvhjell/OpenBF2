@echo off
setlocal
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_vehicle_construct_20260906\compile"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
for %%F in (HudVehicleInformation BfOccupiedVehicleData) do (
  cl /nologo /O2 /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\Menu\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%F.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
type "%BF2_BUILD_OUT%\compile.log"
exit /b 0
