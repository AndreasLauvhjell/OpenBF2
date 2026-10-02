@echo off
setlocal
rem [TEMP] Isolated x86 client compile for the final filter family.
call "%~dp0..\Build\vsdevcmd.cmd" x86
set "TASK_ROOT=%~dp0..\BF2"
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\hud_game_filter_final_20260906\compile"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
for %%F in (Menu\BF2HudManagerGameFilter Menu\HudAmmo Menu\HudSquad Menu\HudQuestionManager Menu\HudInformationAdapter) do (
  cl /nologo /O2 /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /FI"%TASK_ROOT%\Core\BuildConfig.h" /c "%TASK_ROOT%\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj" >>"%BF2_BUILD_OUT%\compile.log" 2>&1
  if errorlevel 1 (type "%BF2_BUILD_OUT%\compile.log" & exit /b 1)
)
exit /b 0
