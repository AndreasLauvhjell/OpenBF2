@echo off
setlocal
rem [TEMP] Whole actual manager/template TUs, native chosen external collaborators; strict x86.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_KIT_MANAGER_OUT set "BF2_KIT_MANAGER_OUT=%TASK_CODE%\Build\ai_kit_20260927\manager\validation"
if not exist "%BF2_KIT_MANAGER_OUT%" mkdir "%BF2_KIT_MANAGER_OUT%"
python -I "%~dp0kit_manager_runtime.py" generate "%BF2_KIT_MANAGER_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1"
set "TASK_AI=%TASK_CODE%\BF2\AI\AIInterface\BFKnowledge"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_AI%\KitTemplateManager.cpp" /Fo"%BF2_KIT_MANAGER_OUT%\manager.obj" >"%BF2_KIT_MANAGER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_AI%\KitTemplate.cpp" /Fo"%BF2_KIT_MANAGER_OUT%\template.obj" >>"%BF2_KIT_MANAGER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_KIT_MANAGER_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0kit_manager_runtime.cpp" /Fo"%BF2_KIT_MANAGER_OUT%\fixture.obj" >>"%BF2_KIT_MANAGER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_KIT_MANAGER_OUT%\kit_manager_runtime.exe" "%BF2_KIT_MANAGER_OUT%\manager.obj" "%BF2_KIT_MANAGER_OUT%\template.obj" "%BF2_KIT_MANAGER_OUT%\fixture.obj" >"%BF2_KIT_MANAGER_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0kit_manager_runtime.py" run "%BF2_KIT_MANAGER_OUT%\kit_manager_runtime.exe" >"%BF2_KIT_MANAGER_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_KIT_MANAGER_OUT%\runtime.log"
exit /b %TASK_RESULT%
:failed
type "%BF2_KIT_MANAGER_OUT%\compile.log"
if exist "%BF2_KIT_MANAGER_OUT%\link.log" type "%BF2_KIT_MANAGER_OUT%\link.log"
exit /b 1
