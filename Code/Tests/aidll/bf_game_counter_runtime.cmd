@echo off
setlocal
rem [TEMP] Exact counter adapter/leaf, real headers, strict MSVC x86.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_GAME_COUNTER_OUT set "BF2_GAME_COUNTER_OUT=%TASK_CODE%\Build\ai_area_update_20260927\signatures\validation"
if not exist "%BF2_GAME_COUNTER_OUT%" mkdir "%BF2_GAME_COUNTER_OUT%"
python -I "%~dp0bf_game_counter_runtime.py" generate "%BF2_GAME_COUNTER_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_GAME_COUNTER_OUT%\counter_actual.cpp" /Fo"%BF2_GAME_COUNTER_OUT%\actual.obj" >"%BF2_GAME_COUNTER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_GAME_COUNTER_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0bf_game_counter_runtime.cpp" /Fo"%BF2_GAME_COUNTER_OUT%\fixture.obj" >>"%BF2_GAME_COUNTER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_GAME_COUNTER_OUT%\bf_game_counter_runtime.exe" "%BF2_GAME_COUNTER_OUT%\actual.obj" "%BF2_GAME_COUNTER_OUT%\fixture.obj" >"%BF2_GAME_COUNTER_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0bf_game_counter_runtime.py" run "%BF2_GAME_COUNTER_OUT%\bf_game_counter_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_GAME_COUNTER_OUT%\compile.log"
if exist "%BF2_GAME_COUNTER_OUT%\link.log" type "%BF2_GAME_COUNTER_OUT%\link.log"
exit /b 1
