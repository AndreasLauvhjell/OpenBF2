@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem SCAFFOLDING - NOT RECOVERED SOURCE. The BF2Audio.dll DIFFERENTIAL TEST:
rem builds the OpenAL logging shim and the scenario host, then runs each
rem scenario against the RETAIL BF2Audio.dll (read from the game directory)
rem and the REBUILT one, and diffs the two AL call logs and the two host logs.
rem
rem   usage:  bf2audio_diff.cmd [scenario|all] [provider] [quality] [voices]
rem
rem   BF2_GAME_DIR       the reference game directory (read only).
rem   BF2_BF2AUDIO_BIN   where bf2audio.cmd put the rebuilt BF2Audio.dll.
rem
rem Nothing is written into the game directory; the retail wrap_oal is copied
rem beside the shim as wrap_oal_real.dll, and the retail engine DLL is loaded
rem by full path from where it is.
rem ===========================================================================
call "%~dp0vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
for %%I in ("%~dp0..") do set CODE=%%~fI
set BF2=%CODE%\BF2
set SRC=%~dp0bf2audio_diff
set OUT=%~dp0obj_bf2audio_diff
set BIN=%~dp0bin_bf2audio
if defined BF2_BF2AUDIO_BIN set "BIN=%BF2_BF2AUDIO_BIN%"
if not defined BF2_GAME_DIR set "BF2_GAME_DIR=D:\B2BF Launcher\Battlefield2"
set SCENARIO=%~1
if "%SCENARIO%"=="" set SCENARIO=all
set PROVIDER=%~2
if "%PROVIDER%"=="" set PROVIDER=Software Audio
set QUALITY=%~3
if "%QUALITY%"=="" set QUALITY=0
set VOICES=%~4
if "%VOICES%"=="" set VOICES=16
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%\shim" mkdir "%OUT%\shim"
if not exist "%OUT%\logs" mkdir "%OUT%\logs"
if not exist "%OUT%\signals" mkdir "%OUT%\signals"

echo [1/4] signals
python "%SRC%\make_signals.py" "%OUT%\signals" >"%OUT%\signals.log" 2>&1
if errorlevel 1 ( type "%OUT%\signals.log" & exit /b 1 )
if exist "%SRC%\signals_extra\*" copy /y "%SRC%\signals_extra\*" "%OUT%\signals\" >nul

echo [2/4] shim
if not exist "%OUT%\shim\wrap_oal_real.dll" copy /y "%BF2_GAME_DIR%\BF2OpenAL.dll" "%OUT%\shim\wrap_oal_real.dll" >nul
python "%SRC%\gen_alshim.py" "%SRC%\wrap_oal_exports.txt" "%OUT%\shim" >"%OUT%\gen.log" 2>&1
if errorlevel 1 ( type "%OUT%\gen.log" & exit /b 1 )
cl /nologo /LD /MT /EHsc /std:c++14 /permissive /W3 /Zi /I "%SRC%" /Fd"%OUT%\shim\alshim.pdb" /Fo"%OUT%\shim\\" ^
   /DWIN32 /D_CRT_SECURE_NO_WARNINGS "%SRC%\alshim.cpp" "%OUT%\shim\alshim_tramp.cpp" ^
   /link /DEF:"%OUT%\shim\alshim.def" /OUT:"%OUT%\shim\BF2OpenAL.dll" /IMPLIB:"%OUT%\shim\alshim.lib" kernel32.lib >"%OUT%\cl_shim.log" 2>&1
if errorlevel 1 ( echo   SHIM BUILD FAILED & type "%OUT%\cl_shim.log" & exit /b 1 )

echo [3/4] host
cl /nologo /MT /EHsc /std:c++14 /permissive /W3 /Zi /Fd"%OUT%\host.pdb" /Fo"%OUT%\host.obj" /Fe"%OUT%\host.exe" ^
   /DBF2_IMAGE_BF2AUDIO /DBF2_PLUGIN_ABI_NATIVE=0 /DDEDICATED=0 /FI"%BF2%\Core\BuildConfig.h" /DWIN32 /D_CRT_SECURE_NO_WARNINGS ^
   "%SRC%\host.cpp" /link kernel32.lib >"%OUT%\cl_host.log" 2>&1
if errorlevel 1 ( echo   HOST BUILD FAILED & type "%OUT%\cl_host.log" & exit /b 1 )

echo [4/4] scenarios (provider "%PROVIDER%", quality %QUALITY%, voices %VOICES%)
set LIST=%SCENARIO%
if /i "%SCENARIO%"=="all" (
    set LIST=
    for /f %%S in ('"%OUT%\host.exe" x x x x list') do set LIST=!LIST! %%S
)
set FAILED=0
for %%S in (%LIST%) do (
    for %%V in (retail rebuilt) do (
        set DLL=%BF2_GAME_DIR%\BF2Audio.dll
        if "%%V"=="rebuilt" set DLL=%BIN%\BF2Audio.dll
        set BF2_ALSHIM_LOG=%OUT%\logs\%%S_%%V.al.log
        "%OUT%\host.exe" "!DLL!" "%BF2_GAME_DIR%" "%OUT%\shim\BF2OpenAL.dll" "%OUT%\signals" %%S "%PROVIDER%" %QUALITY% %VOICES% >"%OUT%\logs\%%S_%%V.host.log" 2>&1
        echo   %%S/%%V exit !ERRORLEVEL!
    )
    set "EXTRA="
    if "%%S"=="ogg" set "EXTRA=nofnv"
    python "%SRC%\diff_al.py" "%OUT%\logs\%%S_retail.al.log" "%OUT%\logs\%%S_rebuilt.al.log" 12 4 !EXTRA! >"%OUT%\logs\%%S.al.diff" 2>&1
    set /a "AL_RC=!ERRORLEVEL!"
    python "%SRC%\diff_al.py" "%OUT%\logs\%%S_retail.host.log" "%OUT%\logs\%%S_rebuilt.host.log" 12 >"%OUT%\logs\%%S.host.diff" 2>&1
    set /a "HOST_RC=!ERRORLEVEL!"
    set "AL_FIRST=" & set "HOST_FIRST="
    set /p "AL_FIRST="<"%OUT%\logs\%%S.al.diff"
    set /p "HOST_FIRST="<"%OUT%\logs\%%S.host.diff"
    echo     AL   !AL_FIRST!
    echo     HOST !HOST_FIRST!
    if not "!AL_RC!!HOST_RC!"=="00" set /a FAILED+=1
)
echo   scenarios with differences: %FAILED%
echo   logs: %OUT%\logs
exit /b 0
