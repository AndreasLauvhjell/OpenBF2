@echo off
setlocal
rem [TEMP] Actual radio family; named uncalled external frontiers require /FORCE.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\.."
if not defined BF2_BUILD_OUT set "BF2_BUILD_OUT=%~dp0..\Build\obj_hud_radio_20260906"
if not exist "%BF2_BUILD_OUT%" mkdir "%BF2_BUILD_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 %HUD_RADIO_EXTRA_FLAGS%"
for %%F in (Menu\HudRadio Menu\HudInformationAdapter Menu\BF2HudManager Menu\BF2HudManagerConfiguration Game\HudInformationLayer Menu\HudBlinker Core\EventManager Core\StringUtil Core\Debug IO\Localization\ExcelLexicon IO\Console\Console) do (
  cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%TASK_ROOT%\Code\BF2\%%F.cpp" /Fo"%BF2_BUILD_OUT%\%%~nF.obj"
  if errorlevel 1 exit /b 1
)
cl %TASK_FLAGS% /FI"%TASK_ROOT%\Code\BF2\Core\BuildConfig.h" /c "%~dp0hud_radio_runtime.cpp" /Fo"%BF2_BUILD_OUT%\fixture.obj"
if errorlevel 1 exit /b 1
link /nologo /FORCE /OPT:REF /INCREMENTAL:NO /DEBUG /SUBSYSTEM:CONSOLE /OUT:"%BF2_BUILD_OUT%\hud_radio_runtime.exe" /PDB:"%BF2_BUILD_OUT%\hud_radio_runtime.pdb" "%BF2_BUILD_OUT%\HudRadio.obj" "%BF2_BUILD_OUT%\HudInformationAdapter.obj" "%BF2_BUILD_OUT%\BF2HudManager.obj" "%BF2_BUILD_OUT%\BF2HudManagerConfiguration.obj" "%BF2_BUILD_OUT%\HudInformationLayer.obj" "%BF2_BUILD_OUT%\HudBlinker.obj" "%BF2_BUILD_OUT%\EventManager.obj" "%BF2_BUILD_OUT%\StringUtil.obj" "%BF2_BUILD_OUT%\Debug.obj" "%BF2_BUILD_OUT%\ExcelLexicon.obj" "%BF2_BUILD_OUT%\Console.obj" "%BF2_BUILD_OUT%\fixture.obj" user32.lib
if errorlevel 1 exit /b 1
"%BF2_BUILD_OUT%\hud_radio_runtime.exe"
exit /b %errorlevel%
