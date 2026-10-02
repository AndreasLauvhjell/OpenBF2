@echo off
setlocal
rem [TEMP] Strict x86 actual-body regression; no /FORCE and no game process.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "ROOT=%~dp0..\BF2"
set "OUT=%~dp0..\Build\animation_weapon_20260923"
if not exist "%OUT%" mkdir "%OUT%"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0extract_weapon_animation.ps1" -OutputDirectory "%OUT%"
if errorlevel 1 exit /b 1
set "FLAGS=/nologo /c /MD /std:c++14 /permissive /EHsc /O2 /Gy /Zc:inline /D_ITERATOR_DEBUG_LEVEL=0 /DDEDICATED=0 /FI%ROOT%\Core\BuildConfig.h"
cl %FLAGS% "%~dp0weapon_animation_runtime.cpp" "%OUT%\GenericFireArm_animation_under_test.cpp" "%ROOT%\Game\Objects\Weapons\GenericFireArm.cpp" "%ROOT%\Game\Objects\Weapons\AnimationComponents\AnimationComp.cpp" "%ROOT%\Game\Objects\Weapons\AnimationComponents\DefaultAnimationComp.cpp" "%ROOT%\Game\Objects\Weapons\AnimationComponents\ThrownAnimationComp.cpp" "%ROOT%\Game\Objects\WeaponComp.cpp" "%ROOT%\Game\Objects\IObjectComponent.cpp" "%ROOT%\Math\Mat4.cpp" "%ROOT%\Math\Vec3.cpp" /Fo"%OUT%\\" >"%OUT%\compile.log" 2>&1
if errorlevel 1 (type "%OUT%\compile.log" & exit /b 1)
link /nologo /OPT:REF /SUBSYSTEM:CONSOLE /OUT:"%OUT%\weapon_animation_runtime.exe" "%OUT%\weapon_animation_runtime.obj" "%OUT%\GenericFireArm_animation_under_test.obj" "%OUT%\AnimationComp.obj" "%OUT%\DefaultAnimationComp.obj" "%OUT%\ThrownAnimationComp.obj" "%OUT%\WeaponComp.obj" "%OUT%\IObjectComponent.obj" "%OUT%\Mat4.obj" "%OUT%\Vec3.obj" >"%OUT%\link.log" 2>&1
if errorlevel 1 (type "%OUT%\link.log" & exit /b 1)
"%OUT%\weapon_animation_runtime.exe" >"%OUT%\runtime.log" 2>&1
set "RESULT=%errorlevel%"
type "%OUT%\runtime.log"
exit /b %RESULT%
