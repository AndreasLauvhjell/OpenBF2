@echo off
setlocal
rem [TEMP] Strict x86 link, actual client fullInvert and relocated arithmetic oracle.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_commander_finish_20260906\inverse_o2"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
if not defined BF2_TEST_OPT set "BF2_TEST_OPT=/O2"
cl /nologo %BF2_TEST_OPT% /EHsc /std:c++14 /permissive /MD /DDEDICATED=0 /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /Fo"%BF2_BUILD_OUT%\\" /Fe"%BF2_BUILD_OUT%\mat4_inverse_runtime.exe" "%TASK_ROOT%\Code\BF2\Math\Mat4Inverse.cpp" "%~dp0mat4_inverse_runtime.cpp" /link /SUBSYSTEM:CONSOLE
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\mat4_inverse_runtime.exe"
exit /b %errorlevel%
