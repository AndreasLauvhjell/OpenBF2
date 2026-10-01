@echo off
setlocal
rem [TEMP] Strict x86 ABI/matrix fixture, no game process or /FORCE.
if not defined BF2_BUILD_OUT exit /b 2
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_SOURCE=%~dp0..\BF2"
cl /nologo /Zs /EHsc /std:c++14 /permissive /DDEDICATED=0 ^
 /FI"%TASK_SOURCE%\Core\BuildConfig.h" "%TASK_SOURCE%\Game\Objects\Game\SoldierAnimation.cpp" ^
 "%TASK_SOURCE%\Game\Objects\GameLogicComp\ArmorGLComp.cpp"
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0extract_soldier_weapon_geometry.ps1" -OutputDirectory "%BF2_BUILD_OUT%"
if errorlevel 1 exit /b 1
cl /nologo /O2 /EHsc /std:c++14 /permissive /MD /Gy /Z7 /DDEDICATED=0 ^
 /FI"%TASK_SOURCE%\Core\BuildConfig.h" /Fo"%BF2_BUILD_OUT%\\" ^
 /Fe"%BF2_BUILD_OUT%\soldier_weapon_geometry_runtime.exe" ^
 "%~dp0soldier_weapon_geometry_runtime.cpp" "%BF2_BUILD_OUT%\Soldier_weapon_geometry_under_test.cpp" ^
 "%TASK_SOURCE%\Math\Mat4.cpp" "%TASK_SOURCE%\Math\Vec3.cpp" ^
 /link /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\soldier_weapon_geometry_runtime.exe"
exit /b %errorlevel%
