@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem SCAFFOLDING - NOT RECOVERED SOURCE. Builds and runs the BF2Audio.dll loopback
rem host (Code\Build\bf2audio_smoke\smoke.cpp) against the REBUILT DLL.
rem
rem   usage:  bf2audio_smoke.cmd [file.wav] [provider display name] [seconds]
rem
rem   BF2_GAME_DIR   the reference game directory holding Memory.dll and
rem                  BF2OpenAL.dll (read only; nothing is written there).
rem   BF2_BF2AUDIO_BIN  where bf2audio.cmd put BF2Audio.dll (default bin_bf2audio).
rem ===========================================================================
call "%~dp0vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
for %%I in ("%~dp0..") do set CODE=%%~fI
set BF2=%CODE%\BF2
set OUT=%~dp0obj_bf2audio_smoke
set BIN=%~dp0bin_bf2audio
if defined BF2_BF2AUDIO_BIN set "BIN=%BF2_BF2AUDIO_BIN%"
if not defined BF2_GAME_DIR set "BF2_GAME_DIR=D:\B2BF Launcher\Battlefield2"
set WAV=%~1
if "%WAV%"=="" set WAV=C:\Windows\Media\tada.wav
set PROVIDER=%~2
if "%PROVIDER%"=="" set PROVIDER=Software Audio
set SECONDS=%~3
if "%SECONDS%"=="" set SECONDS=4
if not exist "%OUT%" mkdir "%OUT%"
cl /nologo /MT /EHsc /std:c++14 /permissive /W3 /Zi /Fd"%OUT%\smoke.pdb" ^
   /DBF2_IMAGE_BF2AUDIO /DBF2_PLUGIN_ABI_NATIVE=0 /DDEDICATED=0 /FI"%BF2%\Core\BuildConfig.h" /DWIN32 /D_CRT_SECURE_NO_WARNINGS ^
   /Fo"%OUT%\smoke.obj" /Fe"%OUT%\smoke.exe" "%~dp0bf2audio_smoke\smoke.cpp" /link kernel32.lib >"%OUT%\cl.log" 2>&1
if errorlevel 1 (
    echo   COMPILE FAIL smoke.cpp
    type "%OUT%\cl.log"
    exit /b 1
)
echo   running: "%OUT%\smoke.exe" "%BIN%\BF2Audio.dll" "%BF2_GAME_DIR%" "%WAV%" "%PROVIDER%" %SECONDS%
"%OUT%\smoke.exe" "%BIN%\BF2Audio.dll" "%BF2_GAME_DIR%" "%WAV%" "%PROVIDER%" %SECONDS%
set RC=%ERRORLEVEL%
echo   smoke exit code: %RC%
exit /b %RC%
