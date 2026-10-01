@echo off
setlocal EnableDelayedExpansion
rem [TEMP] [CHOSEN, not recovered] Strict actual-source manager/module fixture.
if "%~1"=="" ( echo Usage: w64_console_manager_runtime.cmd unique-output-directory & exit /b 2 )
for %%P in ("%~1") do set "MANAGEROUT=%%~fP"
set "SRC=%~dp0..\BF2"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0w64_console_manager_runtime.ps1" -OutputDirectory "%MANAGEROUT%"
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
set "OUT=%MANAGEROUT%\%ARCH%"
if not exist "%OUT%" mkdir "%OUT%"
set FLAGS=/nologo /c /MD /O2 /Gy /Gw /Zc:inline /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"%SRC%\Core\BuildConfig.h" /I"%SRC%"
set FLAGS=%FLAGS% /I"%SRC%\External\python-embedded\py-src-2.3.4\Include" /I"%SRC%\External\python-embedded\py-src-2.3.4\PC" /I"%SRC%\External\zlib-1.2.3" /I"%SRC%\External\zlib-1.2.3\contrib\minizip"
if "%ARCH%"=="x64" set FLAGS=%FLAGS% /we4244 /we4267 /we4311 /we4312
type nul > "%OUT%\compile.log"
type nul > "%OUT%\objects.rsp"
for %%F in (IO\Console\Manager.cpp Math\Math.cpp) do (
    cl %FLAGS% /Fo"%OUT%\%%~nF.obj" "%SRC%\%%F" >> "%OUT%\compile.log" 2>&1
    if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
    echo "%OUT%\%%~nF.obj">> "%OUT%\objects.rsp"
)
rem Compile whole production TUs; runtime selects the two modules without unrelated engine references.
for %%F in (Core\ModuleRegistry.cpp Core\Globals.cpp) do (
    cl %FLAGS% /Fo"%OUT%\%%~nF.obj" "%SRC%\%%F" >> "%OUT%\compile.log" 2>&1
    if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
)
cl %FLAGS% /Fo"%OUT%\module_bodies.obj" "%MANAGEROUT%\module_bodies.cpp" >> "%OUT%\compile.log" 2>&1
if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
echo "%OUT%\module_bodies.obj">> "%OUT%\objects.rsp"
cl %FLAGS% /Fo"%OUT%\fixture.obj" "%~dp0w64_console_manager_runtime.cpp" >> "%OUT%\compile.log" 2>&1
if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
link /nologo /OPT:REF /OPT:ICF /MACHINE:%ARCH% /NODEFAULTLIB:python23.lib /OUT:"%OUT%\manager_runtime.exe" "%OUT%\fixture.obj" @"%OUT%\objects.rsp" > "%OUT%\link.log" 2>&1
if errorlevel 1 ( echo Link failed; see "%OUT%\link.log" & exit /b 1 )
"%OUT%\manager_runtime.exe" > "%OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%OUT%\runtime.log"
exit /b %RESULT%
