@echo off
setlocal EnableDelayedExpansion
rem [TEMP] [CHOSEN, not recovered] Strict actual-source MemoryPool fixture.
if "%~1"=="" ( echo Usage: w64_memory_pool_runtime.cmd unique-output-directory & exit /b 2 )
for %%P in ("%~1") do set "POOLOUT=%%~fP"
set "SRC=%~dp0..\BF2"
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
set "OUT=%POOLOUT%\%ARCH%"
if not exist "%OUT%" mkdir "%OUT%"
set FLAGS=/nologo /c /MD /O2 /Gy /Gw /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"%SRC%\Core\BuildConfig.h" /I"%SRC%"
if "%ARCH%"=="x64" set FLAGS=%FLAGS% /we4244 /we4267 /we4311 /we4312
type nul > "%OUT%\compile.log"
for %%F in (MemoryPool Malloc) do (
    cl %FLAGS% /Fo"%OUT%\%%F.obj" "%SRC%\Core\%%F.cpp" >> "%OUT%\compile.log" 2>&1
    if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
)
cl %FLAGS% /Fo"%OUT%\fixture.obj" "%~dp0w64_memory_pool_runtime.cpp" >> "%OUT%\compile.log" 2>&1
if errorlevel 1 ( type "%OUT%\compile.log" & exit /b 1 )
link /nologo /OPT:REF /OPT:ICF /MACHINE:%ARCH% /OUT:"%OUT%\memory_pool_runtime.exe" "%OUT%\fixture.obj" "%OUT%\MemoryPool.obj" "%OUT%\Malloc.obj" > "%OUT%\link.log" 2>&1
if errorlevel 1 ( type "%OUT%\link.log" & exit /b 1 )
"%OUT%\memory_pool_runtime.exe" > "%OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%OUT%\runtime.log"
exit /b %RESULT%
