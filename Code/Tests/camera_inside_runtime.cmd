@echo off
setlocal EnableDelayedExpansion
rem [TEMP] Diagnostic /FORCE engine fixture: freshly compiled client Camera only.
if not defined BF2_ENGINE_BUILD set "BF2_ENGINE_BUILD=%~dp0..\Build\obj_skinnedmesh_gate_20260922"
if not exist "%BF2_ENGINE_BUILD%\engine_objects.rsp" exit /b 2
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\camera_inside_weapon_agent_20260923"
for %%D in ("%BF2_ENGINE_BUILD%") do set "ENGINE_ABS=%%~fD"
for %%D in ("%BF2_BUILD_OUT%") do set "OUT_ABS=%%~fD"
if /i "!ENGINE_ABS!"=="!OUT_ABS!" exit /b 2
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc /Z7 /Gy /DDEDICATED=0 /FI%ROOT%\Core\BuildConfig.h"
cl %FLAGS% "%ROOT%\Game\Objects\Base\Camera.cpp" /Fo"%BF2_BUILD_OUT%\Camera.obj" >"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
cl %FLAGS% "%~dp0camera_inside_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
set "CAMERA_MATCHES=0"
for /f "delims=" %%L in ('findstr /c:"_Camera.obj" "%BF2_ENGINE_BUILD%\engine_objects.rsp"') do set /a CAMERA_MATCHES+=1 >nul
if not "!CAMERA_MATCHES!"=="1" (echo Expected exactly one Camera object in engine manifest & exit /b 2)
findstr /v /c:"_Camera.obj" "%BF2_ENGINE_BUILD%\engine_objects.rsp" >"%BF2_BUILD_OUT%\engine_objects.rsp"
echo "%BF2_BUILD_OUT%\Camera.obj">>"%BF2_BUILD_OUT%\engine_objects.rsp"
set "PYROOT=%ROOT%\External\python-embedded\OBJ\python\release"
link /nologo /FORCE /DEBUG /NODEFAULTLIB:python23.lib /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\camera_inside_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" /IMPLIB:"%BF2_BUILD_OUT%\fixture.lib" "%BF2_BUILD_OUT%\fixture.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" "%PYROOT%\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib dbghelp.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
copy /y "%PYROOT%\dice_py.dll" "%BF2_BUILD_OUT%\dice_py.dll" >nul
"%BF2_BUILD_OUT%\camera_inside_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %RESULT%
