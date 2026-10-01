@echo off
setlocal
if "%~1"=="" ( echo Usage: w64_startup_runtime.cmd unique-output-directory & exit /b 2 )
for %%P in ("%~1") do set "STARTOUT=%%~fP"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0w64_startup_runtime.ps1" -OutputDirectory "%STARTOUT%"
if errorlevel 1 exit /b 1
for %%A in (x86 x64) do (
    call :build %%A
    if errorlevel 1 exit /b 1
)
exit /b 0
:build
setlocal
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=%1 -no_logo >nul 2>&1
if errorlevel 1 exit /b 1
cl /nologo /MD /O2 /std:c++14 /permissive /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" /I"%STARTOUT%" /Fo"%STARTOUT%\startup_%1.obj" /Fe"%STARTOUT%\startup_%1.exe" "%~dp0w64_startup_runtime.cpp" /link /OPT:REF /MACHINE:%1 > "%STARTOUT%\compile_%1.log" 2>&1
if errorlevel 1 ( type "%STARTOUT%\compile_%1.log" & exit /b 1 )
"%STARTOUT%\startup_%1.exe" > "%STARTOUT%\runtime_%1.log" 2>&1
set "RESULT=%errorlevel%"
type "%STARTOUT%\runtime_%1.log"
exit /b %RESULT%
