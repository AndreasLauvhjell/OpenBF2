@echo off
setlocal
rem [TEMP] Strict MSVC x86; exact BFObjectEnvironment bodies and PlayerManager::getPlayers (2026-09-28).
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_OBJECT_ENVIRONMENT_OUT set "BF2_OBJECT_ENVIRONMENT_OUT=%TASK_CODE%\Build\claude_ai_plan_20260928\object_environment"
if not exist "%BF2_OBJECT_ENVIRONMENT_OUT%" mkdir "%BF2_OBJECT_ENVIRONMENT_OUT%"
python -I "%~dp0object_environment_runtime.py" generate "%BF2_OBJECT_ENVIRONMENT_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
set "TASK_FI=/FI"%TASK_CODE%\BF2\Core\BuildConfig.h""
cl %TASK_FLAGS% %TASK_FI% /FAs /Fa"%BF2_OBJECT_ENVIRONMENT_OUT%\actual.asm" "%BF2_OBJECT_ENVIRONMENT_OUT%\actual.cpp" /Fo"%BF2_OBJECT_ENVIRONMENT_OUT%\actual.obj" >"%BF2_OBJECT_ENVIRONMENT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_OBJECT_ENVIRONMENT_OUT%" %TASK_FI% "%~dp0object_environment_runtime.cpp" /Fo"%BF2_OBJECT_ENVIRONMENT_OUT%\fixture.obj" >>"%BF2_OBJECT_ENVIRONMENT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_OBJECT_ENVIRONMENT_OUT%\object_environment_runtime.exe" "%BF2_OBJECT_ENVIRONMENT_OUT%\actual.obj" "%BF2_OBJECT_ENVIRONMENT_OUT%\fixture.obj" >"%BF2_OBJECT_ENVIRONMENT_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0object_environment_runtime.py" run "%BF2_OBJECT_ENVIRONMENT_OUT%"
exit /b %errorlevel%
:failed
type "%BF2_OBJECT_ENVIRONMENT_OUT%\compile.log"
if exist "%BF2_OBJECT_ENVIRONMENT_OUT%\link.log" type "%BF2_OBJECT_ENVIRONMENT_OUT%\link.log"
exit /b 1
