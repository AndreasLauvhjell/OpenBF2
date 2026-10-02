@echo off
setlocal
rem [TEMP] Strict MSVC x86 numeric material-query fixture; no server and no FORCE link.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1

set "TASK_CODE=%~dp0..\.."
if not defined BF2_MATERIAL_QUERY_OUT set "BF2_MATERIAL_QUERY_OUT=%TASK_CODE%\Build\ai_map_query_20261001\material_query_validation"
if not exist "%BF2_MATERIAL_QUERY_OUT%" mkdir "%BF2_MATERIAL_QUERY_OUT%"

python -I "%~dp0material_query_runtime.py" preflight "%BF2_MATERIAL_QUERY_OUT%"
if errorlevel 1 exit /b 1

set "TASK_FLAGS=/nologo /c /O2 /Ob0 /MD /std:c++14 /permissive /EHsc /W4 /WX /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL=1"
type nul >"%BF2_MATERIAL_QUERY_OUT%\compile.log"
cl %TASK_FLAGS% /I"%BF2_MATERIAL_QUERY_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0material_query_runtime.cpp" /Fo"%BF2_MATERIAL_QUERY_OUT%\fixture.obj" >>"%BF2_MATERIAL_QUERY_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed

set "O=%BF2_MATERIAL_QUERY_OUT%"
link /nologo /MACHINE:X86 /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%O%\material_query_runtime.exe" "%O%\fixture.obj" >"%O%\link.log" 2>&1
if errorlevel 1 goto failed

>"%O%\build.log" echo PASS exact production definitions compiled for AIDLL
>>"%O%\build.log" echo PASS strict MSVC x86 link without FORCE
python -I "%~dp0material_query_runtime.py" run "%O%\material_query_runtime.exe" "%O%"
exit /b %errorlevel%

:failed
if exist "%BF2_MATERIAL_QUERY_OUT%\compile.log" type "%BF2_MATERIAL_QUERY_OUT%\compile.log"
if exist "%BF2_MATERIAL_QUERY_OUT%\link.log" type "%BF2_MATERIAL_QUERY_OUT%\link.log"
exit /b 1
