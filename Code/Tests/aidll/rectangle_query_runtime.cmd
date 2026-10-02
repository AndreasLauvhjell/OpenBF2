@echo off
setlocal
rem [TEMP] Exact wrapper/selector plus whole actual grid/pool, strict MSVC x86.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_RECTANGLE_QUERY_OUT set "BF2_RECTANGLE_QUERY_OUT=%TASK_CODE%\Build\ai_kit_equipment_20260927\rectangle\validation"
if not exist "%BF2_RECTANGLE_QUERY_OUT%" mkdir "%BF2_RECTANGLE_QUERY_OUT%"
python -I "%~dp0rectangle_query_runtime.py" generate "%BF2_RECTANGLE_QUERY_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FAs /Fa"%BF2_RECTANGLE_QUERY_OUT%\actual.asm" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_RECTANGLE_QUERY_OUT%\rectangle_query_actual.cpp" /Fo"%BF2_RECTANGLE_QUERY_OUT%\actual.obj" >"%BF2_RECTANGLE_QUERY_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
for %%F in (AIInformationGrid InformationManager) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIKnowledge\%%F.cpp" /Fo"%BF2_RECTANGLE_QUERY_OUT%\%%F.obj" >>"%BF2_RECTANGLE_QUERY_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
for %%F in (Debug StringUtil) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\Core\%%F.cpp" /Fo"%BF2_RECTANGLE_QUERY_OUT%\%%F.obj" >>"%BF2_RECTANGLE_QUERY_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
cl %TASK_FLAGS% /FAs /Fa"%BF2_RECTANGLE_QUERY_OUT%\fixture.asm" /I"%BF2_RECTANGLE_QUERY_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0rectangle_query_runtime.cpp" /Fo"%BF2_RECTANGLE_QUERY_OUT%\fixture.obj" >>"%BF2_RECTANGLE_QUERY_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_RECTANGLE_QUERY_OUT%\rectangle_query_runtime.exe" "%BF2_RECTANGLE_QUERY_OUT%\actual.obj" "%BF2_RECTANGLE_QUERY_OUT%\AIInformationGrid.obj" "%BF2_RECTANGLE_QUERY_OUT%\InformationManager.obj" "%BF2_RECTANGLE_QUERY_OUT%\Debug.obj" "%BF2_RECTANGLE_QUERY_OUT%\StringUtil.obj" "%BF2_RECTANGLE_QUERY_OUT%\fixture.obj" >"%BF2_RECTANGLE_QUERY_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0rectangle_query_runtime.py" run "%BF2_RECTANGLE_QUERY_OUT%\rectangle_query_runtime.exe" >"%BF2_RECTANGLE_QUERY_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_RECTANGLE_QUERY_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_RECTANGLE_QUERY_OUT%\compile.log"
if exist "%BF2_RECTANGLE_QUERY_OUT%\link.log" type "%BF2_RECTANGLE_QUERY_OUT%\link.log"
exit /b 1
