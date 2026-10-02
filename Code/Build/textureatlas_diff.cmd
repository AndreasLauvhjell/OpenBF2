@echo off
setlocal EnableDelayedExpansion
rem ===========================================================================
rem SCAFFOLDING - NOT RECOVERED SOURCE. The TextureAtlasBuilder.dll PARITY HARNESS: builds the
rem scenario host (textureatlas_diff\host.cpp), writes the synthetic inputs (make_inputs.py), runs
rem each scenario against the RETAIL TextureAtlasBuilder.dll (read from the game directory, never
rem copied) and the REBUILT one, and diffs the two runs (diff_outputs.py: host logs, .tai/.tac bytes,
rem DDS headers and the packed texels of every mip level).
rem
rem   usage:  textureatlas_diff.cmd [scenario^|all] [hal^|none] [rebuilt-only]
rem
rem   hal           a Direct3D 9 HAL device on a hidden window - NEEDS A DESKTOP SESSION WITH A GPU.
rem                 Only this mode renders pages (the .dds files) and fills the .tai.
rem   none          ITextureAtlas::create(nullptr): 0 x 0 pages, every texture reported too large;
rem                 the .tac reader/writer, addTexture's image-info read, the warnings and the .tai
rem                 header still run. No GPU needed.
rem   rebuilt-only  run ONLY the rebuilt NATIVE flavour (no Memory.dll; d3dx9_25.dll comes from the
rem                 DirectX runtime in SysWOW64) - nothing from the game directory is loaded.
rem
rem   BF2_GAME_DIR                    the reference game directory (read only).
rem   BF2_TEXTUREATLAS_BIN            the native rebuilt DLL's directory (default bin_textureatlasbuilder_native).
rem   BF2_TEXTUREATLAS_DIFF_RETAIL=1  REQUIRED to run the retail DLL at all. Running the retail image
rem                                   is the OWNER's decision (AGENTS.md): without this variable the
rem                                   script refuses every mode except rebuilt-only.
rem
rem Nothing is written into the game directory. Work dir: Code\Build\obj_textureatlas_diff
rem (inputs\, results\<retail|rebuilt>\<scenario>\, logs\).
rem ===========================================================================
call "%~dp0vsdevcmd.cmd" x86
if errorlevel 1 goto :fail
for %%I in ("%~dp0..") do set CODE=%%~fI
set SRC=%~dp0textureatlas_diff
set OUT=%~dp0obj_textureatlas_diff
if defined BF2_TEXTUREATLAS_DIFF_OUT set "OUT=%BF2_TEXTUREATLAS_DIFF_OUT%"
if not defined BF2_GAME_DIR set "BF2_GAME_DIR=D:\B2BF Launcher\Battlefield2"
if /i "%~1"=="selftest" goto :selftest
set SCENARIO=%~1
if "%SCENARIO%"=="" set SCENARIO=all
set DEVICE=%~2
if "%DEVICE%"=="" set DEVICE=hal
if /i "%DEVICE%"=="hal" goto :device_ok
if /i "%DEVICE%"=="none" goto :device_ok
echo Usage: textureatlas_diff.cmd [scenario^|all] [hal^|none] [rebuilt-only]
exit /b 2
:device_ok
set ONLY_REBUILT=0
if /i "%~3"=="rebuilt-only" set ONLY_REBUILT=1

if "%ONLY_REBUILT%"=="1" goto :mode_ok
if "%BF2_TEXTUREATLAS_DIFF_RETAIL%"=="1" goto :mode_ok
echo   REFUSED: comparing against the retail TextureAtlasBuilder.dll runs a retail binary.
echo   That is the owner's decision - set BF2_TEXTUREATLAS_DIFF_RETAIL=1 once it is given, or run
echo   "textureatlas_diff.cmd %SCENARIO% %DEVICE% rebuilt-only" to exercise the rebuilt DLL alone.
exit /b 2
:mode_ok

rem The original DLL remains an isolated file-format reference, not a supported rebuilt ABI.
set BIN=%~dp0bin_textureatlasbuilder_native
if defined BF2_TEXTUREATLAS_BIN set "BIN=%BF2_TEXTUREATLAS_BIN%"
if not exist "%BIN%\TextureAtlasBuilder.dll" (
    echo   no rebuilt DLL at "%BIN%" - run Code\Build\textureatlasbuilder.cmd first
    goto :fail
)
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%OUT%\logs" mkdir "%OUT%\logs"
if not exist "%OUT%\results" mkdir "%OUT%\results"

echo [1/3] host
cl /nologo /MT /O2 /EHsc /std:c++14 /permissive /W3 /Zi /Fd"%OUT%\host.pdb" /Fo"%OUT%\host.obj" /Fe"%OUT%\host.exe" ^
   /DWIN32 /D_CRT_SECURE_NO_WARNINGS "%SRC%\host.cpp" /link d3d9.lib user32.lib kernel32.lib >"%OUT%\cl_host.log" 2>&1
if errorlevel 1 (
    type "%OUT%\cl_host.log"
    goto :fail
)

echo [2/3] inputs
python "%SRC%\make_inputs.py" "%OUT%\inputs" >"%OUT%\inputs.log" 2>&1
if errorlevel 1 (
    type "%OUT%\inputs.log"
    goto :fail
)

echo [3/3] scenarios (device %DEVICE%)
set LIST=%SCENARIO%
if /i "%SCENARIO%"=="all" (
    set LIST=
    for /f %%S in ('"%OUT%\host.exe" list') do set LIST=!LIST! %%S
)
set VARIANTS=retail rebuilt
if "%ONLY_REBUILT%"=="1" set VARIANTS=rebuilt
set FAILED=0
for %%S in (%LIST%) do (
    for %%V in (%VARIANTS%) do (
        set "DLL=%BIN%\TextureAtlasBuilder.dll"
        set "DIR=%BF2_GAME_DIR%"
        set STRINGS=native
        if "%%V"=="retail" (
            set "DLL=%BF2_GAME_DIR%\TextureAtlasBuilder.dll"
            set STRINGS=vc71
        )
        if "%ONLY_REBUILT%"=="1" set "DIR=-"
        if exist "%OUT%\inputs\out" rmdir /s /q "%OUT%\inputs\out"
        if exist "%OUT%\results\%%V\%%S" rmdir /s /q "%OUT%\results\%%V\%%S"
        if not exist "%OUT%\results\%%V" mkdir "%OUT%\results\%%V"
        "%OUT%\host.exe" "!DLL!" "!DIR!" "%OUT%\inputs" out %%S !STRINGS! %DEVICE% >"%OUT%\logs\%%S_%%V.log" 2>&1
        echo   %%S/%%V exit !ERRORLEVEL!
        if exist "%OUT%\inputs\out" move "%OUT%\inputs\out" "%OUT%\results\%%V\%%S" >nul
    )
    if "%ONLY_REBUILT%"=="0" (
        python "%SRC%\diff_outputs.py" "%OUT%\results\retail\%%S" "%OUT%\results\rebuilt\%%S" "%OUT%\logs\%%S_retail.log" "%OUT%\logs\%%S_rebuilt.log" >"%OUT%\logs\%%S.diff" 2>&1
        if errorlevel 1 set /a FAILED+=1
        type "%OUT%\logs\%%S.diff"
    )
)
if "%ONLY_REBUILT%"=="0" echo   scenarios with differences: %FAILED%
echo   logs: %OUT%\logs   results: %OUT%\results
exit /b 0

rem ---------------------------------------------------------------------------
rem textureatlas_diff.cmd selftest - the device-free self-test (textureatlas_diff\selftest.cpp): the
rem DLL's platform-neutral TUs compiled straight in, the base atlas (2048 x 2048, no rendering) packing
rem table-driven textures for real. Invariants only - not parity. No GPU, no game directory.
rem ---------------------------------------------------------------------------
:selftest
set TAB=%CODE%\BF2\RendDX9\TextureAtlas\TextureAtlasBuilder
if not exist "%OUT%\selftest" mkdir "%OUT%\selftest"
cl /nologo /MT /O2 /EHsc /std:c++14 /permissive /W3 /DBF2_IMAGE_TEXTUREATLASBUILDER /DDEDICATED=0 /FI"%CODE%\BF2\Core\BuildConfig.h" ^
   /DWIN32 /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /Fo"%OUT%\selftest\\" /Fe"%OUT%\selftest\selftest.exe" ^
   "%SRC%\selftest.cpp" "%TAB%\TextureFormat.cpp" "%TAB%\Packer.cpp" "%TAB%\TextureAtlas.cpp" "%TAB%\TextureAtlasConfiguration.cpp" ^
   /link legacy_stdio_float_rounding.obj >"%OUT%\selftest\cl.log" 2>&1
if errorlevel 1 (
    type "%OUT%\selftest\cl.log"
    goto :fail
)
"%OUT%\selftest\selftest.exe" "%OUT%\selftest"
if errorlevel 1 goto :fail
exit /b 0

:fail
echo   textureatlas_diff.cmd FAILED
exit /b 1
