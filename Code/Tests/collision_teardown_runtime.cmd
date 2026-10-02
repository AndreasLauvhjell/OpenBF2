@echo off
setlocal
rem [TEMP] Component-only checks against a completed isolated dedicated build.
if not defined BF2_BUILD_OUT (
  echo Set BF2_BUILD_OUT to a completed isolated dedicated build directory.
  exit /b 2
)
if not exist "%BF2_BUILD_OUT%\count_factory_free.txt" (
  echo Missing completed dedicated build: "%BF2_BUILD_OUT%\count_factory_free.txt"
  exit /b 2
)
if not defined BF2_COLLISION_MESH (
  echo Set BF2_COLLISION_MESH to the stock flagpole.collisionmesh fixture.
  exit /b 2
)
if not exist "%BF2_COLLISION_MESH%" (
  echo Missing collision mesh fixture: "%BF2_COLLISION_MESH%"
  exit /b 2
)
call "%~dp0..\Build\vsdevcmd.cmd" x86
set TESTOUT=%BF2_BUILD_OUT%\collision_teardown_runtime
set PYROOT=%~dp0..\BF2\External\python-embedded\OBJ\python\release
if not exist "%TESTOUT%" mkdir "%TESTOUT%"
cl /nologo /c /MD /std:c++14 /permissive /EHsc /Z7 /DDEDICATED=1 ^
 /FI"%~dp0..\BF2\Core\BuildConfig.h" /Fo"%TESTOUT%\runtime.obj" "%~dp0collision_teardown_runtime.cpp" ^
 > "%TESTOUT%\compile.log" 2>&1
if errorlevel 1 ( type "%TESTOUT%\compile.log" & exit /b 1 )
if exist "%TESTOUT%\runtime.exe" del /q "%TESTOUT%\runtime.exe"
link /nologo /FORCE /DEBUG /NODEFAULTLIB:python23.lib /subsystem:console ^
 /OUT:"%TESTOUT%\runtime.exe" /PDB:"%TESTOUT%\runtime.pdb" /MAP:"%TESTOUT%\runtime.map" ^
 /IMPLIB:"%TESTOUT%\runtime.lib" "%TESTOUT%\runtime.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" ^
 "%PYROOT%\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib > "%TESTOUT%\link.log" 2>&1
if not exist "%TESTOUT%\runtime.exe" ( type "%TESTOUT%\link.log" & exit /b 1 )
copy /y "%PYROOT%\dice_py.dll" "%TESTOUT%\dice_py.dll" >nul
echo Diagnostic /FORCE fixture; only the named component cases are attested.
"%TESTOUT%\runtime.exe" "%BF2_COLLISION_MESH%"
exit /b %errorlevel%

