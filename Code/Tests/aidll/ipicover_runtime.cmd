@echo off
setlocal
rem [TEMP] Whole IPICover/PlanePolygon TUs, typed observers, strict x86/no FORCE.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_IPICOVER_OUT set "BF2_IPICOVER_OUT=%TASK_CODE%\Build\ai_map_query_20261001\ipicover_validation"
if not exist "%BF2_IPICOVER_OUT%" mkdir "%BF2_IPICOVER_OUT%"
python -I "%~dp0ipicover_runtime.py" generate "%BF2_IPICOVER_OUT%"
if errorlevel 1 exit /b 1
set "O=%BF2_IPICOVER_OUT%"
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL=1"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIKnowledge\IPICover.cpp" /Fo"%O%\IPICover.obj" >"%O%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIInterface\BFKnowledge\PlanePolygon.cpp" /Fo"%O%\PlanePolygon.obj" >>"%O%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\Math\Vec3.cpp" /Fo"%O%\Vec3.obj" >>"%O%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%O%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0ipicover_runtime.cpp" /Fo"%O%\fixture.obj" >>"%O%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /MACHINE:X86 /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%O%\ipicover_runtime.exe" "%O%\IPICover.obj" "%O%\PlanePolygon.obj" "%O%\Vec3.obj" "%O%\fixture.obj" >"%O%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0ipicover_runtime.py" run "%O%\ipicover_runtime.exe"
exit /b %errorlevel%
:failed
if exist "%O%\compile.log" type "%O%\compile.log"
if exist "%O%\link.log" type "%O%\link.log"
exit /b 1
