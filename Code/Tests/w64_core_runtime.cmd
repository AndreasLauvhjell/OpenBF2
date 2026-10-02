@echo off
setlocal EnableDelayedExpansion
rem [TEMP] [CHOSEN, not recovered] Strict actual-source core fixture, both native architectures.
if "%~1"=="" ( echo Usage: w64_core_runtime.cmd unique-output-directory & exit /b 2 )
for %%P in ("%~1") do set "COREOUT=%%~fP"
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
set "OUT=%COREOUT%\%ARCH%"
if not exist "%OUT%" mkdir "%OUT%"
set FLAGS=/nologo /c /MD /O2 /Gy /Gw /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"%SRC%\Core\BuildConfig.h" /I"%SRC%"
if "%ARCH%"=="x64" set FLAGS=%FLAGS% /we4244 /we4267 /we4311 /we4312
type nul > "%OUT%\compile.log"
type nul > "%OUT%\objects.rsp"
for %%F in (IO\Console\ConsoleCompiler.cpp Core\System.cpp Core\StringUtil.cpp) do (
    cl %FLAGS% /Fo"%OUT%\%%~nF.obj" "%SRC%\%%F" >> "%OUT%\compile.log" 2>&1
    if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
    echo "%OUT%\%%~nF.obj">> "%OUT%\objects.rsp"
)
cl %FLAGS% /Fo"%OUT%\fixture.obj" "%~dp0w64_core_runtime.cpp" >> "%OUT%\compile.log" 2>&1
if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
link /nologo /OPT:REF /OPT:ICF /MACHINE:%ARCH% /OUT:"%OUT%\core_runtime.exe" "%OUT%\fixture.obj" @"%OUT%\objects.rsp" dbghelp.lib > "%OUT%\link.log" 2>&1
if errorlevel 1 ( type "%OUT%\link.log" & exit /b 1 )
"%OUT%\core_runtime.exe" > "%OUT%\runtime.log" 2>&1
if errorlevel 1 ( type "%OUT%\runtime.log" & exit /b 1 )
"%OUT%\core_runtime.exe" no-extension >> "%OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%OUT%\runtime.log"
exit /b %RESULT%
