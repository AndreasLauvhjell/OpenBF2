@echo off
setlocal
rem [TEMP] SCAFFOLDING. Uses a caller-owned, completed build.cmd all-TU census.
rem The engine manifest is force-linked because unrelated engine methods remain missing.
rem This proves only the named runtime cases, not strict-link or production-boot completeness.
if not defined BF2_BUILD_OUT (
    echo Set BF2_BUILD_OUT to an isolated completed build.cmd directory first.
    exit /b 2
)
if not exist "%BF2_BUILD_OUT%\engine_objects.rsp" exit /b 2
call "%~dp0..\Build\vsdevcmd.cmd" x86
set TESTOUT=%BF2_BUILD_OUT%\netserver_runtime
set PYROOT=%~dp0..\BF2\External\python-embedded\OBJ\python\release
if not exist "%TESTOUT%" mkdir "%TESTOUT%"
cl /nologo /c /MD /std:c++14 /permissive /EHsc /Zi /DDEDICATED=1 ^
   /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%TESTOUT%\netserver_runtime.obj" ^
   /Fd"%TESTOUT%\compile.pdb" "%~dp0netserver_runtime.cpp" > "%TESTOUT%\compile.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\compile.log" & exit /b 1 )
if exist "%TESTOUT%\netserver_runtime.exe" del /q "%TESTOUT%\netserver_runtime.exe"
link /nologo /FORCE /DEBUG /NODEFAULTLIB:python23.lib /subsystem:console ^
     /OUT:"%TESTOUT%\netserver_runtime.exe" /PDB:"%TESTOUT%\netserver_runtime.pdb" ^
     /MAP:"%TESTOUT%\netserver_runtime.map" /IMPLIB:"%TESTOUT%\netserver_runtime.lib" ^
     "%TESTOUT%\netserver_runtime.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" ^
     "%PYROOT%\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib ^
     > "%TESTOUT%\link.log" 2>&1
if not exist "%TESTOUT%\netserver_runtime.exe" ( type "%TESTOUT%\link.log" & exit /b 1 )
copy /y "%PYROOT%\dice_py.dll" "%TESTOUT%\dice_py.dll" >nul
echo Diagnostic /FORCE runtime gate; only the exercised paths are attested.
"%TESTOUT%\netserver_runtime.exe"
exit /b %errorlevel%
