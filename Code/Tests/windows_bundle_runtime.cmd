@echo off
setlocal
rem [TEMP] SCAFFOLDING. Uses a completed isolated all-TU build, never a production target.
if not defined BF2_BUILD_OUT (
    echo Set BF2_BUILD_OUT to an isolated completed build.cmd directory first.
    exit /b 2
)
if not exist "%BF2_BUILD_OUT%\engine_objects.rsp" exit /b 2
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set TESTOUT=%BF2_BUILD_OUT%\windows_bundle_runtime
set PYROOT=%~dp0..\BF2\External\python-embedded\OBJ\python\release
if not exist "%TESTOUT%" mkdir "%TESTOUT%"
cl /nologo /c /MD /std:c++14 /permissive /EHsc /Zi /DDEDICATED=1 ^
   /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%TESTOUT%\windows_bundle_runtime.obj" ^
   /Fd"%TESTOUT%\compile.pdb" "%~dp0windows_bundle_runtime.cpp" > "%TESTOUT%\compile.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\compile.log" & exit /b 1 )
if exist "%TESTOUT%\windows_bundle_runtime.exe" del /q "%TESTOUT%\windows_bundle_runtime.exe"
link /nologo /FORCE /DEBUG /NODEFAULTLIB:python23.lib /subsystem:console ^
     /OUT:"%TESTOUT%\windows_bundle_runtime.exe" /PDB:"%TESTOUT%\runtime.pdb" ^
     /MAP:"%TESTOUT%\runtime.map" /IMPLIB:"%TESTOUT%\runtime.lib" ^
     "%TESTOUT%\windows_bundle_runtime.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" ^
     "%PYROOT%\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib dbghelp.lib ^
     > "%TESTOUT%\link.log" 2>&1
if not exist "%TESTOUT%\windows_bundle_runtime.exe" ( type "%TESTOUT%\link.log" & exit /b 1 )
copy /y "%PYROOT%\dice_py.dll" "%TESTOUT%\dice_py.dll" >nul
echo Diagnostic /FORCE fixture: window bundle only, no production boot or client test.
"%TESTOUT%\windows_bundle_runtime.exe"
if errorlevel 1 exit /b 1
"%TESTOUT%\windows_bundle_runtime.exe" --truncated > "%TESTOUT%\truncated.log" 2>&1
if not "%errorlevel%"=="86" ( type "%TESTOUT%\truncated.log" & exit /b 1 )
echo PASS truncated update took the expected fatal diagnostic branch
exit /b 0
