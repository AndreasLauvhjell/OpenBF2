@echo off
setlocal
rem [TEMP] Actual BFSquad.cpp, strict x86 link; no /FORCE or registry substitute.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_SQUAD_BRIDGE_OUT set "BF2_SQUAD_BRIDGE_OUT=%TASK_CODE%\Build\ai_squad_20260927\bridge\runtime"
if not exist "%BF2_SQUAD_BRIDGE_OUT%" mkdir "%BF2_SQUAD_BRIDGE_OUT%"
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIInterface\BFKnowledge\BFSquad.cpp" /Fo"%BF2_SQUAD_BRIDGE_OUT%\BFSquad.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0squad_bridge_runtime.cpp" /Fo"%BF2_SQUAD_BRIDGE_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_SQUAD_BRIDGE_OUT%\squad_bridge_runtime.exe" "%BF2_SQUAD_BRIDGE_OUT%\BFSquad.obj" "%BF2_SQUAD_BRIDGE_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_SQUAD_BRIDGE_OUT%\squad_bridge_runtime.exe"
exit /b %errorlevel%
