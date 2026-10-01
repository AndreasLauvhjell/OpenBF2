@echo off
setlocal
rem [TEMP] Strict exact-body formation suite; observer variants are separate executables.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_FORMATION_TICK_OUT set "BF2_FORMATION_TICK_OUT=%TASK_CODE%\Build\ai_formation_tick_20260927\validation"
if not exist "%BF2_FORMATION_TICK_OUT%" mkdir "%BF2_FORMATION_TICK_OUT%"
python -I "%~dp0formation_tick_runtime.py" generate "%BF2_FORMATION_TICK_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
type nul >"%BF2_FORMATION_TICK_OUT%\compile.log"
type nul >"%BF2_FORMATION_TICK_OUT%\link.log"
call :build normal "" formation_tick_runtime
if errorlevel 1 goto failed
call :build leaves /DLEAF_OBSERVERS leaf_observers
if errorlevel 1 goto failed
call :build scheduler /DSCHEDULER_OBSERVERS scheduler_observers
if errorlevel 1 goto failed
python -I "%~dp0formation_tick_runtime.py" run "%BF2_FORMATION_TICK_OUT%"
exit /b %errorlevel%
:build
cl %TASK_FLAGS% %~2 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_FORMATION_TICK_OUT%\actual.cpp" /Fo"%BF2_FORMATION_TICK_OUT%\%~1_actual.obj" >>"%BF2_FORMATION_TICK_OUT%\compile.log" 2>&1
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% %~2 /I"%BF2_FORMATION_TICK_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0formation_tick_runtime.cpp" /Fo"%BF2_FORMATION_TICK_OUT%\%~1_fixture.obj" >>"%BF2_FORMATION_TICK_OUT%\compile.log" 2>&1
if errorlevel 1 exit /b 1
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /OUT:"%BF2_FORMATION_TICK_OUT%\%~3.exe" "%BF2_FORMATION_TICK_OUT%\%~1_actual.obj" "%BF2_FORMATION_TICK_OUT%\%~1_fixture.obj" >>"%BF2_FORMATION_TICK_OUT%\link.log" 2>&1
exit /b %errorlevel%
:failed
type "%BF2_FORMATION_TICK_OUT%\compile.log"
type "%BF2_FORMATION_TICK_OUT%\link.log"
exit /b 1
