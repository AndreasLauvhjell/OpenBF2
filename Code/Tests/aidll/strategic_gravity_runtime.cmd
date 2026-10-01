@echo off
setlocal
rem [TEMP] Three exact production gravity bodies and actual XZ helper; native chosen lifetimes.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_STRATEGIC_GRAVITY_OUT set "BF2_STRATEGIC_GRAVITY_OUT=%TASK_CODE%\Build\ai_selector_20260927\engine_queries\gravity_validation"
if not exist "%BF2_STRATEGIC_GRAVITY_OUT%" mkdir "%BF2_STRATEGIC_GRAVITY_OUT%"
python -I "%~dp0strategic_gravity_runtime.py" generate "%BF2_STRATEGIC_GRAVITY_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_STRATEGIC_GRAVITY_OUT%\strategic_gravity_actual.cpp" /Fo"%BF2_STRATEGIC_GRAVITY_OUT%\actual.obj" >"%BF2_STRATEGIC_GRAVITY_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_STRATEGIC_GRAVITY_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0strategic_gravity_runtime.cpp" /Fo"%BF2_STRATEGIC_GRAVITY_OUT%\fixture.obj" >>"%BF2_STRATEGIC_GRAVITY_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_STRATEGIC_GRAVITY_OUT%\strategic_gravity_runtime.exe" "%BF2_STRATEGIC_GRAVITY_OUT%\actual.obj" "%BF2_STRATEGIC_GRAVITY_OUT%\fixture.obj" >"%BF2_STRATEGIC_GRAVITY_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0strategic_gravity_runtime.py" run "%BF2_STRATEGIC_GRAVITY_OUT%\strategic_gravity_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_STRATEGIC_GRAVITY_OUT%\compile.log"
if exist "%BF2_STRATEGIC_GRAVITY_OUT%\link.log" type "%BF2_STRATEGIC_GRAVITY_OUT%\link.log"
exit /b 1
