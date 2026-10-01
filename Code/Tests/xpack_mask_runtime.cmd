@echo off
setlocal
rem [TEMP] SCAFFOLDING: requires a completed isolated engine build, no server/client is launched.
if not defined BF2_BUILD_OUT (
    echo Set BF2_BUILD_OUT to an isolated completed build.cmd directory first.
    exit /b 2
)
if not exist "%BF2_BUILD_OUT%\engine_objects.rsp" exit /b 2
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set TESTOUT=%BF2_BUILD_OUT%\xpack_mask_runtime
set PYROOT=%~dp0..\BF2\External\python-embedded\OBJ\python\release
if not exist "%TESTOUT%" mkdir "%TESTOUT%"
cl /nologo /c /MD /std:c++14 /permissive /EHsc /Zi /DDEDICATED=1 ^
   /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%TESTOUT%\xpack_mask_runtime.obj" ^
   /Fd"%TESTOUT%\compile.pdb" "%~dp0xpack_mask_runtime.cpp" > "%TESTOUT%\compile.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\compile.log" & exit /b 1 )
link /nologo /FORCE /DEBUG /NODEFAULTLIB:python23.lib /subsystem:console ^
     /OUT:"%TESTOUT%\xpack_mask_runtime.exe" /PDB:"%TESTOUT%\runtime.pdb" ^
     /MAP:"%TESTOUT%\runtime.map" /IMPLIB:"%TESTOUT%\runtime.lib" ^
     "%TESTOUT%\xpack_mask_runtime.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" ^
     "%PYROOT%\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib dbghelp.lib ^
     > "%TESTOUT%\link.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\link.log" & exit /b 1 )
if not exist "%TESTOUT%\xpack_mask_runtime.exe" ( type "%TESTOUT%\link.log" & exit /b 1 )
copy /y "%PYROOT%\dice_py.dll" "%TESTOUT%\dice_py.dll" >nul
echo Diagnostic /FORCE fixture: masks only; no production boot, listener, map or client test.
"%TESTOUT%\xpack_mask_runtime.exe"
exit /b %errorlevel%
