@echo off
setlocal
rem [TEMP] Actual io::ControlMap record copy, reset and destruction x86 fixture (ControlMap.cpp only,
rem strict link). Records are inserted through the fixture friend because the add* bodies are stubs.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\haptic_settings_20260908\runtime_controlmap"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0
type nul >"%BF2_BUILD_OUT%\compile.log"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\IO\Input\ControlMap.cpp" /Fo"%BF2_BUILD_OUT%\ControlMap.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%~dp0control_map_copy_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\control_map_copy_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" "%BF2_BUILD_OUT%\ControlMap.obj" "%BF2_BUILD_OUT%\fixture.obj" kernel32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
"%BF2_BUILD_OUT%\control_map_copy_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
