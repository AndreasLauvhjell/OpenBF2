@echo off
setlocal
rem [TEMP] Two exact production scans; real headers and chosen native lifetimes.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_STRATEGIC_NEIGHBOURS_OUT set "BF2_STRATEGIC_NEIGHBOURS_OUT=%TASK_CODE%\Build\ai_states_20260927\strategic_validation"
if not exist "%BF2_STRATEGIC_NEIGHBOURS_OUT%" mkdir "%BF2_STRATEGIC_NEIGHBOURS_OUT%"
python -I "%~dp0strategic_neighbours_runtime.py" generate "%BF2_STRATEGIC_NEIGHBOURS_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_STRATEGIC_NEIGHBOURS_OUT%\strategic_neighbours_actual.cpp" /Fo"%BF2_STRATEGIC_NEIGHBOURS_OUT%\actual.obj" >"%BF2_STRATEGIC_NEIGHBOURS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_STRATEGIC_NEIGHBOURS_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0strategic_neighbours_runtime.cpp" /Fo"%BF2_STRATEGIC_NEIGHBOURS_OUT%\fixture.obj" >>"%BF2_STRATEGIC_NEIGHBOURS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_STRATEGIC_NEIGHBOURS_OUT%\strategic_neighbours_runtime.exe" "%BF2_STRATEGIC_NEIGHBOURS_OUT%\actual.obj" "%BF2_STRATEGIC_NEIGHBOURS_OUT%\fixture.obj" >"%BF2_STRATEGIC_NEIGHBOURS_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0strategic_neighbours_runtime.py" run "%BF2_STRATEGIC_NEIGHBOURS_OUT%\strategic_neighbours_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_STRATEGIC_NEIGHBOURS_OUT%\compile.log"
if exist "%BF2_STRATEGIC_NEIGHBOURS_OUT%\link.log" type "%BF2_STRATEGIC_NEIGHBOURS_OUT%\link.log"
exit /b 1
