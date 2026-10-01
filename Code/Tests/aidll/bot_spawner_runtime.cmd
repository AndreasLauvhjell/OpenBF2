@echo off
setlocal
rem [TEMP] Exact spawn caller and small helpers; native chosen lifetime/receiver fixture.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_BOT_SPAWNER_OUT set "BF2_BOT_SPAWNER_OUT=%TASK_CODE%\Build\ai_spawn_20260927\validation"
if not exist "%BF2_BOT_SPAWNER_OUT%" mkdir "%BF2_BOT_SPAWNER_OUT%"
python -I "%~dp0bot_spawner_runtime.py" generate "%BF2_BOT_SPAWNER_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_BOT_SPAWNER_OUT%\bot_spawner_actual.cpp" /Fo"%BF2_BOT_SPAWNER_OUT%\actual.obj" >"%BF2_BOT_SPAWNER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_BOT_SPAWNER_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0bot_spawner_runtime.cpp" /Fo"%BF2_BOT_SPAWNER_OUT%\fixture.obj" >>"%BF2_BOT_SPAWNER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_BOT_SPAWNER_OUT%\bot_spawner_runtime.exe" "%BF2_BOT_SPAWNER_OUT%\actual.obj" "%BF2_BOT_SPAWNER_OUT%\fixture.obj" >"%BF2_BOT_SPAWNER_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0bot_spawner_runtime.py" run "%BF2_BOT_SPAWNER_OUT%\bot_spawner_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_BOT_SPAWNER_OUT%\compile.log"
if exist "%BF2_BOT_SPAWNER_OUT%\link.log" type "%BF2_BOT_SPAWNER_OUT%\link.log"
exit /b 1

