@echo off
setlocal
rem [TEMP] Actual x86 factory and DefaultSoundComp regression. No /FORCE or game launch.
rem Usage: weapon_sound_runtime.cmd isolated-output-dir [client^|dedicated]
rem BF2_TEST_FACTORY_SOURCE may select a pre-fix SimpleObjectTemplate.cpp snapshot.
if "%~1"=="" exit /b 2
set "IMAGE=0"
if /I "%~2"=="dedicated" set "IMAGE=1"
if not "%~2"=="" if /I not "%~2"=="client" if /I not "%~2"=="dedicated" exit /b 2
call "%~dp0..\Build\vsdevcmd.cmd" x86
if errorlevel 1 exit /b 1
set "ROOT=%~dp0..\BF2"
set "OUT=%~1"
if not exist "%OUT%" mkdir "%OUT%"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0extract_weapon_sound.ps1" -OutputDirectory "%OUT%"
if errorlevel 1 exit /b 1
set "FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc /O2 /Gy /Gw /Zc:inline /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=%IMAGE% /DBF2_PLUGIN_ABI_NATIVE=1 /D_CRT_SECURE_NO_WARNINGS"
cl %FLAGS% /FI"%ROOT%\Core\BuildConfig.h" /I"%ROOT%\Game\Objects\Base" "%~dp0weapon_sound_runtime.cpp" "%ROOT%\Game\Objects\Weapons\SoundComponents\DefaultSoundComp.cpp" "%OUT%\weapon_sound_under_test.cpp" "%ROOT%\Game\Objects\WeaponComp.cpp" "%ROOT%\Game\Objects\IObjectComponent.cpp" "%ROOT%\Sound\NullSound.cpp" "%ROOT%\Sound\ObjectSoundNode.cpp" "%ROOT%\Math\Mat4.cpp" "%ROOT%\Math\Vec3.cpp" /Fo"%OUT%\\" >"%OUT%\compile.log" 2>&1
if errorlevel 1 (type "%OUT%\compile.log" & exit /b 1)
link /nologo /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE /OUT:"%OUT%\weapon_sound_runtime.exe" "%OUT%\weapon_sound_runtime.obj" "%OUT%\DefaultSoundComp.obj" "%OUT%\weapon_sound_under_test.obj" "%OUT%\WeaponComp.obj" "%OUT%\IObjectComponent.obj" "%OUT%\NullSound.obj" "%OUT%\ObjectSoundNode.obj" "%OUT%\Mat4.obj" "%OUT%\Vec3.obj" >"%OUT%\link.log" 2>&1
if errorlevel 1 (type "%OUT%\link.log" & exit /b 1)
"%OUT%\weapon_sound_runtime.exe" >"%OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%OUT%\runtime.log"
exit /b %RESULT%
