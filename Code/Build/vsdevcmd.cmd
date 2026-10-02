@echo off
rem ===========================================================================
rem MSVC developer-environment resolver shared by the build and test scripts.
rem
rem   usage:  call "<rel>\vsdevcmd.cmd" -arch=x86
rem
rem Resolution order for VsDevCmd.bat:
rem   1. BF2_VSDEVCMD - explicit per-environment override, the full path to
rem      VsDevCmd.bat. Set it (user or machine scope) to work on a team without
rem      editing any script.
rem   2. vswhere.exe - the latest Visual Studio install carrying the x86/x64 VC
rem      tools.
rem
rem Do NOT wrap the VsDevCmd call in setlocal: VsDevCmd sets PATH, INCLUDE and
rem LIB for the CALLER, and a setlocal here would discard them on return.
rem The file has no :labels on purpose: this repository stores .cmd files with
rem LF line endings, and cmd.exe can fail to resolve a label that follows a
rem parenthesised block in such a file.
rem ===========================================================================
set "VSDEVCMD_ARCH=%~1"
rem cmd treats '=' as an argument delimiter, so the older "-arch=x86" form
rem arrives as %~1=-arch, %~2=x86. Accept both that and a bare "x86".
if /i "%VSDEVCMD_ARCH%"=="-arch" set "VSDEVCMD_ARCH=%~2"
if not defined VSDEVCMD_ARCH set "VSDEVCMD_ARCH=x86"

rem Locate vswhere. The path lives under Program Files (x86) on every install.
set "VSDEVCMD_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSDEVCMD_VSWHERE%" set "VSDEVCMD_VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

set "VSDEVCMD_BAT=%BF2_VSDEVCMD%"
if not defined VSDEVCMD_BAT if exist "%VSDEVCMD_VSWHERE%" for /f "usebackq delims=" %%I in (`"%VSDEVCMD_VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath 2^>nul`) do set "VSDEVCMD_BAT=%%I\Common7\Tools\VsDevCmd.bat"

if not defined VSDEVCMD_BAT (
    echo [vsdevcmd] Visual Studio developer environment not found.
    echo [vsdevcmd] Set BF2_VSDEVCMD to your VsDevCmd.bat, or install the x86/x64 VC tools.
    exit /b 1
)
if not exist "%VSDEVCMD_BAT%" (
    echo [vsdevcmd] Visual Studio developer environment not found:
    echo [vsdevcmd]   %VSDEVCMD_BAT%
    echo [vsdevcmd] Set BF2_VSDEVCMD to your VsDevCmd.bat and retry.
    exit /b 1
)
call "%VSDEVCMD_BAT%" -arch=%VSDEVCMD_ARCH% -no_logo >nul 2>&1
exit /b %errorlevel%
