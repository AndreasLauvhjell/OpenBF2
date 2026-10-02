@echo off
setlocal
rem [TEMP] SCAFFOLDING: a completed isolated current engine manifest is required.
if not defined BF2_BUILD_OUT exit /b 2
if not exist "%BF2_BUILD_OUT%\engine_objects.rsp" exit /b 2
call "%~dp0..\Build\vsdevcmd.cmd" x86
set TESTOUT=%BF2_BUILD_OUT%\triggerable_runtime
set PYROOT=%~dp0..\BF2\External\python-embedded\OBJ\python\release
if not exist "%TESTOUT%" mkdir "%TESTOUT%"
cl /nologo /c /MD /std:c++14 /permissive /EHsc /Z7 /DDEDICATED=1 ^
   /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%TESTOUT%\triggerable_runtime.obj" ^
   "%~dp0triggerable_runtime.cpp" > "%TESTOUT%\compile.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\compile.log" & exit /b 1 )
link /nologo /FORCE /DEBUG /NODEFAULTLIB:python23.lib /subsystem:console ^
     /OUT:"%TESTOUT%\triggerable_runtime.exe" /PDB:"%TESTOUT%\runtime.pdb" ^
     /MAP:"%TESTOUT%\runtime.map" /IMPLIB:"%TESTOUT%\runtime.lib" ^
     "%TESTOUT%\triggerable_runtime.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" ^
     "%PYROOT%\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib dbghelp.lib ^
     > "%TESTOUT%\link.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\link.log" & exit /b 1 )
if not exist "%TESTOUT%\triggerable_runtime.exe" exit /b 1
copy /y "%PYROOT%\dice_py.dll" "%TESTOUT%\dice_py.dll" >nul
echo Diagnostic /FORCE fixture: trigger contracts only; no server or game client.
"%TESTOUT%\triggerable_runtime.exe"
exit /b %errorlevel%
