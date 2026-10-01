@echo off
setlocal
rem [TEMP] Four exact bodies, native chosen lifetimes, strict MSVC x86 without /FORCE.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_SPAWN_METADATA_OUT set "BF2_SPAWN_METADATA_OUT=%TASK_CODE%\Build\ai_spawn_20260927\dependencies\validation"
if not exist "%BF2_SPAWN_METADATA_OUT%" mkdir "%BF2_SPAWN_METADATA_OUT%"
python -I "%~dp0spawn_metadata_runtime.py" generate "%BF2_SPAWN_METADATA_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_SPAWN_METADATA_OUT%\metadata_actual.cpp" /Fo"%BF2_SPAWN_METADATA_OUT%\actual.obj" >"%BF2_SPAWN_METADATA_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_SPAWN_METADATA_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0spawn_metadata_runtime.cpp" /Fo"%BF2_SPAWN_METADATA_OUT%\fixture.obj" >>"%BF2_SPAWN_METADATA_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_SPAWN_METADATA_OUT%\spawn_metadata_runtime.exe" "%BF2_SPAWN_METADATA_OUT%\actual.obj" "%BF2_SPAWN_METADATA_OUT%\fixture.obj" >"%BF2_SPAWN_METADATA_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0spawn_metadata_runtime.py" run "%BF2_SPAWN_METADATA_OUT%\spawn_metadata_runtime.exe" >"%BF2_SPAWN_METADATA_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_SPAWN_METADATA_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_SPAWN_METADATA_OUT%\compile.log"
if exist "%BF2_SPAWN_METADATA_OUT%\link.log" type "%BF2_SPAWN_METADATA_OUT%\link.log"
exit /b 1
