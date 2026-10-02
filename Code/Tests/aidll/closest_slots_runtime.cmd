@echo off
setlocal
rem [TEMP] Actual closest-slot bodies; strict native x86, two explicitly separate scopes.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_CLOSEST_SLOTS_OUT set "BF2_CLOSEST_SLOTS_OUT=%TASK_CODE%\Build\ai_formation_status_20260927\dependencies\validation"
if not exist "%BF2_CLOSEST_SLOTS_OUT%" mkdir "%BF2_CLOSEST_SLOTS_OUT%"
python -I "%~dp0closest_slots_runtime.py" generate "%BF2_CLOSEST_SLOTS_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_CLOSEST_SLOTS_OUT%\closest_actual.cpp" /Fo"%BF2_CLOSEST_SLOTS_OUT%\actual.obj" >"%BF2_CLOSEST_SLOTS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_CLOSEST_SLOTS_OUT%\closest_observer.cpp" /Fo"%BF2_CLOSEST_SLOTS_OUT%\observer.obj" >>"%BF2_CLOSEST_SLOTS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0closest_slots_runtime.cpp" /Fo"%BF2_CLOSEST_SLOTS_OUT%\fixture.obj" >>"%BF2_CLOSEST_SLOTS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /DCLOSEST_OBSERVER /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0closest_slots_runtime.cpp" /Fo"%BF2_CLOSEST_SLOTS_OUT%\fixture_observer.obj" >>"%BF2_CLOSEST_SLOTS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_CLOSEST_SLOTS_OUT%\closest_actual.exe" "%BF2_CLOSEST_SLOTS_OUT%\actual.obj" "%BF2_CLOSEST_SLOTS_OUT%\fixture.obj" >"%BF2_CLOSEST_SLOTS_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_CLOSEST_SLOTS_OUT%\closest_observer.exe" "%BF2_CLOSEST_SLOTS_OUT%\observer.obj" "%BF2_CLOSEST_SLOTS_OUT%\fixture_observer.obj" >>"%BF2_CLOSEST_SLOTS_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0closest_slots_runtime.py" run "%BF2_CLOSEST_SLOTS_OUT%"
exit /b %errorlevel%
:failed
type "%BF2_CLOSEST_SLOTS_OUT%\compile.log"
if exist "%BF2_CLOSEST_SLOTS_OUT%\link.log" type "%BF2_CLOSEST_SLOTS_OUT%\link.log"
exit /b 1
