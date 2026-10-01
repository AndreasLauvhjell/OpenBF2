@echo off
setlocal
rem [TEMP] Strict x86 movement prefixes; separate actual and chosen getter scopes.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_MOVEMENT_OUT set "BF2_MOVEMENT_OUT=%TASK_CODE%\Build\ai_member_update_20260927\movement\validation"
if not exist "%BF2_MOVEMENT_OUT%" mkdir "%BF2_MOVEMENT_OUT%"
python -I "%~dp0movement_runtime.py" generate "%BF2_MOVEMENT_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_MOVEMENT_OUT%\actual.cpp" /Fo"%BF2_MOVEMENT_OUT%\actual.obj" >"%BF2_MOVEMENT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_MOVEMENT_OUT%\observer.cpp" /Fo"%BF2_MOVEMENT_OUT%\observer.obj" >>"%BF2_MOVEMENT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_MOVEMENT_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0movement_runtime.cpp" /Fo"%BF2_MOVEMENT_OUT%\fixture.obj" >>"%BF2_MOVEMENT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /DMOVEMENT_OBSERVERS /I"%BF2_MOVEMENT_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0movement_runtime.cpp" /Fo"%BF2_MOVEMENT_OUT%\fixture_observer.obj" >>"%BF2_MOVEMENT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_MOVEMENT_OUT%\actual.exe" "%BF2_MOVEMENT_OUT%\actual.obj" "%BF2_MOVEMENT_OUT%\fixture.obj" >"%BF2_MOVEMENT_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_MOVEMENT_OUT%\observer.exe" "%BF2_MOVEMENT_OUT%\observer.obj" "%BF2_MOVEMENT_OUT%\fixture_observer.obj" >>"%BF2_MOVEMENT_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0movement_runtime.py" run "%BF2_MOVEMENT_OUT%"
exit /b %errorlevel%
:failed
type "%BF2_MOVEMENT_OUT%\compile.log"
if exist "%BF2_MOVEMENT_OUT%\link.log" type "%BF2_MOVEMENT_OUT%\link.log"
exit /b 1
