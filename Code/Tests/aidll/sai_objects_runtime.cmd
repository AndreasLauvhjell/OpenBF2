@echo off
setlocal
rem [TEMP] Exact strategic-object caller and bounded dependencies, strict x86.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_SAI_OBJECTS_OUT set "BF2_SAI_OBJECTS_OUT=%TASK_CODE%\Build\ai_object_update_20260927\validation"
if not exist "%BF2_SAI_OBJECTS_OUT%" mkdir "%BF2_SAI_OBJECTS_OUT%"
python -I "%~dp0sai_objects_runtime.py" generate "%BF2_SAI_OBJECTS_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_SAI_OBJECTS_OUT%\sai_objects_actual.cpp" /Fo"%BF2_SAI_OBJECTS_OUT%\actual.obj" >"%BF2_SAI_OBJECTS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_SAI_OBJECTS_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0sai_objects_runtime.cpp" /Fo"%BF2_SAI_OBJECTS_OUT%\fixture.obj" >>"%BF2_SAI_OBJECTS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_SAI_OBJECTS_OUT%\sai_objects_runtime.exe" "%BF2_SAI_OBJECTS_OUT%\actual.obj" "%BF2_SAI_OBJECTS_OUT%\fixture.obj" >"%BF2_SAI_OBJECTS_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0sai_objects_runtime.py" run "%BF2_SAI_OBJECTS_OUT%\sai_objects_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_SAI_OBJECTS_OUT%\compile.log"
if exist "%BF2_SAI_OBJECTS_OUT%\link.log" type "%BF2_SAI_OBJECTS_OUT%\link.log"
exit /b 1
