@echo off
setlocal
rem [TEMP] Exact squad-strength source, strict MSVC x86, separate manager-observer variant.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_SQUAD_STRENGTH_OUT set "BF2_SQUAD_STRENGTH_OUT=%TASK_CODE%\Build\ai_area_update_20260927\squads\validation"
if not exist "%BF2_SQUAD_STRENGTH_OUT%" mkdir "%BF2_SQUAD_STRENGTH_OUT%"
python -I "%~dp0squad_strength_runtime.py" generate "%BF2_SQUAD_STRENGTH_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_SQUAD_STRENGTH_OUT%\squad_strength_actual.cpp" /Fo"%BF2_SQUAD_STRENGTH_OUT%\actual.obj" >"%BF2_SQUAD_STRENGTH_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_SQUAD_STRENGTH_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0squad_strength_runtime.cpp" /Fo"%BF2_SQUAD_STRENGTH_OUT%\fixture.obj" >>"%BF2_SQUAD_STRENGTH_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_SQUAD_STRENGTH_OUT%\squad_strength_runtime.exe" "%BF2_SQUAD_STRENGTH_OUT%\actual.obj" "%BF2_SQUAD_STRENGTH_OUT%\fixture.obj" >"%BF2_SQUAD_STRENGTH_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /DSQUAD_STRENGTH_OBSERVERS /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_SQUAD_STRENGTH_OUT%\squad_strength_actual.cpp" /Fo"%BF2_SQUAD_STRENGTH_OUT%\observer_actual.obj" >>"%BF2_SQUAD_STRENGTH_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /DSQUAD_STRENGTH_OBSERVERS /I"%BF2_SQUAD_STRENGTH_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0squad_strength_runtime.cpp" /Fo"%BF2_SQUAD_STRENGTH_OUT%\observer_fixture.obj" >>"%BF2_SQUAD_STRENGTH_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_SQUAD_STRENGTH_OUT%\squad_strength_observers.exe" "%BF2_SQUAD_STRENGTH_OUT%\observer_actual.obj" "%BF2_SQUAD_STRENGTH_OUT%\observer_fixture.obj" >>"%BF2_SQUAD_STRENGTH_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0squad_strength_runtime.py" run "%BF2_SQUAD_STRENGTH_OUT%"
exit /b %errorlevel%
:failed
type "%BF2_SQUAD_STRENGTH_OUT%\compile.log"
if exist "%BF2_SQUAD_STRENGTH_OUT%\link.log" type "%BF2_SQUAD_STRENGTH_OUT%\link.log"
exit /b 1
