@echo off
setlocal
rem [TEMP] Exact wrappers with real headers, native chosen lifetimes and typed grids.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_RADIUS_WRAPPERS_OUT set "BF2_RADIUS_WRAPPERS_OUT=%TASK_CODE%\Build\ai_radius_20260927\wrappers\validation"
if not exist "%BF2_RADIUS_WRAPPERS_OUT%" mkdir "%BF2_RADIUS_WRAPPERS_OUT%"
python -I "%~dp0radius_wrappers_runtime.py" generate "%BF2_RADIUS_WRAPPERS_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_RADIUS_WRAPPERS_OUT%\radius_wrappers_actual.cpp" /Fo"%BF2_RADIUS_WRAPPERS_OUT%\actual.obj" >"%BF2_RADIUS_WRAPPERS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_RADIUS_WRAPPERS_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0radius_wrappers_runtime.cpp" /Fo"%BF2_RADIUS_WRAPPERS_OUT%\fixture.obj" >>"%BF2_RADIUS_WRAPPERS_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_RADIUS_WRAPPERS_OUT%\radius_wrappers_runtime.exe" "%BF2_RADIUS_WRAPPERS_OUT%\actual.obj" "%BF2_RADIUS_WRAPPERS_OUT%\fixture.obj" >"%BF2_RADIUS_WRAPPERS_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0radius_wrappers_runtime.py" run "%BF2_RADIUS_WRAPPERS_OUT%\radius_wrappers_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_RADIUS_WRAPPERS_OUT%\compile.log"
if exist "%BF2_RADIUS_WRAPPERS_OUT%\link.log" type "%BF2_RADIUS_WRAPPERS_OUT%\link.log"
exit /b 1

