@echo off
setlocal
rem [TEMP] Strict MSVC x86 MapContainer::init fixture; no server and no FORCE link.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1

set "TASK_CODE=%~dp0..\.."
if not defined BF2_MAP_INIT_OUT set "BF2_MAP_INIT_OUT=%TASK_CODE%\Build\ai_map_query_20261001\init_validation"
if not exist "%BF2_MAP_INIT_OUT%" mkdir "%BF2_MAP_INIT_OUT%"

python -I "%~dp0mapcontainer_init_runtime.py" generate "%BF2_MAP_INIT_OUT%"
if errorlevel 1 exit /b 1

set "TASK_FLAGS=/nologo /c /O2 /MD /std:c++14 /permissive /EHsc /W4 /WX /Gy /DDEDICATED=1 /DBF2_IMAGE_AIDLL"
type nul >"%BF2_MAP_INIT_OUT%\compile.log"
cl %TASK_FLAGS% /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%BF2_MAP_INIT_OUT%\actual.cpp" /Fo"%BF2_MAP_INIT_OUT%\actual.obj" >>"%BF2_MAP_INIT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed
cl %TASK_FLAGS% /I"%BF2_MAP_INIT_OUT%" /FI"%TASK_CODE%\BF2\Core\BuildConfig.h" "%~dp0mapcontainer_init_runtime.cpp" /Fo"%BF2_MAP_INIT_OUT%\fixture.obj" >>"%BF2_MAP_INIT_OUT%\compile.log" 2>&1
if errorlevel 1 goto failed

set "O=%BF2_MAP_INIT_OUT%"
link /nologo /MACHINE:X86 /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%O%\mapcontainer_init_runtime.exe" "%O%\actual.obj" "%O%\fixture.obj" >"%O%\link.log" 2>&1
if errorlevel 1 goto failed

>"%O%\build.log" echo PASS strict MSVC x86 compile
>>"%O%\build.log" echo PASS strict x86 link without FORCE
python -I "%~dp0mapcontainer_init_runtime.py" run "%O%\mapcontainer_init_runtime.exe" "%O%"
exit /b %errorlevel%

:failed
if exist "%BF2_MAP_INIT_OUT%\compile.log" type "%BF2_MAP_INIT_OUT%\compile.log"
if exist "%BF2_MAP_INIT_OUT%\link.log" type "%BF2_MAP_INIT_OUT%\link.log"
exit /b 1

