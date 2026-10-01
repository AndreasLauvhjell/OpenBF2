@echo off
setlocal
rem [TEMP] Exact adapter bodies, real headers and native chosen lifetimes, no /FORCE.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_SELECTOR_QUERIES_OUT set "BF2_SELECTOR_QUERIES_OUT=%TASK_CODE%\Build\ai_selector_20260927\engine_queries\validation"
if not exist "%BF2_SELECTOR_QUERIES_OUT%" mkdir "%BF2_SELECTOR_QUERIES_OUT%"
python -I "%~dp0selector_queries_runtime.py" generate "%BF2_SELECTOR_QUERIES_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_SELECTOR_QUERIES_OUT%\selector_queries_actual.cpp" /Fo"%BF2_SELECTOR_QUERIES_OUT%\actual.obj" >"%BF2_SELECTOR_QUERIES_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_SELECTOR_QUERIES_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0selector_queries_runtime.cpp" /Fo"%BF2_SELECTOR_QUERIES_OUT%\fixture.obj" >>"%BF2_SELECTOR_QUERIES_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_SELECTOR_QUERIES_OUT%\selector_queries_runtime.exe" "%BF2_SELECTOR_QUERIES_OUT%\actual.obj" "%BF2_SELECTOR_QUERIES_OUT%\fixture.obj" >"%BF2_SELECTOR_QUERIES_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0selector_queries_runtime.py" run "%BF2_SELECTOR_QUERIES_OUT%\selector_queries_runtime.exe" >"%BF2_SELECTOR_QUERIES_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_SELECTOR_QUERIES_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_SELECTOR_QUERIES_OUT%\compile.log"
if exist "%BF2_SELECTOR_QUERIES_OUT%\link.log" type "%BF2_SELECTOR_QUERIES_OUT%\link.log"
exit /b 1
