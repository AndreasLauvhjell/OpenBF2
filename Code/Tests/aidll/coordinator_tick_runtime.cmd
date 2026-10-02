@echo off
setlocal
rem [TEMP] Strict MSVC x86; unique output, exact bodies and separately labelled setter observer.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_COORDINATOR_TICK_OUT set "BF2_COORDINATOR_TICK_OUT=%TASK_CODE%\Build\ai_squad_tick_20260927\coordinator\validation"
if not exist "%BF2_COORDINATOR_TICK_OUT%" mkdir "%BF2_COORDINATOR_TICK_OUT%"
python -I "%~dp0coordinator_tick_runtime.py" generate "%BF2_COORDINATOR_TICK_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_COORDINATOR_TICK_OUT%\actual.cpp" /Fo"%BF2_COORDINATOR_TICK_OUT%\actual.obj" >"%BF2_COORDINATOR_TICK_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_COORDINATOR_TICK_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0coordinator_tick_runtime.cpp" /Fo"%BF2_COORDINATOR_TICK_OUT%\fixture.obj" >>"%BF2_COORDINATOR_TICK_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_COORDINATOR_TICK_OUT%\coordinator_tick_runtime.exe" "%BF2_COORDINATOR_TICK_OUT%\actual.obj" "%BF2_COORDINATOR_TICK_OUT%\fixture.obj" >"%BF2_COORDINATOR_TICK_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /DPATH_OBSERVERS /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_COORDINATOR_TICK_OUT%\actual.cpp" /Fo"%BF2_COORDINATOR_TICK_OUT%\observer_actual.obj" >>"%BF2_COORDINATOR_TICK_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /DPATH_OBSERVERS /I"%BF2_COORDINATOR_TICK_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0coordinator_tick_runtime.cpp" /Fo"%BF2_COORDINATOR_TICK_OUT%\observer_fixture.obj" >>"%BF2_COORDINATOR_TICK_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_COORDINATOR_TICK_OUT%\path_observers.exe" "%BF2_COORDINATOR_TICK_OUT%\observer_actual.obj" "%BF2_COORDINATOR_TICK_OUT%\observer_fixture.obj" >>"%BF2_COORDINATOR_TICK_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0coordinator_tick_runtime.py" run "%BF2_COORDINATOR_TICK_OUT%"
exit /b %errorlevel%
:failed
type "%BF2_COORDINATOR_TICK_OUT%\compile.log"
if exist "%BF2_COORDINATOR_TICK_OUT%\link.log" type "%BF2_COORDINATOR_TICK_OUT%\link.log"
exit /b 1
