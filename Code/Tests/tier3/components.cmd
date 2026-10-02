@echo off
setlocal
rem Strict x86 component fixture. The output path must belong to this test run.
if "%~1"=="" exit /b 2
set "OUT=%~f1"
if not exist "%OUT%" mkdir "%OUT%"
call "%~dp0..\..\Build\vsdevcmd.cmd" x86
set "ROOT=%~dp0..\..\BF2"
set FLAGS=/nologo /c /MD /Z7 /std:c++14 /permissive /EHsc /DDEDICATED=1 /FI"%ROOT%\Core\BuildConfig.h"
cl %FLAGS% /Fo"%OUT%\components.obj" "%~dp0components.cpp"
if errorlevel 1 exit /b 1
cl %FLAGS% /Fo"%OUT%\ik.obj" "%ROOT%\Game\Objects\IkTemplateComponent.cpp"
if errorlevel 1 exit /b 1
cl %FLAGS% /Fo"%OUT%\strategic.obj" "%ROOT%\Game\Objects\StrategicObjectComp.cpp"
if errorlevel 1 exit /b 1
cl %FLAGS% /Fo"%OUT%\strategicTemplate.obj" "%ROOT%\Game\Objects\StrategicObjectCompTemplate.cpp"
if errorlevel 1 exit /b 1
cl %FLAGS% /Fo"%OUT%\vec3.obj" "%ROOT%\Math\Vec3.cpp"
if errorlevel 1 exit /b 1
cl %FLAGS% /Fo"%OUT%\mat4.obj" "%ROOT%\Math\Mat4.cpp"
if errorlevel 1 exit /b 1
cl %FLAGS% /Fo"%OUT%\componentBase.obj" "%ROOT%\Game\Objects\IObjectComponent.cpp"
if errorlevel 1 exit /b 1
link /nologo /DEBUG /INCREMENTAL:NO /OUT:"%OUT%\components.exe" /PDB:"%OUT%\components.pdb" "%OUT%\components.obj" "%OUT%\ik.obj" "%OUT%\strategic.obj" "%OUT%\strategicTemplate.obj" "%OUT%\vec3.obj" "%OUT%\mat4.obj" "%OUT%\componentBase.obj"
if errorlevel 1 exit /b 1
"%OUT%\components.exe"
exit /b %errorlevel%
