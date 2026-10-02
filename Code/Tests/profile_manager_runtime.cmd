@echo off
setlocal
rem [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: profile-manager-runtime]
rem Strict x86 owner fixture: production ProfileManager, exact four fixture GP calls.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\swiff_profile_status_20260909\owner_fixture_o2"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /Gw /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /FI"%TASK_ROOT%\Core\BuildConfig.h"
set "TASK_OBJECTS="
type nul >"%BF2_BUILD_OUT%\compile.log"
for %%S in (Game\ProfileManager Game\Persistence\AntiHacks Core\ClassManager Core\Debug Core\StringUtil) do (
    call :compile "%%S"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /c "%~dp0profile_manager_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\profile_manager_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" kernel32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
"%BF2_BUILD_OUT%\profile_manager_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
:compile
set "TASK_SOURCE=%~1"
set "TASK_NAME=%TASK_SOURCE:\=_%"
cl %TASK_FLAGS% /c "%TASK_ROOT%\%TASK_SOURCE%.cpp" /Fo"%BF2_BUILD_OUT%\%TASK_NAME%.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
set TASK_OBJECTS=%TASK_OBJECTS% "%BF2_BUILD_OUT%\%TASK_NAME%.obj"
exit /b 0
