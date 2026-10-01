@echo off
setlocal
rem [TEMP] Real inline prefix accessors; native lifetime/layout comparison, no /FORCE.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_INFORMATION_PREFIX_OUT set "BF2_INFORMATION_PREFIX_OUT=%TASK_CODE%\Build\ai_radius_20260927\information\validation"
if not exist "%BF2_INFORMATION_PREFIX_OUT%" mkdir "%BF2_INFORMATION_PREFIX_OUT%"
cl /nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W3 /DDEDICATED=1 /DBF2_IMAGE_AIDLL /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0information_prefix_runtime.cpp" /Fo"%BF2_INFORMATION_PREFIX_OUT%\fixture.obj" >"%BF2_INFORMATION_PREFIX_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_INFORMATION_PREFIX_OUT%\information_prefix_runtime.exe" "%BF2_INFORMATION_PREFIX_OUT%\fixture.obj" >"%BF2_INFORMATION_PREFIX_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0information_prefix_runtime.py" "%BF2_INFORMATION_PREFIX_OUT%\information_prefix_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_INFORMATION_PREFIX_OUT%\compile.log"
if exist "%BF2_INFORMATION_PREFIX_OUT%\link.log" type "%BF2_INFORMATION_PREFIX_OUT%\link.log"
exit /b 1
