@echo off
setlocal
rem [TEMP] Actual complete courier/manager/registry TUs; strict MSVC x86 link.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_COURIER_OUT set "BF2_COURIER_OUT=%TASK_CODE%\Build\ai_courier_20260927\validation"
if not exist "%BF2_COURIER_OUT%" mkdir "%BF2_COURIER_OUT%"
python -I "%~dp0courier_runtime.py" generate "%BF2_COURIER_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /Gw /Zc:inline /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\Main\AIEventCourier.cpp" /Fo"%BF2_COURIER_OUT%\AIEventCourier.obj" >"%BF2_COURIER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIDLL\AI\AIBot\SquadManager.cpp" /Fo"%BF2_COURIER_OUT%\SquadManager.obj" >>"%BF2_COURIER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
for %%F in (ClassManager Debug StringUtil) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\Core\%%F.cpp" /Fo"%BF2_COURIER_OUT%\%%F.obj" >>"%BF2_COURIER_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
for %%F in (include_engine include_dll) do (
  cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_COURIER_OUT%\%%F.cpp" /Fo"%BF2_COURIER_OUT%\%%F.obj" >>"%BF2_COURIER_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
cl %TASK_FLAGS% /I"%BF2_COURIER_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0courier_runtime.cpp" /Fo"%BF2_COURIER_OUT%\fixture.obj" >>"%BF2_COURIER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
rem Client consumers must see the same complete class in both include orders too.
for %%F in (include_engine include_dll) do (
  cl /nologo /Zs /MD /std:c++14 /permissive /EHsc /W3 /DDEDICATED=0 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_COURIER_OUT%\%%F.cpp" >>"%BF2_COURIER_OUT%\compile.log" 2>&1
  if errorlevel 1 goto failed
)
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_COURIER_OUT%\courier_runtime.exe" "%BF2_COURIER_OUT%\AIEventCourier.obj" "%BF2_COURIER_OUT%\SquadManager.obj" "%BF2_COURIER_OUT%\ClassManager.obj" "%BF2_COURIER_OUT%\Debug.obj" "%BF2_COURIER_OUT%\StringUtil.obj" "%BF2_COURIER_OUT%\include_engine.obj" "%BF2_COURIER_OUT%\include_dll.obj" "%BF2_COURIER_OUT%\fixture.obj" >"%BF2_COURIER_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0courier_runtime.py" run "%BF2_COURIER_OUT%\courier_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_COURIER_OUT%\compile.log"
if exist "%BF2_COURIER_OUT%\link.log" type "%BF2_COURIER_OUT%\link.log"
exit /b 1
