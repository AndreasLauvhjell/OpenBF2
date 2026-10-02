@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem SCAFFOLDING - NOT RECOVERED SOURCE. Builds the REPLACEMENT BF2Audio.dll.
rem
rem   Code\Build\build.cmd        the dedicated server, bf2_w32ded.exe
rem   Code\Build\renddx9.cmd      the replacement RendDX9.dll
rem   Code\Build\swiffplayer.cmd  the replacement SwiffPlayer.dll
rem   Code\Build\bf2audio.cmd     THIS. The OpenAL sound engine BF2.exe loads.
rem
rem IT NEVER WRITES INTO THE GAME DIRECTORY. Output goes to
rem   Code\Build\bin_bf2audio\ (retail) or bin_bf2audio_native\ (native).
rem   The shipped BF2Audio.dll in the reference game directory is the REFERENCE
rem   BINARY. Testing against the real client is a DELIBERATE, SEPARATE act -
rem   see docs\building.md for host ABI and runtime setup.
rem
rem VALIDATE SOUNDENGINE FILES ONLY WITH THIS SCRIPT, RUN VIA THE PowerShell TOOL.
rem   syntaxcheck.cmd passes no image define; the DLL-side ISoundEngine table
rem   (slot 54 = the virtual destructor) only exists under BF2_IMAGE_BF2AUDIO.
rem   Git-Bash `cmd /c` returns 0 without building.
rem
rem   usage:  bf2audio.cmd          VC7.1-compatible ABI (/MT + Memory.dll)
rem           bf2audio.cmd native   native host ABI (/MD, no Memory.dll)
rem   Both modes accept BF2_BF2AUDIO_OUT and BF2_BF2AUDIO_BIN overrides.
rem
rem   BF2AUDIO_VORBIS=<dir>  optional: an include root holding ogg\ and vorbis\
rem     headers plus libogg/libvorbis/libvorbisfile static libs (see the status
rem     doc). Without it the .ogg paths (streams, music) are [FRONTIER] reports
rem     through the trace sink and .wav playback is everything the DLL does.
rem ===========================================================================
call "%~dp0vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
if not "%~1"=="" if /i not "%~1"=="native" (
    echo Usage: bf2audio.cmd [native]
    exit /b 2
)

for %%I in ("%~dp0..") do set CODE=%%~fI
set ABI=%CODE%\BF2AudioAbi
set RD9ABI=%CODE%\RendDX9Abi
set BF2=%CODE%\BF2
set SND=%CODE%\BF2\Sound\SoundEngine
set NATIVE=0
if /i "%~1"=="native" set NATIVE=1
set OUT=%~dp0obj_bf2audio
set BIN=%~dp0bin_bf2audio
set CRT=/MT
set PLUGIN_ABI=/DBF2_PLUGIN_ABI_NATIVE=0
if "%NATIVE%"=="1" (
    set OUT=%~dp0obj_bf2audio_native
    set BIN=%~dp0bin_bf2audio_native
    set CRT=/MD
    set PLUGIN_ABI=/DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0
)
rem Apply overrides AFTER choosing the flavour; neither mode may discard an isolated path.
if defined BF2_BF2AUDIO_OUT set "OUT=%BF2_BF2AUDIO_OUT%"
if defined BF2_BF2AUDIO_BIN set "BIN=%BF2_BF2AUDIO_BIN%"
for %%I in ("%OUT%") do set "OUT=%%~fI"
for %%I in ("%BIN%") do set "BIN=%%~fI"
set MEMORY_LIB="%OUT%\Memory.lib"
if "%NATIVE%"=="1" set MEMORY_LIB=
rem Ogg Vorbis: the vendored upstream sources (Code\BF2\External\libogg-1.3.6 and libvorbis-1.3.7,
rem the decoder set plus vorbisfile) are compiled INTO the DLL, as the shipped image statically
rem links its own (DICE-patched) libvorbis 1.0. BF2AUDIO_NO_VORBIS=1 leaves them out; the .ogg
rem paths then report [FRONTIER] and stay silent.
set OGG=%CODE%\BF2\External\libogg-1.3.6
set VORBIS=%CODE%\BF2\External\libvorbis-1.3.7
set VORBIS_INC=
set VORBIS_LIBS=
set VORBIS_DEF=
set VORBIS_ON=0
if exist "%VORBIS%\lib\vorbisfile.c" if not defined BF2AUDIO_NO_VORBIS (
    set VORBIS_INC=/I "%OGG%\include" /I "%VORBIS%\include"
    set VORBIS_DEF=/DBF2AUDIO_WITH_VORBIS=1
    set VORBIS_ON=1
)

rem /MT              - a PRIVATE static CRT, as the shipped image (it links MSVCR71/MSVCP71 for the
rem                    STL and stdio only; every allocation goes to Memory.dll through operator new).
rem /MD (native)     - shares the rebuilt host's current CRT/STL.
rem /DBF2_IMAGE_BF2AUDIO - selects the DLL-side ISoundEngine table (Sound/NullSound.h slot 54) and
rem                    keeps the server's null-engine static objects out of NullSound.cpp.
rem /DDEDICATED=0 + /FI Core\BuildConfig.h - the IMAGE define; this DLL is part of the CLIENT
rem                    configuration of the shared source.
rem /DNDEBUG         - the shipped image is a release build: no CRT asserts.
set FLAGS=/nologo /c %CRT% /std:c++14 /permissive /EHsc /W3 /FS /Zi ^
 %VORBIS_INC% ^
 /DBF2_IMAGE_BF2AUDIO %PLUGIN_ABI% %VORBIS_DEF% /DDEDICATED=0 /FI"%BF2%\Core\BuildConfig.h" /DWIN32 /D_WINDOWS /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE ^
 /wd4100 /wd4127 /wd4244 /wd4267 /wd4291 /wd4305 /wd4996 /Fd"%OUT%\BF2Audio.pdb"

echo   NATIVE=%NATIVE% CRT=%CRT%
echo   CODE=%CODE%
echo   OUT=%OUT%
echo   BIN=%BIN%
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%" exit /b 1
if not exist "%BIN%" mkdir "%BIN%"
if not exist "%BIN%" exit /b 1
del /q "%OUT%\*.obj" >nul 2>&1
del /q "%OUT%\*.log" >nul 2>&1

rem ---------------------------------------------------------------------------
rem STEP 1 - import library for the shipped Memory.dll (bf_malloc/bf_free/addMemoryName by name).
rem Reuses RendDX9Abi\Memory.def - the shipped BF2Audio.dll imports exactly these three.
rem ---------------------------------------------------------------------------
if "%NATIVE%"=="1" (
    echo [1/4] native ABI - no Memory import library
) else (
    echo [1/4] import library for Memory.dll
    lib /nologo /def:"%RD9ABI%\Memory.def" /machine:x86 /out:"%OUT%\Memory.lib" >"%OUT%\lib.log" 2>&1
    if errorlevel 1 (
        echo   FAILED - see "%OUT%\lib.log"
        type "%OUT%\lib.log"
        exit /b 1
    )
)

rem ---------------------------------------------------------------------------
rem STEP 2 - compile. Explicit manifest, numbered objects (flat output dir).
rem
rem   the shared RendDX9Abi kit: BfMemory (operator new/delete -> Memory.dll, engineAlloc/engineFree
rem     for the PCM buffers), Msvcr71Heap; Frontier.cpp compiled separately with this DLL's log
rem     defines. NOT DllMain.cpp: Win32System.cpp IS this image's DllMain (attested).
rem   Sound\NullSound.cpp: the null Sound / SoundTemplate pair getSoundEngine creates.
rem   the fifteen SoundEngine TUs (ten attested by __FILE__, five placed by link order).
rem ---------------------------------------------------------------------------
echo [2/4] compiling
set SOURCES=^
 "%RD9ABI%\BfMemory.cpp" ^
 "%RD9ABI%\Msvcr71Heap.cpp" ^
 "%BF2%\Sound\NullSound.cpp" ^
 "%SND%\Ambient.cpp" ^
 "%SND%\BfOpenAL.cpp" ^
 "%SND%\BfSoundEngine.cpp" ^
 "%SND%\Music.cpp" ^
 "%SND%\OpenALProvider.cpp" ^
 "%SND%\OpenALSample.cpp" ^
 "%SND%\Reverb.cpp" ^
 "%SND%\Sample.cpp" ^
 "%SND%\Sound.cpp" ^
 "%SND%\SoundDefaults.cpp" ^
 "%SND%\SoundEvaluator.cpp" ^
 "%SND%\SoundFile.cpp" ^
 "%SND%\SoundTrigger.cpp" ^
 "%SND%\WavFileFormat.cpp" ^
 "%SND%\Win32System.cpp"

set OK=0
set BAD=0
set /a OBJN=0
for %%F in (%SOURCES%) do (
    set /a OBJN+=1
    cl %FLAGS% /Fo"%OUT%\!OBJN!_%%~nF.obj" "%%F" >"%OUT%\cl_%%~nF.log" 2>&1
    if errorlevel 1 (
        set /a BAD+=1
        echo   COMPILE FAIL %%~nxF
        type "%OUT%\cl_%%~nF.log"
    ) else (
        set /a OK+=1
    )
)

rem The Ogg Vorbis decoder (C, third-party: no image define, no BuildConfig, its own warning set).
if "%VORBIS_ON%"=="1" (
    set CFLAGS=/nologo /c %CRT% /W3 /FS /Zi /DWIN32 /D_WINDOWS /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /D_CRT_NONSTDC_NO_DEPRECATE ^
     /I "%OGG%\include" /I "%VORBIS%\include" /I "%VORBIS%\lib" /wd4018 /wd4101 /wd4146 /wd4244 /wd4267 /wd4305 /wd4700 /wd4996 /Fd"%OUT%\BF2Audio.pdb"
    for %%F in ("%OGG%\src\bitwise.c" "%OGG%\src\framing.c" "%VORBIS%\lib\analysis.c" "%VORBIS%\lib\bitrate.c" "%VORBIS%\lib\block.c" ^
                "%VORBIS%\lib\codebook.c" "%VORBIS%\lib\envelope.c" "%VORBIS%\lib\floor0.c" "%VORBIS%\lib\floor1.c" "%VORBIS%\lib\info.c" ^
                "%VORBIS%\lib\lookup.c" "%VORBIS%\lib\lpc.c" "%VORBIS%\lib\lsp.c" "%VORBIS%\lib\mapping0.c" "%VORBIS%\lib\mdct.c" ^
                "%VORBIS%\lib\psy.c" "%VORBIS%\lib\registry.c" "%VORBIS%\lib\res0.c" "%VORBIS%\lib\sharedbook.c" "%VORBIS%\lib\smallft.c" ^
                "%VORBIS%\lib\synthesis.c" "%VORBIS%\lib\vorbisfile.c" "%VORBIS%\lib\window.c") do (
        set /a OBJN+=1
        cl !CFLAGS! /Fo"%OUT%\!OBJN!_%%~nF.obj" "%%~F" >"%OUT%\cl_%%~nF.log" 2>&1
        if errorlevel 1 (
            set /a BAD+=1
            echo   COMPILE FAIL %%~nxF
            type "%OUT%\cl_%%~nF.log"
        ) else (
            set /a OK+=1
        )
    )
)

rem Frontier.cpp - the shared instrument, compiled here with this DLL's log file, env var and banner.
set /a OBJN+=1
cl %FLAGS% /DRD9ABI_LOG_FILENAME=\"BF2AudioRebuilt.log\" /DRD9ABI_FRONTIER_ENV=\"BF2_BF2AUDIO_FRONTIER\" /DRD9ABI_LOG_BANNER=\"BF2Audio_replacement\" /Fo"%OUT%\!OBJN!_Frontier.obj" "%RD9ABI%\Frontier.cpp" >"%OUT%\cl_Frontier.log" 2>&1
if errorlevel 1 ( set /a BAD+=1 & echo   COMPILE FAIL Frontier.cpp & type "%OUT%\cl_Frontier.log" ) else ( set /a OK+=1 )

echo   %OK% compiled, %BAD% failed
if not "%BAD%"=="0" exit /b 1

rem ---------------------------------------------------------------------------
rem STEP 3 - link the DLL. /DEF gives the two undecorated exports with the shipped ordinals.
rem dsound.lib - DirectSoundEnumerateA, the device probe in Win32System.cpp (the shipped image's
rem   ONLY DSOUND import). OpenAL is never linked: BfOpenAL.cpp loads BF2OpenAL.dll / OpenAL32.dll
rem   by name at provider open, exactly as shipped.
rem ---------------------------------------------------------------------------
echo [3/4] linking
link /nologo /DLL /MACHINE:X86 /BASE:0x10000000 /DEBUG /OPT:REF /OPT:ICF ^
     /DEF:"%ABI%\BF2Audio.def" ^
     /OUT:"%BIN%\BF2Audio.dll" ^
     /PDB:"%BIN%\BF2Audio.pdb" ^
     /IMPLIB:"%OUT%\BF2Audio.lib" ^
     /MAP:"%OUT%\BF2Audio.map" ^
     "%OUT%\*.obj" %MEMORY_LIB% %VORBIS_LIBS% kernel32.lib user32.lib dsound.lib > "%OUT%\link.log" 2>&1
if errorlevel 1 (
    echo   LINK FAILED - see "%OUT%\link.log"
    type "%OUT%\link.log"
    exit /b 1
)

rem ---------------------------------------------------------------------------
rem STEP 4 - the ABI checks that matter, run every build. findstr, NOT find.
rem ---------------------------------------------------------------------------
echo [4/4] ABI check
dumpbin /nologo /exports "%BIN%\BF2Audio.dll" > "%OUT%\exports.txt"
if errorlevel 1 exit /b 1
dumpbin /nologo /dependents "%BIN%\BF2Audio.dll" > "%OUT%\dependents.txt"
if errorlevel 1 exit /b 1
dumpbin /nologo /imports "%BIN%\BF2Audio.dll" > "%OUT%\imports.txt"
if errorlevel 1 exit /b 1

set ABI_BAD=0
for %%E in (getSoundEngine shutdownSoundEngine) do (
    findstr /r /c:"\<%%E\>" "%OUT%\exports.txt" >nul
    if errorlevel 1 (
        echo   exports:      %%E MISSING
        set /a ABI_BAD+=1
    )
)
findstr /i /c:"DSOUND.dll" "%OUT%\dependents.txt" >nul
if errorlevel 1 (
    echo   imports:      DSOUND.dll MISSING ^(the device probe^)
    set /a ABI_BAD+=1
)
findstr /i /c:"OpenAL32.dll" /c:"BF2OpenAL.dll" "%OUT%\dependents.txt" >nul
if not errorlevel 1 (
    echo   imports:      OpenAL linked statically - the shipped image loads it by name
    set /a ABI_BAD+=1
)
if "%NATIVE%"=="1" (
    findstr /i /c:"Memory.dll" /c:"msvcr71.dll" /c:"msvcp71.dll" "%OUT%\dependents.txt" >nul
    if not errorlevel 1 (
        echo   imports:      retail heap/CRT UNEXPECTED in native ABI
        set /a ABI_BAD+=1
    )
    findstr /i /c:"VCRUNTIME140.dll" "%OUT%\dependents.txt" >nul
    if errorlevel 1 (
        echo   imports:      native shared VCRUNTIME140.dll MISSING
        set /a ABI_BAD+=1
    )
) else (
    findstr /i /c:"Memory.dll" "%OUT%\dependents.txt" >nul
    if errorlevel 1 (
        echo   imports:      Memory.dll MISSING in retail ABI
        set /a ABI_BAD+=1
    )
    findstr /l /c:"?bf_malloc@memory@hfe@dice@@YAPAXIH@Z" "%OUT%\imports.txt" >nul
    if errorlevel 1 (
        echo   imports:      bf_malloc mangled name MISSING
        set /a ABI_BAD+=1
    )
    findstr /l /c:"?bf_free@memory@hfe@dice@@YAXPAX@Z" "%OUT%\imports.txt" >nul
    if errorlevel 1 (
        echo   imports:      bf_free mangled name MISSING
        set /a ABI_BAD+=1
    )
    findstr /l /c:"?addMemoryName@memory@hfe@dice@@YAHPBD@Z" "%OUT%\imports.txt" >nul
    if errorlevel 1 (
        echo   imports:      addMemoryName mangled name MISSING
        set /a ABI_BAD+=1
    )
    findstr /i /c:"VCRUNTIME140.dll" /c:"api-ms-win-crt-" /c:"ucrtbase.dll" "%OUT%\dependents.txt" >nul
    if not errorlevel 1 (
        echo   imports:      shared native CRT UNEXPECTED in retail /MT ABI
        set /a ABI_BAD+=1
    )
)
if not "!ABI_BAD!"=="0" (
    echo   ABI CHECK FAILED - !ABI_BAD! invariant^(s^) violated
    exit /b 1
)
echo   exports and heap/CRT imports OK

echo.
echo   built: %BIN%\BF2Audio.dll
echo   NOTE: this is NOT installed anywhere. See docs\building.md before using this DLL with a client.
exit /b 0
