@echo off
setlocal
rem [TEMP] Whole actual Vec3 TU, strict MSVC x86. No extraction or /FORCE.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "TASK_CODE=%~dp0..\.."
if not defined BF2_BOX_MAX_CORNER_OUT set "BF2_BOX_MAX_CORNER_OUT=%TASK_CODE%\Build\ai_kit_equipment_20260927\geometry"
if not exist "%BF2_BOX_MAX_CORNER_OUT%" mkdir "%BF2_BOX_MAX_CORNER_OUT%"
python -I "%~dp0box_max_corner_runtime.py" record "%BF2_BOX_MAX_CORNER_OUT%"
if errorlevel 1 exit /b 1
set "TASK_FLAGS=/nologo /c /O2 /MD /fp:precise /std:c++14 /permissive /EHsc /W3 /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" /FAs /Fa"%BF2_BOX_MAX_CORNER_OUT%\actual.asm" "%TASK_CODE%\BF2\Math\Vec3.cpp" /Fo"%BF2_BOX_MAX_CORNER_OUT%\actual.obj" >"%BF2_BOX_MAX_CORNER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0box_max_corner_runtime.cpp" /Fo"%BF2_BOX_MAX_CORNER_OUT%\fixture.obj" >>"%BF2_BOX_MAX_CORNER_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%BF2_BOX_MAX_CORNER_OUT%\box_max_corner_runtime.exe" "%BF2_BOX_MAX_CORNER_OUT%\actual.obj" "%BF2_BOX_MAX_CORNER_OUT%\fixture.obj" >"%BF2_BOX_MAX_CORNER_OUT%\link.log" 2>&1
if errorlevel 1 goto failed
python -I "%~dp0box_max_corner_runtime.py" run "%BF2_BOX_MAX_CORNER_OUT%\box_max_corner_runtime.exe"
exit /b %errorlevel%
:failed
type "%BF2_BOX_MAX_CORNER_OUT%\compile.log"
if exist "%BF2_BOX_MAX_CORNER_OUT%\link.log" type "%BF2_BOX_MAX_CORNER_OUT%\link.log"
exit /b 1
