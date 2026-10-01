@echo off
setlocal
rem [TEMP] Component-only console checks using the dedicated engine manifest.
if not defined BF2_BUILD_OUT exit /b 2
if not exist "%BF2_BUILD_OUT%\engine_objects.rsp" exit /b 2
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TESTOUT=%BF2_BUILD_OUT%\listener_settings_runtime"
set "PYROOT=%~dp0..\BF2\External\python-embedded\OBJ\python\release"
if not exist "%TESTOUT%" mkdir "%TESTOUT%"
cl /nologo /c /MD /std:c++14 /permissive /EHsc /Z7 /DDEDICATED=1 /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%TESTOUT%\runtime.obj" "%~dp0listener_settings_runtime.cpp" > "%TESTOUT%\compile.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\compile.log" & exit /b 1 )
link /nologo /FORCE /DEBUG /NODEFAULTLIB:python23.lib /subsystem:console /OUT:"%TESTOUT%\runtime.exe" /PDB:"%TESTOUT%\runtime.pdb" /IMPLIB:"%TESTOUT%\runtime.lib" "%TESTOUT%\runtime.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" "%PYROOT%\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib > "%TESTOUT%\link.log" 2>&1
if errorlevel 2 exit /b %errorlevel%
copy /y "%PYROOT%\dice_py.dll" "%TESTOUT%\dice_py.dll" >nul
"%TESTOUT%\runtime.exe"
exit /b %errorlevel%
