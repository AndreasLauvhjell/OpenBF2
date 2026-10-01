@echo off
setlocal
rem [TEMP] Focused x86 client fixture; not part of a BF2 image.
call "D:\Program Files\Microsoft Visual Studio\18\Insiders\Common7\Tools\VsDevCmd.bat" -arch=x86 -no_logo >nul 2>&1
set "TASK_ROOT=%~dp0..\BF2"
set "TASK_OUT=%~dp0..\Build\hud_bindings_runtime"
if not exist "%TASK_OUT%" mkdir "%TASK_OUT%"
set "TASK_FLAGS=/nologo /EHsc /std:c++14 /permissive /MD /Od /Z7 /Gy /DDEDICATED=0 /DBF2_PLUGIN_ABI_NATIVE=1 /D_ITERATOR_DEBUG_LEVEL=0 /FI"%TASK_ROOT%\Core\BuildConfig.h""
cl %TASK_FLAGS% "%~dp0hud_bindings_runtime.cpp" "%TASK_ROOT%\Core\ConsoleObjectBase.cpp" "%TASK_ROOT%\Game\Objects\HudComp\ModuleHudComp.cpp" "%TASK_ROOT%\Game\Objects\PostProcessCompTemplate.cpp" "%TASK_ROOT%\Game\Objects\AbilityHudCompTemplate.cpp" "%TASK_ROOT%\Game\Objects\VehicleHudCompTemplate.cpp" "%TASK_ROOT%\Game\Objects\VehicleHudIcons.cpp" "%TASK_ROOT%\Game\Objects\WeaponHudCompTemplate.cpp" "%TASK_ROOT%\Game\Objects\WarningHudCompTemplate.cpp" "%TASK_ROOT%\Game\Objects\HelpHudCompTemplate.cpp" "%TASK_ROOT%\Game\Objects\IObjectComponent.cpp" "%TASK_ROOT%\Core\StringUtil.cpp" "%TASK_ROOT%\Math\Vec3.cpp" /Fe"%TASK_OUT%\hud_bindings_runtime.exe" /Fo"%TASK_OUT%\\" /Fd"%TASK_OUT%\hud_bindings_runtime.pdb" /link /OPT:REF /INCREMENTAL:NO
if errorlevel 1 exit /b 1
"%TASK_OUT%\hud_bindings_runtime.exe"
exit /b %errorlevel%
