@echo off
setlocal
rem [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: music-player-runtime]
rem Strict x86 native MusicPlayer fixture; the Game runtime flag alone is fixture-owned.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\music_event_20260909\tests_o2"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set TASK_FLAGS=/nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Z7 /Gy /Gw /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /FI"%TASK_ROOT%\Core\BuildConfig.h"
set "TASK_OBJECTS="
type nul >"%BF2_BUILD_OUT%\compile.log"
for %%S in (Sound\BfSound\MusicPlayer Sound\BfSound Sound\NullSound Core\EventManager Core\ClassManager Core\Debug Core\StringUtil) do (
    call :compile "%%S"
    if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /c "%~dp0music_player_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
link /nologo /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\music_player_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" %TASK_OBJECTS% "%BF2_BUILD_OUT%\fixture.obj" kernel32.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
"%BF2_BUILD_OUT%\music_player_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\runtime.log" & exit /b 1)
type "%BF2_BUILD_OUT%\runtime.log"
for %%T in (state1-mode0 idle-mode4 idle-mode-minus1 corrupt-event3 corrupt-event5) do (
    call :fatal "%%T"
    if errorlevel 1 exit /b 1
)
echo MusicPlayer diagnostic termination: 5/5 passed
exit /b 0
:compile
set "TASK_SOURCE=%~1"
set "TASK_NAME=%TASK_SOURCE:\=_%"
cl %TASK_FLAGS% /c "%TASK_ROOT%\%TASK_SOURCE%.cpp" /Fo"%BF2_BUILD_OUT%\%TASK_NAME%.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
set TASK_OBJECTS=%TASK_OBJECTS% "%BF2_BUILD_OUT%\%TASK_NAME%.obj"
exit /b 0
:fatal
"%BF2_BUILD_OUT%\music_player_runtime.exe" "%~1" >"%BF2_BUILD_OUT%\fatal_%~1.log" 2>&1
if not "%errorlevel%"=="3" (type "%BF2_BUILD_OUT%\fatal_%~1.log" & echo Unexpected fatal exit for %~1 & exit /b 1)
findstr /c:"EXPECTED_DIAGNOSTIC Event " "%BF2_BUILD_OUT%\fatal_%~1.log" >nul
if errorlevel 1 (echo Missing expected diagnostic for %~1 & exit /b 1)
exit /b 0
