@echo off
setlocal
rem [TEMP] Strict optimized x86 EnvMapObject/template fixture, both renderer ABI modes.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if not defined BF2_TEST_ABI_NATIVE set "BF2_TEST_ABI_NATIVE=1"
set "TASK_OUT=%~dp0..\Build\bundled_height_20260923\envtest%BF2_TEST_ABI_NATIVE%"
if not exist "%TASK_OUT%" mkdir "%TASK_OUT%"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc /Gy /O2 /DBF2_IMAGE_RENDDX9 /DBF2_PLUGIN_ABI_NATIVE=%BF2_TEST_ABI_NATIVE% /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0 /FI%~dp0..\BF2\Core\BuildConfig.h"
cl %TASK_FLAGS% "%~dp0envmap_object_runtime.cpp" /Fo"%TASK_OUT%\fixture.obj" >"%TASK_OUT%\compile.log" 2>&1
if errorlevel 1 (
 type "%TASK_OUT%\compile.log"
 exit /b 1
)
cl %TASK_FLAGS% "%~dp0..\RendDX9Abi\Vc71String.cpp" /Fo"%TASK_OUT%\Vc71String.obj" >"%TASK_OUT%\strings.log" 2>&1
if errorlevel 1 (
 type "%TASK_OUT%\strings.log"
 exit /b 1
)
link /nologo /OPT:REF /INCREMENTAL:NO /OUT:"%TASK_OUT%\fixture.exe" "%TASK_OUT%\fixture.obj" "%TASK_OUT%\Vc71String.obj" >"%TASK_OUT%\link.log" 2>&1
if errorlevel 1 (
 type "%TASK_OUT%\link.log"
 exit /b 1
)
"%TASK_OUT%\fixture.exe"
