@echo off
setlocal EnableDelayedExpansion
rem [TEMP] [CHOSEN, not recovered] Actual-source typed client dispatch checks.
rem Both builds link strictly and run without game content, sockets or retail DLLs.
if "%~1"=="" ( echo Usage: client_event_dispatch_runtime.cmd output-directory & exit /b 2 )
for %%P in ("%~1") do set "TESTOUT=%%~fP"
set "SRC=%~dp0..\BF2"
for %%A in (x86 x64) do (
    call :build %%A
    if errorlevel 1 exit /b 1
)
exit /b 0

:build
setlocal
set "ARCH=%~1"
call "%~dp0..\Build\vsdevcmd.cmd" %ARCH%
if errorlevel 1 exit /b 1
set "OUT=%TESTOUT%\%ARCH%"
if not exist "%OUT%" mkdir "%OUT%"
set FLAGS=/nologo /c /MD /O2 /Gy /Gw /std:c++14 /permissive /EHsc /DDEDICATED=0 /FI"%SRC%\Core\BuildConfig.h"
set FLAGS=%FLAGS% /I"%SRC%" /I"%SRC%\External\python-embedded\py-src-2.3.4\Include" /I"%SRC%\External\python-embedded\py-src-2.3.4\PC"
type nul > "%OUT%\compile.log"
type nul > "%OUT%\objects.rsp"
for %%F in (IO\BitStream.cpp IO\Network\NetworkStats.cpp Game\Common\GameEvent.cpp Game\GameEvents\ChallengeEvent.cpp Game\GameEvents\DataBlockEvent.cpp Game\GameEvents\StringBlockEvent.cpp Game\GameEvents\DestroyPlayerEvent.cpp Game\GameEvents\DestroyObjectEvent.cpp Game\GameEvents\EnterVehicleEvent.cpp Game\GameEvents\ExitVehicleEvent.cpp) do (
    cl %FLAGS% /Fo"%OUT%\%%~nF.obj" "%SRC%\%%F" >> "%OUT%\compile.log" 2>&1
    if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
    echo "%OUT%\%%~nF.obj">> "%OUT%\objects.rsp"
)
cl %FLAGS% /Fo"%OUT%\fixture.obj" "%~dp0client_event_dispatch_runtime.cpp" >> "%OUT%\compile.log" 2>&1
if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
link /nologo /OPT:REF /OPT:ICF /MACHINE:%ARCH% /MANIFEST:EMBED /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /NODEFAULTLIB:python23.lib /OUT:"%OUT%\client_event_dispatch_runtime.exe" "%OUT%\fixture.obj" @"%OUT%\objects.rsp" > "%OUT%\link.log" 2>&1
if errorlevel 1 ( type "%OUT%\link.log" & exit /b 1 )
"%OUT%\client_event_dispatch_runtime.exe" > "%OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%OUT%\runtime.log"
if not "%RESULT%"=="0" exit /b %RESULT%
findstr /c:"PASS client event and status dispatch" "%OUT%\runtime.log" >nul
if errorlevel 1 ( echo FAIL %ARCH% fixture exited without its completion marker & exit /b 1 )
exit /b 0
