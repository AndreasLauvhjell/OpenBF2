@echo off
setlocal EnableDelayedExpansion
rem [TEMP] [CHOSEN, not recovered] actual-source rebuilt x86/x64 wire comparison.
rem Pass a unique output directory; strict links, no /FORCE and no production/network run.
if "%~1"=="" ( echo Usage: w64_wire_golden.cmd output-directory & exit /b 2 )
for %%P in ("%~1") do set "GOLDOUT=%%~fP"
set "SRC=%~dp0..\BF2"
for %%A in (x86 x64) do (
    call :build %%A
    if errorlevel 1 exit /b 1
)
fc /b "%GOLDOUT%\x86\wire.bin" "%GOLDOUT%\x64\wire.bin" > "%GOLDOUT%\wire_compare.log"
if errorlevel 1 ( type "%GOLDOUT%\wire_compare.log" & echo FAIL encoded bytes differ & exit /b 1 )
fc /b "%GOLDOUT%\x86\decoded.bin" "%GOLDOUT%\x64\decoded.bin" > "%GOLDOUT%\decoded_compare.log"
if errorlevel 1 ( echo FAIL decoded float bits differ; see "%GOLDOUT%\decoded_compare.log" & exit /b 1 )
echo PASS x86/x64 encoded bytes and decoded float bits match.
exit /b 0

:build
setlocal
set "ARCH=%~1"
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=%ARCH% -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "OUT=%GOLDOUT%\%ARCH%"
if not exist "%OUT%" mkdir "%OUT%"
set FLAGS=/nologo /c /MD /O2 /Gy /Gw /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"%SRC%\Core\BuildConfig.h"
set FLAGS=%FLAGS% /I"%SRC%" /I"%SRC%\External\python-embedded\py-src-2.3.4\Include" /I"%SRC%\External\python-embedded\py-src-2.3.4\PC"
type nul > "%OUT%\compile.log"
type nul > "%OUT%\objects.rsp"
for %%F in (IO\BitStream.cpp Game\Common\GameEvent.cpp Game\GameEvents\BeginRoundEvent.cpp Game\GameEvents\DestroyObjectEvent.cpp Game\GameEvents\EnterVehicleEvent.cpp Game\GameEvents\ExitVehicleEvent.cpp) do (
    cl %FLAGS% /Fo"%OUT%\%%~nF.obj" "%SRC%\%%F" >> "%OUT%\compile.log" 2>&1
    if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
    echo "%OUT%\%%~nF.obj">> "%OUT%\objects.rsp"
)
cl %FLAGS% /Fo"%OUT%\fixture.obj" "%~dp0w64_wire_golden.cpp" >> "%OUT%\compile.log" 2>&1
if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
link /nologo /OPT:REF /OPT:ICF /MACHINE:%ARCH% /NODEFAULTLIB:python23.lib /OUT:"%OUT%\w64_wire_golden.exe" "%OUT%\fixture.obj" @"%OUT%\objects.rsp" > "%OUT%\link.log" 2>&1
if errorlevel 1 ( type "%OUT%\link.log" & exit /b 1 )
"%OUT%\w64_wire_golden.exe" "%OUT%\wire.bin" "%OUT%\decoded.bin" > "%OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%OUT%\runtime.log"
exit /b %RESULT%
