@echo off
setlocal
rem [TEMP] Three exact squad queries, native chosen lifetimes, strict MSVC x86.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_SQUAD_QUERIES_OUT set "BF2_SQUAD_QUERIES_OUT=%TASK_CODE%\Build\ai_selector_20260927\squad_queries\validation"
if not exist "%BF2_SQUAD_QUERIES_OUT%" mkdir "%BF2_SQUAD_QUERIES_OUT%"
python -I "%~dp0squad_queries_runtime.py" generate "%BF2_SQUAD_QUERIES_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_SQUAD_QUERIES_OUT%\squad_queries_actual.cpp" /Fo"%BF2_SQUAD_QUERIES_OUT%\actual.obj" >"%BF2_SQUAD_QUERIES_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0squad_queries_runtime.cpp" /Fo"%BF2_SQUAD_QUERIES_OUT%\fixture.obj" >>"%BF2_SQUAD_QUERIES_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_SQUAD_QUERIES_OUT%\squad_queries_runtime.exe" "%BF2_SQUAD_QUERIES_OUT%\actual.obj" "%BF2_SQUAD_QUERIES_OUT%\fixture.obj" >"%BF2_SQUAD_QUERIES_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0squad_queries_runtime.py" run "%BF2_SQUAD_QUERIES_OUT%\squad_queries_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_SQUAD_QUERIES_OUT%\compile.log"
if exist "%BF2_SQUAD_QUERIES_OUT%\link.log" type "%BF2_SQUAD_QUERIES_OUT%\link.log"
exit /b 1
