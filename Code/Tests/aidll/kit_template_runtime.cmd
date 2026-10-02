@echo off
setlocal
rem [TEMP] Whole production KitTemplate TU; strict native x86, no /FORCE.
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_KIT_TEMPLATE_OUT set "BF2_KIT_TEMPLATE_OUT=%TASK_CODE%\Build\ai_kit_20260927\template\validation"
if not exist "%BF2_KIT_TEMPLATE_OUT%" mkdir "%BF2_KIT_TEMPLATE_OUT%"
cl /nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /DDEDICATED=1 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0kit_template_runtime.cpp" /Fo"%BF2_KIT_TEMPLATE_OUT%\fixture.obj" >"%BF2_KIT_TEMPLATE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl /nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /DDEDICATED=1 /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%TASK_CODE%\BF2\AI\AIInterface\BFKnowledge\KitTemplate.cpp" /Fo"%BF2_KIT_TEMPLATE_OUT%\actual.obj" >>"%BF2_KIT_TEMPLATE_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_KIT_TEMPLATE_OUT%\kit_template_runtime.exe" "%BF2_KIT_TEMPLATE_OUT%\fixture.obj" "%BF2_KIT_TEMPLATE_OUT%\actual.obj" >"%BF2_KIT_TEMPLATE_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0kit_template_runtime.py" "%BF2_KIT_TEMPLATE_OUT%\kit_template_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_KIT_TEMPLATE_OUT%\compile.log"
if exist "%BF2_KIT_TEMPLATE_OUT%\link.log" type "%BF2_KIT_TEMPLATE_OUT%\link.log"
exit /b 1
