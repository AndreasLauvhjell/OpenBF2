@echo off
setlocal
rem [TEMP] Strict x86 client ABI fixture. No /FORCE, renderer, map or network startup.
rem Extracts the unchanged production method/helper prefix so unrelated constructors/vtables
rem do not require a full engine link. The complete production TU is syntax-checked separately.
if not defined BF2_BUILD_OUT (
    echo Set BF2_BUILD_OUT to an agent-owned fixture output directory.
    exit /b 2
)
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
set "TASK_SOURCE=%~dp0..\BF2"
cl /nologo /Zs /EHsc /std:c++14 /permissive /DDEDICATED=0 ^
   /FI"%TASK_SOURCE%\Core\BuildConfig.h" "%TASK_SOURCE%\Game\Objects\Game\AnimatedBundle.cpp"
if errorlevel 1 exit /b 1
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0extract_animated_bundle_pose.ps1" -OutputDirectory "%BF2_BUILD_OUT%"
if errorlevel 1 exit /b 1
cl /nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /Gy /Z7 /DDEDICATED=0 ^
   /FI"%TASK_SOURCE%\Core\BuildConfig.h" /Fo"%BF2_BUILD_OUT%\\" ^
   /Fe"%BF2_BUILD_OUT%\animated_bundle_pose_runtime.exe" ^
   "%~dp0animated_bundle_pose_runtime.cpp" "%BF2_BUILD_OUT%\AnimatedBundle_pose_under_test.cpp" ^
   /link /OPT:REF /INCREMENTAL:NO /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\animated_bundle_pose_runtime.exe"
exit /b %errorlevel%
