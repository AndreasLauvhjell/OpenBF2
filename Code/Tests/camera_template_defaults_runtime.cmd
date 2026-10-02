@echo off
setlocal EnableDelayedExpansion
rem [TEMP] Reuses a completed x86 dedicated object set but replaces CameraTemplate
rem with a freshly compiled source under test. No game, server or renderer runs.
rem Wider uncalled engine frontiers remain unresolved under diagnostic /FORCE.
if not defined BF2_ENGINE_BUILD exit /b 2
if not exist "%BF2_ENGINE_BUILD%\engine_objects.rsp" exit /b 2
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%TASK_ROOT%\Build\geom_fault_20260922\camera_tests"
for %%D in ("%BF2_ENGINE_BUILD%") do set "TASK_ENGINE_ABS=%%~fD"
for %%D in ("%BF2_BUILD_OUT%") do set "TASK_OUT_ABS=%%~fD"
if /i "!TASK_ENGINE_ABS!"=="!TASK_OUT_ABS!" (echo Fixture output must differ from the existing engine build & exit /b 2)
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_CAMERA_TEMPLATE_SOURCE set "BF2_CAMERA_TEMPLATE_SOURCE=%TASK_ROOT%\BF2\Game\Objects\CameraTemplate.cpp"
set "TASK_FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc /Z7 /DDEDICATED=1"
cl %TASK_FLAGS% /FI"%TASK_ROOT%\BF2\Core\BuildConfig.h" /I"%TASK_ROOT%\BF2\Game\Objects" "%BF2_CAMERA_TEMPLATE_SOURCE%" /Fo"%BF2_BUILD_OUT%\CameraTemplate.obj" >"%BF2_BUILD_OUT%\camera_compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\camera_compile.log" & exit /b 1)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\BF2\Core\BuildConfig.h" "%~dp0camera_template_defaults_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj" >"%BF2_BUILD_OUT%\fixture_compile.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\fixture_compile.log" & exit /b 1)
set "TASK_MATCHES=0"
for /f "delims=" %%L in ('findstr /c:"_CameraTemplate.obj" "%BF2_ENGINE_BUILD%\engine_objects.rsp"') do set /a TASK_MATCHES+=1 >nul
if not "!TASK_MATCHES!"=="1" (echo Expected exactly one CameraTemplate object in engine_objects.rsp & exit /b 2)
findstr /v /c:"_CameraTemplate.obj" "%BF2_ENGINE_BUILD%\engine_objects.rsp" >"%BF2_BUILD_OUT%\engine_objects.rsp"
echo "%BF2_BUILD_OUT%\CameraTemplate.obj">>"%BF2_BUILD_OUT%\engine_objects.rsp"
set "TASK_PYROOT=%TASK_ROOT%\BF2\External\python-embedded\OBJ\python\release"
link /nologo /FORCE /DEBUG /NODEFAULTLIB:python23.lib /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\camera_template_defaults_runtime.exe" /PDB:"%BF2_BUILD_OUT%\fixture.pdb" /MAP:"%BF2_BUILD_OUT%\fixture.map" /IMPLIB:"%BF2_BUILD_OUT%\fixture.lib" "%BF2_BUILD_OUT%\fixture.obj" @"%BF2_BUILD_OUT%\engine_objects.rsp" "%TASK_PYROOT%\dice_py.lib" advapi32.lib user32.lib shell32.lib Ws2_32.lib dbghelp.lib >"%BF2_BUILD_OUT%\link.log" 2>&1
if errorlevel 1 (type "%BF2_BUILD_OUT%\link.log" & exit /b 1)
copy /y "%TASK_PYROOT%\dice_py.dll" "%BF2_BUILD_OUT%\dice_py.dll" >nul
"%BF2_BUILD_OUT%\camera_template_defaults_runtime.exe" >"%BF2_BUILD_OUT%\runtime.log" 2>&1
set "TASK_RESULT=%errorlevel%"
type "%BF2_BUILD_OUT%\runtime.log"
exit /b %TASK_RESULT%
