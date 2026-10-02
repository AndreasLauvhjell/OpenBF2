@echo off
setlocal
rem [TEMP] [CHOSEN, not recovered] Actual BotInfo constructor with a strict isolated x86 link.
rem Compile the production constructor verbatim; no /FORCE, invented body or production stubs.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_BOTINFO_OUT set "BF2_BOTINFO_OUT=%TASK_CODE%\Build\ai_botinfo_validation_20260927"
if not exist "%BF2_BOTINFO_OUT%" mkdir "%BF2_BOTINFO_OUT%"
python "%~dp0botinfo_extract.py" "%BF2_BOTINFO_OUT%\BotInfo_actual.cpp"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_BOTINFO_OUT%\BotInfo_actual.cpp" /Fo"%BF2_BOTINFO_OUT%\BotInfo_actual.obj"
if errorlevel 1 exit /b 1
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0botinfo_runtime.cpp" /Fo"%BF2_BOTINFO_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_BOTINFO_OUT%\botinfo_runtime.exe" "%BF2_BOTINFO_OUT%\BotInfo_actual.obj" "%BF2_BOTINFO_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
"%BF2_BOTINFO_OUT%\botinfo_runtime.exe"
exit /b %errorlevel%
