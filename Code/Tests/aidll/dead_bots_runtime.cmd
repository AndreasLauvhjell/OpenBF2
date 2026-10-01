@echo off
setlocal
rem [TEMP] Exact getter/setter, chosen native lifetimes, strict MSVC x86.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_DEAD_BOTS_OUT set "BF2_DEAD_BOTS_OUT=%TASK_CODE%\Build\ai_kit_assignment_20260927\dead_bots\validation"
if not exist "%BF2_DEAD_BOTS_OUT%" mkdir "%BF2_DEAD_BOTS_OUT%"
python -I "%~dp0dead_bots_runtime.py" generate "%BF2_DEAD_BOTS_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FAs /Fa"%BF2_DEAD_BOTS_OUT%\actual.asm" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_DEAD_BOTS_OUT%\dead_bots_actual.cpp" /Fo"%BF2_DEAD_BOTS_OUT%\actual.obj" >"%BF2_DEAD_BOTS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FAs /Fa"%BF2_DEAD_BOTS_OUT%\fixture.asm" /I"%BF2_DEAD_BOTS_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0dead_bots_runtime.cpp" /Fo"%BF2_DEAD_BOTS_OUT%\fixture.obj" >>"%BF2_DEAD_BOTS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_DEAD_BOTS_OUT%\dead_bots_runtime.exe" "%BF2_DEAD_BOTS_OUT%\actual.obj" "%BF2_DEAD_BOTS_OUT%\fixture.obj" >"%BF2_DEAD_BOTS_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0dead_bots_runtime.py" run "%BF2_DEAD_BOTS_OUT%\dead_bots_runtime.exe" >"%BF2_DEAD_BOTS_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_DEAD_BOTS_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_DEAD_BOTS_OUT%\compile.log"
if exist "%BF2_DEAD_BOTS_OUT%\link.log" type "%BF2_DEAD_BOTS_OUT%\link.log"
exit /b 1
