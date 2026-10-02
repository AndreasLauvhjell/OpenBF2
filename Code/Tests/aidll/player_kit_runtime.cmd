@echo off
setlocal
rem [TEMP] One verbatim production getter, strict MSVC x86, chosen native collaborators.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_PLAYER_KIT_OUT set "BF2_PLAYER_KIT_OUT=%TASK_CODE%\Build\ai_kit_policy_20260927\player\validation"
if not exist "%BF2_PLAYER_KIT_OUT%" mkdir "%BF2_PLAYER_KIT_OUT%"
python -I "%~dp0player_kit_runtime.py" generate "%BF2_PLAYER_KIT_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
cl %TASK_FLAGS% /FAs /Fa"%BF2_PLAYER_KIT_OUT%\actual.asm" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_PLAYER_KIT_OUT%\player_kit_actual.cpp" /Fo"%BF2_PLAYER_KIT_OUT%\actual.obj" >"%BF2_PLAYER_KIT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FAs /Fa"%BF2_PLAYER_KIT_OUT%\fixture.asm" /I"%BF2_PLAYER_KIT_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0player_kit_runtime.cpp" /Fo"%BF2_PLAYER_KIT_OUT%\fixture.obj" >>"%BF2_PLAYER_KIT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_PLAYER_KIT_OUT%\player_kit_runtime.exe" "%BF2_PLAYER_KIT_OUT%\actual.obj" "%BF2_PLAYER_KIT_OUT%\fixture.obj" >"%BF2_PLAYER_KIT_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0player_kit_runtime.py" run "%BF2_PLAYER_KIT_OUT%\player_kit_runtime.exe" >"%BF2_PLAYER_KIT_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_PLAYER_KIT_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_PLAYER_KIT_OUT%\compile.log"
if exist "%BF2_PLAYER_KIT_OUT%\link.log" type "%BF2_PLAYER_KIT_OUT%\link.log"
exit /b 1
