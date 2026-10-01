@echo off
setlocal EnableDelayedExpansion
rem [TEMP] [CHOSEN, not recovered] Strict fixture; dumps only this process to the private output.
if "%~1"=="" ( echo Usage: w64_crash_dump_runtime.cmd unique-output-directory & exit /b 2 )
for %%P in ("%~1") do set "CRASHOUT=%%~fP"
set "SRC=%~dp0..\BF2"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0w64_crash_dump_runtime.ps1" -OutputDirectory "%CRASHOUT%"
if errorlevel 1 exit /b 1
for %%A in (x86 x64) do (
    call :build %%A
    if errorlevel 1 exit /b 1
)
exit /b 0
:build
setlocal
set "ARCH=%~1"
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=%ARCH% -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
set "OUT=%CRASHOUT%\%ARCH%"
if not exist "%OUT%" mkdir "%OUT%"
set FLAGS=/nologo /c /MD /O2 /Gy /Gw /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"%SRC%\Core\BuildConfig.h" /I"%SRC%" /I"%CRASHOUT%"
if "%ARCH%"=="x64" set FLAGS=%FLAGS% /we4244 /we4267 /we4311 /we4312
type nul > "%OUT%\compile.log"
type nul > "%OUT%\objects.rsp"
for %%F in (Core\System.cpp Core\StringUtil.cpp) do (
    cl %FLAGS% /Fo"%OUT%\%%~nF.obj" "%SRC%\%%F" >> "%OUT%\compile.log" 2>&1
    if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
    echo "%OUT%\%%~nF.obj">> "%OUT%\objects.rsp"
)
cl %FLAGS% /Fo"%OUT%\fixture.obj" "%~dp0w64_crash_dump_runtime.cpp" >> "%OUT%\compile.log" 2>&1
if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
link /nologo /OPT:REF /OPT:ICF /MACHINE:%ARCH% /OUT:"%OUT%\crash_runtime.exe" "%OUT%\fixture.obj" @"%OUT%\objects.rsp" dbghelp.lib > "%OUT%\link.log" 2>&1
if errorlevel 1 ( type "%OUT%\link.log" & exit /b 1 )
"%OUT%\crash_runtime.exe" "%OUT%" > "%OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%OUT%\runtime.log"
exit /b %RESULT%
